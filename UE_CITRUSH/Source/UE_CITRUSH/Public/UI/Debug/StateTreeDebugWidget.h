// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Enemy/AiEnemy/AIDirectiveComponent.h"    // EAIDirectiveState, FOnAIStateChanged
#include "Enemy/Components/EnemyCombatComponent.h"  // FOnDamageReceived
#include "StateTreeDebugWidget.generated.h"

class APixelEnemy;
class UTextBlock;
class UProgressBar;
class UEnemyAISubsystem;

/**
 * StateTree 로컬 AI 실시간 디버그 위젯
 *
 * [StateTree 상태 패널]
 *   - AI 모드: LOCAL (StateTree) / SERVER (FSM) 색상 구분
 *   - UStateTreeComponent 실행 상태 (Running / Stopped)
 *
 * [컨텍스트 데이터 패널]  ← FEnemyContextEvaluator가 매 틱 갱신하는 값을 미러링
 *   - 서버 연결 상태
 *   - 가장 가까운 Racer 거리 (임계값 2000 초과 시 빨강)
 *   - 가장 가까운 Pellet 거리 (임계값 1500 이하 시 노랑)
 *   - 가장 가까운 Coin 거리
 *   - 체력 비율 (HP 바 + 0.3 저체력 임계값 색상)
 *   - 파워 펠릿 활성 여부
 *
 * [조건 판별 패널]  ← FEnemyCondition_* 5종 실시간 평가
 *   - ServerConnected / HasNearbyRacer / HasNearbyPellet / LowHealth / HasPowerPellet
 *   - PASS(초록) / FAIL(빨강) 색상 표시
 *
 * [추론된 현재 태스크]
 *   - 조건 우선순위로부터 현재 StateTree가 선택했을 태스크를 추론
 *   - ServerCommand / Retreat / ConsumePellet / ChaseRacer / Patrol 중 하나
 *
 * [AI Perception 패널]
 *   - 감지된 타겟 수 (활성/비활성)
 *   - 시야(Sight) / 청각(Hearing) / 피격(Damage) 감지 여부
 *
 * [이벤트 로그]
 *   - StateTree 활성화/비활성화, 조건 변경, 데미지, 서버 연결 변경 실시간 기록
 *
 * [사용법]
 *   1. Blueprint에서 이 클래스를 부모로 위젯 생성 후 BindWidget 슬롯 배치
 *   2. BeginPlay 등에서 InitWithEnemy(PixelEnemy) 호출
 *   3. NativeDestruct 시 델리게이트 자동 해제
 */
