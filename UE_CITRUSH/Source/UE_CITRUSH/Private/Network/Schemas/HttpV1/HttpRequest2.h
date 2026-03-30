// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RequestHttp.h" 
#include "HttpRequest2.generated.h"

/**
 * AI Agent HTTP API v1.4.0 - Request Structures
 *
 * 이 파일은 AI 서버로 전송할 HTTP Request 구조체 v1.4.0을 정의합니다.
 * Protocol 문서: AgentLog/[AI 에이전트]언리얼팀 통합 연동 가이드_v1.4.0
 *
 * 주요 엔드포인트:
 * - GET  /api/v1/health
 * - POST /api/v1/match/start
 * - POST /api/v1/get_decision (핵심)
 * - POST /api/v1/match/end
 * - POST /api/v1/overseer/tts (NEW in v1.2.0)
 * - GET  /api/v1/commander/report (NEW in v1.2.0)
 *
 * v1.4.0 주요 변경사항:
 * - request_num 필드 추가 (요청 식별자, 순서 보장)
 * - current_directive_code 필드 추가 (현재 실행 중인 명령 코드)
 * - nav_frame 필드 추가 (UE NavSystemLLMData 매핑)
 * - racer_nav_delta 필드 추가 (DRIVER 전용 내비게이션 델타)
 * - p_point_locations 필드 추가 (P-Point 시스템)
 * - rotation_yaw_deg, forward_vector 필드 추가 (팩맨 방향)
 *
 * v1.2.0 변경사항:
 * - Steam ID 기반 유저 시스템 (player_id + player_name + player_type)
 * - NavMesh 경로 코스트/거리 필드 추가
 * - pacman_main.speed 필드 추가
 * - enemy_main → pacman_main, minions → clones 명칭 변경
 * - Overseer TTS API 추가
 *
 * 기존 v1.1 구조체 재사용:
 * - FWebVectorRequest, FWebVelocityRequest
 * - FEquippedRune, FCommanderInteraction
 * - FCashStatus, FCommanderRecentAction, FManagementStats
 * - FCommanderContext, FPlayerStats, FPPelletLocation
 */

// ============================================================================
// Match Start/End Requests v1.2.0
// ============================================================================

/**
 * 매치 시작 요청 v1.2.0
 * POST /api/v1/match/start
 *
 * v1.2.0 변경사항:
 * - player_types[] 추가 ("COMMANDER" | "DRIVER")
 * - player_ids는 Steam ID로 사용
 * - 인덱스 규칙: 0 = COMMANDER, 1~3 = DRIVER
 */
USTRUCT(BlueprintType)
struct FMatchStartRequest2
{
	GENERATED_BODY()

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	FString room_id;

	/** 참여 플레이어 Steam ID 목록 (필수)
	 * 인덱스 규칙: 0 = COMMANDER, 1~3 = DRIVER
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	TArray<FString> player_ids;

	/** 참여 플레이어 닉네임 목록 (필수, v1.2.0)
	 * 스팀 닉네임 또는 인게임 닉네임
	 * player_ids와 동일한 인덱스 순서
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	TArray<FString> player_names;

	/** 참여 플레이어 타입 목록 (필수, v1.2.0)
	 * "COMMANDER" 또는 "DRIVER"
	 * 인덱스 규칙: 0 = COMMANDER, 1~3 = DRIVER
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	TArray<FString> player_types;

	/** 맵 ID (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	FString map_id;

	/** 게임 모드 (선택): RANKED, CASUAL 등 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	FString mode;

	/** 매치 시작 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	FString match_start_time;
};

/**
 * 매치 종료 요청 v1.2.0
 * POST /api/v1/match/end
 * v1.1과 동일, FPlayerStats 재사용
 */
USTRUCT(BlueprintType)
struct FMatchEndRequest2
{
	GENERATED_BODY()

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	FString room_id;

	/** 매치 결과 (필수): PLAYERS_WIN, ENEMY_WIN, DRAW, ABORTED */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	FString result;

	/** 실제 경기 진행 시간(초) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	int32 match_duration_seconds = 0;

	/** 승리 팀 식별자 (선택): PLAYERS, ENEMY */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	FString winner_team;

	/** 종료 사유 (선택): TIME_OVER, PACMAN_DEAD, PACMAN_CAPTURED, SURRENDER, DISCONNECT, MANUAL_ABORT */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	FString end_reason;

