// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Debug/StateTreeDebugWidget.h"

#include "Enemy/PixelEnemy.h"
#include "Player/AbstractRacer.h"
#include "Player/Stats/Coin/CoinActor.h"
#include "Enemy/AiEnemy/AIDirectiveComponent.h"
#include "Enemy/Components/EnemyCombatComponent.h"
#include "Enemy/Pellet/PelletActor.h"
#include "GAS/AttributeSet/ASEnemy.h"
#include "Subsystems/EnemyAISubsystem.h"
#include "Subsystems/BaseAISubsystem.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Misc/DateTime.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StateTreeComponent.h"

// ============================================================
// Lifecycle
// ============================================================

void UStateTreeDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PushEventLog(TEXT("[Widget] Ready. Call InitWithEnemy()."));
	FlushEventLog();
}

void UStateTreeDebugWidget::NativeDestruct()
{
	InitWithEnemy(nullptr);
	Super::NativeDestruct();
}

void UStateTreeDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(TrackedEnemy) || !IsVisible()) return;

	TickAccumulator += InDeltaTime;
	if (TickAccumulator < PollInterval) return;

	TickAccumulator = 0.f;

	// 컨텍스트 값 먼저 캐싱 → 조건/태스크 패널이 재사용
	RefreshContextPanel();
	RefreshConditionsPanel();
	RefreshInferredTaskPanel();
	RefreshStateTreePanel();
	RefreshPerceptionPanel();
}

// ============================================================
// Init
// ============================================================

void UStateTreeDebugWidget::InitWithEnemy(APixelEnemy* Enemy)
{
	// 이전 Enemy 델리게이트 해제
	if (IsValid(TrackedEnemy))
	{
		if (UAIDirectiveComponent* DC = TrackedEnemy->AIDirectiveComponent)
		{
			DC->OnAIStateChanged.RemoveDynamic(this, &UStateTreeDebugWidget::OnAIStateChanged);
		}
		if (UEnemyCombatComponent* CC = TrackedEnemy->CombatComponent)
		{
			CC->OnDamageReceived.RemoveDynamic(this, &UStateTreeDebugWidget::OnDamageReceived);
		}
		if (TrackedEnemy->EnemyAISubsystem)
		{
			TrackedEnemy->EnemyAISubsystem->OnConnectionChanged.Remove(ConnectionChangedHandle);
		}
	}

	TrackedEnemy = Enemy;
	EventLogBuffer.Reset();
	CachedRacerDist    = FLT_MAX;
	CachedPelletDist   = FLT_MAX;
	CachedCoinDist     = FLT_MAX;
	CachedHealthRatio  = 1.0f;
	bCachedPowerPellet = false;
	bCachedServerConn  = false;

	if (!IsValid(Enemy))
	{
		PushEventLog(TEXT("[Tracking] Cleared."));
		FlushEventLog();
		return;
	}

	// 새 Enemy 델리게이트 바인딩
	if (UAIDirectiveComponent* DC = Enemy->AIDirectiveComponent)
	{
		DC->OnAIStateChanged.AddDynamic(this, &UStateTreeDebugWidget::OnAIStateChanged);
	}
	if (UEnemyCombatComponent* CC = Enemy->CombatComponent)
	{
		CC->OnDamageReceived.AddDynamic(this, &UStateTreeDebugWidget::OnDamageReceived);
	}
	if (Enemy->EnemyAISubsystem)
	{
		ConnectionChangedHandle = Enemy->EnemyAISubsystem->OnConnectionChanged.AddUObject(
			this, &UStateTreeDebugWidget::OnAIConnectionChanged);
	}

	PushEventLog(FString::Printf(TEXT("[Init] Tracking: %s"), *Enemy->GetEnemyID()));
	FlushEventLog();

	// 바인딩 직후 즉시 초기 상태 반영
	RefreshContextPanel();
	RefreshConditionsPanel();
	RefreshInferredTaskPanel();
	RefreshStateTreePanel();
	RefreshPerceptionPanel();
}

// ============================================================
// Polling — StateTree 상태 패널
// ============================================================

