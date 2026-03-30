// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ResponseHttp.h"  // 기존 v1.1 구조체 재사용
#include "HttpResponse2.generated.h"

/**
 * AI Agent HTTP API v1.4.0 - Response Structures
 *
 * 이 파일은 AI 서버로부터 수신할 HTTP Response 구조체 v1.4.0을 정의합니다.
 * Protocol 문서: AgentLog/[AI 에이전트]언리얼팀 통합 연동 가이드_v1.4.0
 *
 * 주요 응답:
 * - POST /api/v1/match/start → Response
 * - POST /api/v1/get_decision → Response (핵심)
 * - POST /api/v1/match/end → Response
 * - GET /api/v1/health → Response
 * - POST /api/v1/overseer/tts → Response (NEW in v1.2.0)
 * - GET /api/v1/commander/report → Response (NEW in v1.2.0)
 *
 * v1.4.0 주요 변경사항:
 * - request_num 응답 필드 추가 (요청 순서 검증용)
 * - Match End Response meta 구조 변경 (cleanup_latency_ms, report_enabled 추가)
 * - Match End Statistics 필드명 변경 (total_ai_decisions, avg_decision_latency_ms)
 *
 * v1.2.0 변경사항:
 * - Overseer TTS API 추가 (LOCAL_CLIP, SERVER_AUDIO 모드)
 * - Commander Report API 추가
 * - overseer_tts_trigger 힌트 추가 (get_decision 응답에 포함)
 *
 * 기존 v1.1 구조체 재사용:
 * - FHealthResponse, FMatchStartResponse (v1.1 호환)
 * - FMetaInfo, FDecisionInfo, FBrainCamData 등
 * - FErrorDetails, FErrorInfo, FFailedResponse
 */

// ============================================================================
// Overseer TTS Response (NEW in v1.2.0)
// ============================================================================

/**
 * TTS 모드 Enum v1.2.0
 */
UENUM(BlueprintType)
enum class ETTSMode2 : uint8
{
	/** 로컬 클립 재생 모드 (임시, 12/8 데모용) */
	LOCAL_CLIP    UMETA(DisplayName = "Local Clip"),

	/** 서버 오디오 모드 (최종, 동적 TTS) */
	SERVER_AUDIO  UMETA(DisplayName = "Server Audio")
};

/**
 * TTS 대상 정보 v1.2.0
 */
USTRUCT(BlueprintType)
struct FTTSTarget2
{
	GENERATED_BODY()

	/** 대상 플레이어 Steam ID (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString player_id;

	/** 대상 플레이어 닉네임 (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString player_name;

	/** 대상 플레이어 타입 (필수): "COMMANDER" 또는 "DRIVER" */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString player_type;
};

/**
 * Overseer TTS 응답 v1.2.0
 * POST /api/v1/overseer/tts
 *
 * AI 서버에서 TTS 큐를 조회한 응답입니다.
 * 두 가지 모드:
 * 1. LOCAL_CLIP: 로컬 사운드 클립 재생 (1~20)
 * 2. SERVER_AUDIO: 서버에서 Opus 인코딩된 음성 전송 (Base64)
 */
USTRUCT(BlueprintType)
struct FOverseerTTSResponse2
{
	GENERATED_BODY()

	/** 상태 (필수): "success", "no_tts", "failed" */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString status;

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString room_id;

	/** 이벤트 ID (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString event_id;

	/** 대상 정보 (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FTTSTarget2 target;

	/** HUD에 표시할 텍스트 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString display_text;

	/** TTS 모드 (필수): LOCAL_CLIP 또는 SERVER_AUDIO */
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

	/** 오디오 포맷 (SERVER_AUDIO 모드에서 필수): "OPUS", "MP3" 등 */
	UPROPERTY(BlueprintReadOnly, Category = "TTS|ServerAudio")
	FString audio_format;

	/** Base64 인코딩된 오디오 데이터 (SERVER_AUDIO 모드에서 필수) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS|ServerAudio")
	FString audio_base64;

	/** ======== 공통 필드 ======== */

