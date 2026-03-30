// EnemyStateTreeEvaluators.cpp

#include "Enemy/StateTree/EnemyStateTreeEvaluators.h"
#include "Enemy/PixelEnemy.h"
#include "Enemy/AiEnemy/AIDirectiveComponent.h"
#include "Enemy/Components/EnemyPelletComponent.h"
#include "Player/AbstractRacer.h"
#include "Player/Stats/Coin/CoinActor.h"
#include "Enemy/Pellet/PelletActor.h"
#include "Subsystems/EnemyAISubsystem.h"
#include "GAS/AttributeSet/ASEnemy.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

// ============================================================================
// FEnemyContextEvaluator
// ============================================================================

void FEnemyContextEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	FEnemyStateTreeContextData& Data = Context.GetInstanceData<FEnemyStateTreeContextData>(*this);

	// Get Owner from StateTree Component's Owner
	if (const AActor* Owner = Cast<AActor>(Context.GetOwner()))
	{
		Data.OwnerEnemy = const_cast<APixelEnemy*>(Cast<APixelEnemy>(Owner));
		if (Data.OwnerEnemy)
		{
			Data.DirectiveComponent = Data.OwnerEnemy->AIDirectiveComponent;
		}
	}
}

void FEnemyContextEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FEnemyStateTreeContextData& Data = Context.GetInstanceData<FEnemyStateTreeContextData>(*this);

	if (!Data.OwnerEnemy)
	{
		return;
	}

	// Server authority check (Multiplayer)
	if (!Data.OwnerEnemy->HasAuthority())
	{
		return;
	}

	// Update server connection status
	Data.bIsServerConnected = IsServerConnected(Data.OwnerEnemy);

	// Update nearby object distances
	Data.NearestRacerDistance = FindNearestRacerDistance(Data.OwnerEnemy);
	Data.NearestPelletDistance = FindNearestPelletDistance(Data.OwnerEnemy);
	Data.NearestCoinDistance = FindNearestCoinDistance(Data.OwnerEnemy);

	// Update health ratio from GAS AttributeSet
	if (Data.OwnerEnemy->attributeSet)
	{
		float MaxHP = Data.OwnerEnemy->attributeSet->GetMaxHealth();
		Data.HealthRatio = (MaxHP > 0.0f)
			? Data.OwnerEnemy->attributeSet->GetHealth() / MaxHP
			: 1.0f;
	}

	// Update power pellet status from PelletComponent
	Data.bHasPowerPellet = Data.OwnerEnemy->PelletComponent
		? Data.OwnerEnemy->PelletComponent->IsPowerPelletActive()
		: false;
}

float FEnemyContextEvaluator::FindNearestRacerDistance(const APixelEnemy* Enemy) const
{
	if (!Enemy)
	{
		return FLT_MAX;
	}

	const AAbstractRacer* NearestRacer = Enemy->FindNearestRacer();
	if (!NearestRacer)
	{
		return FLT_MAX;
	}

	return FVector::Dist(Enemy->GetActorLocation(), NearestRacer->GetActorLocation());
}

float FEnemyContextEvaluator::FindNearestPelletDistance(const APixelEnemy* Enemy) const
{
	if (!Enemy || !Enemy->GetWorld())
	{
		return FLT_MAX;
	}

	UEnemyAISubsystem* Subsystem = Enemy->GetWorld()->GetGameInstance()->GetSubsystem<UEnemyAISubsystem>();
	if (!Subsystem)
	{
		return FLT_MAX;
	}

	const TArray<TWeakObjectPtr<APelletActor>>& CachedPellets = Subsystem->GetCachedPellets();

	float MinDistance = FLT_MAX;
	FVector EnemyLocation = Enemy->GetActorLocation();

	for (const TWeakObjectPtr<APelletActor>& PelletPtr : CachedPellets)
	{
		APelletActor* Pellet = PelletPtr.Get();
		if (!Pellet || !Pellet->IsAvailable())
		{
			continue;
		}

		float Distance = FVector::Dist(EnemyLocation, Pellet->GetActorLocation());
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
		}
	}

	return MinDistance;
}

