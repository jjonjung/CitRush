// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/LobbyHUD.h"

#include "UI/Lobby/LobbyWidget.h"
#include "UI/LoadingScreen/LoadingWidget.h"
#include "Utility/WidgetBlueprintLoader.h"
#include "Data/WidgetBlueprintDataAsset.h"
#include "GameFlow/LobbyGameState.h"
#include "Player/CitRushPlayerState.h"

ALobbyHUD::ALobbyHUD()
{
	const UWidgetBlueprintLoader* loader = UWidgetBlueprintLoader::Get();
	TSoftObjectPtr<UWidgetBlueprintDataAsset> PDA_WBP =  loader->PDA_WBP.LoadSynchronous();

	loadingWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey("Loading");
	lobbyWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey("Lobby");
}

void ALobbyHUD::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwningPlayerController()) {return;}
	lobbyWidget = CreateWidget<ULobbyWidget>(GetOwningPlayerController(), lobbyWidgetClass);
	lobbyWidget->SetOwningPlayer(GetOwningPlayerController());
		
	if (ALobbyGameState* lGS = GetWorld()->GetGameState<ALobbyGameState>())
	{
		lGS->OnAddedPlayer.AddUObject(this, &ALobbyHUD::OnAddedPlayer);
		lGS->OnRemovedPlayer.AddUObject(this, &ALobbyHUD::OnRemovedPlayer);
		lGS->OnGameStarted.AddUObject(this, &ALobbyHUD::OnTransition);
		lGS->OnUpdatedPlayerArray.AddUObject(this, &ALobbyHUD::OnUpdatePlayerList);
	}
	
	lobbyWidget->AddToViewport(0);
}

void ALobbyHUD::OnAddedPlayer(ACitRushPlayerState* cPS)
{
	if (!IsValid(cPS)) {return;}
	lobbyWidget->AddPlayer(cPS);
}

void ALobbyHUD::OnRemovedPlayer(ACitRushPlayerState* cPS)
{
	if (!IsValid(cPS)) {return;}
	lobbyWidget->RemovePlayer(cPS);
}

void ALobbyHUD::OnUpdatePlayerList(const TArray<ACitRushPlayerState*>& players)
{
	lobbyWidget->UpdatePlayerList(players);
}

void ALobbyHUD::OnTransition()
{
	lobbyWidget->RemoveFromParent();
	if (ALobbyGameState* lGS = GetWorld()->GetGameState<ALobbyGameState>())
	{
		lGS->OnAddedPlayer.Clear();
		lGS->OnRemovedPlayer.Clear();
		lGS->OnGameStarted.Clear();
		lGS->OnUpdatedPlayerArray.Clear();
	}
}
