// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SendWebSocket.h"  // 기존 v1.1 구조체 재사용
#include "GenericPlatform/GenericPlatformHttp.h"
#include "SendWebSocket2.generated.h"

/**
 * AI Agent WebSocket API v1.2.0 - Send Structures
 *
 * 이 파일은 AI 서버로 전송할 WebSocket 메시지 구조체 v1.2.0을 정의합니다.
 * Protocol 문서: AgentLog/[AI 에이전트]언리얼팀 통합 연동 가이드_v1.2.0
 *
 * WebSocket 엔드포인트:
 * - ws://{server}:8000/ws/v1/commander (v1.2.0, Commander STT)
 *
 * 전송 데이터:
 * 1. Commander STT Events (JSON + Base64-Opus 오디오)
 * 2. Heartbeat PONG 메시지 (JSON Text 프레임)
 *
 * v1.2.0 주요 변경사항:
 * - Commander STT stt_event 추가 (start/partial/final)
 * - 오디오 포맷: Opus 바이너리 (Base64 인코딩)
 * - speaker 정보 포함
 *
 * 기존 v1.1 구조체 재사용:
 * - FWebSocketPongMessage (Heartbeat PONG)
 * - FWebSocketSTTConnectionParams (연결 파라미터)
 */

// ============================================================================
// Commander STT Event (NEW in v1.2.0)
// ============================================================================

/**
 * STT 이벤트 단계 Enum v1.2.0
 */
UENUM(BlueprintType)
enum class ESTTEventPhase2 : uint8
{
	/** 발화 시작 */
	Start     UMETA(DisplayName = "Start"),

	/** 중간 인식 결과 (부분) */
	Partial   UMETA(DisplayName = "Partial"),

	/** 최종 인식 결과 */
	Final     UMETA(DisplayName = "Final")
};

/**
 * STT 화자 정보 v1.2.0
 */
USTRUCT(BlueprintType)
struct FSTTSpeaker2
{
	GENERATED_BODY()

	/** 화자 Steam ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Speaker")
	FString player_id;

	/** 화자 닉네임 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Speaker")
	FString player_name;

	/** 화자 타입 (필수): "COMMANDER" 또는 "DRIVER" */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Speaker")
	FString player_type;
};

/**
 * STT 오디오 정보 v1.2.0
 */
USTRUCT(BlueprintType)
struct FSTTAudio2
{
	GENERATED_BODY()

	/** 오디오 포맷 (필수): "OPUS" */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Audio")
	FString format = TEXT("OPUS");
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Audio")
	int32 sample_rate = 16000; 
	
	/** Base64 인코딩된 Opus 오디오 데이터 (필수)
	 * Steam에서 받은 Opus 바이너리를 그대로 Base64로 인코딩
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Audio")
	FString base64;

	/*/** 오디오 청크 크기(bytes) (선택)
	 * 디버깅/로깅용
	 #1#
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Audio")
	int32 chunk_size = 0;

	/** 오디오 시퀀스 번호 (선택)
	 * 패킷 순서 추적용
	 #1#
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Audio")
	int32 sequence = 0;*/
};

USTRUCT(BlueprintType)
struct FCommanderSttTarget2
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Target")
	FString player_id;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Target")
	FString player_name;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT|Target")
	FString player_type;
};

/**
 * Commander STT 이벤트 v1.2.0
 * ws://{server}:8000/ws/v1/commander
 *
 * Commander의 음성을 AI 서버로 전송하는 메시지입니다.
 * JSON Text 프레임으로 전송합니다.
 */
USTRUCT(BlueprintType)
struct FCommanderSTTEvent2
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): "stt_event" */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	FString type = TEXT("stt_event");

	/** 이벤트 단계 (필수): "start", "partial", "final" */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	FString phase;

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	FString room_id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	FString commander_id;

	/** 화자 정보 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	FSTTSpeaker2 speaker;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	FCommanderSttTarget2 target;

	/** 오디오 정보 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	FSTTAudio2 audio;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	int32 chunk_index = -1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	bool is_silent = false;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "STT")
	FString timestamp;

	/** 기본 생성자 */
	FCommanderSTTEvent2()
	{
		type = TEXT("stt_event");
		timestamp = FDateTime::UtcNow().ToIso8601();
	}
};

// ============================================================================
// Connection Parameters v1.2.0
// ============================================================================

/**
 * Commander WebSocket 연결 파라미터 v1.2.0
 *
 * 연결 URL 생성에 사용되는 파라미터입니다.
 * ws://{server}:8000/ws/v1/commander?room_id={room_id}&commander_id={commander_id}
 */
USTRUCT(BlueprintType)
struct FCommanderWebSocketConnectionParams2
{
	GENERATED_BODY()

	/** 서버 IP 주소 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Connection")
	FString ServerIP;

	/** 서버 포트 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Connection")
	int32 ServerPort = 8000;

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Connection")
	FString RoomID;

	/** 지휘관 Steam ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Connection")
	FString CommanderID;

	/**
	 * WebSocket 연결 URL 생성
	 * @return ws://{ServerIP}:{ServerPort}/ws/v1/commander?room_id={RoomID}&commander_id={CommanderID}
	 *
	 * 한글 등 특수문자를 포함한 파라미터를 URL 인코딩하여 처리합니다.
	 */
	FString GenerateURL() const
	{
		// Query Parameter URL 인코딩 (한글 지원)
		//FString EncodedRoomID = FGenericPlatformHttp::UrlEncode(RoomID);
		//FString EncodedCommanderID = FGenericPlatformHttp::UrlEncode(CommanderID);

		return ServerPort > 0 ?
			FString::Printf(
				TEXT("ws://%s:%d/ws/v1/commander?room_id=%s&commander_id=%s"),
					*ServerIP, ServerPort, *RoomID, *CommanderID)
			: FString::Printf(
			TEXT("ws://%s/ws/v1/commander?room_id=%s&commander_id=%s"),
				*ServerIP, *RoomID, *CommanderID)
		;
	}

	/** 파라미터 유효성 검증 */
	bool IsValid() const
	{
		return !ServerIP.IsEmpty() && ServerPort > 0 && !RoomID.IsEmpty() && !CommanderID.IsEmpty();
	}
};

// ============================================================================
// Type Aliases (기존 v1.1 구조체 재사용)
// ============================================================================

/**
 * 다음 v1.1 구조체들은 v1.2.0에서도 그대로 사용 가능:
 * - FWebSocketPongMessage (Heartbeat PONG)
 *
 * Heartbeat 처리:
 * 1. 서버가 30초 비활동 시 PING 전송
 * 2. 클라이언트는 10초 이내에 PONG 응답
 * 3. 무응답 시 재연결
 */
