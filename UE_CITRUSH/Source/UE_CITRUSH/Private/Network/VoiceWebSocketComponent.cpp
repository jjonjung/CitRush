#include "Network/VoiceWebSocketComponent.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Network/SteamSubsystem.h"
#include "Player/Components/SteamVoiceComponent.h"
#include "Player/Components/VoiceCaptureComponent.h"
#include "JsonObjectConverter.h"
#include "Json.h"
#include "Misc/Base64.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY_CLASS(UVoiceWebSocketComponent, CitRushVoiceWebSocketLog)

// ============================================================================
// Constructor & Lifecycle
// ============================================================================

UVoiceWebSocketComponent::UVoiceWebSocketComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 초기 상태 설정
	State = EWebSocketState::Default;
	TargetRacer = ETargetRacer::None;
	ConnectionParams = FCommanderWebSocketConnectionParams2();

	// Test 용 IP, Port (v1.2.0)
	ConnectionParams.ServerIP = "127.0.0.1";
	ConnectionParams.ServerPort = 8000;
	// v1.2.0: room_id, commander_id는 InitWebsocketProperties()에서 설정
}

void UVoiceWebSocketComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// ========================================================================
	// 1. 네트워크 권한 검증 (서버만 실행)
	// ========================================================================
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] 클라이언트에서는 비활성화됨 (서버 전용)"));
		return;
	}
	stateEnumPtr = StaticEnum<EWebSocketState>();
}

void UVoiceWebSocketComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Delegate 언바인딩 (AbstractCommander에서 바인딩했으므로 여기서 해제)
	if (SteamVoiceComponent)
	{
		SteamVoiceComponent->OnCaptureVoiceBuffer.RemoveAll(this);
	}

	if (VoiceCaptureComponent)
	{
		VoiceCaptureComponent->OnAudioCaptured.RemoveAll(this);
	}

	// WebSocket 연결 종료
	if (WebSocket && WebSocket->IsConnected())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] EndPlay - WebSocket 연결 종료"));
		WebSocket->Close(EWebSocketCloseCode::NormalClosure, TEXT("Game Ended"));
	}

	WebSocket.Reset();
	SetState(EWebSocketState::Disconnected);

	Super::EndPlay(EndPlayReason);
}

// ============================================================================
// Public API - Connection
// ============================================================================

void UVoiceWebSocketComponent::Create()
{
	if (!InitWebsocketProperties()) {return;}
	
	if (!ConnectionParams.IsValid())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] ConnectionParams가 유효하지 않습니다. ServerIP=%s, Port=%d"),
			*ConnectionParams.ServerIP, ConnectionParams.ServerPort);
		return;
	}

	FWebSocketsModule module = FWebSocketsModule::Get();
	module.OnWebSocketCreated.AddUObject(this, &UVoiceWebSocketComponent::OnWebSocketCreated_Internal);

	// WebSocket 인스턴스 생성 (v1.2.0)
	FString WebSocketURL = ConnectionParams.GenerateURL();
	UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] v1.2.0 URL: %s"), *WebSocketURL);
	module.CreateWebSocket(WebSocketURL, TArray<FString>());
	
	/*if (!WebSocket.IsValid())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] Create() - WebSocket 인스턴스 생성 실패"));
		HandleError(TEXT("Failed to create WebSocket instance"));
		return;
	}*/

	SetState(EWebSocketState::Loaded);	
}

void UVoiceWebSocketComponent::Connect()
{	
	if (!WebSocket.IsValid())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] Connect() - WebSocket API Not Loaded. State=%d"), static_cast<int32>(State));
		return;
	}
	
	// 이미 연결 중이거나 연결됨
	if (State == EWebSocketState::Connecting || State == EWebSocketState::Connected || State == EWebSocketState::Transmitting)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] Connect() - 이미 연결됨 또는 연결 중. State=%d"), static_cast<int32>(State));
		return;
	}


	// ========================================================================
	// 연결 시작
	// ========================================================================
	SavePath = FPaths::ProjectSavedDir() / FDateTime::Now().ToString(TEXT("%Y%m%d-%H_%M_%S"));
	
	SetState(EWebSocketState::Connecting);
	WebSocket->Connect();
}

