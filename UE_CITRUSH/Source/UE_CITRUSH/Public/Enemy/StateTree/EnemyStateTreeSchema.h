// EnemyStateTreeSchema.h
// StateTree Context for PixelEnemy AI (Multiplayer-Safe)

#pragma once

#include "CoreMinimal.h"
#include "StateTreeSchema.h"
#include "EnemyStateTreeSchema.generated.h"

class APixelEnemy;
class UAIDirectiveComponent;

/**
 * StateTree Context Data for PixelEnemy
 * Server-Only: StateTree runs only on server (HasAuthority)
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FEnemyStateTreeContextData
{
	GENERATED_BODY()

	/** Owner PixelEnemy Reference */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APixelEnemy> OwnerEnemy = nullptr;

	/** AIDirectiveComponent Reference */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<UAIDirectiveComponent> DirectiveComponent = nullptr;

	/** Is Server Connected (AI Server) */
	UPROPERTY(EditAnywhere, Category = "Context")
	bool bIsServerConnected = false;

	/** Nearest Racer Distance */
	UPROPERTY(EditAnywhere, Category = "Context")
	float NearestRacerDistance = FLT_MAX;

	/** Nearest Pellet Distance */
	UPROPERTY(EditAnywhere, Category = "Context")
	float NearestPelletDistance = FLT_MAX;

	/** Nearest Coin Distance */
	UPROPERTY(EditAnywhere, Category = "Context")
	float NearestCoinDistance = FLT_MAX;

	/** Current Health Ratio (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, Category = "Context")
	float HealthRatio = 1.0f;

	/** Is Power Pellet Active */
	UPROPERTY(EditAnywhere, Category = "Context")
	bool bHasPowerPellet = false;
};

/**
 * StateTree Schema for PixelEnemy
 * Defines what context data is available in the StateTree
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta = (DisplayName = "Enemy AI Schema"))
class UE_CITRUSH_API UEnemyStateTreeSchema : public UStateTreeSchema
{
	GENERATED_BODY()

public:
	UEnemyStateTreeSchema();

protected:
	virtual bool IsStructAllowed(const UScriptStruct* InScriptStruct) const override;
	virtual bool IsClassAllowed(const UClass* InClass) const override;
	virtual bool IsExternalItemAllowed(const UStruct& InStruct) const override;
};