	/** 생성 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "TTS")
	FString created_at;
};

// ============================================================================
// Commander Report Response (NEW in v1.2.0)
// ============================================================================

/**
 * 지휘관 강점/약점 항목 v1.2.0
 */
USTRUCT(BlueprintType)
struct FCommanderStrengthWeakness2
{
	GENERATED_BODY()

	/** 항목 제목 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString title;

	/** 항목 설명 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString description;

	/** 점수 0~100 (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	float score = 0.0f;
};

/**
 * 지휘관 리포트 통계 v1.2.0
 */
USTRUCT(BlueprintType)
struct FCommanderReportStats2
{
	GENERATED_BODY()

	/** 음성 지시 횟수 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report|Stats")
	int32 voice_command_count = 0;

	/** UI/버튼 지시 횟수 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report|Stats")
	int32 button_command_count = 0;

	/** 아이템 효율 점수 0~1 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report|Stats")
	float item_efficiency_score = 0.0f;

	/** 평균 대응 시간(초) (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report|Stats")
	float average_response_time_seconds = 0.0f;

	/** 플레이어별 관심 분포 (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "Report|Stats")
	TMap<FString, int32> player_attention_distribution;
};

/**
 * 지휘관 리포트 응답 v1.2.0
 * GET /api/v1/commander/report?room_id={room_id}
 *
 * 매치 종료 후 언제든 재조회 가능한 리포트입니다.
 */
USTRUCT(BlueprintType)
struct FCommanderReportResponse2
{
	GENERATED_BODY()

	/** 상태 (필수): "success", "not_found", "failed" */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString status;

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString room_id;

	/** 지휘관 Steam ID (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString commander_id;

	/** 지휘관 닉네임 (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString commander_name;

	/** 전체 점수 0~100 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	float overall_score = 0.0f;

	/** 전체 평가 등급 (선택): S, A, B, C, D */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString grade;

	/** 강점 목록 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	TArray<FCommanderStrengthWeakness2> strengths;

	/** 약점 목록 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	TArray<FCommanderStrengthWeakness2> weaknesses;

	/** 통계 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FCommanderReportStats2 statistics;

	/** 리포트 생성 시각 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString created_at;
};

// ============================================================================
// Get Decision Response v1.2.0 - Overseer TTS Trigger 추가
// ============================================================================

/**
 * Overseer TTS 트리거 힌트 v1.2.0
 *
 * /get_decision 응답에 포함되어, 언리얼이 TTS를 조회할 타이밍을 알립니다.
 */
USTRUCT(BlueprintType)
struct FOverseerTTSTrigger2
{
	GENERATED_BODY()

	/** TTS 대기 중 여부 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "Overseer")
	bool has_pending_tts = false;

	/** 게임 방 ID (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "Overseer")
	FString room_id;

	/** 이벤트 ID (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "Overseer")
	FString event_id;

	/** 트리거 이유 (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "Overseer")
	FString reason;

	/** 대상 플레이어 ID (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "Overseer")
	FString target_player_id;
};

/**
 * 의사결정 응답 v1.2.0
 * POST /api/v1/get_decision
 *
 * v1.2.0 변경사항:
 * - overseer_tts_trigger 필드 추가
 *
 * 기존 v1.1 구조체 재사용:
 * - FMetaInfo, FDecisionInfo, FBrainCamData
 */
USTRUCT(BlueprintType)
struct FGetDecisionResponse2
{
	GENERATED_BODY()

	/** 상태 (필수): "success" - v1.1 호환 */
	UPROPERTY(BlueprintReadOnly, Category = "Decision")
	FString status;

	/** 타임스탬프 ISO 8601 (필수) - v1.1 호환 */
	UPROPERTY(BlueprintReadOnly, Category = "Decision")
	FString timestamp;

	/** 게임 방 ID (필수) - v1.1 호환 */
	UPROPERTY(BlueprintReadOnly, Category = "Decision")
	FString room_id;