void UVoiceWebSocketComponent::Disconnect()
{
	// WebSocket이 없거나 이미 연결 해제됨
	if (!WebSocket.IsValid() || State == EWebSocketState::Disconnected || State == EWebSocketState::Default)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] Disconnect() - 이미 연결 해제됨. State=%d"), static_cast<int32>(State));
		return;
	}

	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] Disconnect() - 연결 종료 시작"));

	// 전송 중이면 먼저 중지
	if (State == EWebSocketState::Transmitting)
	{
		StopTransmission();
	}

	SetState(EWebSocketState::Disconnecting);

	if (WebSocket->IsConnected())
	{
		WebSocket->Close(EWebSocketCloseCode::NormalClosure, TEXT("User Disconnect"));
	}
	else
	{
		// 이미 연결이 끊어진 경우
		SetState(EWebSocketState::Disconnected);
	}
}

// ============================================================================
// Public API - Voice Transmission
// ============================================================================

void UVoiceWebSocketComponent::StartTransmissionTo(ETargetRacer Target)
{
	if (Target == ETargetRacer::None)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] StartTransmissionTo() - Target이 None입니다"));
		StopTransmission();
		return;
	}
	
	if (State == EWebSocketState::Connected || State == EWebSocketState::Transmitting)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] StartTransmissionTo() - Target=%d (v1.2.0)"), static_cast<int32>(Target));

		TargetRacer = Target;

		// v1.2.0: STT Event phase 추적 초기화
		bFirstAudioChunk = true;
		AudioSequence = 0;

		SetState(EWebSocketState::Transmitting);
		return;
	}
	
	UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] StartTransmissionTo() - WebSocket이 연결되지 않았습니다. State=%d"), static_cast<int32>(State));
	HandleError(TEXT("WebSocket not connected"));
	return;
	
	/*// 연결 확인

	// Target 검증

	if (TargetRacer == Target)
	{
		return;
	}
	
	// ========================================================================
	// Target 변경 시 기존 세션 종료
	// ========================================================================
	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] StartTransmissionTo() - Target 변경: %d -> %d"),
		static_cast<int32>(TargetRacer), static_cast<int32>(Target));

	// 오디오 캡처 중지
	if (VoiceCaptureComponent && VoiceCaptureComponent->IsCapturing())
	{
		VoiceCaptureComponent->StopCapture();
	}
	else if (SteamVoiceComponent)
	{
		SteamVoiceComponent->StopVoiceChat();
	}

	TargetRacer = Target;

	// ========================================================================
	// Session Start 메시지 전송
	// ========================================================================
	FString JsonString;
	if (FWebSocketSendUtils::SerializeSessionStart(static_cast<int32>(Target), JsonString))
	{
		WebSocket->Send(JsonString);
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] Session Start 전송: Target=%d"), static_cast<int32>(Target));
	}
	else
	{
		UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] Session Start 직렬화 실패!"));
		HandleError(TEXT("Failed to serialize session_start"));
		return;
	}

	// 상태 변경
	SetState(EWebSocketState::Transmitting);

	// 오디오 캡처 시작
	if (VoiceCaptureComponent)
	{
		if (!VoiceCaptureComponent->IsCapturing())
		{
			VoiceCaptureComponent->StartCapture();
		}
	}
	else if (SteamVoiceComponent)
	{
		if (!SteamVoiceComponent->IsCapturing())
		{
			SteamVoiceComponent->StartVoiceChat();
		}
	}
	else
	{
		UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] VoiceCaptureComponent가 없습니다!"));
	}*/
}

