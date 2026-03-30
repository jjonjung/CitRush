// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Debug/EnemyDebugWidget.h"

#include "Enemy/PixelEnemy.h"
#include "Enemy/AiEnemy/AIDirectiveComponent.h"
#include "Enemy/Components/EnemyCombatComponent.h"
#include "GAS/AttributeSet/ASEnemy.h"
#include "Subsystems/EnemyAISubsystem.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Misc/DateTime.h"

// ============================================================
// Lifecycle
// ============================================================

void UEnemyDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PushEventLog(TEXT("[Widget] Ready. Call InitWithEnemy()."));
	FlushEventLog();
}

void UEnemyDebugWidget::NativeDestruct()
{
	// TrackedEnemy 델리게이트 정리 (nullptr 전달로 해제 처리 위임)
	InitWithEnemy(nullptr);
	Super::NativeDestruct();
}

void UEnemyDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(TrackedEnemy) || !IsVisible()) return;

	TickAccumulator += InDeltaTime;
	if (TickAccumulator < PollInterval) return;

	TickAccumulator = 0.f;
	RefreshAIPanel();
	RefreshServerPanel();
	RefreshCombatPanel();
}

// ============================================================
// Init
// ============================================================

void UEnemyDebugWidget::InitWithEnemy(APixelEnemy* Enemy)
{
	// 이전 Enemy 델리게이트 해제
	if (IsValid(TrackedEnemy))
	{
		if (UAIDirectiveComponent* DC = TrackedEnemy->AIDirectiveComponent)
		{
			DC->OnAIStateChanged.RemoveDynamic(this, &UEnemyDebugWidget::OnAIStateChanged);
		}
		if (UEnemyCombatComponent* CC = TrackedEnemy->CombatComponent)
		{
			CC->OnDamageReceived.RemoveDynamic(this, &UEnemyDebugWidget::OnDamageReceived);
			CC->OnInvulnerabilityChanged.RemoveDynamic(this, &UEnemyDebugWidget::OnInvulnerabilityChanged);
			CC->OnEnemyDeath.RemoveDynamic(this, &UEnemyDebugWidget::OnEnemyDeath);
		}
		// non-dynamic 델리게이트는 핸들로 해제
		if (TrackedEnemy->EnemyAISubsystem)
		{
			TrackedEnemy->EnemyAISubsystem->OnConnectionChanged.Remove(ConnectionChangedHandle);
		}
	}

	TrackedEnemy = Enemy;
	EventLogBuffer.Reset();

	if (!IsValid(Enemy))
	{
		PushEventLog(TEXT("[Tracking] Cleared."));
		FlushEventLog();
		return;
	}

	// 새 Enemy 델리게이트 바인딩
	// - OnAIStateChanged     : FSM 전환 즉시 로그 + UI 반영   (dynamic)
	// - OnDamageReceived     : 데미지 피드백 즉시 반영         (dynamic)
	// - OnInvulnerabilityChanged : 무적 상태 색상 즉시 전환   (dynamic)
	// - OnEnemyDeath         : 사망 이벤트 로그               (dynamic)
	// - OnConnectionChanged  : 서버 연결 상태 변경 즉시 반영  (non-dynamic, 핸들 저장)
	if (UAIDirectiveComponent* DC = Enemy->AIDirectiveComponent)
	{
		DC->OnAIStateChanged.AddDynamic(this, &UEnemyDebugWidget::OnAIStateChanged);
	}
	if (UEnemyCombatComponent* CC = Enemy->CombatComponent)
	{
		CC->OnDamageReceived.AddDynamic(this, &UEnemyDebugWidget::OnDamageReceived);
		CC->OnInvulnerabilityChanged.AddDynamic(this, &UEnemyDebugWidget::OnInvulnerabilityChanged);
		CC->OnEnemyDeath.AddDynamic(this, &UEnemyDebugWidget::OnEnemyDeath);
	}
	if (Enemy->EnemyAISubsystem)
	{
		ConnectionChangedHandle = Enemy->EnemyAISubsystem->OnConnectionChanged.AddUObject(
			this, &UEnemyDebugWidget::OnAIConnectionChanged);
	}

	PushEventLog(FString::Printf(TEXT("[Init] Tracking: %s"), *Enemy->GetEnemyID()));
	FlushEventLog();

	// 바인딩 직후 즉시 초기 상태 반영
	RefreshAIPanel();
	RefreshServerPanel();
	RefreshCombatPanel();
}

// ============================================================
// Polling
// ============================================================

