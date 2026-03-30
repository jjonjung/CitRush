// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InGameMenu/InGameMenuWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "GameFlow/CitRushGameState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Network/SteamSubsystem.h"
#include "Player/CitRushPlayerState.h"

void UInGameMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	continueButton->OnClicked.AddDynamic(this, &UInGameMenuWidget::OnClickedContinue);
	exitButton->OnClicked.AddDynamic(this, &UInGameMenuWidget::OnClickedExit);
	
	commanderGuideImage->SetVisibility(ESlateVisibility::Collapsed);
	racerGuideImage->SetVisibility(ESlateVisibility::Collapsed);
	if (ACitRushPlayerState* cPS = GetOwningPlayerState<ACitRushPlayerState>())
	{
		switch (cPS->GetPlayerInfo().playerRole)
		{
		case EPlayerRole::Commander:
			commanderGuideImage->SetVisibility(ESlateVisibility::Visible);
			break;
		case EPlayerRole::Racer:
			racerGuideImage->SetVisibility(ESlateVisibility::Visible);
			break;
		default:
			break;
		}
		
	}
}

void UInGameMenuWidget::OnClickedContinue()
{
	SetVisibility(ESlateVisibility::Collapsed);
	GetOwningPlayer()->SetShowMouseCursor(false);
	GetOwningPlayer()->SetInputMode(FInputModeGameOnly());
}

void UInGameMenuWidget::OnClickedExit()
{
	if (ACitRushGameState* lGS = GetWorld()->GetGameState<ACitRushGameState>())
	{
		if (ACitRushPlayerState* cPS = GetOwningPlayerState<ACitRushPlayerState>())
		{
			lGS->NetMulticastRPC_QuitMatch(cPS);
		}
	}
}
