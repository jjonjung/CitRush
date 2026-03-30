// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EndGameMode.generated.h"


/**
 *
 */
UCLASS()
class UE_CITRUSH_API AEndGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEndGameMode();
	
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
};