void UStateTreeDebugWidget::RefreshStateTreePanel()
{
	if (!IsValid(TrackedEnemy)) return;

	const bool bLocalAI = TrackedEnemy->IsLocalAIActive();

	// AI 모드 레이블
	if (Text_STMode)
	{
		Text_STMode->SetText(FText::FromString(
			bLocalAI ? TEXT("STATE TREE (Local)") : TEXT("SERVER FSM")));
		Text_STMode->SetColorAndOpacity(FSlateColor(
			bLocalAI ? FLinearColor(1.f, 0.6f, 0.1f)    // 주황: 로컬 StateTree
			         : FLinearColor(0.2f, 1.f, 0.4f)));  // 초록: 서버 FSM
	}

	// UStateTreeComponent 실행 상태
	if (Text_STRunStatus)
	{
		bool bRunning = false;
		if (const UStateTreeComponent* STC = TrackedEnemy->StateTreeComponent)
		{
			bRunning = STC->IsRunning();
		}
		Text_STRunStatus->SetText(FText::FromString(
			bRunning ? TEXT("Running") : TEXT("Stopped")));
		Text_STRunStatus->SetColorAndOpacity(FSlateColor(
			bRunning ? FLinearColor(0.2f, 1.f, 0.4f)
			         : FLinearColor(0.5f, 0.5f, 0.5f)));
	}
}

// ============================================================
// Polling — 컨텍스트 데이터 패널 (FEnemyContextEvaluator 미러)
// ============================================================

void UStateTreeDebugWidget::RefreshContextPanel()
{
	if (!IsValid(TrackedEnemy)) return;

	// 캐시 갱신
	bCachedServerConn  = EvalCondition_ServerConnected();
	CachedRacerDist    = GetNearestRacerDistance();
	CachedPelletDist   = GetNearestPelletDistance();
	CachedCoinDist     = GetNearestCoinDistance();
	CachedHealthRatio  = GetHealthRatio();
	bCachedPowerPellet = HasPowerPellet();

	// 서버 연결
	if (Text_ServerConn)
	{
		Text_ServerConn->SetText(FText::FromString(
			bCachedServerConn ? TEXT("Server: CONNECTED") : TEXT("Server: DISCONNECTED")));
		Text_ServerConn->SetColorAndOpacity(FSlateColor(
			bCachedServerConn ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor::Red));
	}

	// 가장 가까운 Racer — 임계값(2000) 이하일 때 빨강
	if (Text_RacerDist)
	{
		const bool bInRange = (CachedRacerDist <= RacerDistThreshold);
		const FString RacerStr = (CachedRacerDist >= FLT_MAX / 2.f)
			? TEXT("Racer: - (None)")
			: FString::Printf(TEXT("Racer: %.0f cm"), CachedRacerDist);
		Text_RacerDist->SetText(FText::FromString(RacerStr));
		Text_RacerDist->SetColorAndOpacity(FSlateColor(
			bInRange ? FLinearColor(1.f, 0.3f, 0.3f) : FLinearColor(0.85f, 0.85f, 0.85f)));
	}

	// 가장 가까운 Pellet — 임계값(1500) 이하일 때 노랑
	if (Text_PelletDist)
	{
		const bool bNear = (CachedPelletDist <= PelletDistThreshold);
		const FString PelletStr = (CachedPelletDist >= FLT_MAX / 2.f)
			? TEXT("Pellet: - (None)")
			: FString::Printf(TEXT("Pellet: %.0f cm"), CachedPelletDist);
		Text_PelletDist->SetText(FText::FromString(PelletStr));
		Text_PelletDist->SetColorAndOpacity(FSlateColor(
			bNear ? FLinearColor(1.f, 0.9f, 0.2f) : FLinearColor(0.85f, 0.85f, 0.85f)));
	}

	// 가장 가까운 Coin
	if (Text_CoinDist)
	{
		const FString CoinStr = (CachedCoinDist >= FLT_MAX / 2.f)
			? TEXT("Coin: - (None)")
			: FString::Printf(TEXT("Coin: %.0f cm"), CachedCoinDist);
		Text_CoinDist->SetText(FText::FromString(CoinStr));
		Text_CoinDist->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f)));
	}

	// HP 바 — 0.3 이하 빨강, 이상 초록
	if (Bar_Health)
	{
		const float Ratio = FMath::Clamp(CachedHealthRatio, 0.f, 1.f);
		Bar_Health->SetPercent(Ratio);
		Bar_Health->SetFillColorAndOpacity(FLinearColor::LerpUsingHSV(
			FLinearColor(0.9f, 0.1f, 0.1f),   // 빨강 (저체력)
			FLinearColor(0.2f, 0.9f, 0.3f),   // 초록 (고체력)
			Ratio));
	}

	// HP 수치 텍스트
	if (Text_HealthRatio)
	{
		const UASEnemy* AS = TrackedEnemy->attributeSet;
		if (AS)
		{
			Text_HealthRatio->SetText(FText::FromString(
				FString::Printf(TEXT("HP: %.2f  (%.0f / %.0f)"),
					CachedHealthRatio,
					AS->GetHealth(),
					AS->GetMaxHealth())));
		}
		// 저체력 경고 색상
		Text_HealthRatio->SetColorAndOpacity(FSlateColor(
			CachedHealthRatio < LowHealthThreshold
				? FLinearColor(1.f, 0.3f, 0.3f)
				: FLinearColor(0.85f, 0.85f, 0.85f)));
	}

	// 파워 펠릿
	if (Text_PowerPellet)
	{
		Text_PowerPellet->SetText(FText::FromString(
			bCachedPowerPellet ? TEXT("P-Pellet: ON") : TEXT("P-Pellet: OFF")));
		Text_PowerPellet->SetColorAndOpacity(FSlateColor(
			bCachedPowerPellet
				? FLinearColor(0.6f, 0.2f, 1.f)    // 보라: 활성
				: FLinearColor(0.5f, 0.5f, 0.5f)));  // 회색: 비활성
	}
}

