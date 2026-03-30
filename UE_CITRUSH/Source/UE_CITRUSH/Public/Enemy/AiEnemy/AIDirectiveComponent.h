// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "AIDirectiveComponent.generated.h"

class UAbilitySystemComponent;
class AAbstractEnemy;
class UAIDataManagerComponent;
struct FGetDecisionResponse2;
class ACoinActor;
class UEnemyAISubsystem;

/**
 * AI State 정의 (AI 서버 directive_code와 매핑)
 * Note: 델리게이트보다 먼저 선언되어야 함
 */
UENUM(BlueprintType)
enum class EAIDirectiveState : uint8
{
	Idle = 0,
	Ambush = 1,            // directive_code 1: 매복 후 기습
	MoveToLocation = 2,    // directive_code 2: 지정 위치로 이동
	Intercept = 3,         // directive_code 3: 플레이어 차단
	Chase = 4,             // directive_code 4: 플레이어 추격
	Retreat = 5,           // directive_code 5: 후퇴 (안전지대로 이동)
	Patrol = 6,            // directive_code 6: 순찰
	ConsumePoint = 7,      // directive_code 7: P-Point 섭취
	ConsumePellet = 8,     // directive_code 8: 파워 펠릿 섭취
	Guard = 9,             // directive_code 9: 방어
	Flank = 10,            // directive_code 10: 측면 우회
	FakeRetreat = 11,      // directive_code 11: 기만 후퇴 (v1.4.0 변경)
	CoinChase = 50,        // Coin 추적 (폴백 모드)
	NetworkFallback = 99   // 네트워크 실패 시
};

/** Delegate: AI State 변경 시 브로드캐스트 (클라이언트 동기화용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIStateChanged, EAIDirectiveState, NewState, EAIDirectiveState, OldState);

/**
 * Directive 파라미터 구조체
 * FSM_data JSON 파일의 모든 params 필드를 포함합니다.
 */
USTRUCT(BlueprintType)
struct FDirectiveParams
{
	GENERATED_BODY()

	// ========== 공통 파라미터 ==========
	//safe_zone_position
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FVector TargetPosition = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FString TargetPlayerId;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float SpeedFactor = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	int32 Priority = 0;

	// ========== AMBUSH 파라미터 ==========

	/** 플레이어 감지 거리 (AMBUSH) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|AMBUSH")
	float TriggerDistance = 300.0f;

	/** 매복 대기 시간 (AMBUSH) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|AMBUSH")
	float WaitDuration = 5.0f;

	// ========== MOVE_TO_LOCATION 파라미터 ==========

	/** 목표 도달 판정 허용 오차 (MOVE_TO_LOCATION) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|MoveToLocation")
	float Tolerance = 1000.0f;

	// ========== INTERCEPT 파라미터 ==========

	/** 접근 각도 (INTERCEPT) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Intercept")
	float ApproachAngle = 45.0f;

	/** 요격 거리 (INTERCEPT) */	UPROPERTY(BlueprintReadWrite, Category = "AI|Intercept")
	float InterceptDistance = 2000.0f;

	/** 공격성 0~1 (INTERCEPT) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Intercept")
	float Aggressiveness = 1.5f;

	// ========== CHASE 파라미터 ==========

	/** 최대 추격 지속 시간 (CHASE) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Chase")
	float MaxChaseDuration = 15.0f;

	// ========== RETREAT 파라미터 ==========

	/** 안전 지대 위치 (RETREAT) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Retreat")
	FVector SafeZonePosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Directive")
	FString PelletId;

	// ========== PATROL 파라미터 ==========

	/** 순찰 구역 이름 (PATROL) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Patrol")
	FString PatrolZone;

	/** 순찰 속도 배율 (PATROL) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Patrol")
	float PatrolSpeed = 1.f;

	/** 순찰 경로 포인트 목록 (PATROL) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Patrol")
	TArray<FVector> Waypoints;

	// ========== CONSUME_P_POINT 파라미터 ==========

	/** P-Point ID (CONSUME_P_POINT) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|ConsumePoint")
	FString PPointId;

	/** 긴급 우선순위 (CONSUME_P_POINT) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|ConsumePoint")
	int32 EmergencyPriority = 0;

	// ========== FAKE_RETREAT 파라미터 ==========

	/** 기만 후퇴 지속 시간 (FAKE_RETREAT) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|FakeRetreat")
	float FakeRetreatDuration = 2.5f;

	/** 역습 위치 (FAKE_RETREAT) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|FakeRetreat")
	FVector CounterAttackPosition = FVector::ZeroVector;

	// ========== FLANK 파라미터 ==========

	/** 측면 우회 방향: LEFT, RIGHT (FLANK) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Flank")
	FString FlankDirection;

	/** 측면 우회 거리 (FLANK) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Flank")
	float FlankDistance = 60.0f;

	// ========== GUARD 파라미터 ==========
	/** 방어 대상 (GUARD) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Guard")
	FString GuardTarget;

	/** 방어 반경 (GUARD) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Guard")
	float GuardRadius = 2000.0f;

	/** 방어 지속 시간 (GUARD) */
	UPROPERTY(BlueprintReadWrite, Category = "AI|Guard")
	float GuardDuration = 3.0f;
};

