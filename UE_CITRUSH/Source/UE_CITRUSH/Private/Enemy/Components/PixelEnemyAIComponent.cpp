#include "Enemy/Components/PixelEnemyAIComponent.h"

#include "JsonUtilities.h"
#include "Interfaces/IHttpResponse.h"

DEFINE_LOG_CATEGORY_CLASS(UPixelEnemyAIComponent, PixelEnemyAILog)

// Constructor & Lifecycle
UPixelEnemyAIComponent::UPixelEnemyAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 기본 서버 설정
	/*ServerURL = TEXT("http://34.64.186.153");
	ServerPort = 8000;
	RequestTimeout = 5.0f;*/
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
		ServerPort,
		IniFile
	);

	GConfig->GetFloat(
		TEXT("/Script/UE_CITRUSH.BaseAISubsystem"),
		TEXT("RequestTimeout"),
		RequestTimeout,
		IniFile
	);

	bIsRequestPending = false;
}

void UPixelEnemyAIComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(PixelEnemyAILog, Warning, TEXT("[PixelEnemyAI] Not Authority - AI requests disabled for %s"), *GetOwner()->GetName());
		return;
	}
}

void UPixelEnemyAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAutoDecisionRequests();

	Super::EndPlay(EndPlayReason);
}

// Public API Functions
void UPixelEnemyAIComponent::RequestDecision(const FGetDecisionRequest2& Request)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(PixelEnemyAILog, Warning, TEXT("[PixelEnemyAI] RequestDecision called on non-authority actor"));
		return;
	}

	if (bIsRequestPending)
	{
		UE_LOG(PixelEnemyAILog, Warning, TEXT("[PixelEnemyAI] Request already pending, skipping"));
		return;
	}

	FString JsonBody;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Request, JsonBody))
	{
		UE_LOG(PixelEnemyAILog, Error, TEXT("[PixelEnemyAI] Failed to serialize decision request"));
		OnDecisionResponse.Broadcast(false, FGetDecisionResponse2());
		return;
	}

	// 요청 저장 (자동 반복용)
	LastDecisionRequest = Request;
	bIsRequestPending = true;

	UE_LOG(PixelEnemyAILog, Log, TEXT("[PixelEnemyAI] Requesting decision for %s"), *GetOwner()->GetName());

	TWeakObjectPtr<UPixelEnemyAIComponent> WeakThis(this);
	SendPostRequest(JsonBody, [WeakThis](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (!WeakThis.IsValid())
		{
			return;
		}
		UPixelEnemyAIComponent* StrongThis = WeakThis.Get();
		StrongThis->bIsRequestPending = false;

		StrongThis->HandleResponse(Response, bWasSuccessful, [WeakThis](const FString& Content, int32 StatusCode)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			UPixelEnemyAIComponent* StrongThisInner = WeakThis.Get();

			FGetDecisionResponse2 DecisionResponse;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(Content, &DecisionResponse, 0, 0))
			{
				UE_LOG(PixelEnemyAILog, Log, TEXT("[PixelEnemyAI] Decision received - Squad Objective: %s, Confidence: %.2f"),
					DecisionResponse.decision.squad_objective.IsEmpty() ? TEXT("None") : *DecisionResponse.decision.squad_objective,
					DecisionResponse.decision.confidence);

				// 유닛 명령 로그
				for (const FUnitCommand& Cmd : DecisionResponse.decision.unit_commands)
				{
					UE_LOG(PixelEnemyAILog, Verbose, TEXT("  -> Unit: %s, Directive: %d (%s)"),
						Cmd.unit_id.IsEmpty() ? TEXT("Unknown") : *Cmd.unit_id,
						Cmd.directive_code,
						Cmd.directive_name.IsEmpty() ? TEXT("Unknown") : *Cmd.directive_name);
				}

				StrongThisInner->OnDecisionResponse.Broadcast(true, DecisionResponse);
			}
			else
			{
				UE_LOG(PixelEnemyAILog, Error, TEXT("[PixelEnemyAI] Failed to parse decision response"));
				StrongThisInner->OnDecisionResponse.Broadcast(false, FGetDecisionResponse2());
			}
		});
	});
}

