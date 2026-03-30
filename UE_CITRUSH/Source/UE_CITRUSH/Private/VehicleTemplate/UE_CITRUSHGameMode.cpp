// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHGameMode.h"
#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHPlayerController.h"
#include "Player/CitRushPlayerState.h"

AUE_CITRUSHGameMode::AUE_CITRUSHGameMode()
{
	PlayerControllerClass = AUE_CITRUSHPlayerController::StaticClass();
	PlayerStateClass = ACitRushPlayerState::StaticClass();
}