/**
 * AI Directive FSM 컴포넌트
 *
 * 역할:
 * - AIDataManagerComponent로부터 directive_code 수신
 * - directive_code를 FSM State로 전환
 * - State별 행동 실행 (MoveToLocation, Retreat 등)
 * - GAS와 통합 (GameplayTag 기반 State 관리)
 *
 * 주의:
 * - HTTP 통신은 AIDataManagerComponent가 담당
 * - 이 Component는 FSM 실행만 담당
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UAIDirectiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIDirectiveComponent();

	/** Replication Setup */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ========== Directive 처리 ==========

	/** AIDataManagerComponent의 OnDecisionResponse2 Delegate로부터 호출됨 (레거시) */
	UFUNCTION()
	void OnAIDecisionReceived(bool bSuccess, FGetDecisionResponse2 Response);

	/** directive_code를 State로 전환 (Subsystem 방식) */
	void ProcessDirective(int32 DirectiveCode, const FDirectiveParams& Params);

	/** 마지막 Directive 수신 시간 갱신 (타임아웃 방지) */
	void UpdateLastDirectiveTime();

	// ========== State 관리 ==========

	/** State 전환 */
	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void TransitionToState(EAIDirectiveState NewState, const FDirectiveParams& Params);

	/** 현재 State 실행 */
	void UpdateCurrentState(float DeltaTime);

	/** 현재 State Getter */
	UFUNCTION(BlueprintPure, Category = "AI|State")
	EAIDirectiveState GetCurrentState() const { return CurrentState; }

	/** Goal이 GoalMaxAge보다 오래되었는지 확인 */
	UFUNCTION(BlueprintPure, Category = "AI|State")
	bool IsGoalStale() const;

	/** Delegate: State 변경 시 브로드캐스트 (서버 + 클라이언트 모두 받음) */
	UPROPERTY(BlueprintAssignable, Category = "AI|Events")
	FOnAIStateChanged OnAIStateChanged;

	/** 현재 목표 위치 Getter */
	UFUNCTION(BlueprintPure, Category = "AI|State")
	FVector GetTargetPosition() const { return ReplicatedTargetPosition; }

	// ========== State별 실행 함수 ==========

	void ExecuteIdle(float DeltaTime);
	void ExecuteAmbush(float DeltaTime);           // 1: 매복 후 기습
	void ExecuteMoveToLocation(float DeltaTime);   // 2: 지정 위치 이동
	void ExecuteIntercept(float DeltaTime);        // 3: 플레이어 차단
	void ExecuteChase(float DeltaTime);            // 4: 플레이어 추격
	void ExecuteRetreat(float DeltaTime);          // 5: 후퇴
	void ExecutePatrol(float DeltaTime);           // 6: 순찰
	void ExecuteConsumePoint(float DeltaTime);     // 7: P-Point 섭취
	void ExecuteConsumePellet(float DeltaTime);    // 8: 파워 펠릿 섭취
	void ExecuteGuard(float DeltaTime);            // 9: 방어
	void ExecuteFlank(float DeltaTime);            // 10: 측면 우회
	void ExecuteFakeRetreat(float DeltaTime);      // 11: 기만 후퇴
	void ExecuteCoinChase(float DeltaTime);        // Fallback: Coin 추적

	// ========== Coin/Pellet/Racer 추적 ==========

	/** 가장 가까운 Coin 찾기 */
	ACoinActor* FindNearestCoin();

	/** 가장 가까운 Pellet 찾기 */
	class APelletActor* FindNearestPellet();

	/** 가장 가까운 VehicleDemoCejCar 찾기 */
	class AVehicleDemoCejCar* FindNearestVehicleCar();

	/** Coin으로 이동 */
	void MoveToCoin(float DeltaTime);