void UPixelEnemyAIComponent::StartAutoDecisionRequests(float IntervalSeconds)
{
	if (!GetWorld())
	{
		UE_LOG(PixelEnemyAILog, Error, TEXT("[PixelEnemyAI] Cannot start auto requests - Invalid World"));
		return;
	}

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(PixelEnemyAILog, Warning, TEXT("[PixelEnemyAI] StartAutoDecisionRequests called on non-authority actor"));
		return;
	}

	StopAutoDecisionRequests();

	GetWorld()->GetTimerManager().SetTimer(
		AutoDecisionTimer,
		[this]()
		{
			if (LastDecisionRequest.room_id.IsEmpty())
			{
				UE_LOG(PixelEnemyAILog, Warning, TEXT("[PixelEnemyAI] No decision request to repeat - call RequestDecision() first"));
				return;
			}

			RequestDecision(LastDecisionRequest);
		},
		IntervalSeconds,
		true  // 반복
	);
}

void UPixelEnemyAIComponent::StopAutoDecisionRequests()
{
	if (!GetWorld())
	{
		return;
	}

	if (AutoDecisionTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoDecisionTimer);
		UE_LOG(PixelEnemyAILog, Log, TEXT("[PixelEnemyAI] Stopped auto decision requests"));
	}
}

// ============================================================================
// Internal HTTP Functions
// ============================================================================
void UPixelEnemyAIComponent::SendPostRequest(const FString& JsonBody, TFunction<void(FHttpResponsePtr, bool)> OnComplete)
{
	FHttpModule& HttpModule = FHttpModule::Get();
	TSharedRef<IHttpRequest> HttpRequest = HttpModule.CreateRequest();

	const FString URL = ServerPort > 0 ?
		FString::Printf(TEXT("%s:%d%s"), *ServerURL, ServerPort, DecisionEndpoint)
		: FString::Printf(TEXT("%s%s"), *ServerURL, DecisionEndpoint);

	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(JsonBody);
	HttpRequest->SetTimeout(RequestTimeout);

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			OnComplete(Response, bWasSuccessful);
		}
	);

	if (HttpRequest->ProcessRequest())
	{
		UE_LOG(PixelEnemyAILog, Verbose, TEXT("[PixelEnemyAI] POST Request Sent: %s"), *URL);
	}
	else
	{
		UE_LOG(PixelEnemyAILog, Error, TEXT("[PixelEnemyAI] Failed to send POST request: %s"), *URL);
		OnComplete(nullptr, false);
	}
}

void UPixelEnemyAIComponent::HandleResponse(FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(const FString&, int32)> OnSuccess)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(PixelEnemyAILog, Error, TEXT("[PixelEnemyAI] HTTP Request Failed - No response"));
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

void UPixelEnemyAIComponent::HandleErrorResponse(int32 StatusCode, const FString& ResponseContent)
{
	UE_LOG(PixelEnemyAILog, Error, TEXT("[PixelEnemyAI] HTTP Error %d: %s"), StatusCode, *ResponseContent);

	// FFailedResponse 파싱 시도
	FFailedResponse FailedResp;
	if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseContent, &FailedResp, 0, 0))
	{
		UE_LOG(PixelEnemyAILog, Error, TEXT("  -> Error Code: %s"), *FailedResp.error.code);
		UE_LOG(PixelEnemyAILog, Error, TEXT("  -> Error Message: %s"), *FailedResp.error.message);

		OnHttpError.Broadcast(StatusCode, FailedResp.error.message, FailedResp);
	}
	else
	{
		// 파싱 실패 시 원본 메시지로 브로드캐스트
		OnHttpError.Broadcast(StatusCode, ResponseContent, FFailedResponse());
	}
}
