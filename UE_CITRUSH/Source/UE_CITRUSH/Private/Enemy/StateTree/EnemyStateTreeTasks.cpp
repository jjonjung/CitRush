// EnemyStateTreeTasks.cpp

#include "Enemy/StateTree/EnemyStateTreeTasks.h"
#include "Enemy/PixelEnemy.h"
#include "Enemy/AiEnemy/AIDirectiveComponent.h"
#include "Player/AbstractRacer.h"
#include "Player/Stats/Coin/CoinActor.h"
#include "Enemy/Pellet/PelletActor.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Kismet/GameplayStatics.h"

// ============================================================================
// Base Task Helpers
// ============================================================================

APixelEnemy* FEnemyTaskBase::GetOwnerEnemy(FStateTreeExecutionContext& Context) const
{
	FEnemyTaskInstanceData& Data = Context.GetInstanceData<FEnemyTaskInstanceData>(*this);

	if (!Data.OwnerEnemy)
	{
		if (const AActor* Owner = Cast<AActor>(Context.GetOwner()))
		{
			Data.OwnerEnemy = const_cast<APixelEnemy*>(Cast<APixelEnemy>(Owner));
		}
	}

	return Data.OwnerEnemy;
}

UAIDirectiveComponent* FEnemyTaskBase::GetDirectiveComponent(FStateTreeExecutionContext& Context) const
{
	FEnemyTaskInstanceData& Data = Context.GetInstanceData<FEnemyTaskInstanceData>(*this);

	if (!Data.DirectiveComponent)
	{
		if (APixelEnemy* Enemy = GetOwnerEnemy(Context))
		{
			Data.DirectiveComponent = Enemy->AIDirectiveComponent;
		}
	}

	return Data.DirectiveComponent;
}

// ============================================================================
// Chase Racer Task
// ============================================================================

EStateTreeRunStatus FEnemyTask_ChaseRacer::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	APixelEnemy* Enemy = GetOwnerEnemy(Context);
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);

	if (!Enemy || !Directive)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Server authority check
	if (!Enemy->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	FEnemyTaskInstanceData& Data = Context.GetInstanceData<FEnemyTaskInstanceData>(*this);
	Data.ElapsedTime = 0.0f;

	// Find target racer
	AAbstractRacer* TargetRacer = Enemy->FindNearestRacer();
	if (!TargetRacer)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Setup directive params
	FDirectiveParams Params;
	Params.TargetPosition = TargetRacer->GetActorLocation();
	Params.SpeedFactor = SpeedFactor;
	Params.MaxChaseDuration = MaxDuration;
	Params.Aggressiveness = Aggressiveness;

	// Execute CHASE directive (code 4)
	Directive->ProcessDirective(4, Params);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyTask_ChaseRacer::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FEnemyTaskInstanceData& Data = Context.GetInstanceData<FEnemyTaskInstanceData>(*this);
	Data.ElapsedTime += DeltaTime;

	// Check timeout
	if (Data.ElapsedTime >= MaxDuration)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// Continue running
	return EStateTreeRunStatus::Running;
}

void FEnemyTask_ChaseRacer::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// Cleanup if needed
}

// ============================================================================
// Intercept Racer Task
// ============================================================================

EStateTreeRunStatus FEnemyTask_InterceptRacer::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	APixelEnemy* Enemy = GetOwnerEnemy(Context);
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);

	if (!Enemy || !Directive || !Enemy->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	AAbstractRacer* TargetRacer = Enemy->FindNearestRacer();
	if (!TargetRacer)
	{
		return EStateTreeRunStatus::Failed;
	}

	FDirectiveParams Params;
	Params.ApproachAngle = ApproachAngle;
	Params.InterceptDistance = InterceptDistance;
	Params.Aggressiveness = 0.8f;

	// Execute INTERCEPT directive (code 3)
	Directive->ProcessDirective(3, Params);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyTask_InterceptRacer::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);
	if (!Directive)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Check if intercept completed (transitioned to Chase or Idle)
	EAIDirectiveState CurrentState = Directive->GetCurrentState();
	if (CurrentState != EAIDirectiveState::Intercept)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

// ============================================================================
// Consume Pellet Task
// ============================================================================

EStateTreeRunStatus FEnemyTask_ConsumePellet::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	APixelEnemy* Enemy = GetOwnerEnemy(Context);
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);

	if (!Enemy || !Directive || !Enemy->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	FDirectiveParams Params;
	Params.EmergencyPriority = EmergencyPriority;

	// Execute CONSUME_PELLET directive (code 8)
	Directive->ProcessDirective(8, Params);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyTask_ConsumePellet::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);
	if (!Directive)
	{
		return EStateTreeRunStatus::Failed;
	}

	EAIDirectiveState CurrentState = Directive->GetCurrentState();
	if (CurrentState != EAIDirectiveState::ConsumePellet)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

