// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/EndGameMode.h"

#include "GameFlow/EndGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Player/Controller/CitRushPlayerController.h"
#include "UI/HUD/EndHUD.h"

AEndGameMode::AEndGameMode()
{
	HUDClass = AEndHUD::StaticClass();
	GameStateClass = AEndGameState::StaticClass();
	PlayerControllerClass = ACitRushPlayerController::StaticClass();
	PlayerStateClass = ACitRushPlayerState::StaticClass();
	
}

void AEndGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
}
