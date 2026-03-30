// EnemyStateTreeTasks.h
// StateTree Tasks for PixelEnemy AI Actions

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionContext.h"
#include "Enemy/StateTree/EnemyStateTreeSchema.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnemyStateTreeTasks.generated.h"

class UEnvQuery;
class APixelEnemy;
class UAIDirectiveComponent;

/**
 * Task Instance Data (shared across tasks)
 */
USTRUCT()
struct FEnemyTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<APixelEnemy> OwnerEnemy = nullptr;

	UPROPERTY()
	TObjectPtr<UAIDirectiveComponent> DirectiveComponent = nullptr;

	UPROPERTY()
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY()
	float ElapsedTime = 0.0f;
};

// ============================================================================
// Base Task
// ============================================================================

/**
 * Base Task for Enemy AI Actions
 */
USTRUCT()
struct UE_CITRUSH_API FEnemyTaskBase : public FStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyTaskInstanceData;

protected:
	virtual const UStruct* GetInstanceDataType() const override { return FEnemyTaskInstanceData::StaticStruct(); }

	/** Helper: Get Owner Enemy from Context */
	APixelEnemy* GetOwnerEnemy(FStateTreeExecutionContext& Context) const;

	/** Helper: Get Directive Component from Context */
	UAIDirectiveComponent* GetDirectiveComponent(FStateTreeExecutionContext& Context) const;
};

// ============================================================================
// Chase Task
// ============================================================================

/**
 * Task: Chase nearest racer
 * Uses AIPerception for target detection
 */
USTRUCT(meta = (DisplayName = "Chase Racer"))
struct UE_CITRUSH_API FEnemyTask_ChaseRacer : public FEnemyTaskBase
{
	GENERATED_BODY()

	/** Chase speed factor */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	float SpeedFactor = 1.2f;

	/** Max chase duration (seconds) */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	float MaxDuration = 10.0f;

	/** Aggressiveness (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, Category = "Parameters", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Aggressiveness = 0.5f;

protected:
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ============================================================================
// Intercept Task (with EQS)
// ============================================================================

/**
 * Task: Intercept racer using EQS for optimal position
 */
USTRUCT(meta = (DisplayName = "Intercept Racer (EQS)"))
struct UE_CITRUSH_API FEnemyTask_InterceptRacer : public FEnemyTaskBase
{
	GENERATED_BODY()

	/** EQS Query for finding intercept position */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> InterceptQuery;

	/** Approach angle (degrees) */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	float ApproachAngle = 45.0f;

	/** Intercept distance */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	float InterceptDistance = 500.0f;

protected:
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ============================================================================
// Consume Pellet Task
// ============================================================================

/**
 * Task: Move to and consume nearest pellet
 */
USTRUCT(meta = (DisplayName = "Consume Pellet"))
struct UE_CITRUSH_API FEnemyTask_ConsumePellet : public FEnemyTaskBase
{
	GENERATED_BODY()

	/** Emergency priority (higher = faster) */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	int32 EmergencyPriority = 0;

protected:
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ============================================================================
// Retreat Task (with EQS)
// ============================================================================

/**
 * Task: Retreat to safe position using EQS
 */
USTRUCT(meta = (DisplayName = "Retreat (EQS)"))
struct UE_CITRUSH_API FEnemyTask_Retreat : public FEnemyTaskBase
{
	GENERATED_BODY()

	/** EQS Query for finding safe position */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> SafePositionQuery;

	/** Retreat speed factor */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	float SpeedFactor = 1.0f;

protected:
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ============================================================================
// Patrol Task
// ============================================================================

/**
 * Task: Patrol area (coin chase fallback)
 */
USTRUCT(meta = (DisplayName = "Patrol / Coin Chase"))
struct UE_CITRUSH_API FEnemyTask_Patrol : public FEnemyTaskBase
{
	GENERATED_BODY()

	/** Patrol speed factor */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	float PatrolSpeed = 0.8f;

	/** Prefer coins over random patrol */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	bool bPreferCoins = true;

protected:
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ============================================================================
// Server Command Task (passthrough to AIDirectiveComponent)
// ============================================================================

/**
 * Task: Execute server command via AIDirectiveComponent
 * Used when server is connected
 */
USTRUCT(meta = (DisplayName = "Execute Server Command"))
struct UE_CITRUSH_API FEnemyTask_ServerCommand : public FEnemyTaskBase
{
	GENERATED_BODY()

protected:
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ============================================================================
// Flank Task (with EQS)
// ============================================================================

/**
 * Task: Flank racer from side using EQS
 */
USTRUCT(meta = (DisplayName = "Flank Racer (EQS)"))
struct UE_CITRUSH_API FEnemyTask_Flank : public FEnemyTaskBase
{
	GENERATED_BODY()

	/** EQS Query for finding flank position */
	UPROPERTY(EditAnywhere, Category = "EQS")
	TObjectPtr<UEnvQuery> FlankQuery;

	/** Flank direction (LEFT or RIGHT, empty for auto) */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	FString FlankDirection = TEXT("AUTO");

	/** Flank distance */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	float FlankDistance = 800.0f;

protected:
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