// ============================================================================
// Retreat Task
// ============================================================================

EStateTreeRunStatus FEnemyTask_Retreat::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	APixelEnemy* Enemy = GetOwnerEnemy(Context);
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);

	if (!Enemy || !Directive || !Enemy->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	FDirectiveParams Params;
	Params.SpeedFactor = SpeedFactor;

	// Calculate retreat position (away from nearest racer)
	AAbstractRacer* NearestRacer = Enemy->FindNearestRacer();
	if (NearestRacer)
	{
		FVector DirFromRacer = Enemy->GetActorLocation() - NearestRacer->GetActorLocation();
		DirFromRacer.Z = 0.0f;
		DirFromRacer.Normalize();
		Params.TargetPosition = Enemy->GetActorLocation() + (DirFromRacer * 2000.0f);
	}

	// Execute RETREAT directive (code 5)
	Directive->ProcessDirective(5, Params);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyTask_Retreat::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);
	if (!Directive)
	{
		return EStateTreeRunStatus::Failed;
	}

	EAIDirectiveState CurrentState = Directive->GetCurrentState();
	if (CurrentState != EAIDirectiveState::Retreat)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

// ============================================================================
// Patrol Task
// ============================================================================

EStateTreeRunStatus FEnemyTask_Patrol::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	APixelEnemy* Enemy = GetOwnerEnemy(Context);
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);

	if (!Enemy || !Directive || !Enemy->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	if (bPreferCoins)
	{
		// Try coin chase first
		ACoinActor* NearestCoin = Enemy->FindNearestCoin();
		if (NearestCoin)
		{
			FDirectiveParams Params;
			Params.TargetPosition = NearestCoin->GetActorLocation();
			Params.SpeedFactor = PatrolSpeed;

			// Execute CONSUME_P_POINT directive (code 7)
			Directive->ProcessDirective(7, Params);
			return EStateTreeRunStatus::Running;
		}
	}

	// Fallback to patrol
	FDirectiveParams Params;
	Params.PatrolSpeed = PatrolSpeed;

	// Execute PATROL directive (code 6)
	Directive->ProcessDirective(6, Params);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyTask_Patrol::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// Patrol continues indefinitely
	return EStateTreeRunStatus::Running;
}

// ============================================================================
// Server Command Task
// ============================================================================

EStateTreeRunStatus FEnemyTask_ServerCommand::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// Server command mode - AIDirectiveComponent handles everything
	// This task just monitors the state
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyTask_ServerCommand::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// Continue running while server is connected
	// Transition conditions will handle switching to local AI
	return EStateTreeRunStatus::Running;
}

// ============================================================================
// Flank Task
// ============================================================================

EStateTreeRunStatus FEnemyTask_Flank::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	APixelEnemy* Enemy = GetOwnerEnemy(Context);
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);

	if (!Enemy || !Directive || !Enemy->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	AAbstractRacer* TargetRacer = Enemy->FindNearestRacer();
	if (!TargetRacer)
	{
		return EStateTreeRunStatus::Failed;
	}

	FDirectiveParams Params;
	Params.FlankDistance = FlankDistance;

	// Auto-determine flank direction based on position
	if (FlankDirection.Equals(TEXT("AUTO"), ESearchCase::IgnoreCase))
	{
		FVector ToRacer = TargetRacer->GetActorLocation() - Enemy->GetActorLocation();
		FVector RacerRight = FVector::CrossProduct(TargetRacer->GetActorForwardVector(), FVector::UpVector);

		float DotProduct = FVector::DotProduct(ToRacer.GetSafeNormal(), RacerRight);
		Params.FlankDirection = (DotProduct > 0) ? TEXT("LEFT") : TEXT("RIGHT");
	}
	else
	{
		Params.FlankDirection = FlankDirection;
	}

	// Execute FLANK directive (code 10)
	Directive->ProcessDirective(10, Params);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyTask_Flank::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	UAIDirectiveComponent* Directive = GetDirectiveComponent(Context);
	if (!Directive)
	{
		return EStateTreeRunStatus::Failed;
	}

	EAIDirectiveState CurrentState = Directive->GetCurrentState();
	if (CurrentState != EAIDirectiveState::Flank)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}
