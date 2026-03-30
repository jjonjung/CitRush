// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/End/TeamListWidget.h"
#include "Components/TextBlock.h"

void UTeamListWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UTeamListWidget::SetPlayerNames(TArray<FString> playerNames)
{
	playerNameValue_0->SetText(FText::FromString(playerNames[0]));
	playerNameValue_1->SetText(FText::FromString(playerNames[1]));
	playerNameValue_2->SetText(FText::FromString(playerNames[2]));
	playerNameValue_3->SetText(FText::FromString(playerNames[3]));
}
