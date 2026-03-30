#include "Network/AIDataManagerComponent.h"

#include "JsonUtilities.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/KismetSystemLibrary.h"

DEFINE_LOG_CATEGORY_CLASS(UAIDataManagerComponent, CitRushHttpLog)

// ============================================================================
// Constructor & Lifecycle
// ============================================================================

UAIDataManagerComponent::UAIDataManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 엔드포인트 설정 초기화 (v1.2.0)
	Endpoints.Add(EHttpEndpoint::Health,         FHttpEndpointConfig(TEXT("api/v1/health"),            TEXT("GET"),  5.0f));
	Endpoints.Add(EHttpEndpoint::MatchStart,     FHttpEndpointConfig(TEXT("api/v1/match/start"),       TEXT("POST"), 5.0f));
	Endpoints.Add(EHttpEndpoint::GetDecision,    FHttpEndpointConfig(TEXT("api/v1/get_decision"),      TEXT("POST"), 5.0f));
	Endpoints.Add(EHttpEndpoint::OverseerTTS,    FHttpEndpointConfig(TEXT("api/v1/overseer/tts"),      TEXT("POST"), 5.0f));  // v1.2.0 신규
	Endpoints.Add(EHttpEndpoint::CommanderReport, FHttpEndpointConfig(TEXT("api/v1/commander/report"), TEXT("GET"),  5.0f));  // v1.2.0 신규
	Endpoints.Add(EHttpEndpoint::MatchEnd,       FHttpEndpointConfig(TEXT("api/v1/match/end"),         TEXT("POST"), 5.0f));

	/*ServerURL = TEXT("http://34.64.186.153");
	PortURL = 8000;*/

	const FString IniFile = FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini");
	GConfig->GetString(
		TEXT("/Script/UE_CITRUSH.BaseAISubsystem"),
		TEXT("ServerURL"),
		ServerURL,
		IniFile
	);

	GConfig->GetInt(
		TEXT("/Script/UE_CITRUSH.BaseAISubsystem"),
		TEXT("ServerPort"),
		PortURL,
		IniFile
	);


}

void UAIDataManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Authority 체크 (서버에서만 실행)
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(CitRushHttpLog, Warning, TEXT("[HttpComponent] Not Authority - HTTP requests disabled"));
		return;
	}

	UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Initialized - Server URL: %s:%d"), *ServerURL, PortURL);
}

// ============================================================================
// Public API Functions
// ============================================================================

void UAIDataManagerComponent::CheckHealth()
{
	const FHttpEndpointConfig* Config = GetEndpointConfig(EHttpEndpoint::Health);
	if (!Config)
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Health endpoint not configured"));
		OnHealthCheckResponse.Broadcast(false, FHealthResponse());
		return;
	}

	UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Checking server health..."));

	SendGetRequest(*Config, [this](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		HandleResponse(Response, bWasSuccessful, [this](const FString& Content, int32 StatusCode)
		{
			FHealthResponse HealthResponse;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(Content, &HealthResponse, 0, 0))
			{
				UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Health Check Success - Status: %s, Version: %s, Raw: %s"),
					*HealthResponse.status, *HealthResponse.version, *Content);
				OnHealthCheckResponse.Broadcast(true, HealthResponse);
			}
			else
			{
				UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to parse health response"));
				OnHealthCheckResponse.Broadcast(false, FHealthResponse());
			}
		});
	});
}