void UEnemyDebugWidget::RefreshAIPanel()
{
	if (!IsValid(TrackedEnemy)) return;

	const bool bLocalAI = TrackedEnemy->IsLocalAIActive();

	// AI 모드 레이블
	// - 서버 연결됨  → 초록 "AI SERVER"
	// - 서버 끊김   → 주황 "STATE TREE (Local)"
	if (Text_AIMode)
	{
		Text_AIMode->SetText(FText::FromString(
			bLocalAI ? TEXT("STATE TREE (Local)") : TEXT("AI SERVER")));
		Text_AIMode->SetColorAndOpacity(FSlateColor(
			bLocalAI ? FLinearColor(1.f, 0.6f, 0.1f)     // 주황: 로컬 폴백
			         : FLinearColor(0.2f, 1.f, 0.4f)));  // 초록: 서버 연결
	}

	// 현재 Directive State 이름
	if (Text_CurrentState)
	{
		FString StateStr;
		if (bLocalAI)
		{
			StateStr = TEXT("(StateTree Active)");
		}
		else if (const UAIDirectiveComponent* DC = TrackedEnemy->AIDirectiveComponent)
		{
			StateStr = GetDirectiveStateName(DC->GetCurrentState());
		}
		else
		{
			StateStr = TEXT("-");
		}
		Text_CurrentState->SetText(FText::FromString(StateStr));
	}

	// 서버 연결 상태 힌트 (옵션 슬롯)
	if (Text_ServerStatus)
	{
		const bool bDisconnected = bLocalAI;
		Text_ServerStatus->SetText(FText::FromString(
			bDisconnected ? TEXT("Server: DISCONNECTED") : TEXT("Server: CONNECTED")));
		Text_ServerStatus->SetColorAndOpacity(FSlateColor(
			bDisconnected ? FLinearColor::Red : FLinearColor::Green));
	}
}

void UEnemyDebugWidget::RefreshServerPanel()
{
	if (!IsValid(TrackedEnemy) || !TrackedEnemy->EnemyAISubsystem) return;

	UEnemyAISubsystem* AISubsys = TrackedEnemy->EnemyAISubsystem;

	// URL + Port
	if (Text_ServerURL)
	{
		FString URL;
		int32 Port;
		AISubsys->GetServerSettings(URL, Port);
		Text_ServerURL->SetText(FText::FromString(
			FString::Printf(TEXT("%s:%d"), *URL, Port)));
	}

	// 연결 상태 (IsConnected는 UBaseAISubsystem의 bIsConnected 기반)
	if (Text_ConnectionStatus)
	{
		const bool bConnected = AISubsys->IsConnected();
		Text_ConnectionStatus->SetText(FText::FromString(
			bConnected ? TEXT("CONNECTED") : TEXT("DISCONNECTED")));
		Text_ConnectionStatus->SetColorAndOpacity(FSlateColor(
			bConnected ? FLinearColor(0.2f, 1.f, 0.4f) : FLinearColor(1.f, 0.25f, 0.25f)));
	}

	// Auto Decision Request 활성화 여부
	if (Text_AutoRequest)
	{
		const bool bAutoOn = AISubsys->IsAutoRequestActive();
		Text_AutoRequest->SetText(FText::FromString(
			FString::Printf(TEXT("Auto Request: %s"), bAutoOn ? TEXT("ON") : TEXT("OFF"))));
		Text_AutoRequest->SetColorAndOpacity(FSlateColor(
			bAutoOn ? FLinearColor(0.6f, 0.9f, 1.f) : FLinearColor(0.5f, 0.5f, 0.5f)));
	}

	// 등록된 Enemy 수
	if (Text_RegisteredEnemies)
	{
		Text_RegisteredEnemies->SetText(FText::FromString(
			FString::Printf(TEXT("Enemies: %d"), AISubsys->GetRegisteredEnemyCount())));
	}
}