	/** 매치 종료 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	FString match_end_time;

	/** 플레이어별 통계 (선택) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Match")
	TArray<FPlayerStats> player_stats;
};

// ============================================================================
// Get Decision Request v1.2.0 - AI Squad Context
// ============================================================================

/**
 * 팩맨 본체 상태 v1.4.0
 *
 * v1.2.0 변경사항:
 * - enemy_main → pacman_main으로 명칭 변경
 * - speed 필드 추가 (현재 이동 속도 m/s)
 *
 * v1.4.0 변경사항:
 * - rotation_yaw_deg 추가 (팩맨의 Yaw 회전값, 도 단위)
 * - forward_vector 추가 (팩맨의 전방 벡터)
 */
USTRUCT(BlueprintType)
struct FPacmanMain2
{
	GENERATED_BODY()

	/** 위치 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pacman")
	FWebVectorRequest position;

	/** 체력 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pacman")
	float hp = 100.0f;

	/** 현재 이동 속도 (필수, v1.2.0)
	 * 단위: m/s (미터/초)
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pacman")
	float speed = 0.0f;

	/** 포획 게이지 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pacman")
	float capture_gauge = 0.0f;

	/** 무적 상태 여부 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pacman")
	bool is_invulnerable = false;

	/** 회전값 Yaw (선택, v1.4.0)
	 * 단위: 도(degree)
	 * 팩맨이 바라보는 방향의 Yaw 각도
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pacman")
	float rotation_yaw_deg = 0.0f;

	/** 전방 벡터 (선택, v1.4.0)
	 * 팩맨의 정면 방향 벡터 (정규화됨)
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pacman")
	FWebVectorRequest forward_vector;

	/** NavMesh 최소 비용 (선택)
	 * 가장 가까운 추격자까지의 NavMesh 경로 비용
	 * -1.0f = 경로 없음 또는 계산 불가
	 /*#1#
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pacman")
	float nav_cost_min = -1.0f;

	/** NavMesh 평균 비용 (선택)
	 * 모든 추격자까지의 NavMesh 경로 비용 평균
	 * -1.0f = 경로 없음 또는 계산 불가
	 #1#
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Pacman")
	float nav_cost_avg = -1.0f;*/ //삭제됨 
};

/**
 * 클론(분신) 유닛 상태 v1.2.0
 *
 * v1.2.0 변경사항:
 * - minion → clone으로 명칭 변경
 * - 구조는 FMinionUnit과 동일
 */
USTRUCT(BlueprintType)
struct FCloneUnit2
{
	GENERATED_BODY()

	/** 유닛 ID (필수): clone_1, clone_2 등 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Clone")
	FString unit_id;

	/** 위치 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Clone")
	FWebVectorRequest position;

	/** 체력 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Clone")
	float hp = 1.0f;

	/** 생존 여부 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Clone")
	bool is_alive = true;

	/** 이동 속도 스칼라 (v1.5.0) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Clone")
	float speed = 0.0f;

	/** 바라보는 방향 Yaw (도 단위, v1.5.0) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Clone")
	float rotation_yaw_deg = 0.0f;

	/** 전방 벡터 (v1.5.0) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Clone")
	FWebVectorRequest forward_vector;
};

/**
 * AI 스쿼드 컨텍스트 v1.2.0 (팩맨 본체 + 클론들)
 *
 * v1.2.0 변경사항:
 * - enemy_main → pacman_main
 * - minions → clones
 */
USTRUCT(BlueprintType)
struct FAISquadContext2
{
	GENERATED_BODY()

	/** 팩맨 본체 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AISquad")
	FPacmanMain2 pacman_main;

	/** 클론들 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AISquad")
	TArray<FCloneUnit2> clones;

	/** P-Pellet 재사용 대기 시간(초) (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AISquad")
	float p_pellet_cooldown = 0.0f;

	/** 마지막 P-Pellet 사용 시각 ISO 8601 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AISquad")
	FString last_pellet_consumed_at;
};

// ============================================================================
// Get Decision Request v1.2.0 - Player Team Context
// ============================================================================

/**
 * 플레이어 이벤트 v1.2.0
 *
 * v1.2.0 변경사항:
 * - event_code 필드 추가 (int32)
 */
USTRUCT(BlueprintType)
struct FPlayerEvent2
{
	GENERATED_BODY()

	/** 이벤트 코드 (필수, v1.2.0)
	 * 공통 enum 테이블 기준
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Event")
	int32 event_code = 0;

	/** 이벤트 타입 (필수): FUEL_LOW, HP_LOW, RISK_HIGH, STATION_CLOSED */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Event")
	FString event_type;

	/** 심각도 (필수): INFO, LOW, MEDIUM, HIGH, CRITICAL */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Event")
	FString severity;