// ============================================================
// Polling — 조건 판별 패널 (FEnemyCondition_* 5종)
// ============================================================

void UStateTreeDebugWidget::RefreshConditionsPanel()
{
	if (!IsValid(TrackedEnemy)) return;

	// ServerConnected
	ApplyConditionResult(Text_CondServerConn, bCachedServerConn,
		TEXT("ServerConnected: PASS"), TEXT("ServerConnected: FAIL"));

	// HasNearbyRacer (< 2000cm)
	const bool bRacerNear = EvalCondition_HasNearbyRacer();
	if (Text_CondNearbyRacer)
	{
		const FString Label = FString::Printf(TEXT("NearbyRacer (< %.0f): %s"),
			RacerDistThreshold, bRacerNear ? TEXT("PASS") : TEXT("FAIL"));
		Text_CondNearbyRacer->SetText(FText::FromString(Label));
		Text_CondNearbyRacer->SetColorAndOpacity(FSlateColor(
			bRacerNear ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.3f, 0.3f)));
	}

	// HasNearbyPellet (< 1500cm)
	const bool bPelletNear = EvalCondition_HasNearbyPellet();
	if (Text_CondNearbyPellet)
	{
		const FString Label = FString::Printf(TEXT("NearbyPellet (< %.0f): %s"),
			PelletDistThreshold, bPelletNear ? TEXT("PASS") : TEXT("FAIL"));
		Text_CondNearbyPellet->SetText(FText::FromString(Label));
		Text_CondNearbyPellet->SetColorAndOpacity(FSlateColor(
			bPelletNear ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.3f, 0.3f)));
	}

	// LowHealth (< 0.3)
	const bool bLowHP = EvalCondition_LowHealth();
	if (Text_CondLowHealth)
	{
		const FString Label = FString::Printf(TEXT("LowHealth (< %.1f): %s"),
			LowHealthThreshold, bLowHP ? TEXT("PASS") : TEXT("FAIL"));
		Text_CondLowHealth->SetText(FText::FromString(Label));
		Text_CondLowHealth->SetColorAndOpacity(FSlateColor(
			bLowHP ? FLinearColor(1.f, 0.3f, 0.3f) : FLinearColor(0.2f, 1.f, 0.4f)));
	}

	// HasPowerPellet
	ApplyConditionResult(Text_CondPowerPellet, bCachedPowerPellet,
		TEXT("PowerPellet: PASS"), TEXT("PowerPellet: FAIL"));
}

