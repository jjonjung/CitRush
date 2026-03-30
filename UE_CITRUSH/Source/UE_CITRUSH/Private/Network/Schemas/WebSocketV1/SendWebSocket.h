// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "SendWebSocket.generated.h"

/**
 * AI Agent WebSocket API v1.1 - Send Structures
 *
 * 이 파일은 AI 서버로 전송할 WebSocket 메시지 구조체를 정의합니다.
 * Protocol 문서: AgentLog/Protocol.md, AgentLog/NetworkModule.md
 *
 * WebSocket 엔드포인트:
 * - ws://{server}:8000/ws/v1/overseer/stt
 *
 * 전송 데이터:
 * 1. PCM 오디오 데이터 (Binary 프레임)
 * 2. PONG 메시지 (JSON Text 프레임)
 * 3. Session Start/End (아직 논의 전)
 */

// ============================================================================
// WebSocket Connection Parameters
// ============================================================================

/**
 * WebSocket STT 연결 파라미터
 *
 * 연결 URL 생성에 사용되는 파라미터입니다.
 * ws://{server}:8000/ws/v1/overseer/stt?room_id={room_id}&commander_id={commander_id}
 */
USTRUCT(BlueprintType)
struct FWebSocketSTTConnectionParams
{
	GENERATED_BODY()

	/** 서버 IP 주소 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString ServerIP = "";

	/** 서버 포트 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 ServerPort = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Version = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Service = "";
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FString, FString> Params;

	/**
	 * WebSocket 연결 URL 생성
	 * @return ws://{ServerIP}:{ServerPort}/
	 */
	FString GenerateURL() const
	{
		FString URL = "ws://";
		URL += ServerPort > 0 ?
			FString::Printf(TEXT("%s:%d/"),	*ServerIP, ServerPort)
			: FString::Printf(TEXT("%s/"), *ServerIP)
		;
		URL += FString::Printf(TEXT("ws/v%d/"), Version);
		URL += Service;
		FString IntegratedParam(TEXT("?"));
		for (TPair<FString, FString> Param : Params)
		{
			if (GetNum(IntegratedParam) > 1)
			{
				IntegratedParam += TEXT("&");
			}//commander?room_id=ROOM_001&commander_id=COMMANDER_001
			IntegratedParam += Param.Key + TEXT("=") + Param.Value;
		}
		return URL + IntegratedParam;
	}

	/**
	 * WebSocket Protocols 생성
	 * @return ws/v1/overseer/stt?room_id={RoomID}&commander_id={CommanderID}
	 */
	TArray<FString> GenerateProtocols() const
	{
		/*TArray<FString> Protocols;
		Protocols.Add(TEXT("ws://"));		
		return Protocols; */
		return {};
	}

	/** 파라미터 유효성 검증 */
	bool IsValid() const
	{
		return !ServerIP.IsEmpty() && Version > 0;
	}
};

// ============================================================================
// Heartbeat PONG Message (JSON)
// ============================================================================

/**
 * Heartbeat PONG 응답 메시지
 *
 * 서버가 PING 메시지를 보내면 클라이언트는 5초 이내에 PONG을 응답해야 합니다.
 * JSON Text 프레임으로 전송합니다.
 *
 * 전송 방식: WebSocket->Send(JsonString) [Text Frame]
 */
USTRUCT(BlueprintType)
struct FWebSocketPongMessage
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): "pong" */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString type;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString timestamp;

	FWebSocketPongMessage()
	{
		type = TEXT("pong");
		timestamp = FDateTime::UtcNow().ToIso8601();
	}
};

// ============================================================================
// Session Management Messages (JSON)
// ============================================================================

/**
 * STT 세션 시작 메시지
 *
 * 지휘관이 특정 Racer에게 음성 지시를 시작할 때 전송합니다.
 * Target Racer가 변경될 때마다 새로운 세션이 시작됩니다.
 *
 * 전송 방식: WebSocket->Send(JsonString) [Text Frame]
 *
 * 사용 시나리오:
 * 1. 지휘관이 Racer1 선택 → session_start (target_racer=1) 전송
 * 2. PCM 청크들 전송...
 * 3. 지휘관이 Racer2로 변경 → 현재 버퍼 flush → session_start (target_racer=2) 전송
 * 4. 새로운 PCM 청크들 전송...
 */
USTRUCT(BlueprintType)
struct FWebSocketSessionStart
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): "session_start" */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString type;

	/** 대상 Racer ID (필수): 1=Racer1, 2=Racer2, 3=Racer3 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 target_racer = 0;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString timestamp;

	FWebSocketSessionStart()
	{
		type = TEXT("session_start");
		timestamp = FDateTime::UtcNow().ToIso8601();
	}

	FWebSocketSessionStart(int32 InTargetRacer)
		: target_racer(InTargetRacer)
	{
		type = TEXT("session_start");
		timestamp = FDateTime::UtcNow().ToIso8601();
	}
};

/**
 * STT 세션 종료 메시지 (선택적)
 *
 * 지휘관이 마이크를 완전히 끌 때 전송합니다.
 * 서버가 현재 STT 버퍼를 최종 처리하도록 알립니다.
 *
 * 전송 방식: WebSocket->Send(JsonString) [Text Frame]
 *
 * 참고: Target 변경 시에는 session_end 없이 바로 새로운 session_start를 보내면 됩니다.
 * 서버는 새 session_start를 받으면 이전 세션을 자동 종료합니다.
 */
