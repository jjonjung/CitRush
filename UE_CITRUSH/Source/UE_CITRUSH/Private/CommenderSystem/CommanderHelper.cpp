// Fill out your copyright notice in the Description page of Project Settings.

#include "CommenderSystem/CommanderHelper.h"
#include "Player/CommenderCharacter.h"
#include "UI/CommenderHUDWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFlow/CitRushGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Kismet/GameplayStatics.h"

ACommenderCharacter* UCommanderHelper::FindCommander(UWorld* World, bool bRequireHUDWidget)
{
	if (!World)
	{
		return nullptr;
	}

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (ACommenderCharacter* Commander = Cast<ACommenderCharacter>(PC->GetPawn()))
		{
			if (!bRequireHUDWidget || IsValid(Commander->CommenderHUDWidget.Get()))
			{
				return Commander;
			}
		}
	}

	if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
	{
		TArray<ACitRushPlayerState*> Commanders = GameState->GetPlayerStatesByRole(EPlayerRole::Commander);
		for (ACitRushPlayerState* PS : Commanders)
		{
			if (ACommenderCharacter* Commander = PS->GetPawn<ACommenderCharacter>())
			{
				if (!bRequireHUDWidget || IsValid(Commander->CommenderHUDWidget.Get()))
				{
					return Commander;
				}
			}
		}
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACommenderCharacter::StaticClass(), FoundActors);
	
	for (AActor* Actor : FoundActors)
	{
		if (ACommenderCharacter* Commander = Cast<ACommenderCharacter>(Actor))
		{
			if (!bRequireHUDWidget || IsValid(Commander->CommenderHUDWidget.Get()))
			{
				return Commander;
			}
		}
	}

	return nullptr;
}

