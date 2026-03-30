// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DataTypeObjectManager.generated.h"

/**
 * 
 */
UCLASS(Config=Game, meta=(DisplayName="DataTypeObjectManager"))
class UE_CITRUSH_API UDataTypeObjectManager : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere)
	TMap<FName, TSoftObjectPtr<UDataTable>> DataTableDict;

	UPROPERTY(Config, EditAnywhere)
	TMap<FName, TSoftObjectPtr<UCurveFloat>> CurveFloatDict;

	UFUNCTION(BlueprintCallable)
	UDataTable* GetTableAsset(FName key) const;
	UFUNCTION(BlueprintCallable)
	UCurveFloat* GetCurveFloatAsset(FName key) const;

	static const UDataTypeObjectManager* Get()
	{
		return GetDefault<UDataTypeObjectManager>();
	}
};