// ============================================================
// Polling — 추론된 현재 태스크 패널
// ============================================================

void UStateTreeDebugWidget::RefreshInferredTaskPanel()
{
	if (!IsValid(TrackedEnemy)) return;
	if (!Text_InferredTask) return;

	const FString TaskName = InferCurrentTask();
	Text_InferredTask->SetText(FText::FromString(TaskName));

	// 태스크별 색상 구분
	FLinearColor TaskColor = FLinearColor(0.85f, 0.85f, 0.85f);
	if (TaskName.Contains(TEXT("Inactive")))       TaskColor = FLinearColor(0.5f, 0.5f, 0.5f);
	else if (TaskName.Contains(TEXT("Server")))    TaskColor = FLinearColor(0.2f, 1.f, 0.4f);
	else if (TaskName.Contains(TEXT("Retreat")))   TaskColor = FLinearColor(1.f, 0.3f, 0.3f);
	else if (TaskName.Contains(TEXT("Pellet")))    TaskColor = FLinearColor(1.f, 0.9f, 0.2f);
	else if (TaskName.Contains(TEXT("Chase")))     TaskColor = FLinearColor(1.f, 0.5f, 0.1f);
	else if (TaskName.Contains(TEXT("Patrol")))    TaskColor = FLinearColor(0.4f, 0.7f, 1.f);

	Text_InferredTask->SetColorAndOpacity(FSlateColor(TaskColor));
}

// ============================================================
// Polling — AI Perception 패널
// ============================================================

void UStateTreeDebugWidget::RefreshPerceptionPanel()
{
	if (!IsValid(TrackedEnemy)) return;

	const TArray<FPerceivedTargetInfo> Targets = TrackedEnemy->GetPerceivedTargets();
	const int32 TotalCount  = Targets.Num();
	const int32 ActiveCount = Targets.FilterByPredicate(
		[](const FPerceivedTargetInfo& T){ return T.bIsCurrentlySensed; }).Num();

	if (Text_PerceivedCount)
	{
		Text_PerceivedCount->SetText(FText::FromString(
			FString::Printf(TEXT("Targets: %d  (Active: %d)"), TotalCount, ActiveCount)));
	}

	// 가장 최근 활성 타겟 상세 정보
	if (Text_PerceptionDetail)
	{
		// 활성 타겟 중 거리 가장 가까운 것 선택
		const FPerceivedTargetInfo* NearestActive = nullptr;
		float MinDist = FLT_MAX;
		for (const FPerceivedTargetInfo& T : Targets)
		{
			if (!T.bIsCurrentlySensed || !IsValid(T.Actor)) continue;
			const float D = FVector::Dist(TrackedEnemy->GetActorLocation(), T.Actor->GetActorLocation());
			if (D < MinDist) { MinDist = D; NearestActive = &T; }
		}

		if (NearestActive)
		{
			// 감지 수단 표시
			FString SenseStr;
			if (NearestActive->bWasSensedBySight)   SenseStr += TEXT("Sight ");
			if (NearestActive->bWasSensedByHearing) SenseStr += TEXT("Hearing ");
			if (NearestActive->bWasSensedByDamage)  SenseStr += TEXT("Damage ");
			if (SenseStr.IsEmpty()) SenseStr = TEXT("Unknown");

			Text_PerceptionDetail->SetText(FText::FromString(
				FString::Printf(TEXT("[%s]\nDist: %.0f cm  |  Sense: %s\nStrength: %.2f"),
					*NearestActive->Actor->GetName(),
					MinDist,
					*SenseStr.TrimEnd(),
					NearestActive->StimulusStrength)));
			Text_PerceptionDetail->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f)));
		}
		else
		{
			Text_PerceptionDetail->SetText(FText::FromString(TEXT("(No active target)")));
			Text_PerceptionDetail->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
		}
	}
}

// ============================================================
// Delegate Callbacks
// ============================================================

