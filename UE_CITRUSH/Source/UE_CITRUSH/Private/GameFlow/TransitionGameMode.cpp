// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/TransitionGameMode.h"

#include "GameFlow/CitRushGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Player/Controller/CitRushPlayerController.h"
#include "UI/HUD/TransitionHUD.h"

ATransitionGameMode::ATransitionGameMode()
{
	bUseSeamlessTravel = true;
	PlayerControllerClass = ACitRushPlayerController::StaticClass();
	PlayerStateClass = ACitRushPlayerState::StaticClass();
}