void UEnemyDebugWidget::RefreshCombatPanel()
{
	if (!IsValid(TrackedEnemy)) return;

	const UASEnemy* AS = TrackedEnemy->attributeSet;
	if (!AS) return;

	const float Health    = AS->GetHealth();
	const float MaxHealth = AS->GetMaxHealth();
	const float Speed     = AS->GetSpeed();
	const float Attack    = AS->GetAttackPower();
	const float DetRange  = AS->GetDetectionRange();

	// HP 바: 비율에 따라 빨강(저체력) → 초록(고체력) 색상 보간
	if (Bar_Health)
	{
		const float Ratio = (MaxHealth > 0.f)
			? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;

		Bar_Health->SetPercent(Ratio);
		Bar_Health->SetFillColorAndOpacity(FLinearColor::LerpUsingHSV(
			FLinearColor(0.9f, 0.1f, 0.1f),   // 빨강 (저체력)
			FLinearColor(0.2f, 0.9f, 0.3f),   // 초록 (고체력)
			Ratio));
	}

	if (Text_HPValue)
	{
		Text_HPValue->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), Health, MaxHealth)));
	}

	// 무적 상태 (우선순위: 데미지 무적 > P-Pellet 무적 > 일반)
	if (Text_InvulStatus)
	{
		if (const UEnemyCombatComponent* CC = TrackedEnemy->CombatComponent)
		{
			const bool bUntouchable = CC->IsUntouchable();
			const bool bPowerPellet = CC->bPowerPelletActive;

			FString      InvulStr;
			FLinearColor InvulColor;

			if (bUntouchable)
			{
				InvulStr   = TEXT("INVULNERABLE [Damage]");
				InvulColor = FLinearColor(1.f, 0.85f, 0.f);    // 노랑
			}
			else if (bPowerPellet)
			{
				InvulStr   = TEXT("INVULNERABLE [P-Pellet]");
				InvulColor = FLinearColor(0.6f, 0.2f, 1.f);    // 보라
			}
			else
			{
				InvulStr   = TEXT("Normal");
				InvulColor = FLinearColor(0.65f, 0.65f, 0.65f);
			}

			Text_InvulStatus->SetText(FText::FromString(InvulStr));
			Text_InvulStatus->SetColorAndOpacity(FSlateColor(InvulColor));
		}
	}

	if (Text_Speed)
	{
		Text_Speed->SetText(FText::FromString(
			FString::Printf(TEXT("Speed: %.0f"), Speed)));
	}
	if (Text_AttackPower)
	{
		Text_AttackPower->SetText(FText::FromString(
			FString::Printf(TEXT("ATK: %.1f"), Attack)));
	}
	if (Text_DetectionRange)
	{
		Text_DetectionRange->SetText(FText::FromString(
			FString::Printf(TEXT("Det.Range: %.0f"), DetRange)));
	}
}

// ============================================================
// Delegate Callbacks
// (폴링 대기 없이 즉시 로그 + UI 반영)
// ============================================================

void UEnemyDebugWidget::OnAIStateChanged(EAIDirectiveState NewState, EAIDirectiveState OldState)
{
	PushEventLog(FString::Printf(TEXT("[State] %s -> %s"),
		*GetDirectiveStateName(OldState), *GetDirectiveStateName(NewState)));
	FlushEventLog();
	RefreshAIPanel();
}

void UEnemyDebugWidget::OnDamageReceived(float DamageAmount, FVector HitLocation)
{
	PushEventLog(FString::Printf(TEXT("[Dmg] -%.1f  @ (%.0f, %.0f, %.0f)"),
		DamageAmount, HitLocation.X, HitLocation.Y, HitLocation.Z));
	FlushEventLog();
	RefreshCombatPanel();
}

void UEnemyDebugWidget::OnInvulnerabilityChanged(bool bIsInvulnerable)
{
	PushEventLog(FString::Printf(TEXT("[Combat] Invulnerable: %s"),
		bIsInvulnerable ? TEXT("ON") : TEXT("OFF")));
	FlushEventLog();
	RefreshCombatPanel();
}

void UEnemyDebugWidget::OnEnemyDeath()
{
	PushEventLog(TEXT("[DEAD] Enemy destroyed."));
	FlushEventLog();
}

void UEnemyDebugWidget::OnAIConnectionChanged(bool bConnected)
{
	// non-dynamic 델리게이트 콜백 — 연결 상태 변경 즉시 모든 패널 갱신
	PushEventLog(FString::Printf(TEXT("[Network] Server %s"),
		bConnected ? TEXT("CONNECTED") : TEXT("DISCONNECTED")));
	FlushEventLog();

	// 연결 상태 변경은 AI 모드(StateTree/Server)와 서버 패널 모두에 영향
	RefreshAIPanel();
	RefreshServerPanel();
}

// ============================================================
// Helpers
// ============================================================

void UEnemyDebugWidget::PushEventLog(const FString& Message)
{
	// 타임스탬프 포함, 최신 항목이 맨 위에 쌓임
	const FString Entry = FString::Printf(
		TEXT("[%s] %s"), *FDateTime::Now().ToString(TEXT("%H:%M:%S")), *Message);

	EventLogBuffer.Insert(Entry, 0);

	if (EventLogBuffer.Num() > MaxLogEntries)
	{
		EventLogBuffer.SetNum(MaxLogEntries);
	}
}

void UEnemyDebugWidget::FlushEventLog()
{
	if (!Text_EventLog) return;
	Text_EventLog->SetText(FText::FromString(FString::Join(EventLogBuffer, TEXT("\n"))));
}

FString UEnemyDebugWidget::GetDirectiveStateName(EAIDirectiveState State)
{
	const UEnum* EnumPtr = StaticEnum<EAIDirectiveState>();
	if (!EnumPtr) return TEXT("Unknown");
	// GetDisplayValueAsText: UMETA(DisplayName) 우선, 없으면 열거자 이름 반환
	return EnumPtr->GetDisplayValueAsText(State).ToString();
}