void UVoiceWebSocketComponent::StopTransmission()
{
	// 전송 중이 아니면 무시
	if (State != EWebSocketState::Transmitting)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] StopTransmission() - 전송 중이 아닙니다. State=%d"), static_cast<int32>(State));
		return;
	}

	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] StopTransmission() - Target=%d (v1.2.0)"), static_cast<int32>(TargetRacer));

	// ========================================================================
	// v1.2.0: Final phase STT Event 전송 (빈 오디오)
	// ========================================================================
	FCommanderSTTEvent2 FinalEvent;
	FinalEvent.phase = TEXT("final");
	FinalEvent.room_id = ConnectionParams.RoomID;
	FinalEvent.commander_id = ConnectionParams.CommanderID;
	FinalEvent.speaker.player_id = ConnectionParams.CommanderID/*FBase64::Encode(ConnectionParams.CommanderID, EBase64Mode::UrlSafe)*/;  // Start/Partial과 동일하게 인코딩
	FinalEvent.speaker.player_name = TEXT("Commander");  // TODO: 실제 닉네임 가져오기
	FinalEvent.speaker.player_type = TEXT("COMMANDER");
	FinalEvent.target.player_id = "TestTargetPlayerID";
	FinalEvent.target.player_name = "TestTargetPlayerName";
	FinalEvent.target.player_type = "Racer";
	FinalEvent.audio.format = TEXT("PCM");  // PCM으로 통일
	FinalEvent.audio.base64 = TEXT("");  // Final은 빈 오디오
	FinalEvent.audio.sample_rate = 0;
	FinalEvent.chunk_index = AudioSequence;
	FinalEvent.timestamp = FDateTime::UtcNow().ToIso8601();

	// JSON 직렬화
	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(FinalEvent, JsonString))
	{
		WebSocket->Send(JsonString);
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] STT Event (final) 전송"));
	}
	else
	{
		UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] STT Event (final) 직렬화 실패!"));
	}
	AudioSequence = 0;
	// Target 초기화
	TargetRacer = ETargetRacer::None;

	// 상태 변경
	SetState(EWebSocketState::Connected);
}

// ============================================================================

// Protected Methods

// ============================================================================

bool UVoiceWebSocketComponent::RegisterCaptureComponent(UActorComponent* voiceActorComponent)
{
	if (Cast<USteamVoiceComponent>(voiceActorComponent))
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("register Steam Voice"));
		SteamVoiceComponent = Cast<USteamVoiceComponent>(voiceActorComponent);
		SteamVoiceComponent->OnCaptureVoiceBuffer.AddDynamic(this,
			&UVoiceWebSocketComponent::OnVoiceDataCaptured
		);
		SteamVoiceComponent->OnVoiceTargetChanged.AddDynamic(this, &UVoiceWebSocketComponent::StartTransmissionTo);
		bUsingSteamVoice = true;
		return true;
	}
	if (Cast<UVoiceCaptureComponent>(voiceActorComponent))
	{
		VoiceCaptureComponent = Cast<UVoiceCaptureComponent>(voiceActorComponent);
		VoiceCaptureComponent->OnAudioCaptured.AddDynamic(this,
			&UVoiceWebSocketComponent::OnVoiceDataCaptured
		);
		bUsingSteamVoice = false;
		return true;
	}
	
	return false;
}

void UVoiceWebSocketComponent::MatchEnd()
{
	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] MatchEnd() - 매치 종료, WebSocket 연결 종료"));

	// 전송 중이면 먼저 중지
	if (State == EWebSocketState::Transmitting)
	{
		StopTransmission();
	}

	// WebSocket 연결 종료
	if (WebSocket.IsValid() && WebSocket->IsConnected())
	{
		WebSocket->Close(170, TEXT("Match Ended"));
	}

	SetState(EWebSocketState::Disconnected);
}

