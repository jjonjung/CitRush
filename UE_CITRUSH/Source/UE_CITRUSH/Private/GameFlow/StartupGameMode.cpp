// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/StartupGameMode.h"

#include "Data/MapDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CitRushPlayerState.h"
#include "Player/Controller/CitRushPlayerController.h"
#include "Utility/MapAssetLoader.h"

AStartupGameMode::AStartupGameMode()
{
	PlayerControllerClass = ACitRushPlayerController::StaticClass();
	PlayerStateClass = ACitRushPlayerState::StaticClass();
	
	const UMapAssetLoader* mapAssetLoader = UMapAssetLoader::Get();
	mapAssets = mapAssetLoader->PDA_Map.LoadSynchronous();
	if (mapAssets)
	{
		FMapInfo info;
		mapAssets->GetMapInfoByKey(TEXT("MainMenu"), info);
		mainMenuLevelURL = info.GetMapAsset().GetAssetName();
	}
}

void AStartupGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (mapAssets && mainMenuLevelURL.IsEmpty())
	{
		FMapInfo info;
		mapAssets->GetMapInfoByKey(TEXT("MainMenu"), info);
		mainMenuLevelURL = info.GetMapAsset().GetAssetName();
	}
}

void AStartupGameMode::EnterToMainMenu()
{
	if (mainMenuLevelURL.IsEmpty()) {return;}
	UGameplayStatics::OpenLevel(GetWorld(), FName(mainMenuLevelURL), false);
}
