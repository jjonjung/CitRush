// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/MainMenuGameState.h"

#include "Utility/WidgetBlueprintLoader.h"
#include "Data/WidgetBlueprintDataAsset.h"
#include "GameFlow/LocalDataFlowSubsystem.h"
#include "Utility/DebugHelper.h"

#include "UI/MainMenu/MainMenuWidget.h"

AMainMenuGameState::AMainMenuGameState()
{
	const UWidgetBlueprintLoader* loader = UWidgetBlueprintLoader::Get();
	TSoftObjectPtr<UWidgetBlueprintDataAsset> PDA_WBP =  loader->PDA_WBP.LoadSynchronous();
	if (!PDA_WBP.IsValid())
	{
		CITRUSH_LOG("WBP Primary Data Asset is not valid");
		return;
	}
	menuWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey("MainMenu");
}

void AMainMenuGameState::BeginPlay()
{
	Super::BeginPlay();
	CITRUSH_LOG("start main menu widget set");
	if (!IsValid(menuWidgetClass)) {CITRUSH_LOG("InValid main menu widget Class"); return;}
	
	APlayerController* pc = GetWorld()->GetFirstPlayerController();
	if (!IsValid(pc)) {return;}
	
	UMainMenuWidget* menuWidget = CreateWidget<UMainMenuWidget>(pc, menuWidgetClass);
	menuWidget->AddToViewport(0);

	GetGameInstance()->GetSubsystem<ULocalDataFlowSubsystem>()->ResetParticipantData();
}