protected:
	// ========== 설정 ==========

	/** Goal 유효 기간 (초) - 이 시간 이상 새 명령이 없으면 Goal이 stale로 간주됨 */
	UPROPERTY(EditAnywhere, Category = "AI|Settings", meta = (ClampMin = "4.0", ClampMax = "15.0"))
	float GoalMaxAge = 7.0f;

	// ========== GAS 연동 ==========

	/** State별 GameplayTag (GAS 통합) */
	UPROPERTY(EditDefaultsOnly, Category = "AI|GAS")
	TMap<EAIDirectiveState, FGameplayTag> StateTags;

	// ========== 상태 변수 (Replicated for Multiplayer) ==========

	/** 현재 AI State - Replicated to clients */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentState, Category = "AI|State")
	EAIDirectiveState CurrentState = EAIDirectiveState::Idle;

	/** 현재 Directive 파라미터 - Replicated for movement sync */
	UPROPERTY(Replicated)
	FDirectiveParams CurrentParams;

	/** 현재 목표 위치 - Replicated for visual feedback */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "AI|State")
	FVector ReplicatedTargetPosition = FVector::ZeroVector;

	/** OnRep: State 변경 시 클라이언트에서 호출 */
	UFUNCTION()
	void OnRep_CurrentState();

	/** State 시작 시간 */
	float StateStartTime = 0.f;

	/** 마지막 AI 서버 응답 시간 */
	float LastDirectiveTime = 0.f;

	// ========== 캐싱 ==========

	/** Owner Enemy (캐싱) */
	UPROPERTY()
	TObjectPtr<AAbstractEnemy> OwnerEnemy;

	/** AbilitySystemComponent (캐싱) */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	/** AIDataManagerComponent (캐싱) */
	UPROPERTY()
	TObjectPtr<UAIDataManagerComponent> AIDataManager;

	/** EnemyAISubsystem 참조 (캐싱 - GetAllActorsOfClass 대체) */
	UPROPERTY()
	TObjectPtr<UEnemyAISubsystem> CachedAISubsystem;

	/** 현재 추적 중인 Coin (Fallback 모드) */
	UPROPERTY()
	TObjectPtr<ACoinActor> TargetCoin;

	/** 현재 추적 중인 Pellet */
	UPROPERTY()
	TObjectPtr<class APelletActor> TargetPellet;

	/** PATROL: 현재 순찰 포인트 인덱스 */
	int32 CurrentWaypointIndex = 0;

	/** AMBUSH: 현재 매복 상태 (0: 이동 중, 1: 대기 중, 2: 공격) */
	int32 AmbushPhase = 0;

	/** CHASE: 추격 시작 시간 */
	float ChaseStartTime = 0.0f;

	/** GUARD: 방어 시작 시간 */
	float GuardStartTime = 0.0f;

	/** FLANK: 측면 우회 목표 위치 */
	FVector FlankTargetPosition = FVector::ZeroVector;

	// ========== NavMesh 경로 ==========

	/** 현재 NavMesh 경로 포인트 목록 */
	UPROPERTY()
	TArray<FVector> CurrentPathPoints;

	/** 현재 목표로 하는 경로 포인트 인덱스 */
	int32 CurrentPathPointIndex = 0;

	/** 경로 포인트 도달 판정 거리 (cm) */
	float PathPointTolerance = 100.0f;

	// ========== 헬퍼 함수 ==========

	/** NavMesh 경로 찾기
	 * @param StartLocation 시작 위치
	 * @param EndLocation 목표 위치
	 * @param OutPathPoints 경로 포인트 배열 (출력)
	 * @return 경로 찾기 성공 여부
	 */
	bool FindNavMeshPath(const FVector& StartLocation, const FVector& EndLocation, TArray<FVector>& OutPathPoints);

	/** NavMesh 경로 비용 계산 (최단 거리 탐색용)
	 * @param StartLocation 시작 위치
	 * @param EndLocation 목표 위치
	 * @return 경로 비용 (실패 시 -1.0f)
	 */
	float GetNavPathCost(const FVector& StartLocation, const FVector& EndLocation);

	/** NavMesh 경로를 따라 이동
	 * @param DeltaTime 프레임 시간
	 * @param SpeedFactor 속도 배율
	 * @return 목표 도달 여부
	 */
	bool MoveAlongPath(float DeltaTime, float SpeedFactor = 1.0f);

	/** 플레이어 ID로 실제 Pawn 찾기 */
	APawn* FindPlayerByID(const FString& PlayerID);

	/** 가장 가까운 플레이어 찾기 */
	APawn* FindNearestPlayer();

	/** State에 대응하는 GameplayTag 반환 */
	FGameplayTag GetStateTag(EAIDirectiveState State) const;

	/**
	 * SetActorLocation 기반 이동 헬퍼
	 * AddMovementInput 대신 사용 (AIController 의존성 제거)
	 * @param Direction 이동 방향 (정규화 필요)
	 * @param SpeedFactor 속도 배율
	 * @param DeltaTime 프레임 시간
	 */
	void MoveInDirection(const FVector& Direction, float SpeedFactor, float DeltaTime);

private:
	int32 CoinEatCount = 0; // 이 적이 먹은 코인 개수
	int32 CoinEatitemUse = 2; // 발생 개수

	// 카메라를 숨기는 타이머 핸들
	FTimerHandle CameraHideTimerHandle;

	// Commander 플레이어에게만 카메라 페이드 적용
	void HidePlayerCamera();
	void ShowPlayerCamera();
};
