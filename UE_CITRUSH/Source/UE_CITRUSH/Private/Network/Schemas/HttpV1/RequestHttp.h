// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RequestHttp.generated.h"

/**
 * AI Agent HTTP API v1.1 - Request Structures
 *
 * 이 파일은 AI 서버로 전송할 HTTP Request 구조체를 정의합니다.
 * Protocol 문서: AgentLog/Protocol.md
 *
 * 주요 엔드포인트:
 * - GET  /api/v1/health
 * - POST /api/v1/match/start
 * - POST /api/v1/get_decision (핵심)
 * - POST /api/v1/match/end
 */

// ============================================================================
// Endpoint Configuration (엔드포인트 설정)
// ============================================================================

/**
 * HTTP 엔드포인트 설정
 *
 * 각 API 엔드포인트의 경로, 메서드, 타임아웃을 정의합니다.
 * Blueprint에서 수정 가능하며, HttpComponent에서 사용됩니다.
 */
USTRUCT(BlueprintType)
struct FHttpEndpointConfig
{
	GENERATED_BODY()

	/** 엔드포인트 경로 (예: /api/v1/health) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Endpoint")
	FString Path;

	/** HTTP 메서드 (GET, POST) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Endpoint")
	FString Method = TEXT("POST");

	/** 타임아웃 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Endpoint")
	float Timeout = 5.0f;

	FHttpEndpointConfig() = default;

	FHttpEndpointConfig(const FString& InPath, const FString& InMethod, float InTimeout = 5.0f)
		: Path(InPath)
		, Method(InMethod)
		, Timeout(InTimeout)
	{
	}
};

// ============================================================================
// Common Structures (공통 구조체)
// ============================================================================

/**
 * 3D 위치 벡터 (Web 통신용)
 */
USTRUCT(BlueprintType)
struct FWebVectorRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float x = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float y = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float z = 0.0f;
};

/**
 * 3D 속도 벡터 (Web 통신용)
 */
USTRUCT(BlueprintType)
struct FWebVelocityRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float x = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float y = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float z = 0.0f;
};

// ============================================================================
// Match Start/End Requests (매치 시작/종료)
// ============================================================================

/**
 * 매치 시작 요청
 * POST /api/v1/match/start
 */
USTRUCT(BlueprintType)
struct FMatchStartRequest
{
	GENERATED_BODY()

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString room_id;

	/** 참여 플레이어 ID 목록 (필수)
	 * Match 시작 시 신경써야함!!!
	 0 : 지휘자, 1 ~ 3 : 순서가 정해진 Racer
	 * 아래의 배열들 Index와 Racer의 순번(음성 Input 1,2,3) 과 매치되어야 함.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> player_ids;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> player_names;

	/** 맵 ID (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString map_id;

	/** 게임 모드 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString mode;

	/** 매치 시작 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString match_start_time;
};

/**
 * 플레이어 통계
 */
USTRUCT(BlueprintType)
struct FPlayerStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString player_id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 score = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 deaths = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 assists = 0;
};

/**
 * 매치 종료 요청
 * POST /api/v1/match/end
 */
USTRUCT(BlueprintType)
struct FMatchEndRequest
{
	GENERATED_BODY()

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString room_id;

	/** 매치 결과 (필수): PLAYERS_WIN, ENEMY_WIN, DRAW, ABORTED */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString result;

	/** 최종 점수 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 final_score = 0;

	/** 실제 경기 진행 시간(초) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 match_duration_seconds = 0;

	/** 승리 팀 식별자 (선택): PLAYERS, ENEMY */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString winner_team;

	/** 종료 사유 (선택): TIME_OVER, ENEMY_CAPTURED, SURRENDER, DISCONNECT */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString end_reason;

	/** 플레이어별 통계 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FPlayerStats> player_stats;
};

// ============================================================================
// Get Decision Request (의사결정 요청) - 핵심 API
// ============================================================================

/**
 * NavFrame 정보 (v1.4.0)
 * UE FNavSystemLLMData 매핑 데이터
 */
USTRUCT(BlueprintType)
struct FNavFrame
{
	GENERATED_BODY()

