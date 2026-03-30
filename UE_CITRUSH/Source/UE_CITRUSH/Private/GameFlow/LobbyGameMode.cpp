// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/LobbyGameMode.h"

#include "GameFlow/LobbyGameState.h"
#include "Player/CitRushPlayerState.h"

#include "Player/Controller/CitRushPlayerController.h"
#include "Player/UIPawn.h"
#include "UI/HUD/LobbyHUD.h"

ALobbyGameMode::ALobbyGameMode()
{
	bUseSeamlessTravel = true;

	DefaultPawnClass = AUIPawn::StaticClass();
	PlayerControllerClass = ACitRushPlayerController::StaticClass();
	GameStateClass = ALobbyGameState::StaticClass();
	PlayerStateClass = ACitRushPlayerState::StaticClass();
	HUDClass = ALobbyHUD::StaticClass();
	
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
}

APlayerController* ALobbyGameMode::ProcessClientTravel(FString& URL, bool bSeamless, bool bAbsolute)
{
	APlayerController* pc = Super::ProcessClientTravel(URL, bSeamless, bAbsolute);
	//CITRUSH_TIME("CitRush_Loading");
	if (pc != nullptr)
	{
		if (ACitRushPlayerState* localCPS = pc->GetPlayerState<ACitRushPlayerState>())
		{
			localCPS->ServerRPC_SetLoadingState(ELoadingState::StartClientTravel);
			localCPS->UpdateLoadingProgress(0.00f);
			UE_LOG(LogTemp, Log, TEXT("[LobbyGameMode] Start Travel LoadingState for player %s"), *localCPS->GetPlayerName());
		}
	}
	return pc;
}

void ALobbyGameMode::ProcessServerTravel(const FString& URL, bool bAbsolute)
{
	Super::ProcessServerTravel(URL, bAbsolute);
	//CITRUSH_TIME("CitRush_Loading");
	ACitRushPlayerState* hostCPS = GetWorld()->GetFirstPlayerController()->GetPlayerState<ACitRushPlayerState>();
	hostCPS->ServerRPC_SetLoadingState(ELoadingState::StartServerTravel);
	hostCPS->UpdateLoadingProgress(0.05f);
	UE_LOG(LogTemp, Log, TEXT("[LobbyGameMode] Start Travel LoadingState for player %s"), *hostCPS->GetPlayerName());
}

void ALobbyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StartGameFunc.Reset();
	sessionCode = "";
	Super::EndPlay(EndPlayReason);
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	// Steam Subsys에서 감지
	//CITRUSH_TIME("CitRush_Loading");
}

bool ALobbyGameMode::StartGame()
{
	if (!HasAuthority()) {return false;}

	// UE::Core::Private::Function::TFunctionRefBase::operator()
	OUT FName mapName;
	bool bStarted = StartGameFunc(mapName, GameState->PlayerArray.Num());
	if (bStarted)
	{
		CITRUSH_TIME("CitRush_Loading");
		GetGameState<ALobbyGameState>()->StartGame();
	}
	return bStarted;
}