UCLASS()
class UE_CITRUSH_API UStateTreeDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 추적할 Enemy 설정 및 델리게이트 바인딩.
	 * 이전에 추적 중인 Enemy가 있으면 자동으로 해제 후 교체.
	 * @param Enemy 추적할 APixelEnemy (nullptr 전달 시 추적 해제)
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug|StateTree")
	void InitWithEnemy(APixelEnemy* Enemy);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** PollInterval마다 컨텍스트 데이터 + 조건 패널 갱신 */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// ========== Polling ==========

	/** StateTree 상태 패널 (Local/Server 모드, IsRunning) */
	void RefreshStateTreePanel();

	/** FEnemyContextEvaluator 미러 — 컨텍스트 데이터 패널 갱신 */
	void RefreshContextPanel();

	/** FEnemyCondition_* 5종 평가 — 조건 판별 패널 갱신 */
	void RefreshConditionsPanel();

	/** 조건 우선순위로부터 현재 태스크 추론 — 태스크 패널 갱신 */
	void RefreshInferredTaskPanel();

	/** AIPerception 타겟 정보 패널 갱신 */
	void RefreshPerceptionPanel();

	// ========== Delegate Callbacks ==========

	/** AIDirectiveComponent::OnAIStateChanged → 로그 + 전 패널 즉시 갱신 */
	UFUNCTION()
	void OnAIStateChanged(EAIDirectiveState NewState, EAIDirectiveState OldState);

	/** EnemyCombatComponent::OnDamageReceived → 로그 + 컨텍스트/조건 즉시 갱신 */
	UFUNCTION()
	void OnDamageReceived(float DamageAmount, FVector HitLocation);

	/**
	 * UBaseAISubsystem::OnConnectionChanged → 서버 연결 변경 즉시 반영
	 * (non-dynamic multicast delegate, UFUNCTION 불필요)
	 */
	void OnAIConnectionChanged(bool bConnected);

	// ========== Context Data Helpers (FEnemyContextEvaluator 미러) ==========

	/** 가장 가까운 Racer 거리 계산 (APawn/AAbstractRacer 기반) */
	float GetNearestRacerDistance() const;

	/** 가장 가까운 Pellet 거리 계산 (APelletActor 기반) */
	float GetNearestPelletDistance() const;

	/** 가장 가까운 Coin 거리 계산 (FindNearestCoin 위임) */
	float GetNearestCoinDistance() const;

	/** 현재 체력 비율 (0.0 ~ 1.0) */
	float GetHealthRatio() const;

	/** 파워 펠릿 활성 여부 */
	bool HasPowerPellet() const;

	// ========== Condition Helpers ==========

	/** FEnemyCondition_ServerConnected: !bIsLocalAIActive */
	bool EvalCondition_ServerConnected() const;

	/** FEnemyCondition_HasNearbyRacer: NearestRacerDist < 2000 */
	bool EvalCondition_HasNearbyRacer() const;

	/** FEnemyCondition_HasNearbyPellet: NearestPelletDist < 1500 */
	bool EvalCondition_HasNearbyPellet() const;

	/** FEnemyCondition_LowHealth: HealthRatio < 0.3 */
	bool EvalCondition_LowHealth() const;

	/** FEnemyCondition_HasPowerPellet: bPowerPelletActive */
	bool EvalCondition_HasPowerPellet() const;

	// ========== Task Inference ==========

	/**
	 * 조건 우선순위로부터 현재 활성 태스크 이름 추론
	 * ServerCommand > Retreat > ConsumePellet > ChaseRacer > Patrol
	 */
	FString InferCurrentTask() const;

	// ========== Event Log ==========

	/** 이벤트 로그 버퍼에 타임스탬프 포함 메시지 추가 (최신 항목이 맨 위) */
	void PushEventLog(const FString& Message);

	/** 버퍼 내용을 Text_EventLog에 반영 */
	void FlushEventLog();

	// ========== Helpers ==========

	/** 조건 결과를 색상 포함 텍스트로 반환 ("PASS" 초록 / "FAIL" 빨강) */
	static void ApplyConditionResult(class UTextBlock* TextBlock, bool bPassed,
	                                 const FString& PassLabel = TEXT("PASS"),
	                                 const FString& FailLabel = TEXT("FAIL"));

	/** EAIDirectiveState → DisplayName 문자열 */
	static FString GetDirectiveStateName(EAIDirectiveState State);

	// ========== State ==========

	/** 현재 추적 중인 Enemy */
	UPROPERTY()
	TObjectPtr<APixelEnemy> TrackedEnemy;

	/** OnConnectionChanged 핸들 (non-dynamic) */
	FDelegateHandle ConnectionChangedHandle;

	/** 캐시된 컨텍스트 값 — RefreshContextPanel에서 갱신, 조건 패널/태스크에서 재사용 */
	float CachedRacerDist    = FLT_MAX;
	float CachedPelletDist   = FLT_MAX;
	float CachedCoinDist     = FLT_MAX;
	float CachedHealthRatio  = 1.0f;
	bool  bCachedPowerPellet = false;
	bool  bCachedServerConn  = false;

	// ========== Thresholds (FEnemyCondition_* 동기화) ==========

	static constexpr float RacerDistThreshold  = 2000.0f;  // FEnemyCondition_HasNearbyRacer
	static constexpr float PelletDistThreshold = 1500.0f;  // FEnemyCondition_HasNearbyPellet
	static constexpr float LowHealthThreshold  = 0.3f;     // FEnemyCondition_LowHealth

	// ========== Log ==========

	static constexpr int32 MaxLogEntries = 8;
	TArray<FString> EventLogBuffer;

	// ========== Polling ==========

	static constexpr float PollInterval = 0.1f;
	float TickAccumulator = 0.f;

	// ============================================================
	// Widget Bindings
	// ============================================================

	// [StateTree 상태 패널]

	/** AI 모드 레이블 - "STATE TREE (Local)" (주황) / "SERVER FSM" (초록) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_STMode;

	/** UStateTreeComponent::IsRunning() - "Running" (초록) / "Stopped" (회색) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_STRunStatus;

	// [컨텍스트 데이터 패널]

	/** 서버 연결 - "CONNECTED" / "DISCONNECTED" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ServerConn;

	/** 가장 가까운 Racer 거리 - "Racer: 1234 cm" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RacerDist;

	/** 가장 가까운 Pellet 거리 - "Pellet: 800 cm" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PelletDist;

	/** 가장 가까운 Coin 거리 - "Coin: 500 cm" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CoinDist;

	/** HP 프로그레스 바 (체력 비율에 따라 초록→빨강 보간, 0.3 이하 빨강) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Health;

	/** HP 수치 텍스트 - "0.75 (75/100)" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_HealthRatio;

	/** 파워 펠릿 - "P-Pellet: ON" (보라) / "P-Pellet: OFF" (회색) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PowerPellet;

	// [조건 판별 패널]

	/** FEnemyCondition_ServerConnected */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CondServerConn;

	/** FEnemyCondition_HasNearbyRacer (< 2000cm) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CondNearbyRacer;

	/** FEnemyCondition_HasNearbyPellet (< 1500cm) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CondNearbyPellet;

	/** FEnemyCondition_LowHealth (< 0.3) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CondLowHealth;

	/** FEnemyCondition_HasPowerPellet */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CondPowerPellet;

	// [추론된 현재 태스크]

	/** 조건 우선순위 기반 추론 태스크 이름 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_InferredTask;

	// [AI Perception 패널]

	/** 감지된 타겟 수 - "Targets: 2 (Active: 1)" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PerceivedCount;

	/** 가장 최근 타겟 정보 - "Sight/Hearing/Damage + 거리" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PerceptionDetail;

	// [이벤트 로그]

	/** StateTree 전환 / 데미지 / 조건 변경 / 연결 변경 실시간 로그 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_EventLog;
};
