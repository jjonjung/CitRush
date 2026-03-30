// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Private/Network/Schemas/WebSocketV1/SendWebSocket2.h"
#include "Private/Network/Schemas/WebSocketV1/ReceiveWebSocket2.h"
#include "Data/CitRushPlayerTypes.h"
#include "VoiceWebSocketComponent.generated.h"

class USteamVoiceComponent;

UENUM(BlueprintType)
enum class EWebSocketState : uint8
{
	Default = 0,		// 0프로그램 시작 초기
	Initialized,		// 1소켓 프로퍼티들 초기화
	Loaded,				// 2모듈 로드
	Connecting,			// 3연결 중 (비동기 연결 시도)
	Connected,			// 4연결됨 & 대기 중 (마이크 꺼짐)
	Transmitting,		// 5음성 전송 중 (특정 Racer에게 말하는 중)
	Disconnecting,		// 6연결 종료 중
	Disconnected,		// 7연결 안됨 및 종료
	Error				// 8오류
};

class IWebSocket;

/**
 * WebSocket STT(Speech-to-Text) 음성 전송 Component
 * Commander의 음성을 Opus로 인코딩하여 WebSocket 서버로 전송
 */

// ============================================================================
// Blueprint Delegates
// ============================================================================

/** WebSocket 연결 성공 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWebSocketConnected);

/** WebSocket 연결 종료 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWebSocketDisconnected, int32, StatusCode, FString, Reason, bool, bWasClean);

/** WebSocket 연결 오류 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebSocketError, FString, ErrorMessage);

/** STT Result 수신 v1.2.0 (phase 기반) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSTTResultReceived, FString, Phase, FString, Text, float, Confidence);

/** TTS Push 이벤트 수신 v1.2.0 (NEW) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTTSPushReceived,  int32, LocalClipIndex, FString, DisplayText, const TArray<uint8>&, AudioData);

UCLASS(ClassGroup=(CitRushNetwork), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UVoiceWebSocketComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(CitRushVoiceWebSocketLog, Log, All);

public:
	UVoiceWebSocketComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/* ========================================================================
	 * Public API
	 *	// 1. 서버 정보 설정
		VoiceWebSocket->ConnectionParams.ServerIP = "URL";
		VoiceWebSocket->ConnectionParams.ServerPort = 0 // 필요 없을 수 있음;

	 *	// 2. 연결
		VoiceWebSocket->Connect();

	 * 	// 3. 이벤트 바인딩
	  	VoiceWebSocket->OnConnected.AddDynamic(this, &AMyActor::HandleWebSocketConnected);
	  	VoiceWebSocket->OnDisconnected.AddDynamic(this, &AMyActor::HandleWebSocketDisconnected);
	  	
	 * 	// 4. 음성 전송 시작
	  	VoiceWebSocket->StartTransmissionTo(ETargetRacer::Racer1);
	  	
	 * 	// 5. 음성 전송 중지
	  	VoiceWebSocket->StopTransmission();
	  	
	 * 	// 6. 연결 종료
	  	VoiceWebSocket->Disconnect();
	 * ========================================================================*/
	
	UFUNCTION(BlueprintCallable, Category = "WebSocket|Connection")
	void Create();
	
	/** WebSocket 서버에 연결 */
	UFUNCTION(BlueprintCallable, Category = "WebSocket|Connection")
	void Connect();

	/** WebSocket 연결 종료 */
	UFUNCTION(BlueprintCallable, Category = "WebSocket|Connection")
	void Disconnect();

	/** 특정 Racer에게 음성 전송 시작 */
	UFUNCTION(BlueprintCallable, Category = "WebSocket|Voice")
	void StartTransmissionTo(ETargetRacer Target);

	/** 음성 전송 중지 */
	UFUNCTION(BlueprintCallable, Category = "WebSocket|Voice")
	void StopTransmission();

	/** 현재 타겟 Racer 가져오기 */
	UFUNCTION(BlueprintPure, Category = "WebSocket|Voice")
	FORCEINLINE ETargetRacer GetTargetRacer() const { return TargetRacer; }

	/** 현재 WebSocket 상태 가져오기 */
	UFUNCTION(BlueprintPure, Category = "WebSocket|Connection")
	FORCEINLINE EWebSocketState GetState() const { return State; }

	/** WebSocket 연결 여부 확인 */
	UFUNCTION(BlueprintPure, Category = "WebSocket|Connection")
	bool IsConnected() const { return State == EWebSocketState::Connected || State == EWebSocketState::Transmitting; }

	// ========================================================================
	// Blueprint Events
	// ========================================================================

	/** WebSocket 연결 성공 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "WebSocket|Events")
	FOnWebSocketConnected OnConnected;

	/** WebSocket 연결 종료 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "WebSocket|Events")
	FOnWebSocketDisconnected OnDisconnected;

	/** WebSocket 오류 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "WebSocket|Events")
	FOnWebSocketError OnError;

	/** STT Result 수신 이벤트 v1.2.0 */
	UPROPERTY(BlueprintAssignable, Category = "WebSocket|Events")
	FOnSTTResultReceived OnSTTResultReceived;

	/** TTS Push 이벤트 수신 v1.2.0 */
	UPROPERTY(BlueprintAssignable, Category = "WebSocket|Events")
	FOnTTSPushReceived OnTTSPushReceived;
	UFUNCTION(BlueprintCallable, Category = "WebSocket|Voice")
	bool RegisterCaptureComponent(UActorComponent* voiceActorComponent);
	