void UStateTreeDebugWidget::OnAIStateChanged(EAIDirectiveState NewState, EAIDirectiveState OldState)
{
	PushEventLog(FString::Printf(TEXT("[FSM] %s -> %s"),
		*GetDirectiveStateName(OldState), *GetDirectiveStateName(NewState)));

	// 서버→로컬 또는 로컬→서버 전환 시 StateTree 패널 즉시 갱신
	if (NewState == EAIDirectiveState::NetworkFallback || OldState == EAIDirectiveState::NetworkFallback)
	{
		PushEventLog(FString::Printf(TEXT("[StateTree] %s"),
			IsValid(TrackedEnemy) && TrackedEnemy->IsLocalAIActive()
				? TEXT("ACTIVATED (Local Fallback)")
				: TEXT("DEACTIVATED (Server Restored)")));
	}
	FlushEventLog();
	RefreshStateTreePanel();
	RefreshContextPanel();
	RefreshConditionsPanel();
	RefreshInferredTaskPanel();
}

void UStateTreeDebugWidget::OnDamageReceived(float DamageAmount, FVector HitLocation)
{
	PushEventLog(FString::Printf(TEXT("[Dmg] -%.1f"), DamageAmount));
	FlushEventLog();
	// 데미지 후 체력/조건/태스크 즉시 갱신
	RefreshContextPanel();
	RefreshConditionsPanel();
	RefreshInferredTaskPanel();
}

void UStateTreeDebugWidget::OnAIConnectionChanged(bool bConnected)
{
	PushEventLog(FString::Printf(TEXT("[Network] Server %s → StateTree %s"),
		bConnected ? TEXT("CONNECTED") : TEXT("DISCONNECTED"),
		bConnected ? TEXT("OFF") : TEXT("ON")));
	FlushEventLog();
	RefreshStateTreePanel();
	RefreshContextPanel();
	RefreshConditionsPanel();
	RefreshInferredTaskPanel();
}

// ============================================================
// Context Data Helpers (FEnemyContextEvaluator 미러)
// ============================================================

float UStateTreeDebugWidget::GetNearestRacerDistance() const
{
	if (!IsValid(TrackedEnemy)) return FLT_MAX;

	// FindNearestRacer() 결과로 거리 계산
	const AActor* Racer = TrackedEnemy->FindNearestRacer();
	if (!IsValid(Racer)) return FLT_MAX;
	return FVector::Dist(TrackedEnemy->GetActorLocation(), Racer->GetActorLocation());
}

float UStateTreeDebugWidget::GetNearestPelletDistance() const
{
	if (!IsValid(TrackedEnemy)) return FLT_MAX;

	TArray<AActor*> Pellets;
	UGameplayStatics::GetAllActorsOfClass(
		TrackedEnemy->GetWorld(), APelletActor::StaticClass(), Pellets);

	float MinDist = FLT_MAX;
	for (const AActor* P : Pellets)
	{
		if (!IsValid(P)) continue;
		const float D = FVector::Dist(TrackedEnemy->GetActorLocation(), P->GetActorLocation());
		if (D < MinDist) MinDist = D;
	}
	return MinDist;
}

float UStateTreeDebugWidget::GetNearestCoinDistance() const
{
	if (!IsValid(TrackedEnemy)) return FLT_MAX;

	const AActor* Coin = TrackedEnemy->FindNearestCoin();
	if (!IsValid(Coin)) return FLT_MAX;
	return FVector::Dist(TrackedEnemy->GetActorLocation(), Coin->GetActorLocation());
}

float UStateTreeDebugWidget::GetHealthRatio() const
{
	if (!IsValid(TrackedEnemy)) return 1.0f;
	const UASEnemy* AS = TrackedEnemy->attributeSet;
	if (!AS) return 1.0f;
	const float Max = AS->GetMaxHealth();
	return (Max > 0.f) ? FMath::Clamp(AS->GetHealth() / Max, 0.f, 1.f) : 0.f;
}

bool UStateTreeDebugWidget::HasPowerPellet() const
{
	if (!IsValid(TrackedEnemy)) return false;
	if (const UEnemyCombatComponent* CC = TrackedEnemy->CombatComponent)
		return CC->bPowerPelletActive;
	return false;
}

// ============================================================
// Condition Helpers (FEnemyCondition_* 임계값 동기화)
// ============================================================

