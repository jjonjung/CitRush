// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ReceiveWebSocket.h"  // 기존 v1.1 구조체 재사용
#include "SendWebSocket2.h"    // FSTTSpeaker2, FTTSTarget2 재사용을 위해
#include "Network/Schemas/HttpV1/HttpResponse2.h"     // FTTSTarget2 재사용을 위해
#include "ReceiveWebSocket2.generated.h"

/**
 * AI Agent WebSocket API v1.2.0 - Receive Structures
 *
 * 이 파일은 AI 서버로부터 수신할 WebSocket 메시지 구조체 v1.2.0을 정의합니다.
 * Protocol 문서: AgentLog/[AI 에이전트]언리얼팀 통합 연동 가이드_v1.2.0
 *
 * WebSocket 엔드포인트:
 * - ws://{server}:8000/ws/v1/commander (v1.2.0, Commander)
 *
 * 수신 데이터:
 * 1. Heartbeat PING 메시지 (JSON Text 프레임)
 * 2. STT 결과 메시지 (JSON Text 프레임)
 * 3. TTS Push 이벤트 (JSON Text 프레임, NEW in v1.2.0)
 * 4. 연결 종료 메시지
 *
 * v1.2.0 주요 변경사항:
 * - STT Result 변경 (phase별 응답: start/partial/final)
 * - TTS Push 이벤트 추가 (tts_mode: LOCAL_CLIP/SERVER_AUDIO)
 * - audio_base64 (Opus) 포함
 *
 * 기존 v1.1 구조체 재사용:
 * - FWebSocketPingMessage (Heartbeat PING)
 * - FWebSocketCloseInfo, FWebSocketErrorInfo (연결 관리)
 */

// ============================================================================
// STT Result Message v1.2.0
// ============================================================================

/**
 * STT 인식 결과 메시지 v1.2.0
 * ws://{server}:8000/ws/v1/commander
 *
 * AI 서버가 음성을 텍스트로 변환한 결과를 전송합니다.
 * JSON Text 프레임으로 수신합니다.
 *
 * v1.2.0 변경사항:
 * - phase 필드 추가 (start/partial/final)
 * - speaker 정보 추가
 */
USTRUCT(BlueprintType)
struct FCommanderSTTResult2
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): "stt_result" */
	UPROPERTY(BlueprintReadOnly, Category = "STT")
	FString type;

	/** 결과 단계 (필수): "start", "partial", "final" */
	UPROPERTY(BlueprintReadOnly, Category = "STT")
	FString phase;

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "STT")
	FString room_id;

	/** 화자 정보 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "STT")
	FSTTSpeaker2 speaker;

	/** 인식된 텍스트 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "STT")
	FString text;

	/** 신뢰도 0~1 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "STT")
	float confidence = 0.0f;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "STT")
	FString timestamp;

	/** STT 결과 메시지인지 확인 */
	bool IsSTTResult() const
	{
		return type.Equals(TEXT("stt_result"), ESearchCase::IgnoreCase);
	}

	/** Final 단계 결과인지 확인 */
	bool IsFinal() const
	{
		return phase.Equals(TEXT("final"), ESearchCase::IgnoreCase);
	}

	/** Partial 단계 결과인지 확인 */
	bool IsPartial() const
	{
		return phase.Equals(TEXT("partial"), ESearchCase::IgnoreCase);
	}

	/** 신뢰도가 임계값 이상인지 확인 */
	bool IsConfidentEnough(float Threshold = 0.7f) const
	{
		return confidence >= Threshold;
	}
};

// ============================================================================
// TTS Push Event (NEW in v1.2.0)
// ============================================================================

/**
 * Overseer TTS Push 이벤트 v1.2.0
 * ws://{server}:8000/ws/v1/commander
 *
 * Overseer가 지휘관에게 음성/텍스트 알림을 보낼 때 사용합니다.
 * JSON Text 프레임으로 수신합니다.
 *
 * 두 가지 모드:
 * 1. LOCAL_CLIP: 로컬 사운드 클립 재생 (1~20)
 * 2. SERVER_AUDIO: 서버에서 Opus 인코딩된 음성 전송 (Base64)
 */
USTRUCT(BlueprintType)
struct FTTSPushEvent2
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): "tts_push" */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString type;

	/** 소스 (필수): "overseer" */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString source;

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString room_id;

	/** 이벤트 ID (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString event_id;

	/** 대상 정보 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FTTSTarget2 target;

	/** HUD에 표시할 텍스트 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString display_text;

	/** TTS 모드 (필수): "LOCAL_CLIP" 또는 "SERVER_AUDIO" */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString tts_mode;

	/** ======== LOCAL_CLIP 모드 전용 필드 ======== */

	/** 로컬 클립 인덱스 1~20 (LOCAL_CLIP 모드에서 필수) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS|LocalClip")
	int32 local_clip_index = 0;

	/** 사운드 큐 ID (LOCAL_CLIP 모드에서 선택) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS|LocalClip")
	FString cue_id;

	/** ======== SERVER_AUDIO 모드 전용 필드 ======== */

	/** 오디오 포맷 (SERVER_AUDIO 모드에서 필수): "OPUS" */
	UPROPERTY(BlueprintReadOnly, Category = "TTS|ServerAudio")
	FString audio_format;

	/** Base64 인코딩된 Opus 오디오 데이터 (SERVER_AUDIO 모드에서 필수) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS|ServerAudio")
	FString audio_base64;

	/** ======== 공통 필드 ======== */

	/** 생성 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString created_at;

	/** TTS Push 이벤트인지 확인 */
	bool IsTTSPush() const
	{
		return type.Equals(TEXT("tts_push"), ESearchCase::IgnoreCase);
	}

	/** LOCAL_CLIP 모드인지 확인 */
	bool IsLocalClipMode() const
	{
		return tts_mode.Equals(TEXT("LOCAL_CLIP"), ESearchCase::IgnoreCase);
	}

	/** SERVER_AUDIO 모드인지 확인 */
	bool IsServerAudioMode() const
	{
		return tts_mode.Equals(TEXT("SERVER_AUDIO"), ESearchCase::IgnoreCase);
	}
};

// ============================================================================
// Type Aliases (기존 v1.1 구조체 재사용)
// ============================================================================

/**
 * 다음 v1.1 구조체들은 v1.2.0에서도 그대로 사용 가능:
 * - FWebSocketPingMessage (Heartbeat PING)
 * - FWebSocketCloseInfo (연결 종료 정보)
 * - FWebSocketErrorInfo (연결 오류 정보)
 *
 * Heartbeat 처리:
 * 1. 서버가 30초 비활동 시 PING 전송
 * 2. 클라이언트는 10초 이내에 PONG 응답
 * 3. 무응답 시 재연결
 *
 * WebSocket 종료 코드:
 * - 1000: Normal Closure (정상 종료)
 * - 1001: Going Away (서버/클라이언트 종료)
 * - 1002: Protocol Error (프로토콜 오류)
 * - 1006: Abnormal Closure (비정상 종료)
 * - 1011: Internal Server Error (서버 내부 오류)
 */
