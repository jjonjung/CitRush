// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ResponseHttp.generated.h"

/**
 * AI Agent HTTP API v1.1 - Response Structures
 *
 * 이 파일은 AI 서버로부터 수신할 HTTP Response 구조체를 정의합니다.
 * Protocol 문서: AgentLog/Protocol.md
 *
 * 주요 응답:
 * - POST /api/v1/match/start → Response
 * - POST /api/v1/get_decision → Response (핵심)
 * - POST /api/v1/match/end → Response
 * - GET /api/v1/health → Response
 */

// ============================================================================
// Common Response Structures (공통 응답 구조체)
// ============================================================================

/**
 * 3D 위치 벡터 (Response용)
 */
USTRUCT(BlueprintType)
struct FWebVectorResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float x = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float y = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float z = 0.0f;
};

/**
 * 에러 상세 정보
 */
USTRUCT(BlueprintType)
struct FErrorDetails
{
	GENERATED_BODY()

	/** 누락된 필드 목록 (선택) */
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> missing_fields;

	/** 타임아웃 시간(ms) (선택) */
	UPROPERTY(BlueprintReadOnly)
	int32 timeout_ms = 0;

	/** 경과 시간(ms) (선택) */
	UPROPERTY(BlueprintReadOnly)
	int32 elapsed_ms = 0;

	/** LLM 에러 메시지 (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString llm_error;

	/** 재시도 대기 시간(초) (선택) */
	UPROPERTY(BlueprintReadOnly)
	int32 retry_after_seconds = 0;

	/** 폴백 가능 여부 (선택) */
	UPROPERTY(BlueprintReadOnly)
	bool fallback_available = false;

	/** 타임스탬프 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString timestamp;
};

/**
 * 에러 정보
 */
USTRUCT(BlueprintType)
struct FErrorInfo
{
	GENERATED_BODY()

	/** 에러 코드 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString code;

	/** 에러 메시지 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString message;

	/** 에러 상세 정보 (선택) */
	UPROPERTY(BlueprintReadOnly)
	FErrorDetails details;
};

/**
 * 실패 응답 (공통)
 * HTTP 400, 408, 500 등
 */
USTRUCT(BlueprintType)
struct FFailedResponse
{
	GENERATED_BODY()

	/** 상태 (필수): "failed" */
	UPROPERTY(BlueprintReadOnly)
	FString status;

	/** 에러 정보 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FErrorInfo error;
};

// ============================================================================
// Health Check Response (헬스체크)
// ============================================================================

/**
 * 헬스체크 응답
 * GET /api/v1/health
 */
USTRUCT(BlueprintType)
struct FHealthResponse
{
	GENERATED_BODY()

	/** 상태 (필수): "healthy" */
	UPROPERTY(BlueprintReadOnly)
	FString status;

	/* 서비스 정보 (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString service;
	
	/** 버전 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString version;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString timestamp;
};

// ============================================================================
// Match Start/End Responses (매치 시작/종료)
// ============================================================================

/**
 * 매치 시작 응답
 * POST /api/v1/match/start
 */
USTRUCT(BlueprintType)
struct FMatchStartResponse
{
	GENERATED_BODY()

	/** 상태 (필수): "success" */
	UPROPERTY(BlueprintReadOnly)
	FString status;

	/** 메시지 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString message;
};

/**
 * 매치 종료 통계
 */
USTRUCT(BlueprintType)
struct FMatchStatistics
{
	GENERATED_BODY()

	/** 총 의사결정 요청 수 (필수) */
	UPROPERTY(BlueprintReadOnly)
	int32 total_decisions = 0;

	/** 평균 응답 시간(ms) (필수) */
	UPROPERTY(BlueprintReadOnly)
	float average_response_time_ms = 0.0f;
};

/**
 * 매치 종료 응답
 * POST /api/v1/match/end
 */
USTRUCT(BlueprintType)
struct FMatchEndResponse
{
	GENERATED_BODY()

	/** 상태 (필수): "success" */
	UPROPERTY(BlueprintReadOnly)
	FString status;

	/** 메시지 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString message;

	/** 통계 (선택) */
	UPROPERTY(BlueprintReadOnly)
	FMatchStatistics statistics;
};

// ============================================================================
// Get Decision Response (의사결정 응답) - 핵심 API
// ============================================================================

/**
 * 메타 정보
 */
USTRUCT(BlueprintType)
struct FMetaInfo
{
	GENERATED_BODY()

	/** AI 적(Enemy) 서버 버전 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString version;

	/** 규칙/지식 베이스 버전 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString ontology_version;

	/** 서버 처리 시간(ms) (필수) */
	UPROPERTY(BlueprintReadOnly)
	float latency_ms = 0.0f;

	/** 폴백 사용 여부 (필수) */
	UPROPERTY(BlueprintReadOnly)
	bool fallback_used = false;

	/** 처리 모듈 목록 (필수) */
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> agent_pipeline;

	/** LLM 사용 토큰 수 (필수) */
	UPROPERTY(BlueprintReadOnly)
	int32 llm_tokens = 0;
};

/**
 * 유닛 명령 파라미터
 */
USTRUCT(BlueprintType)
struct FUnitCommandParams
{
	GENERATED_BODY()

	/** target_position (선택) */
	UPROPERTY(BlueprintReadOnly)
	FWebVectorResponse target_position;

	UPROPERTY(BlueprintReadOnly)
	FWebVectorResponse safe_zone_position;

	/** speed_factor (선택) */
	UPROPERTY(BlueprintReadOnly)
	float speed_factor = 1.0f;

	/** priority (선택) - v1.4.0 integer */
	UPROPERTY(BlueprintReadOnly)
	int32 priority = 0;

