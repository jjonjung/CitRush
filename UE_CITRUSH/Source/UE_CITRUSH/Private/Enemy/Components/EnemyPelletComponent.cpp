// EnemyPelletComponent.cpp

#include "Enemy/Components/EnemyPelletComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

UEnemyPelletComponent::UEnemyPelletComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEnemyPelletComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEnemyPelletComponent, bPowerPellet);
	DOREPLIFETIME(UEnemyPelletComponent, Cooldown);
}

void UEnemyPelletComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize shield as hidden
	UpdateShieldVisual(false);
}

void UEnemyPelletComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clear all timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InvulnerabilityTimer);
		World->GetTimerManager().ClearTimer(CooldownTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void UEnemyPelletComponent::OnPelletCollected(float Duration)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// Activate power pellet
	bool bWasActive = bPowerPellet;
	bPowerPellet = true;

	// Update visual
	UpdateShieldVisual(true);

	// Record consumption time (ISO 8601)
	LastConsumedAt = FDateTime::UtcNow().ToIso8601();

	// Set cooldown
	Cooldown = CooldownDuration;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Clear existing timers
	World->GetTimerManager().ClearTimer(InvulnerabilityTimer);
	World->GetTimerManager().ClearTimer(CooldownTimer);

	// Start invulnerability timer
	World->GetTimerManager().SetTimer(
		InvulnerabilityTimer,
		this,
		&UEnemyPelletComponent::OnInvulnerabilityEnd,
		Duration,
		false
	);

	// Start cooldown timer (decreases every second)
	World->GetTimerManager().SetTimer(
		CooldownTimer,
		this,
		&UEnemyPelletComponent::DecreaseCooldown,
		1.0f,
		true
	);

	// Broadcast event
	if (!bWasActive)
	{
		OnPelletStateChanged.Broadcast(true);
	}
}

void UEnemyPelletComponent::OnInvulnerabilityEnd()
{
	bPowerPellet = false;
	UpdateShieldVisual(false);

	// Broadcast event
	OnPelletStateChanged.Broadcast(false);
}

void UEnemyPelletComponent::DecreaseCooldown()
{
	Cooldown = FMath::Max(0.0f, Cooldown - 1.0f);

	// Broadcast cooldown update
	OnPelletCooldownChanged.Broadcast(Cooldown);

	// Stop timer when cooldown reaches zero
	if (Cooldown <= 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CooldownTimer);
		}
	}
}

void UEnemyPelletComponent::OnRep_PowerPellet()
{
	// Client-side: Update visual and broadcast event
	UpdateShieldVisual(bPowerPellet);
	OnPelletStateChanged.Broadcast(bPowerPellet);
}

void UEnemyPelletComponent::UpdateShieldVisual(bool bShow)
{
	if (ShieldMesh)
	{
		ShieldMesh->SetVisibility(bShow);
	}
}