	/** 요청 식별자 (필수, v1.4.0)
	 * Request의 request_num과 일치해야 함
	 * 순서 검증에 사용
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Decision")
	FString request_num;

	/** 메타 정보 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadOnly, Category = "Decision")
	FMetaInfo meta;

	/** 의사결정 정보 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadOnly, Category = "Decision")
	FDecisionInfo decision;

	/** 브레인캠 데이터 (선택) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadOnly, Category = "Decision")
	FBrainCamData brain_cam_data;

	/** Overseer TTS 트리거 힌트 (선택, v1.2.0)
	 * has_pending_tts가 true이면 언리얼은 POST /api/v1/overseer/tts를 호출
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Decision|Overseer")
	FOverseerTTSTrigger2 overseer_tts_trigger;
};

// ============================================================================
// Match End Response v1.4.0
// ============================================================================

/**
 * Match End Meta 정보 v1.4.0
 * POST /api/v1/match/end 응답의 meta 필드
 */
USTRUCT(BlueprintType)
struct FMatchEndMeta2
{
	GENERATED_BODY()

	/** 응답 스펙 버전 (필수)
	 * 예: "1.4.0"
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FString version;

	/** match/end 요청 처리 지연 시간(ms) (필수)
	 * 서버가 요청 수신부터 응답 생성 완료까지 걸린 시간
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	float cleanup_latency_ms = 0.0f;

	/** 보고서 기능 활성 여부 (필수)
	 * v1.4.0 베타: false 고정 (보고서 생성/반환 안 함)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	bool report_enabled = false;
};

/**
 * Match End Statistics v1.4.0
 * 경기 통계 정보 (베타 최소 필드)
 */
USTRUCT(BlueprintType)
struct FMatchEndStatistics2
{
	GENERATED_BODY()

	/** 경기 동안 /get_decision 호출된 총 횟수 (선택)
	 * 기존: total_decisions
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	int32 total_ai_decisions = 0;

	/** /get_decision 요청당 평균 응답 지연 시간(ms) (선택)
	 * 기존: average_response_time_ms
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	float avg_decision_latency_ms = 0.0f;
};

/**
 * Match End Response v1.4.0
 * POST /api/v1/match/end 응답
 *
 * v1.4.0 변경사항:
 * - meta 구조 변경 (cleanup_latency_ms, report_enabled 추가)
 * - statistics 필드명 변경 (total_ai_decisions, avg_decision_latency_ms)
 * - commander_report는 null 고정 (베타)
 */
USTRUCT(BlueprintType)
struct FMatchEndResponse2
{
	GENERATED_BODY()

	/** 상태 (필수): "success" */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FString status;

	/** 생성 시각 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FString generated_at;

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FString room_id;

	/** 메시지 (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FString message;

	/** 매치 결과 (필수): PLAYERS_WIN, ENEMY_WIN, DRAW, ABORTED */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FString result;

	/** 승리 팀 (선택): PLAYERS, ENEMY */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FString winner_team;

	/** 종료 사유 (선택): PACMAN_DEAD, TIME_OVER, SURRENDER, DISCONNECT */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FString end_reason;

	/** 실제 경기 진행 시간(초) (선택) */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	int32 game_duration_seconds = 0;

	/** 메타 정보 (필수, v1.4.0) */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FMatchEndMeta2 meta;

	/** 통계 정보 (필수) */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FMatchEndStatistics2 statistics;

	/** Commander 보고서 (선택)
	 * v1.4.0 베타: 항상 null (보고서 생성 안 함)
	 * JSON 파싱 시 필드 존재 여부만 확인, 내용은 무시
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MatchEnd")
	FString commander_report; // null 표현용 빈 문자열
};

// ============================================================================
// Type Aliases (기존 v1.1 구조체 재사용)
// ============================================================================

/**
 * 다음 v1.1 구조체들은 v1.2.0에서도 그대로 사용 가능:
 * - FHealthResponse (헬스체크)
 * - FMatchStartResponse, FMatchEndResponse, FMatchStatistics (매치 시작/종료)
 * - FErrorDetails, FErrorInfo, FFailedResponse (에러 처리)
 * - FWebVectorResponse (공통 구조체)
 * - FMetaInfo, FDecisionInfo, FBrainCamData (의사결정 응답)
 * - FUnitCommand, FUnitCommandParams (유닛 명령)
 * - FBrainCamPerception, FBrainCamReasoning, FBrainCamDecision (브레인캠)
 * - FRetrievedDoc (전술 문서)
 */