	/** 현재 수치 값 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Event")
	float value = 0.0f;

	/** 경고 기준 값 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Event")
	float threshold = 0.0f;
};

/**
 * 레이서 내비게이션 델타 정보 (v1.4.0)
 * DRIVER 전용 - 팩맨 기준 위치/속도 변화량
 */
USTRUCT(BlueprintType)
struct FRacerNavDelta
{
	GENERATED_BODY()

	/** 플레이어 인덱스 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NavDelta")
	int32 player_index = 0;

	/** 팩맨 기준 상대 각도 (도) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NavDelta")
	float relative_angle_to_pacman_deg = 0.0f;

	/** 위치 변화량 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NavDelta")
	FWebVectorRequest delta_position;

	/** 직선 거리 변화량 (cm) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NavDelta")
	float delta_straight_distance = 0.0f;

	/** 경로 거리 변화량 (cm) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NavDelta")
	float delta_path_distance = 0.0f;

	/** 경로 비용 변화량 (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NavDelta")
	float delta_path_cost = 0.0f;

	/** 이동 방향 변화 (도) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NavDelta")
	float movement_direction_change_deg = 0.0f;

	/** 평균 속도 (cm/s) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NavDelta")
	float average_speed = 0.0f;

	/** 상대 방위 변화 (도) (선택) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NavDelta")
	float relative_bearing_change_deg = 0.0f;
};

/**
 * 플레이어/팀 컨텍스트 v1.4.0
 *
 * v1.2.0 변경사항:
 * - player_name 추가 (스팀 닉네임/인게임 닉네임)
 * - player_type 추가 ("COMMANDER" | "DRIVER")
 * - distance_to_pacman 추가 (유클리드 직선 거리)
 * - navmesh_cost_to_pacman 추가 (NavMesh 최단 경로 길이, 핵심 필드)
 * - navmesh_path_valid 추가 (경로 계산 성공 여부)
 * - events: FPlayerEvent2 사용
 *
 * v1.4.0 변경사항:
 * - navmesh_distance_to_pacman 추가 (NavMesh 경로 거리)
 * - racer_nav_delta 추가 (DRIVER 전용 델타 정보)
 *
 * 기존 v1.1 구조체 재사용:
 * - FWebVectorRequest (position)
 * - FWebVelocityRequest (velocity)
 * - FEquippedRune (equipped_runes)
 * - FCommanderInteraction (commander_interaction)
 */
USTRUCT(BlueprintType)
struct FPlayerTeamContext2
{
	GENERATED_BODY()

	/** 플레이어 Steam ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	FString player_id;

	/** 플레이어 닉네임 (필수, v1.2.0)
	 * 스팀 닉네임 또는 인게임 닉네임
	 * 로그/리포트에 사용
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	FString player_name;

	/** 플레이어 타입 (필수, v1.2.0)
	 * "COMMANDER" 또는 "DRIVER"
	 * 역할 구분용
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	FString player_type;

	/** 역할 (필수): 1=COMMANDER, 2=DRIVER1, 3=DRIVER2, 4=DRIVER3
	 * 내부 코드/호환성 목적
	 * player_type과 의미 일치
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	int32 role = 2;

	/** 위치 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	FWebVectorRequest position;

	/** 속도 벡터 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	FWebVelocityRequest velocity;

	/** 체력 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	float hp = 100.0f;

	/** 팩맨까지 유클리드(직선) 거리 (선택, v1.2.0)
	 * 단위: m (미터)
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player|NavMesh")
	float distance_to_pacman = 0.0f;

	/** 팩맨까지 NavMesh 최단 경로 비용 (선택, v1.2.0)
	 * 단위: cm 
	 * 핵심 필드: AI가 실제 주행 경로 기반으로 판단할 때 사용
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player|NavMesh")
	float navmesh_cost_to_pacman = 0.0f;

	/** 팩맨까지 NavMesh 경로 거리 (선택, v1.4.0)
	 * 단위: m (미터)
	 * cost와 별도로 실제 경로 길이
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player|NavMesh")
	float navmesh_distance_to_pacman = 0.0f;

	/** NavMesh 경로 계산 성공 여부 (선택, v1.2.0)
	 * true: 경로 유효함
	 * false: 경로 계산 실패 또는 경로 없음
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player|NavMesh")
	bool navmesh_path_valid = false;

	/** 부스터 사용 중 여부 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	bool is_boosting = false;

	/** 최근 행동 라벨 (필수): back_attack_attempt, normal_drive, patrolling 등 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	FString recent_action;

	/** 레이서 내비게이션 델타 (선택, v1.4.0)
	 * DRIVER 전용 - 팩맨 기준 위치/속도 변화량
	 * COMMANDER는 빈 구조체
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player|NavMesh")
	FRacerNavDelta racer_nav_delta;

	/** 이벤트 목록 (필수, 없으면 빈 배열) - v1.2.0 구조체 사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	TArray<FPlayerEvent2> events;

	/** 장착한 룬 목록 (필수, 없으면 빈 배열) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	TArray<FEquippedRune> equipped_runes;

	/** 지휘관 상호작용 정보 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Player")
	FCommanderInteraction commander_interaction;
};

// ============================================================================
// Get Decision Request v1.2.0 - Map Context
// ============================================================================

/**
 * P-Point(코인) 위치 정보
 * 맵에 스폰되는 CoinActor 위치를 AI에게 전달하는 구조체
 */
USTRUCT(BlueprintType)
struct FPPointLocation
{
	GENERATED_BODY()

