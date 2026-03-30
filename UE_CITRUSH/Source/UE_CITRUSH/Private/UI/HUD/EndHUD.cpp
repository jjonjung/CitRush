// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/EndHUD.h"

#include "Data/WidgetBlueprintDataAsset.h"
#include "Player/CitRushPlayerState.h"

#include "Utility/WidgetBlueprintLoader.h"

#include "UI/End/EndTransitionWidget.h"
#include "UI/End/EndWidget.h"


AEndHUD::AEndHUD()
{
	const UWidgetBlueprintLoader* loader = UWidgetBlueprintLoader::Get();
	UWidgetBlueprintDataAsset* PDA_WBP =  loader->PDA_WBP.LoadSynchronous();

	transitionWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey(TEXT("EndTransition"));
	endWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey(TEXT("End"));
}

void AEndHUD::BeginPlay()
{
	Super::BeginPlay();

	// TODO : EndMatchEvent 를 HTTP Response에 Binding
	
	endWidget = CreateWidget<UEndWidget>(GetWorld(), endWidgetClass);
	endWidget->AddToViewport(0);
	
	transitionWidget = CreateWidget<UEndTransitionWidget>(GetWorld(), transitionWidgetClass);
	transitionWidget->AddToViewport(1);

	FTimerHandle transitionTimer;
	GetWorldTimerManager().SetTimer(transitionTimer, [&]()->void
	{
		this->EndMatchEvent();
	}, 5.f, false);
}

void AEndHUD::EndMatchEvent()
{
	transitionWidget->RemoveFromParent();
	// Racer의 결과는 무엇을, 어떻게 전달할 것인가.
}