void UAIDataManagerComponent::StartMatch(/*const FMatchStartRequest& Request*/)
{
	// Debug용
	FMatchStartRequest Request;
	Request.room_id = "room_id_UE";
	Request.player_ids.Add(FString::FromInt(GetOwner()->GetUniqueID()));
	Request.map_id = "map_id_UE";
	Request.mode = "PVE";
	Request.match_start_time = FString::FromInt(UKismetSystemLibrary::GetGameTimeInSeconds(GetWorld()));
	const FHttpEndpointConfig* Config = GetEndpointConfig(EHttpEndpoint::MatchStart);
	if (!Config)
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] MatchStart endpoint not configured"));
		OnMatchStartResponse.Broadcast(false, FMatchStartResponse());
		return;
	}

	UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Starting match - Room: %s"), *Request.room_id);

	// Request를 JSON으로 직렬화
	FString JsonBody;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Request, JsonBody))
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to serialize StartMatch request"));
		OnMatchStartResponse.Broadcast(false, FMatchStartResponse());
		return;
	}

	SendPostRequest(*Config, JsonBody, [this](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		HandleResponse(Response, bWasSuccessful, [this](const FString& Content, int32 StatusCode)
		{
			FMatchStartResponse MatchResponse;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(Content, &MatchResponse, 0, 0))
			{
				UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Match Started - Status: %s, Message: %s"),
					*MatchResponse.status, *MatchResponse.message);
				OnMatchStartResponse.Broadcast(true, MatchResponse);
			}
			else
			{
				UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to parse match start response"));
				OnMatchStartResponse.Broadcast(false, FMatchStartResponse());
			}
		});
	});
}

void UAIDataManagerComponent::GetDecision(const FGetDecisionRequest2& Request)
{
	const FHttpEndpointConfig* Config = GetEndpointConfig(EHttpEndpoint::GetDecision);
	if (!Config)
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] GetDecision endpoint not configured"));
		OnDecisionResponse.Broadcast(false, FGetDecisionResponse2());
		return;
	}

	// Request를 JSON으로 직렬화
	FString JsonBody;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Request, JsonBody))
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to serialize GetDecision request (v1.2.0)"));
		OnDecisionResponse.Broadcast(false, FGetDecisionResponse2());
		return;
	}

	// 자동 요청용으로 저장
	LastDecisionRequest = Request;

	TWeakObjectPtr<UAIDataManagerComponent> WeakThis(this);
	SendPostRequest(*Config, JsonBody, [WeakThis](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (!WeakThis.IsValid())
		{
			return;
		}
		UAIDataManagerComponent* StrongThis = WeakThis.Get();

		StrongThis->HandleResponse(Response, bWasSuccessful, [WeakThis](const FString& Content, int32 StatusCode)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			UAIDataManagerComponent* StrongThisInner = WeakThis.Get();

			FGetDecisionResponse2 DecisionResponse;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(Content, &DecisionResponse, 0, 0))
			{
				UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Decision Received (v1.2.0) - Squad Objective: %s, Confidence: %.2f"),
					DecisionResponse.decision.squad_objective.IsEmpty() ? TEXT("None") : *DecisionResponse.decision.squad_objective,
					DecisionResponse.decision.confidence);

				// 유닛 명령 로그
				for (const FUnitCommand& Cmd : DecisionResponse.decision.unit_commands)
				{
					UE_LOG(CitRushHttpLog, Verbose, TEXT("  -> Unit: %s, Directive: %d (%s)"),
						Cmd.unit_id.IsEmpty() ? TEXT("Unknown") : *Cmd.unit_id,
						Cmd.directive_code,
						Cmd.directive_name.IsEmpty() ? TEXT("Unknown") : *Cmd.directive_name);
				}

				// v1.2.0: Overseer TTS Trigger 확인
				if (DecisionResponse.overseer_tts_trigger.has_pending_tts)
				{
					UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Overseer TTS Pending - EventID: %s"),
						*DecisionResponse.overseer_tts_trigger.event_id);

					// 자동으로 TTS 요청 (선택적)
					FOverseerTTSRequest2 TTSRequest;
					TTSRequest.room_id = DecisionResponse.overseer_tts_trigger.room_id;
					TTSRequest.event_id = DecisionResponse.overseer_tts_trigger.event_id;
					StrongThisInner->GetOverseerTTS(TTSRequest);
				}

				StrongThisInner->OnDecisionResponse.Broadcast(true, DecisionResponse);
			}
			else
			{
				UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to parse decision response (v1.2.0)"));
				StrongThisInner->OnDecisionResponse.Broadcast(false, FGetDecisionResponse2());
			}
		});
	});
}