bool UVoiceWebSocketComponent::InitWebsocketProperties()
{
	// ========================================================================
	// 1. 모듈 로드 확인
	// ========================================================================
	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] WebSockets 모듈이 로드되지 않았습니다!"));
		HandleError(TEXT("WebSockets module not loaded"));
		return false;
	}

	// ========================================================================
	// 2. Steam Subsystem에서 연결 정보 가져오기 (v1.2.0)
	// ========================================================================
	USteamSubsystem* SteamSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USteamSubsystem>();
	if (!SteamSubsystem)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] SteamSubsystem을 찾을 수 없습니다!"));
		HandleError(TEXT("SteamSubsystem not found"));
		return false;
	}

	// room_id 설정 (v1.2.0: RoomID 필드 사용)
	if (const TSharedPtr<FOnlineSessionInfo> SessionInfo = SteamSubsystem->GetCurrentSessionInfo())
	{
		ConnectionParams.RoomID = SessionInfo->GetSessionId().ToString();
	}
	else
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] SessionInfo를 가져올 수 없습니다. 테스트용 RoomID 사용"));
		ConnectionParams.RoomID = TEXT("ROOM_001");
	}

	// commander_id 설정 (v1.2.0: CommanderID 필드 사용)
	// TODO: Steam ID를 실제로 가져와야 함
	ConnectionParams.CommanderID = SteamSubsystem->GetSteamNickname();

	// ServerIP와 ServerPort는 Blueprint 또는 GameMode에서 설정되어야 함
	if (!ConnectionParams.IsValid())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] ConnectionParams가 유효하지 않습니다. Connect() 호출 전에 설정해야 합니다."));
		return false;
	}

	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] 초기화 스택 종료. State=%d"), static_cast<int32>(State));
	SetState(EWebSocketState::Initialized);
	return true;
}

// ============================================================================
// WebSocket Callbacks
// ============================================================================

void UVoiceWebSocketComponent::OnWebSocketCreated_Internal(const TSharedPtr<class IWebSocket>& createdWebSocket, const TArray<FString>& Protocols, const FString& Url)
{
	if (!createdWebSocket.IsValid())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] Socket 생성 실패."));
		return;
	}

	WebSocket = createdWebSocket;
	for (const FString& Protocol : Protocols)
	{
		URL += Protocol;
	}
	URL += Url;
	tempURL = URL;
	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] Socket 생성 완료.\n		URL=%s\n"), *URL);
	// ========================================================================
	// WebSocket 콜백 등록
	// ========================================================================
	// 연결 성공
	WebSocket->OnConnected().AddUObject(this, &UVoiceWebSocketComponent::OnWebSocketConnected_Internal);
	// 메시지 수신
	WebSocket->OnMessage().AddUObject(this, &UVoiceWebSocketComponent::OnWebSocketMessage_Internal);
	// 연결 종료
	WebSocket->OnClosed().AddUObject(this, &UVoiceWebSocketComponent::OnWebSocketClosed_Internal);
	// 연결 오류
	WebSocket->OnConnectionError().AddUObject(this, &UVoiceWebSocketComponent::OnWebSocketConnectionError_Internal);

}

void UVoiceWebSocketComponent::OnWebSocketConnected_Internal()
{
	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] WebSocket 연결 성공! URL=%s"), *URL);

	SetState(EWebSocketState::Connected);

	// Blueprint 이벤트 브로드캐스트
	OnConnected.Broadcast();
}