protected:
	// ========================================================================
	// Protected Methods
	// ========================================================================
	UFUNCTION(BlueprintCallable, Category = "WebSocket|Voice")
	void SendAudioChunk(FCommanderSTTEvent2& STTEvent, const TArray<uint8>& chunkData);

	/** 매치 종료 시 연결 종료 */
	void MatchEnd();

private:
	bool InitWebsocketProperties();
	
	// ========================================================================
	// WebSocket Callbacks
	// ========================================================================

	void OnWebSocketCreated_Internal(const TSharedPtr<IWebSocket>&  createdWebSocket, const TArray<FString>& Protocols, const FString& Url);
	
	/** WebSocket 연결 성공 콜백 */
	void OnWebSocketConnected_Internal();

	/** WebSocket 메시지 수신 콜백 */
	void OnWebSocketMessage_Internal(const FString& Message);

	/** WebSocket 연결 종료 콜백 */
	void OnWebSocketClosed_Internal(int32 StatusCode, const FString& Reason, bool bWasClean);

	/** WebSocket 연결 오류 콜백 */
	void OnWebSocketConnectionError_Internal(const FString& Error);

	// ========================================================================
	// Internal Methods
	// ========================================================================

	/** PING 메시지 처리 (v1.1 재사용) */
	void HandlePingMessage(const FWebSocketPingMessage& PingMsg);

	/** STT Result 메시지 처리 v1.2.0 */
	void HandleSTTResult(const FCommanderSTTResult2& ResultMsg);

	/** TTS Push 이벤트 처리 v1.2.0 */
	void HandleTTSPush(const FTTSPushEvent2& PushMsg);

	/** PONG 메시지 전송 (v1.1 재사용) */
	void SendPong();

	/** State 변경 */
	void SetState(EWebSocketState NewState);

	/** 오류 상태로 전환 */
	void HandleError(const FString& ErrorMessage);

protected:
	// ========================================================================
	// Configuration
	// ========================================================================

	/** WebSocket 연결 파라미터 v1.2.0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WebSocket|Config")
	FCommanderWebSocketConnectionParams2 ConnectionParams;

	/** VoiceCapture Component 참조 (자동으로 찾음) */
	UPROPERTY()
	USteamVoiceComponent* SteamVoiceComponent = nullptr;
	/** VoiceCapture Component 참조 (자동으로 찾음) */
	UPROPERTY()
	class UVoiceCaptureComponent* VoiceCaptureComponent = nullptr;



private:
	// ========================================================================
	// Delegate Callbacks
	// ========================================================================

	/** 음성 데이터 수신 콜백 (Opus 또는 PCM) - 통일된 콜백 */
	UFUNCTION()
	void OnVoiceDataCaptured(const TArray<uint8>& VoiceData);

	/** 현재 사용 중인 캡처 타입 */
	bool bUsingSteamVoice = false;

	/** STT Event phase 추적 (v1.2.0) */
	bool bFirstAudioChunk = true;

	/** 오디오 시퀀스 번호 (v1.2.0) */
	int32 AudioSequence = 0;

	// ========================================================================
	// State Variables
	// ========================================================================

	/** 현재 WebSocket 상태 */
	UPROPERTY(BlueprintReadOnly, Category = "WebSocket|State", meta = (AllowPrivateAccess = "true"))
	EWebSocketState State = EWebSocketState::Default;

	UPROPERTY()
	UEnum* stateEnumPtr = nullptr;

	/** 현재 타겟 Racer */
	UPROPERTY(BlueprintReadOnly, Category = "WebSocket|State", meta = (AllowPrivateAccess = "true"))
	ETargetRacer TargetRacer = ETargetRacer::None;

	/** WebSocket 인스턴스 */
	TSharedPtr<IWebSocket> WebSocket;

	/** 생성된 WebSocket URL */
	UPROPERTY(BlueprintReadOnly, Category = "WebSocket|Config", meta = (AllowPrivateAccess = "true"))
	FString URL;
	FString tempURL;

	/** 마지막 오류 메시지 */
	UPROPERTY(BlueprintReadOnly, Category = "WebSocket|State", meta = (AllowPrivateAccess = "true"))
	FString LastErrorMessage;

	//TODO : Debugging. 지우기
	FString SavePath = "";
	int32 count = 0;
};