void UAIDataManagerComponent::EndMatch(/*const FMatchEndRequest& Request*/)
{
	FMatchEndRequest Request;
	Request.room_id = "room_id_UE";
	Request.result = "ABORTED";
	const FHttpEndpointConfig* Config = GetEndpointConfig(EHttpEndpoint::MatchEnd);
	if (!Config)
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] MatchEnd endpoint not configured"));
		OnMatchEndResponse.Broadcast(false, FMatchEndResponse());
		return;
	}

	UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Ending match - Room: %s, Result: %s"),
		*Request.room_id, *Request.result);

	// Request를 JSON으로 직렬화
	FString JsonBody;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Request, JsonBody))
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to serialize EndMatch request"));
		OnMatchEndResponse.Broadcast(false, FMatchEndResponse());
		return;
	}

	SendPostRequest(*Config, JsonBody, [this](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		HandleResponse(Response, bWasSuccessful, [this](const FString& Content, int32 StatusCode)
		{
			FMatchEndResponse EndResponse;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(Content, &EndResponse, 0, 0))
			{
				UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Match Ended - Status: %s, Message: %s"),
					*EndResponse.status, *EndResponse.message);

				OnMatchEndResponse.Broadcast(true, EndResponse);
			}
			else
			{
				UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to parse match end response"));
				OnMatchEndResponse.Broadcast(false, FMatchEndResponse());
			}
		});
	});

	// 자동 의사결정 요청 중지
	StopAutoDecisionRequests();
}

void UAIDataManagerComponent::GetOverseerTTS(const FOverseerTTSRequest2& Request)
{
	const FHttpEndpointConfig* Config = GetEndpointConfig(EHttpEndpoint::OverseerTTS);
	if (!Config)
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] OverseerTTS endpoint not configured"));
		OnOverseerTTSResponse.Broadcast(false, FOverseerTTSResponse2());
		return;
	}

	UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Requesting Overseer TTS - RoomID: %s, EventID: %s"),
		*Request.room_id, *Request.event_id);

	// Request를 JSON으로 직렬화
	FString JsonBody;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Request, JsonBody))
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to serialize OverseerTTS request"));
		OnOverseerTTSResponse.Broadcast(false, FOverseerTTSResponse2());
		return;
	}

	SendPostRequest(*Config, JsonBody, [this](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		HandleResponse(Response, bWasSuccessful, [this](const FString& Content, int32 StatusCode)
		{
			FOverseerTTSResponse2 TTSResponse;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(Content, &TTSResponse, 0, 0))
			{
				UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Overseer TTS Received - Mode: %s, Text: %s"),
					*TTSResponse.tts_mode, *TTSResponse.display_text);

				if (TTSResponse.tts_mode.Equals(TEXT("LOCAL_CLIP")))
				{
					UE_LOG(CitRushHttpLog, Log, TEXT("  -> Local Clip Index: %d, CueID: %s"),
						TTSResponse.local_clip_index, *TTSResponse.cue_id);
				}
				else if (TTSResponse.tts_mode.Equals(TEXT("SERVER_AUDIO")))
				{
					UE_LOG(CitRushHttpLog, Log, TEXT("  -> Server Audio Format: %s, Base64 Length: %d"),
						*TTSResponse.audio_format, TTSResponse.audio_base64.Len());
				}

				OnOverseerTTSResponse.Broadcast(true, TTSResponse);
			}
			else
			{
				UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to parse OverseerTTS response"));
				OnOverseerTTSResponse.Broadcast(false, FOverseerTTSResponse2());
			}
		});
	});
}