	/** 타임스탬프 (초) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float timestamp_sec = 0.0f;

	/** 델타 타임 (초) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float delta_time_sec = 0.0f;

	/** 드라이버 간 평균 거리 (cm) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float inter_driver_avg_distance = 0.0f;

	/** 드라이버 간 평균 거리 변화량 (cm) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float delta_inter_driver_avg_distance = 0.0f;
};

/**
 * 전역 게임 컨텍스트 (v1.4.0)
 *
 * v1.4.0 변경사항:
 * - nav_frame 추가 (NavSystemLLMData 매핑)
 */
USTRUCT(BlueprintType)
struct FGlobalContext
{
	GENERATED_BODY()

	/** 남은 경기 시간(초) (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float remaining_time = 0.0f;

	/** 경기 진행 단계 (필수): EARLY_GAME, MID_GAME, LATE_GAME */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString game_phase;

	/** 경기 시작 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString match_start_time;

	/** NavFrame 정보 (선택, v1.4.0) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FNavFrame nav_frame;
};

/**
 * 적(Enemy) 본체 상태
 */
USTRUCT(BlueprintType)
struct FEnemyMain
{
	GENERATED_BODY()

	/** 위치 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FWebVectorRequest position;

	/** 체력 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float hp = 100.0f;

	/** 포획 게이지 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float capture_gauge = 0.0f;

	/** 무적 상태 여부 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool is_invulnerable = false;
};

/**
 * 미니언(Minion) 유닛 상태 (적의 분신)
 */
USTRUCT(BlueprintType)
struct FMinionUnit
{
	GENERATED_BODY()

	/** 유닛 ID (필수): minion_1, minion_2 등 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString unit_id;

	/** 위치 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FWebVectorRequest position;

	/** 체력 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float hp = 1.0f;

	/** 생존 여부 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool is_alive = true;
};

/**
 * AI 스쿼드 컨텍스트 (적 본체 + 미니언)
 */
USTRUCT(BlueprintType)
struct FAISquadContext
{
	GENERATED_BODY()

	/** 적 본체 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FEnemyMain enemy_main;

	/** 미니언들 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FMinionUnit> minions;

	/** P-Pellet 재사용 대기 시간(초) (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float p_pellet_cooldown = 0.0f;

	/** 마지막 P-Pellet 사용 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString last_pellet_consumed_at;
};

/**
 * 플레이어 이벤트
 */
USTRUCT(BlueprintType)
struct FPlayerEvent
{
	GENERATED_BODY()

	/** 이벤트 타입 (필수): FUEL_LOW, HP_LOW, RISK_HIGH, STATION_CLOSED */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString event_type;

	/** 심각도 (필수): INFO, LOW, MEDIUM, HIGH, CRITICAL */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString severity;

	/** 현재 수치 값 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float value = 0.0f;

	/** 경고 기준 값 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float threshold = 0.0f;
};

/**
 * 장착한 룬 정보
 */
USTRUCT(BlueprintType)
struct FEquippedRune
{
	GENERATED_BODY()

	/** 룬 식별자 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString rune_id;

	/** 룬 이름 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString name;

	/** 남은 지속시간(초) (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float duration_remaining = 0.0f;
};

/**
 * 지휘관 상호작용 정보
 */
USTRUCT(BlueprintType)
struct FCommanderInteraction
{
	GENERATED_BODY()

	/** 마지막 음성(STT) 지시 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString last_voice_command_at;

	/** 마지막 UI/버튼 기반 지시 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString last_button_command_at;
};

/**
 * 플레이어/팀 컨텍스트
 */
USTRUCT(BlueprintType)
struct FPlayerTeamContext
{
	GENERATED_BODY()

