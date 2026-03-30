// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/TransitionHUD.h"

#include "Data/WidgetBlueprintDataAsset.h"
#include "UI/LoadingScreen/LoadingWidget.h"
#include "Utility/WidgetBlueprintLoader.h"

ATransitionHUD::ATransitionHUD()
{
	const UWidgetBlueprintLoader* loader = UWidgetBlueprintLoader::Get();
	PDA_WBP =  loader->PDA_WBP.LoadSynchronous();

	loadingWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey(TEXT("Loading"));
}

void ATransitionHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!loadingWidgetClass)
	{
		loadingWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey(TEXT("Loading"));
	}
	
	loadingWidget = CreateWidget<ULoadingWidget>(GetWorld(), loadingWidgetClass);
	loadingWidget->AddToViewport(5);
}

void ATransitionHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}
