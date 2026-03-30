// EnemyCombatComponent.cpp

#include "Enemy/Components/EnemyCombatComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Engine/World.h"

UEnemyCombatComponent::UEnemyCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEnemyCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEnemyCombatComponent, bUntouchable);
}

void UEnemyCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// Setup flash timeline if curve is available
	SetupFlashTimeline();
}

void UEnemyCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clear timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InvulnerabilityTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void UEnemyCombatComponent::SetupFlashTimeline()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Create timeline if not exists
	if (!FlashTimeline)
	{
		FlashTimeline = NewObject<UTimelineComponent>(Owner, TEXT("CombatFlashTimeline"));
		if (FlashTimeline)
		{
			FlashTimeline->RegisterComponent();
			FlashTimeline->SetLooping(false);
		}
	}

	// Setup material instance
	if (OwnerMesh && !FlashMaterial)
	{
		FlashMaterial = OwnerMesh->CreateDynamicMaterialInstance(0);
	}

	// Bind curve to timeline
	if (FlashTimeline && FlashCurve && FlashMaterial)
	{
		FOnTimelineFloat ProgressCallback;
		ProgressCallback.BindDynamic(this, &UEnemyCombatComponent::OnFlashProgress);
		FlashTimeline->AddInterpFloat(FlashCurve, ProgressCallback);
	}
}

void UEnemyCombatComponent::StartDamageInvulnerability(float Duration)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// Set untouchable state
	bool bWasUntouchable = bUntouchable;
	bUntouchable = true;

	// Play flash effect
	PlayFlashEffect();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Clear existing timer
	World->GetTimerManager().ClearTimer(InvulnerabilityTimer);

	// Start invulnerability timer
	World->GetTimerManager().SetTimer(
		InvulnerabilityTimer,
		this,
		&UEnemyCombatComponent::OnInvulnerabilityEnd,
		Duration > 0.0f ? Duration : InvulnerabilityDuration,
		false
	);

	// Broadcast event
	if (!bWasUntouchable)
	{
		OnInvulnerabilityChanged.Broadcast(true);
	}
}

void UEnemyCombatComponent::EndInvulnerability()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (!bUntouchable)
	{
		return; // Already not invulnerable
	}

	bUntouchable = false;
	StopFlashEffect();

	// Broadcast event
	OnInvulnerabilityChanged.Broadcast(false);

	// Clear timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InvulnerabilityTimer);
	}
}

void UEnemyCombatComponent::OnInvulnerabilityEnd()
{
	bUntouchable = false;
	StopFlashEffect();

	// Broadcast event
	OnInvulnerabilityChanged.Broadcast(false);
}

void UEnemyCombatComponent::OnRep_Untouchable()
{
	// Client-side: Broadcast event
	OnInvulnerabilityChanged.Broadcast(bUntouchable);

	// Update visual
	if (bUntouchable)
	{
		PlayFlashEffect();
	}
	else
	{
		StopFlashEffect();
	}
}

bool UEnemyCombatComponent::IsInvulnerable() const
{
	return bUntouchable || bPowerPelletActive;
}

void UEnemyCombatComponent::PlayFlashEffect()
{
	if (FlashTimeline)
	{
		FlashTimeline->PlayFromStart();
	}
}

void UEnemyCombatComponent::StopFlashEffect()
{
	if (FlashTimeline)
	{
		FlashTimeline->Stop();
	}

	// Reset material
	if (FlashMaterial)
	{
		FlashMaterial->SetScalarParameterValue(TEXT("FlashIntensity"), 1.0f);
	}
}

void UEnemyCombatComponent::OnFlashProgress(float Value)
{
	if (FlashMaterial)
	{
		FlashMaterial->SetScalarParameterValue(TEXT("FlashIntensity"), Value);
	}
}