	/** P-Point 개체 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PPoint")
	FString id;

	/** X 좌표 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PPoint")
	float x = 0.0f;

	/** Y 좌표 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PPoint")
	float y = 0.0f;

	/** Z 좌표 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PPoint")
	float z = 0.0f;

	/** 현재 활성(획득 가능) 상태 (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PPoint")
	bool available = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PPoint")
	float cooldown = 0.f;
};

/**
 * 맵 컨텍스트 v1.2.0
 *
 * v1.2.0 변경사항:
 * - enemy_spawn → pacman_spawn으로 명칭 변경
 *
 * 기존 v1.1 구조체 재사용:
 * - FPPelletLocation (p_pellet_locations)
 */
USTRUCT(BlueprintType)
struct FMapContext2
{
	GENERATED_BODY()

	/** 팩맨 스폰 위치 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map")
	FWebVectorRequest pacman_spawn;

	/** P-Pellet 위치 목록 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map")
	TArray<FPPelletLocation> p_pellet_locations;

	/** P-Point(코인) 위치 목록 (필수) - 레벨에 스폰된 CoinActor 위치들 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map")
	TArray<FPPointLocation> p_point_locations;
};

// ============================================================================
// Get Decision Request v1.2.0 - Main Request
// ============================================================================

/**
 * 의사결정 요청 v1.2.0
 * POST /api/v1/get_decision
 *
 * v1.2.0 변경사항:
 * - ai_squad_context: FAISquadContext2 사용 (pacman_main, clones)
 * - player_team_context: FPlayerTeamContext2 사용 (NavMesh 필드 포함)
 * - map_context: FMapContext2 사용 (pacman_spawn)
 *
 * 기존 v1.1 구조체 재사용:
 * - FGlobalContext (global_context) - v1.1과 동일
 * - FCommanderContext (commander_context) - v1.1과 동일
 */
USTRUCT(BlueprintType)
struct FGetDecisionRequest2
{
	GENERATED_BODY()

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Decision")
	FString room_id;

	/** 요청 식별자 (필수, v1.4.0)
	 * 형식: {room_id}_{timestamp}_{num}
	 * 예: "GAME_12345_1700000000_001"
	 * 용도: 순서 보장, 중복 방지, 로그 추적
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Decision")
	FString request_num;

	/** 현재 실행 중인 Directive 코드 (필수, v1.4.0)
	 * 0 = 명령 없음 또는 완료
	 * 1~11 = 현재 실행 중인 명령 코드
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Decision")
	int32 current_directive_code = 0;

	/** 전역 게임 컨텍스트 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Decision")
	FGlobalContext global_context;

	/** AI 스쿼드 컨텍스트 (필수) - v1.2.0 구조체 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Decision")
	FAISquadContext2 ai_squad_context;

	/** 플레이어/팀 컨텍스트 배열 (필수) - v1.2.0 구조체 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Decision")
	TArray<FPlayerTeamContext2> player_team_context;

	/** 지휘관 컨텍스트 (필수) - v1.1 구조체 재사용 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Decision")
	FCommanderContext commander_context;

	/** 맵 컨텍스트 (필수) - v1.2.0 구조체 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Decision")
	FMapContext2 map_context;
};

// ============================================================================
// Overseer TTS Request (NEW in v1.2.0)
// ============================================================================

/**
 * Overseer TTS 요청 v1.2.0
 * POST /api/v1/overseer/tts
 *
 * 언리얼 → AI 서버로 TTS 지시를 조회하는 API입니다.
 * AI 서버는 내부 TTS 큐에서 대기 중인 알림을 하나씩 반환합니다.
 */
USTRUCT(BlueprintType)
struct FOverseerTTSRequest2
{
	GENERATED_BODY()

	/** 게임 방 ID (필수) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Overseer")
	FString room_id;

	/** 이벤트 ID (선택)
	 * 특정 이벤트를 조회할 때 사용
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Overseer")
	FString event_id;
};
