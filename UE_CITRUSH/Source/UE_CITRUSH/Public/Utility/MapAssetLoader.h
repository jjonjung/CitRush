// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MapAssetLoader.generated.h"

class UMapDataAsset;

/**
 * MapDataAsset 로더 DeveloperSettings. PDA_Map을 Config로 관리
 */
UCLASS(Config=Game, meta=(DisplayName="MapAssetLoader"))
class UE_CITRUSH_API UMapAssetLoader : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** MapDataAsset Primary Data Asset (Config) */
	UPROPERTY(Config, EditAnywhere, Category = "Maps", meta=(AllowedClasses="/Script/UE_CITRUSH.MapAssetData"))
	TSoftObjectPtr<UMapDataAsset> PDA_Map;

	/** Singleton Getter */
	static const UMapAssetLoader* Get()
	{
		return GetDefault<UMapAssetLoader>();
	}
};