void UVoiceWebSocketComponent::OnWebSocketMessage_Internal(const FString& Message)
{
	// ========================================================================
	// v1.2.0: JSON 메시지 파싱
	// ========================================================================
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] JSON 파싱 실패: %s"), *Message);
		return;
	}

	// type 필드로 메시지 타입 판별
	FString MessageType = JsonObject->GetStringField(TEXT("type"));

	// ========================================================================
	// PING 메시지 (v1.1 재사용)
	// ========================================================================
	if (MessageType.Equals(TEXT("ping"), ESearchCase::IgnoreCase))
	{
		FWebSocketPingMessage PingMsg;
		if (FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &PingMsg))
		{
			HandlePingMessage(PingMsg);
		}
	}
	// ========================================================================
	// STT Result 메시지 (v1.2.0)
	// ========================================================================
	else if (MessageType.Equals(TEXT("stt_result"), ESearchCase::IgnoreCase))
	{
		FCommanderSTTResult2 STTResult;
		if (FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &STTResult))
		{
			HandleSTTResult(STTResult);
		}
		else
		{
			UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] STT Result 파싱 실패"));
		}
	}
	// ========================================================================
	// TTS Push 이벤트 (v1.2.0)
	// ========================================================================
	else if (MessageType.Equals(TEXT("tts_push"), ESearchCase::IgnoreCase))
	{
		FTTSPushEvent2 TTSPush;
		if (FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &TTSPush))
		{
			HandleTTSPush(TTSPush);
		}
		else
		{
			UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] TTS Push 파싱 실패"));
		}
	}
	// ========================================================================
	// 알 수 없는 메시지
	// ========================================================================
	else
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] 알 수 없는 메시지 타입: %s"), *MessageType);
	}
}

void UVoiceWebSocketComponent::OnWebSocketClosed_Internal(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] WebSocket 연결 종료. Code=%d, Reason=%s, Clean=%d"),
		StatusCode, *Reason, bWasClean);

	// 전송 중이었다면 중지
	if (State == EWebSocketState::Transmitting)
	{
		if (VoiceCaptureComponent && VoiceCaptureComponent->IsCapturing())
		{
			VoiceCaptureComponent->StopCapture();
		}
		TargetRacer = ETargetRacer::None;
	}

	SetState(EWebSocketState::Disconnected);

	// Blueprint 이벤트 브로드캐스트
	OnDisconnected.Broadcast(StatusCode, Reason, bWasClean);

	// 재연결 필요 여부 판단 (선택적)
	FWebSocketCloseInfo CloseInfo;
	CloseInfo.StatusCode = StatusCode;
	CloseInfo.Reason = Reason;
	CloseInfo.bWasClean = bWasClean;

	if (CloseInfo.ShouldReconnect())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] 비정상 종료 감지. 재연결 필요할 수 있습니다."));
		// 자동 재연결 로직은 GameMode 또는 Blueprint에서 구현

		return;
	}

	URL = "";
}

void UVoiceWebSocketComponent::OnWebSocketConnectionError_Internal(const FString& Error)
{
	
	UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] WebSocket 연결 오류: %s"), *Error);
	UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] WebSocket 오류 URL: %s"), *tempURL);
	HandleError(Error);
	URL = "";

	// Blueprint 이벤트 브로드캐스트
	OnError.Broadcast(Error);
}

// ============================================================================
// Internal Methods
// ============================================================================

void UVoiceWebSocketComponent::HandlePingMessage(const FWebSocketPingMessage& PingMsg)
{
	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] PING 수신: %s (v1.1 재사용)"), *PingMsg.timestamp);

	// PONG 응답
	SendPong();
}

void UVoiceWebSocketComponent::HandleSTTResult(const FCommanderSTTResult2& ResultMsg)
{
	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] STT Result (v1.2.0): Phase=%s, Text=\"%s\", Confidence=%.2f"),
		*ResultMsg.phase, *ResultMsg.text, ResultMsg.confidence);

	// Blueprint 이벤트 브로드캐스트
	OnSTTResultReceived.Broadcast(ResultMsg.phase, ResultMsg.text, ResultMsg.confidence);

	// Final 단계인 경우 추가 처리 (선택적)
	if (ResultMsg.IsFinal())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] STT Final Result 수신 완료"));
	}
}