bool UStateTreeDebugWidget::EvalCondition_ServerConnected() const
{
	// FEnemyCondition_ServerConnected: 서버 연결 = !bIsLocalAIActive
	if (!IsValid(TrackedEnemy)) return false;
	return !TrackedEnemy->IsLocalAIActive();
}

bool UStateTreeDebugWidget::EvalCondition_HasNearbyRacer() const
{
	return CachedRacerDist <= RacerDistThreshold;
}

bool UStateTreeDebugWidget::EvalCondition_HasNearbyPellet() const
{
	return CachedPelletDist <= PelletDistThreshold;
}

bool UStateTreeDebugWidget::EvalCondition_LowHealth() const
{
	return CachedHealthRatio < LowHealthThreshold;
}

bool UStateTreeDebugWidget::EvalCondition_HasPowerPellet() const
{
	return bCachedPowerPellet;
}

// ============================================================
// Task Inference
// ============================================================

FString UStateTreeDebugWidget::InferCurrentTask() const
{
	if (!IsValid(TrackedEnemy)) return TEXT("- (No Enemy)");

	// StateTree가 비활성이면 서버 FSM 동작 중
	if (!TrackedEnemy->IsLocalAIActive())
		return TEXT("[Inactive] StateTree OFF — Server FSM Active");

	// StateTree 활성: 서버 연결됨 → ServerCommand 태스크
	if (bCachedServerConn)
		return TEXT("[ServerCommand] Execute Server Command");

	// 저체력 + 파워 펠릿 없음 → Retreat
	if (EvalCondition_LowHealth() && !bCachedPowerPellet)
		return TEXT("[Retreat] Low HP — Retreating to Safe Zone");

	// 펠릿 근처 + 파워 펠릿 없음 → ConsumePellet
	if (EvalCondition_HasNearbyPellet() && !bCachedPowerPellet)
		return TEXT("[ConsumePellet] Nearby Pellet — Moving to Consume");

	// Racer 근처 → ChaseRacer (거리에 따라 Chase 또는 Intercept 추론)
	if (EvalCondition_HasNearbyRacer())
	{
		if (CachedRacerDist < 800.0f)
			return TEXT("[ChaseRacer] Close Range — Direct Chase");
		return TEXT("[InterceptRacer / ChaseRacer] Mid Range — Chase or Intercept");
	}

	// 기본 → Patrol / Coin Chase
	const FString CoinInfo = (CachedCoinDist < FLT_MAX / 2.f)
		? FString::Printf(TEXT("  Coin: %.0f cm"), CachedCoinDist)
		: TEXT("  No Coins");
	return FString::Printf(TEXT("[Patrol / CoinChase] Wandering%s"), *CoinInfo);
}

// ============================================================
// Event Log
// ============================================================

void UStateTreeDebugWidget::PushEventLog(const FString& Message)
{
	const FString Entry = FString::Printf(
		TEXT("[%s] %s"), *FDateTime::Now().ToString(TEXT("%H:%M:%S")), *Message);
	EventLogBuffer.Insert(Entry, 0);
	if (EventLogBuffer.Num() > MaxLogEntries)
		EventLogBuffer.SetNum(MaxLogEntries);
}

void UStateTreeDebugWidget::FlushEventLog()
{
	if (!Text_EventLog) return;
	Text_EventLog->SetText(FText::FromString(FString::Join(EventLogBuffer, TEXT("\n"))));
}

// ============================================================
// Helpers
// ============================================================

void UStateTreeDebugWidget::ApplyConditionResult(
	UTextBlock* TextBlock, bool bPassed,
	const FString& PassLabel, const FString& FailLabel)
{
	if (!TextBlock) return;
	TextBlock->SetText(FText::FromString(bPassed ? PassLabel : FailLabel));
	TextBlock->SetColorAndOpacity(FSlateColor(
		bPassed ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.3f, 0.3f)));
}

FString UStateTreeDebugWidget::GetDirectiveStateName(EAIDirectiveState State)
{
	const UEnum* EnumPtr = StaticEnum<EAIDirectiveState>();
	if (!EnumPtr) return TEXT("Unknown");
	return EnumPtr->GetDisplayValueAsText(State).ToString();
}