float FEnemyContextEvaluator::FindNearestCoinDistance(const APixelEnemy* Enemy) const
{
	if (!Enemy)
	{
		return FLT_MAX;
	}

	const ACoinActor* NearestCoin = Enemy->FindNearestCoin();
	if (!NearestCoin)
	{
		return FLT_MAX;
	}

	return FVector::Dist(Enemy->GetActorLocation(), NearestCoin->GetActorLocation());
}

bool FEnemyContextEvaluator::IsServerConnected(const APixelEnemy* Enemy) const
{
	if (!Enemy)
	{
		return false;
	}

	// Check via EnemyAISubsystem
	UEnemyAISubsystem* Subsystem = Enemy->GetWorld()->GetGameInstance()->GetSubsystem<UEnemyAISubsystem>();
	if (Subsystem)
	{
		return Subsystem->IsConnected();
	}

	return false;
}

// ============================================================================
// Conditions
// ============================================================================

bool FEnemyCondition_ServerConnected::TestCondition(FStateTreeExecutionContext& Context) const
{
	// Get owner directly from Context
	const APixelEnemy* Enemy = Cast<APixelEnemy>(Context.GetOwner());
	if (!Enemy)
	{
		return bInvert;
	}

	// Check via EnemyAISubsystem
	bool bResult = false;
	if (UWorld* World = Enemy->GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UEnemyAISubsystem* Subsystem = GI->GetSubsystem<UEnemyAISubsystem>())
			{
				bResult = Subsystem->IsConnected();
			}
		}
	}

	return bInvert ? !bResult : bResult;
}

bool FEnemyCondition_HasNearbyPellet::TestCondition(FStateTreeExecutionContext& Context) const
{
	const APixelEnemy* Enemy = Cast<APixelEnemy>(Context.GetOwner());
	if (!Enemy || !Enemy->GetWorld())
	{
		return false;
	}

	UEnemyAISubsystem* Subsystem = Enemy->GetWorld()->GetGameInstance()->GetSubsystem<UEnemyAISubsystem>();
	if (!Subsystem)
	{
		return false;
	}

	const TArray<TWeakObjectPtr<APelletActor>>& CachedPellets = Subsystem->GetCachedPellets();

	FVector EnemyLocation = Enemy->GetActorLocation();
	for (const TWeakObjectPtr<APelletActor>& PelletPtr : CachedPellets)
	{
		APelletActor* Pellet = PelletPtr.Get();
		if (!Pellet || !Pellet->IsAvailable())
		{
			continue;
		}

		float Distance = FVector::Dist(EnemyLocation, Pellet->GetActorLocation());
		if (Distance < DistanceThreshold)
		{
			return true;
		}
	}

	return false;
}

bool FEnemyCondition_HasNearbyRacer::TestCondition(FStateTreeExecutionContext& Context) const
{
	const APixelEnemy* Enemy = Cast<APixelEnemy>(Context.GetOwner());
	if (!Enemy)
	{
		return false;
	}

	const AAbstractRacer* NearestRacer = Enemy->FindNearestRacer();
	if (!NearestRacer)
	{
		return false;
	}

	float Distance = FVector::Dist(Enemy->GetActorLocation(), NearestRacer->GetActorLocation());
	return Distance < DistanceThreshold;
}

bool FEnemyCondition_LowHealth::TestCondition(FStateTreeExecutionContext& Context) const
{
	const APixelEnemy* Enemy = Cast<APixelEnemy>(Context.GetOwner());
	if (!Enemy)
	{
		return false;
	}

	// Get health ratio from GAS AttributeSet
	if (Enemy->attributeSet)
	{
		float CurrentHealth = Enemy->attributeSet->GetHealth();
		float MaxHealth = Enemy->attributeSet->GetMaxHealth();
		if (MaxHealth > 0.0f)
		{
			float HealthRatio = CurrentHealth / MaxHealth;
			return HealthRatio < HealthThreshold;
		}
	}

	return false;
}

bool FEnemyCondition_HasPowerPellet::TestCondition(FStateTreeExecutionContext& Context) const
{
	const APixelEnemy* Enemy = Cast<APixelEnemy>(Context.GetOwner());
	if (!Enemy)
	{
		return false;
	}

	// Check via PelletComponent or legacy flag
	if (Enemy->PelletComponent)
	{
		return Enemy->PelletComponent->IsPowerPelletActive();
	}

	return false;
}