	/** target_player_id (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString target_player_id;

	/** aggressiveness (선택): HIGH, MEDIUM, LOW */
	UPROPERTY(BlueprintReadOnly)
	FString aggressiveness;

	/** p_point_id (선택) - v1.4.0 추가 */
	UPROPERTY(BlueprintReadOnly)
	FString p_point_id;

	/** pellet_id (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString pellet_id;

	/** emergency_priority (선택) - v1.4.0 추가 (int) */
	UPROPERTY(BlueprintReadOnly)
	int32 emergency_priority = 0;

	/** patrol_zone (선택) - 복잡한 구조는 FJsonObjectWrapper 사용 고려 */
	UPROPERTY(BlueprintReadOnly)
	FString patrol_zone;

	/** guard_target (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString guard_target;

	/** flank_direction (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString flank_direction;

	/** formation_type (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString formation_type;

	/** fake_retreat_duration (선택) */
	UPROPERTY(BlueprintReadOnly)
	float fake_retreat_duration = 0.0f;

	/** counter_attack_position (선택) */
	UPROPERTY(BlueprintReadOnly)
	FWebVectorResponse counter_attack_position;
};

/**
 * 유닛 명령
 */
USTRUCT(BlueprintType)
struct FUnitCommand
{
	GENERATED_BODY()

	/** 유닛 ID (필수): enemy_main, clone_1 등 */
	UPROPERTY(BlueprintReadOnly)
	FString unit_id;

	/** 명령 코드 (필수): 1~12 */
	UPROPERTY(BlueprintReadOnly)
	int32 directive_code = 0;

	/** 명령 이름 (필수, 디버깅용): AMBUSH, MOVE_TO_LOCATION 등 */
	UPROPERTY(BlueprintReadOnly)
	FString directive_name;

	/** 명령 파라미터 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FUnitCommandParams params;

};

/**
 * 의사결정 정보
 */
USTRUCT(BlueprintType)
struct FDecisionInfo
{
	GENERATED_BODY()

	/** 스쿼드 목표 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString squad_objective;

	/** 추론 설명 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString reasoning;

	/** 유닛별 명령 목록 (필수) */
	UPROPERTY(BlueprintReadOnly)
	TArray<FUnitCommand> unit_commands;

	/** 신뢰도 0~1 (필수) */
	UPROPERTY(BlueprintReadOnly)
	float confidence = 0.0f;
};

/**
 * 브레인캠 - Perception (상황 인식)
 */
USTRUCT(BlueprintType)
struct FBrainCamPerception
{
	GENERATED_BODY()

	/** 제목 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString title;

	/** 요약 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString summary;

	/** 위협 수준 (선택): CRITICAL, HIGH, MEDIUM, LOW */
	UPROPERTY(BlueprintReadOnly)
	FString threat_level;
};

/**
 * 브레인캠 - 참고 전술 문서
 */
USTRUCT(BlueprintType)
struct FRetrievedDoc
{
	GENERATED_BODY()

	/** 전술 ID (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString tactic_id;

	/** 전술 제목 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString title;

	/** 유사도 0~1 (필수) */
	UPROPERTY(BlueprintReadOnly)
	float similarity = 0.0f;
};

/**
 * 브레인캠 - Reasoning (추론)
 */
USTRUCT(BlueprintType)
struct FBrainCamReasoning
{
	GENERATED_BODY()

	/** 제목 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString title;

	/** 참고한 전술 문서 목록 (필수) */
	UPROPERTY(BlueprintReadOnly)
	TArray<FRetrievedDoc> retrieved_docs;

	/** 선택된 전술 ID (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString selected_tactic_id;

	/** 추가 설명 (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString notes;
};

/**
 * 브레인캠 - Decision (최종 결정)
 */
USTRUCT(BlueprintType)
struct FBrainCamDecision
{
	GENERATED_BODY()

	/** 제목 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString title;

	/** 최종 선택 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString final_choice;

	/** 명령 요약 (선택) */
	UPROPERTY(BlueprintReadOnly)
	FString unit_commands_summary;
};

/**
 * 브레인캠 데이터 (AI 사고 과정 시각화)
 */
USTRUCT(BlueprintType)
struct FBrainCamData
{
	GENERATED_BODY()

	/** 상황 인식 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FBrainCamPerception perception;

	/** 추론 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FBrainCamReasoning reasoning;

	/** 최종 결정 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FBrainCamDecision decision;
};

/**
 * 의사결정 응답 (핵심 API)
 * POST /api/v1/get_decision
 */
USTRUCT(BlueprintType)
struct FGetDecisionResponse
{
	GENERATED_BODY()

	/** 상태 (필수): "success" */
	UPROPERTY(BlueprintReadOnly)
	FString status;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString timestamp;

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString room_id;

	/** 메타 정보 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FMetaInfo meta;

	/** 의사결정 정보 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FDecisionInfo decision;

	/** 브레인캠 데이터 (선택) */
	UPROPERTY(BlueprintReadOnly)
	FBrainCamData brain_cam_data;

	/** 폴백 사용 여부 (필수) */
	UPROPERTY(BlueprintReadOnly)
	bool fallback_used = false;
};

// ============================================================================
// OVERSEER TTS Response (AI → 언리얼)
// ============================================================================

/**
 * OVERSEER TTS 수신 응답
 * 언리얼이 AI 서버로부터 TTS 요청을 받았을 때 반환하는 응답
 */
USTRUCT(BlueprintType)
struct FOverseerTTSAcknowledge
{
	GENERATED_BODY()

	/** 상태 (필수): "ok" */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString status;

	/** 재생 여부 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool played = true;

	/** 메시지 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString message;
};