void UVoiceWebSocketComponent::HandleTTSPush(const FTTSPushEvent2& PushMsg)
{
	UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] TTS Push (v1.2.0): Mode=%s, Text=\"%s\""),
		*PushMsg.tts_mode, *PushMsg.display_text);

	TArray<uint8> decodedAudioData;
	int32 LocalClipIndex = PushMsg.IsLocalClipMode() ? PushMsg.local_clip_index : -1;
	
	// SERVER_AUDIO 모드인 경우
	if (LocalClipIndex < 0 /*PushMsg.IsServerAudioMode()*/)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] TTS SERVER_AUDIO: Format=%s, Base64 Length=%d"),
			*PushMsg.audio_format, PushMsg.audio_base64.Len());
		
		if (! FBase64::Decode(PushMsg.audio_base64, decodedAudioData) )
		{
			UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] Base64 TTS의 Decoding을 실패하였습니다."));
			decodedAudioData.Empty();
		}
	}
	// LOCAL_CLIP 모드인 경우
	else /*if (PushMsg.IsLocalClipMode())*/
	{
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] TTS LOCAL_CLIP: Index=%d, CueID=%s"),
			PushMsg.local_clip_index, *PushMsg.cue_id);
		// TODO: 로컬 사운드 클립 재생 로직
	}

	// Blueprint 이벤트 브로드캐스트. TODO: LocalClipIndex < 0 및 decodedAudioData.Num() > 0 검증
	OnTTSPushReceived.Broadcast(LocalClipIndex, PushMsg.display_text, decodedAudioData);
}

void UVoiceWebSocketComponent::SendPong()
{
	if (!WebSocket.IsValid() || !WebSocket->IsConnected())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] SendPong() - WebSocket이 연결되지 않았습니다"));
		return;
	}

	// v1.1 재사용: FWebSocketPongMessage
	FWebSocketPongMessage PongMsg;
	PongMsg.type = TEXT("pong");
	PongMsg.timestamp = FDateTime::UtcNow().ToIso8601();

	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(PongMsg, JsonString))
	{
		WebSocket->Send(JsonString);
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] PONG 전송 (v1.1 재사용)"));
	}
	else
	{
		UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] PONG 직렬화 실패!"));
	}
}

void UVoiceWebSocketComponent::SetState(EWebSocketState NewState)
{
	if (State == NewState)
	{
		return;
	}

	EWebSocketState OldState = State;
	State = NewState;

	if (stateEnumPtr)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] State 변경: %s -> %s"),
			*stateEnumPtr->GetValueAsString(OldState), *stateEnumPtr->GetValueAsString(NewState));
		
	}
}

void UVoiceWebSocketComponent::HandleError(const FString& ErrorMessage)
{
	UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] 오류: %s"), *ErrorMessage);

	LastErrorMessage = ErrorMessage;
	SetState(EWebSocketState::Error);

	// 전송 중이었다면 중지
	if (TargetRacer != ETargetRacer::None)
	{
		if (VoiceCaptureComponent && VoiceCaptureComponent->IsCapturing())
		{
			VoiceCaptureComponent->StopCapture();
		}
		TargetRacer = ETargetRacer::None;
	}
}

#if WITH_STEAMWORKS
#include "steam/isteamuser.h"
#endif

