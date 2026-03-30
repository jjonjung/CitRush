// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Data/WidgetBlueprintDataAsset.h"
#include "WidgetBlueprintLoader.generated.h"

/**
 * WidgetBlueprintDataAsset 로더 DeveloperSettings. PDA_WBP를 Config로 관리
 */
UCLASS(Config = Game, meta=(DisplayName = "WBP_Loader"))
class UE_CITRUSH_API UWidgetBlueprintLoader : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** WidgetBlueprintDataAsset Primary Data Asset (Config) */
	UPROPERTY(Config, EditAnywhere, Category = "WidgetBlueprintLoader", meta=(AllowedClasses="/Script/UE_CITRUSH.WidgetBlueprintAssetData"))
	TSoftObjectPtr<UWidgetBlueprintDataAsset> PDA_WBP;

	/** Singleton Getter */
	static const UWidgetBlueprintLoader* Get()
	{
		return GetDefault<UWidgetBlueprintLoader>();
	}
};
