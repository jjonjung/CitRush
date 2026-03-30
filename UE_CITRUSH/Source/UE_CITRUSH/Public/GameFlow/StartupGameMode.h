// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StartupGameMode.generated.h"

class UMapDataAsset;
/**
 * 
 */
UCLASS()
class UE_CITRUSH_API AStartupGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStartupGameMode();
	
protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable)
	void EnterToMainMenu();

private:
	UPROPERTY()
	const UMapDataAsset* mapAssets;
	UPROPERTY()
	FString mainMenuLevelURL;
};
