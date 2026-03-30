// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenu/MatchMakingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Data/CustomizeSessionSettings.h"
#include "Data/MapDataAsset.h"
#include "GameFlow/MainMenuGameMode.h"
#include "Player/CitRushPlayerState.h"
#include "Utility/MapAssetLoader.h"

void UMatchMakingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const UMapAssetLoader* settings = UMapAssetLoader::Get();
	
	mapList = settings->PDA_Map.LoadSynchronous();
	
	matchBlurPanel->SetVisibility(ESlateVisibility::Collapsed);

	selectCommanderButton->OnClicked.AddDynamic(this, &UMatchMakingWidget::OnSelectCommander);
	selectRacerButton->OnClicked.AddDynamic(this, &UMatchMakingWidget::OnSelectRacer);
	cancelButton->OnClicked.AddDynamic(this, &UMatchMakingWidget::OnCancelMatchMaking);
}

void UMatchMakingWidget::OnMatchMaking()
{
	matchBlurPanel->SetVisibility(ESlateVisibility::Visible);
}

void UMatchMakingWidget::OnCancelMatchMaking()
{
	if (AMainMenuGameMode* mmGM = GetWorld()->GetAuthGameMode<AMainMenuGameMode>())
	{
		matchBlurPanel->SetVisibility(ESlateVisibility::Collapsed);
		if (!mmGM->CancelSearchSession())
		{
			// TODO : Cancel 실패 메시지
			return;
		}
	}
}

void UMatchMakingWidget::OnSelectCommander()
{
	ACitRushPlayerState* cPS = GetOwningPlayerState<ACitRushPlayerState>();
	if (!cPS) {return;}

	cPS->ServerRPC_RoleChange(EPlayerRole::Commander); 
	if (AMainMenuGameMode* mmGM = GetWorld()->GetAuthGameMode<AMainMenuGameMode>())
	{
		
		FMapInfo mapInfo;
		if (!mapList->GetMapInfoByKey(TEXT("Game_Rank"), mapInfo)) {return;}
	
		FCustomizeSessionSettings customizedSessionSettings;
		customizedSessionSettings.mapInfo = mapInfo;
		customizedSessionSettings.maxPlayerNum = 4;
		customizedSessionSettings.displayName = TEXT("Rank");  // 추후 시간 - ELO Rating 조합으로
		OnMatchMaking();
		if (!mmGM->CreateLobby(customizedSessionSettings))
		{
			// TODO : MatchMaking 실패 메시지
			return;
		}
	}
}

void UMatchMakingWidget::OnSelectRacer()
{
	ACitRushPlayerState* cPS = GetOwningPlayerState<ACitRushPlayerState>();
	if (!cPS) {return;}
	
	cPS->ServerRPC_RoleChange(EPlayerRole::Racer); 
	if (AMainMenuGameMode* mmGM = GetWorld()->GetAuthGameMode<AMainMenuGameMode>())
	{
		OnMatchMaking();
		if (!mmGM->SearchLobbies())
		{
			// TODO : MatchMaking 실패 메시지
		}
	}
}
