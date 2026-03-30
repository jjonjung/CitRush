// EnemyStateTreeEvaluators.h
// StateTree Evaluators for PixelEnemy AI

#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeConditionBase.h"
#include "StateTreeExecutionContext.h"
#include "Enemy/StateTree/EnemyStateTreeSchema.h"
#include "EnemyStateTreeEvaluators.generated.h"

class APixelEnemy;
class AAbstractRacer;
class APelletActor;
class ACoinActor;

/**
 * Evaluator: Updates Context Data every tick
 * - Server Connection Status
 * - Nearby Objects (Racer, Pellet, Coin)
 * - Health Status
 */
USTRUCT(meta = (DisplayName = "Enemy Context Evaluator"))
struct UE_CITRUSH_API FEnemyContextEvaluator : public FStateTreeEvaluatorBase
{
	GENERATED_BODY()

	using FInstanceDataType = FEnemyStateTreeContextData;

	/** Detection Range for nearby objects */
	UPROPERTY(EditAnywhere, Category = "Settings")
	float DetectionRange = 3000.0f;

	/** Low Health Threshold (for retreat decision) */
	UPROPERTY(EditAnywhere, Category = "Settings")
	float LowHealthThreshold = 0.3f;

	/** Output: Context Data */
	UPROPERTY(EditAnywhere, Category = "Output")
	FStateTreeStructRef ContextData;

	// FStateTreeEvaluatorBase interface
	virtual const UStruct* GetInstanceDataType() const override { return FEnemyStateTreeContextData::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	/** Find nearest racer and return distance */
	float FindNearestRacerDistance(const APixelEnemy* Enemy) const;

	/** Find nearest pellet and return distance */
	float FindNearestPelletDistance(const APixelEnemy* Enemy) const;

	/** Find nearest coin and return distance */
	float FindNearestCoinDistance(const APixelEnemy* Enemy) const;

	/** Check if AI server is connected */
	bool IsServerConnected(const APixelEnemy* Enemy) const;
};

/**
 * Condition: Check if Server is Connected
 * Note: StateTree conditions are structs - no virtual functions allowed
 */
USTRUCT(meta = (DisplayName = "Is Server Connected"))
struct UE_CITRUSH_API FEnemyCondition_ServerConnected : public FStateTreeConditionBase
{
	GENERATED_BODY()

	/** Invert condition (true = disconnected) */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;

	bool TestCondition(FStateTreeExecutionContext& Context) const;
};

/**
 * Condition: Check if Pellet is Nearby
 */
USTRUCT(meta = (DisplayName = "Has Nearby Pellet"))
struct UE_CITRUSH_API FEnemyCondition_HasNearbyPellet : public FStateTreeConditionBase
{
	GENERATED_BODY()

	/** Distance threshold to consider "nearby" */
	UPROPERTY(EditAnywhere, Category = "Condition")
	float DistanceThreshold = 1500.0f;

	bool TestCondition(FStateTreeExecutionContext& Context) const;
};

/**
 * Condition: Check if Racer is Nearby
 */
USTRUCT(meta = (DisplayName = "Has Nearby Racer"))
struct UE_CITRUSH_API FEnemyCondition_HasNearbyRacer : public FStateTreeConditionBase
{
	GENERATED_BODY()

	/** Distance threshold to consider "nearby" */
	UPROPERTY(EditAnywhere, Category = "Condition")
	float DistanceThreshold = 2000.0f;

	bool TestCondition(FStateTreeExecutionContext& Context) const;
};

/**
 * Condition: Check if Health is Low
 */
USTRUCT(meta = (DisplayName = "Is Low Health"))
struct UE_CITRUSH_API FEnemyCondition_LowHealth : public FStateTreeConditionBase
{
	GENERATED_BODY()

	/** Health ratio threshold (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthThreshold = 0.3f;

	bool TestCondition(FStateTreeExecutionContext& Context) const;
};

/**
 * Condition: Check if Power Pellet is Active
 */
USTRUCT(meta = (DisplayName = "Has Power Pellet"))
struct UE_CITRUSH_API FEnemyCondition_HasPowerPellet : public FStateTreeConditionBase
{
	GENERATED_BODY()

	bool TestCondition(FStateTreeExecutionContext& Context) const;
};
