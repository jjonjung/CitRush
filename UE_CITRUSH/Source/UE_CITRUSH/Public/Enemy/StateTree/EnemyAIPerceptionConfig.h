// EnemyAIPerceptionConfig.h
// AIPerception Configuration for PixelEnemy

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIPerceptionConfig.generated.h"

/**
 * AI Perception Configuration Data
 * Blueprint-editable settings for Sight and Hearing
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FEnemyPerceptionConfig
{
	GENERATED_BODY()

	// ========== Sight Config ==========

	/** Enable sight perception */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
	bool bEnableSight = true;

	/** Sight radius (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight", meta = (EditCondition = "bEnableSight"))
	float SightRadius = 3000.0f;

	/** Lose sight radius (cm) - should be larger than SightRadius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight", meta = (EditCondition = "bEnableSight"))
	float LoseSightRadius = 3500.0f;

	/** Peripheral vision angle (degrees, 0-180) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight", meta = (EditCondition = "bEnableSight", ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngle = 90.0f;

	/** Detection by affiliation (enemy, neutral, friendly) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight", meta = (EditCondition = "bEnableSight"))
	FAISenseAffiliationFilter SightAffiliationFilter;

	/** Auto success range from last seen location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight", meta = (EditCondition = "bEnableSight"))
	float AutoSuccessRangeFromLastSeen = 500.0f;

	/** Point of view backward offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight", meta = (EditCondition = "bEnableSight"))
	float PointOfViewBackwardOffset = 0.0f;

	/** Near clipping radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight", meta = (EditCondition = "bEnableSight"))
	float NearClippingRadius = 0.0f;

	// ========== Hearing Config ==========

	/** Enable hearing perception */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hearing")
	bool bEnableHearing = true;

	/** Hearing range (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hearing", meta = (EditCondition = "bEnableHearing"))
	float HearingRange = 5000.0f;

	/** Detection by affiliation for hearing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hearing", meta = (EditCondition = "bEnableHearing"))
	FAISenseAffiliationFilter HearingAffiliationFilter;

	// ========== Damage Config ==========

	/** Enable damage perception */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bEnableDamage = true;

	// ========== General Config ==========

	/** Max age for stimuli (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	float MaxStimuliAge = 5.0f;

	/** Dominant sense */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	TSubclassOf<class UAISense> DominantSense;

	FEnemyPerceptionConfig()
	{
		// Default affiliation: detect enemies and neutrals
		SightAffiliationFilter.bDetectEnemies = true;
		SightAffiliationFilter.bDetectNeutrals = true;
		SightAffiliationFilter.bDetectFriendlies = false;

		HearingAffiliationFilter.bDetectEnemies = true;
		HearingAffiliationFilter.bDetectNeutrals = true;
		HearingAffiliationFilter.bDetectFriendlies = false;
	}
};

/**
 * Perceived Target Info
 * Stores information about a perceived actor
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FPerceivedTargetInfo
{
	GENERATED_BODY()

	/** The perceived actor */
	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	TObjectPtr<AActor> Actor = nullptr;

	/** Last known location */
	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	FVector LastKnownLocation = FVector::ZeroVector;

	/** Is currently sensed */
	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	bool bIsCurrentlySensed = false;

	/** Was sensed via sight */
	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	bool bWasSensedBySight = false;

	/** Was sensed via hearing */
	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	bool bWasSensedByHearing = false;

	/** Was sensed via damage */
	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	bool bWasSensedByDamage = false;

	/** Time since last perceived */
	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	float TimeSinceLastSensed = 0.0f;

	/** Stimulus strength (0.0 ~ 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	float StimulusStrength = 0.0f;
};