void UVoiceWebSocketComponent::OnVoiceDataCaptured(const TArray<uint8>& VoiceData)
{
	// 전송 중이 아니면 무시
	if (State != EWebSocketState::Transmitting)
	{
		return;
	}
	
	FCommanderSTTEvent2 STTEvent;
	// Room ID, Speaker 정보 설정
	STTEvent.room_id = ConnectionParams.RoomID;
	STTEvent.commander_id = ConnectionParams.CommanderID;
	STTEvent.speaker.player_id = ConnectionParams.CommanderID/*FBase64::Encode(ConnectionParams.CommanderID, EBase64Mode::UrlSafe)*/;
	STTEvent.speaker.player_name = TEXT("Commander");  // TODO: 실제 닉네임 가져오기
	STTEvent.speaker.player_type = TEXT("COMMANDER");
	STTEvent.target.player_id = "TestTargetPlayerID";
	STTEvent.target.player_name = "TestTargetPlayerName";
	STTEvent.target.player_type = "Racer";
	
#if STEAM_SDK_ENABLED
	uint32 cbVoiceBufferSize = VoiceData.Num() * 10;
	if (cbVoiceBufferSize < 5120) cbVoiceBufferSize = 5120;
	OUT TArray<uint8> voiceData;
	voiceData.SetNum(cbVoiceBufferSize);
	OUT uint32 bytesRead = 0;
		
	EVoiceResult decompressResult = SteamUser()->DecompressVoice(
		VoiceData.GetData(),
		VoiceData.Num(),
		voiceData.GetData(),
		cbVoiceBufferSize,     
		&bytesRead,
		16000
	);

	STTEvent.audio.format = TEXT("PCM");
	if (decompressResult == EVoiceResult::k_EVoiceResultOK && bytesRead > 0)
	{
		voiceData.SetNum(bytesRead);
		SendAudioChunk(STTEvent, voiceData);
		return;
	}
	UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] Decompress Result : %d"), decompressResult);
	
#endif
}

void UVoiceWebSocketComponent::SendAudioChunk(FCommanderSTTEvent2& STTEvent, const TArray<uint8>& chunkData)
{
	// 전송 중이 아니면 무시
	if (State != EWebSocketState::Transmitting)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] SendAudioChunk() - 전송 중이 아닙니다. State=%d"), static_cast<int32>(State));
		return;
	}

	// WebSocket 연결 확인
	if (!WebSocket.IsValid() || !WebSocket->IsConnected())
	{
		UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] SendAudioChunk() - WebSocket이 연결되지 않았습니다"));
		HandleError(TEXT("WebSocket disconnected during transmission"));
		return;
	}

	// 데이터 검증
	if (chunkData.Num() == 0)
	{
		UE_LOG(CitRushVoiceWebSocketLog, Warning, TEXT("[VoiceWebSocket] SendAudioChunk() - Opus 데이터가 비어있습니다"));
		return;
	}

	// Phase 결정: 첫 청크는 "start", 이후는 "partial"
	if (bFirstAudioChunk)
	{
		STTEvent.phase = TEXT("start");
		bFirstAudioChunk = false;
		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] STT Event phase: start"));
	}
	else
	{
		STTEvent.phase = TEXT("partial");
	}

	// Base64로 인코딩
	STTEvent.audio.base64 = FBase64::Encode(chunkData);
	STTEvent.audio.sample_rate = 16000;
	STTEvent.chunk_index = AudioSequence++;
	
	// TODO: Debug용 Voice Chunk 지우기
	/*FFileHelper::SaveStringToFile(
			TEXT("############ Raw uint8[] Data ############\n")
			+ FString::FromBlob(chunkData.GetData(), chunkData.Num())
			+ TEXT("\n\n############ Base64 Data ############\n")
			+ STTEvent.audio.base64
		, *(SavePath / FString::FromInt(AudioSequence) + TEXT(".txt"))
		, FFileHelper::EEncodingOptions::ForceUTF8
	);*/
	
	// 타임스탬프
	STTEvent.timestamp = FDateTime::UtcNow().ToIso8601();

	// JSON 직렬화
	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(STTEvent, JsonString))
	{
		// JSON Text 프레임으로 전송
		WebSocket->Send(JsonString);

		UE_LOG(CitRushVoiceWebSocketLog, Log, TEXT("[VoiceWebSocket] STT Event 전송 - Phase=%s, Size=%d bytes, Seq=%d"),
			*STTEvent.phase, chunkData.Num(), AudioSequence - 1);
	}
	else
	{
		UE_LOG(CitRushVoiceWebSocketLog, Error, TEXT("[VoiceWebSocket] STT Event 직렬화 실패!"));
	}
}