USTRUCT(BlueprintType)
struct FWebSocketSessionEnd
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): "session_end" */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString type;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString timestamp;

	FWebSocketSessionEnd()
	{
		type = TEXT("session_end");
		timestamp = FDateTime::UtcNow().ToIso8601();
	}
};

// ============================================================================
// Audio PCM Data (Binary)
// ============================================================================

/**
 * PCM 오디오 청크 데이터
 *
 * WebSocket Binary 프레임으로 전송되는 오디오 데이터입니다.
 * USTRUCT가 아닌 TArray<uint8>를 직접 사용합니다.
 *
 * 오디오 포맷:
 * - 샘플레이트: 16,000 Hz
 * - 채널: Mono (1채널)
 * - 포맷: 16-bit PCM, little-endian (signed int16)
 * - 청크 크기: 100~200ms 분량
 *   - 100ms = 16,000 * 0.1s * 2 byte = 3,200 bytes
 *   - 200ms = 16,000 * 0.2s * 2 byte = 6,400 bytes
 *
 * 전송 방식:
 * WebSocket->Send(PCMData.GetData(), PCMData.Num(), true) [Binary Frame]
 *
 * 사용 예시:
 * @code
 * TArray<uint8> PCMData;
 * CaptureMicrophoneAudio(PCMData);  // 마이크에서 PCM 데이터 수집
 * WebSocket->Send(PCMData.GetData(), PCMData.Num(), true);
 * @endcode
 */
struct FAudioPCMChunk
{
	/** PCM 오디오 데이터 (Raw Binary) */
	TArray<uint8> Data;

	/** 샘플 수 (참고용) */
	int32 SampleCount = 0;

	/** 지속 시간(ms) (참고용) */
	float DurationMs = 0.0f;

	FAudioPCMChunk()
		: SampleCount(0)
		, DurationMs(0.0f)
	{
	}

	/**
	 * PCM 데이터 설정
	 * @param InData - 16-bit PCM 데이터 배열
	 * @param SampleRate - 샘플레이트 (기본: 16000 Hz)
	 */
	void SetPCMData(const TArray<uint8>& InData, int32 SampleRate = 16000)
	{
		Data = InData;
		// 16-bit = 2 bytes per sample
		SampleCount = InData.Num() / 2;
		DurationMs = (SampleCount / static_cast<float>(SampleRate)) * 1000.0f;
	}

	/** 데이터 크기 (bytes) */
	int32 GetDataSize() const
	{
		return Data.Num();
	}

	/** 데이터가 유효한지 확인 */
	bool IsValid() const
	{
		return Data.Num() > 0 && SampleCount > 0;
	}

	/** 권장 청크 크기 범위 확인 (100~200ms) */
	bool IsRecommendedSize() const
	{
		return DurationMs >= 100.0f && DurationMs <= 200.0f;
	}
};

// ============================================================================
// Utility Functions (유틸리티)
// ============================================================================

/**
 * WebSocket Send 유틸리티 클래스
 *
 * WebSocket 전송 관련 헬퍼 함수들입니다.
 */
class FWebSocketSendUtils
{
public:
	/**
	 * PONG 메시지를 JSON 문자열로 직렬화
	 * @param OutJsonString - 생성된 JSON 문자열
	 * @return 성공 여부
	 */
	static bool SerializePongMessage(FString& OutJsonString)
	{
		FWebSocketPongMessage PongMsg;
		// FJsonObjectConverter 사용하여 직렬화
		return FJsonObjectConverter::UStructToJsonObjectString(PongMsg, OutJsonString);
	}

	/**
	 * Session Start 메시지를 JSON 문자열로 직렬화
	 * @param TargetRacer - 대상 Racer ID (1=Racer1, 2=Racer2, 3=Racer3)
	 * @param OutJsonString - 생성된 JSON 문자열
	 * @return 성공 여부
	 */
	static bool SerializeSessionStart(int32 TargetRacer, FString& OutJsonString)
	{
		FWebSocketSessionStart SessionMsg(TargetRacer);
		return FJsonObjectConverter::UStructToJsonObjectString(SessionMsg, OutJsonString);
	}

	/**
	 * Session End 메시지를 JSON 문자열로 직렬화
	 * @param OutJsonString - 생성된 JSON 문자열
	 * @return 성공 여부
	 */
	static bool SerializeSessionEnd(FString& OutJsonString)
	{
		FWebSocketSessionEnd SessionMsg;
		return FJsonObjectConverter::UStructToJsonObjectString(SessionMsg, OutJsonString);
	}

	/**
	 * PCM 데이터 유효성 검증
	 * @param PCMData - 검증할 PCM 데이터
	 * @param MinSize - 최소 크기 (bytes)
	 * @param MaxSize - 최대 크기 (bytes)
	 * @return 유효 여부
	 */
	static bool ValidateVoiceData(const TArray<uint8>& voiceData, int32 MinSize = 3200, int32 MaxSize = 6400)
	{
		return voiceData.Num() >= MinSize && voiceData.Num() <= MaxSize;
	}

	/**
	 * PCM 데이터 크기를 지속시간(ms)으로 변환
	 * @param DataSize - 데이터 크기 (bytes)
	 * @param SampleRate - 샘플레이트 (기본: 16000 Hz)
	 * @return 지속시간 (ms)
	 */
	static float VoiceDataSizeToDuration(int32 DataSize, int32 SampleRate = 16000)
	{
		// 16-bit PCM = 2 bytes per sample
		int32 SampleCount = DataSize / 2;
		return (SampleCount / static_cast<float>(SampleRate)) * 1000.0f;
	}
};