void UAIDataManagerComponent::GetCommanderReport(const FString& RoomID)
{
	const FHttpEndpointConfig* Config = GetEndpointConfig(EHttpEndpoint::CommanderReport);
	if (!Config)
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] CommanderReport endpoint not configured"));
		OnCommanderReportResponse.Broadcast(false, FCommanderReportResponse2());
		return;
	}

	UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Requesting Commander Report - RoomID: %s"), *RoomID);

	// GET 요청이므로 Query Parameter로 room_id 추가
	FHttpEndpointConfig ModifiedConfig = *Config;
	ModifiedConfig.Path = FString::Printf(TEXT("%s?room_id=%s"), *Config->Path, *RoomID);

	SendGetRequest(ModifiedConfig, [this](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		HandleResponse(Response, bWasSuccessful, [this](const FString& Content, int32 StatusCode)
		{
			FCommanderReportResponse2 ReportResponse;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(Content, &ReportResponse, 0, 0))
			{
				UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Commander Report Received - Grade: %s, Score: %.2f"),
					*ReportResponse.grade, ReportResponse.overall_score);

				UE_LOG(CitRushHttpLog, Log, TEXT("  -> Strengths: %d, Weaknesses: %d"),
					ReportResponse.strengths.Num(), ReportResponse.weaknesses.Num());

				OnCommanderReportResponse.Broadcast(true, ReportResponse);
			}
			else
			{
				UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to parse CommanderReport response"));
				OnCommanderReportResponse.Broadcast(false, FCommanderReportResponse2());
			}
		});
	});
}

void UAIDataManagerComponent::StartAutoDecisionRequests(float IntervalSeconds)
{
	if (!GetWorld())
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Cannot start auto requests - Invalid World"));
		return;
	}

	// 기존 타이머가 있으면 정리
	StopAutoDecisionRequests();

	UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Starting auto decision requests - Interval: %.2f seconds"), IntervalSeconds);

	// 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		AutoDecisionTimer,
		[this]()
		{
			if (LastDecisionRequest.room_id.IsEmpty())
			{
				UE_LOG(CitRushHttpLog, Warning, TEXT("[HttpComponent] No decision request to repeat - call GetDecision() first"));
				return;
			}

			GetDecision(LastDecisionRequest);
		},
		IntervalSeconds,
		true  // 반복
	);
}

void UAIDataManagerComponent::StopAutoDecisionRequests()
{
	if (!GetWorld())
	{
		return;
	}

	if (AutoDecisionTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoDecisionTimer);
		UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] Stopped auto decision requests"));
	}
}

// ============================================================================
// Internal HTTP Functions
// ============================================================================

void UAIDataManagerComponent::SendGetRequest(const FHttpEndpointConfig& Config, TFunction<void(FHttpResponsePtr, bool)> OnComplete)
{
	FHttpModule& HttpModule = FHttpModule::Get();
	TSharedRef<IHttpRequest> HttpRequest = HttpModule.CreateRequest();

	const FString URL = PortURL > 0 ?
		ServerURL + TEXT(":") + FString::FromInt(PortURL) + Config.Path
		: ServerURL + Config.Path;
	
	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(Config.Method);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetTimeout(Config.Timeout);

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			OnComplete(Response, bWasSuccessful);
		}
	);

	if (HttpRequest->ProcessRequest())
	{
		UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] GET Request Sent: %s"), *URL);
	}
	else
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to send GET request: %s"), *URL);
		OnComplete(nullptr, false);
	}
}

void UAIDataManagerComponent::SendPostRequest(const FHttpEndpointConfig& Config, const FString& JsonBody, TFunction<void(FHttpResponsePtr, bool)> OnComplete)
{
	FHttpModule& HttpModule = FHttpModule::Get();
	TSharedRef<IHttpRequest> HttpRequest = HttpModule.CreateRequest();

	const FString URL = PortURL > 0 ?
		ServerURL + TEXT(":") + FString::FromInt(PortURL) + Config.Path
		: ServerURL + Config.Path;
	UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] : %s"), *URL);
	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(Config.Method);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(JsonBody);
	HttpRequest->SetTimeout(Config.Timeout);

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			OnComplete(Response, bWasSuccessful);
		}
	);

	if (HttpRequest->ProcessRequest())
	{
		UE_LOG(CitRushHttpLog, Log, TEXT("[HttpComponent] POST Request Sent: %s"), *URL);
		UE_LOG(CitRushHttpLog, VeryVerbose, TEXT("[HttpComponent] Request Body: %s"), *JsonBody);
	}
	else
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] Failed to send POST request: %s"), *URL);
		OnComplete(nullptr, false);
	}
}

