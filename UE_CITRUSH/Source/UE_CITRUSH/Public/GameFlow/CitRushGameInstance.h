// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Utility/DebugHelper.h"
#include "CitRushGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class UE_CITRUSH_API UCitRushGameInstance : public UGameInstance
{
	GENERATED_BODY()
	DECLARE_CLASS_LOG(CitRushGameInstance);

public:
	virtual void Init() override;

	virtual void ReturnToMainMenu() override;

protected:
	void ResetMoviePlayerSettings();
	void SetCustomMoviePlayerSettings();

public:
	void PlayLoadingMoviePlayer();
	void StopLoadingMoviePlayer();
	
	void OnPostWorldInit(UWorld* world, UWorld::InitializationValues IVS);
	void OnStartInGame(const UWorld* world);

private:
	virtual void OnNetworkFailure(UWorld* world, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

protected:
	
private:
	bool bNeedToPlayLoadingScreen = true;
};