	/** 플레이어 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString player_id;

	/** 역할 (필수): 1=COMMANDER, 2=DRIVER1, 3=DRIVER2, 4=DRIVER3 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 role = 2;

	/** 위치 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FWebVectorRequest position;

	/** 속도 벡터 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FWebVelocityRequest velocity;

	/** 체력 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float hp = 100.0f;

	/** 부스터 사용 중 여부 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool is_boosting = false;

	/** 최근 행동 라벨 (필수): back_attack_attempt, normal_drive, patrolling 등 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString recent_action;

	/** 이벤트 목록 (필수, 없으면 빈 배열) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FPlayerEvent> events;

	/** 장착한 룬 목록 (필수, 없으면 빈 배열) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FEquippedRune> equipped_runes;

	/** 지휘관 상호작용 정보 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FCommanderInteraction commander_interaction;
};

/**
 * 캐시 상태
 */
USTRUCT(BlueprintType)
struct FCashStatus
{
	GENERATED_BODY()

	/** 현재 사용 가능한 캐쉬 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 current_cash = 0;

	/** 이번 매치에서 사용한 캐쉬 총합 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 total_cash_spent = 0;

	/** 이번 매치에서 회수/획득한 캐쉬 총합 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 total_cash_collected = 0;
};

/**
 * 지휘관 최근 액션
 */
USTRUCT(BlueprintType)
struct FCommanderRecentAction
{
	GENERATED_BODY()

	/** 행동 발생 시각 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString timestamp;

	/** 행동 타입 (필수): GIVE_ITEM, SPEND_CASH, COLLECT_CASH, REFUND_CASH */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString action_type;

	/** 대상 플레이어 ID (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString target_player_id;

	/** 관련 아이템 ID (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString item_id;

	/** 캐쉬 변화량 (필수, 사용 시 음수, 회수 시 양수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 cash_delta = 0;
};

/**
 * 지휘관 관리 통계
 */
USTRUCT(BlueprintType)
struct FManagementStats
{
	GENERATED_BODY()

	/** 음성(STT) 지시 횟수 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 voice_command_count = 0;

	/** UI/버튼 지시 횟수 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 button_command_count = 0;

	/** 플레이어별 관심 횟수/비율 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FString, int32> player_attention_distribution;

	/** 아이템 효율 지수 0~1 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float item_efficiency_score = 0.0f;

	/** 평균 대응 속도(초) (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float average_response_time_seconds = 0.0f;
};

/**
 * 지휘관 컨텍스트
 */
USTRUCT(BlueprintType)
struct FCommanderContext
{
	GENERATED_BODY()

	/** 지휘관 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString commander_id;

	/** 캐시 상태 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FCashStatus cash_status;

	/** 최근 액션 목록 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FCommanderRecentAction> recent_actions;

	/** 관리 통계 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FManagementStats management_stats;
};

/**
 * P-Pellet 위치 정보
 */
USTRUCT(BlueprintType)
struct FPPelletLocation
{
	GENERATED_BODY()

	/** P-Pellet 개체 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString id;

	/** X 좌표 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float x = 0.0f;

	/** Y 좌표 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float y = 0.0f;

	/** Z 좌표 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float z = 0.0f;

	/** 현재 활성(먹을 수 있는) 상태 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool available = true;

	/** 리스폰 대기 시간(초) (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float cooldown = 0.0f;
};

/**
 * 맵 컨텍스트
 */
USTRUCT(BlueprintType)
struct FMapContext
{
	GENERATED_BODY()

	/** 적(Enemy) 스폰 지점 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FWebVectorRequest enemy_spawn;

	/** P-Pellet 위치 목록 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FPPelletLocation> p_pellet_locations;
};

/**
 * 의사결정 요청 (핵심 API)
 * POST /api/v1/get_decision
 *
 * 2~3초 주기로 호출
 */
USTRUCT(BlueprintType)
struct FGetDecisionRequest
{
	GENERATED_BODY()

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString room_id;

	/** 전역 게임 컨텍스트 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGlobalContext global_context;

	/** AI 스쿼드 컨텍스트 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FAISquadContext ai_squad_context;

	/** 플레이어/팀 컨텍스트 배열 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FPlayerTeamContext> player_team_context;

	/** 지휘관 컨텍스트 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FCommanderContext commander_context;

	/** 맵 컨텍스트 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FMapContext map_context;
};