void UAIDataManagerComponent::HandleResponse(FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(const FString&, int32)> OnSuccess)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] HTTP Request Failed - No response"));
		HandleErrorResponse(0, TEXT("No response received"));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString Content = Response->GetContentAsString();

	// 성공 응답 (200)
	if (StatusCode == 200)
	{
		OnSuccess(Content, StatusCode);
		return;
	}

	// 에러 응답 처리
	HandleErrorResponse(StatusCode, Content);
}

void UAIDataManagerComponent::HandleErrorResponse(int32 StatusCode, const FString& ResponseContent)
{
	UE_LOG(CitRushHttpLog, Error, TEXT("[HttpComponent] HTTP Error %d: %s"), StatusCode, *ResponseContent);

	// FFailedResponse 파싱 시도
	FFailedResponse FailedResp;
	if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseContent, &FailedResp, 0, 0))
	{
		UE_LOG(CitRushHttpLog, Error, TEXT("  -> Error Code: %s"), *FailedResp.error.code);
		UE_LOG(CitRushHttpLog, Error, TEXT("  -> Error Message: %s"), *FailedResp.error.message);

		OnHttpError.Broadcast(StatusCode, FailedResp.error.message, FailedResp);
	}
	else
	{
		// 파싱 실패 시 원본 메시지로 브로드캐스트
		OnHttpError.Broadcast(StatusCode, ResponseContent, FFailedResponse());
	}
}

// ============================================================================
// Helper Functions
// ============================================================================

const FHttpEndpointConfig* UAIDataManagerComponent::GetEndpointConfig(EHttpEndpoint Endpoint) const
{
	return Endpoints.Find(Endpoint);
}

// ============================================================================
// 사용 예시 (주석)
// ============================================================================

/*
// Blueprint 또는 C++에서 사용 예시:

void AMyGameMode::BeginPlay()
{
	Super::BeginPlay();

	// HttpComponent 가져오기
	UAIDataManagerComponent* HttpComp = FindComponentByClass<UAIDataManagerComponent>();
	if (!HttpComp)
	{
		return;
	}

	// 1. 델리게이트 바인딩
	HttpComp->OnHealthCheckResponse.AddDynamic(this, &AMyGameMode::OnHealthResponse);
	HttpComp->OnDecisionResponse.AddDynamic(this, &AMyGameMode::OnDecisionReceived);
	HttpComp->OnHttpError.AddDynamic(this, &AMyGameMode::OnHttpErrorOccurred);

	// 2. 헬스체크
	HttpComp->CheckHealth();

	// 3. 매치 시작
	FMatchStartRequest StartReq;
	StartReq.room_id = TEXT("room_001");
	StartReq.player_ids = {TEXT("player1"), TEXT("player2"), TEXT("player3")};
	StartReq.map_id = TEXT("map_default");
	StartReq.mode = TEXT("standard");
	HttpComp->StartMatch(StartReq);

	// 4. 의사결정 요청 시작 (2.5초마다 자동)
	FGetDecisionRequest DecisionReq;
	DecisionReq.room_id = TEXT("room_001");
	// ... 게임 상태 설정 ...

	HttpComp->GetDecision(DecisionReq);  // 첫 요청
	HttpComp->StartAutoDecisionRequests(2.5f);  // 자동 반복 시작
}

void AMyGameMode::OnDecisionReceived(bool bSuccess, FGetDecisionResponse Response)
{
	if (!bSuccess)
	{
		return;
	}

	// AI 명령 처리
	for (const FUnitCommand& Cmd : Response.decision.unit_commands)
	{
		ApplyUnitCommand(Cmd.unit_id, Cmd.directive_code, Cmd.params);
	}
}

void AMyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 매치 종료
	UAIDataManagerComponent* HttpComp = FindComponentByClass<UAIDataManagerComponent>();
	if (HttpComp)
	{
		FMatchEndRequest EndReq;
		EndReq.room_id = TEXT("room_001");
		EndReq.result = TEXT("PLAYERS_WIN");
		EndReq.final_score = 1500;
		HttpComp->EndMatch(EndReq);
	}

	Super::EndPlay(EndPlayReason);
}
*/