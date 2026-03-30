// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/End/EndWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "GameFlow/LocalDataFlowSubsystem.h"
#include "Player/CitRushPlayerState.h"
#include "UI/End/TeamListWidget.h"
#include "UI/End/DetailReportWidget.h"

void UEndWidget::NativeConstruct()
{
	Super::NativeConstruct();

	teamListButton->OnClicked.AddDynamic(this, &UEndWidget::OnClickTeamList);
	detailReportButton->OnClicked.AddDynamic(this, &UEndWidget::OnClickDetailReport);
	exitButton->OnClicked.AddDynamic(this, &UEndWidget::OnClickExit);

	if (ULocalDataFlowSubsystem* data = GetGameInstance()->GetSubsystem<ULocalDataFlowSubsystem>())
	{
		for (const TPair<FName, FString>& pair : data->collectedStringData)
		{
			if (pair.Key == "Time")
			{
				detailReportWidget->elapsedTimeText->SetText(FText::FromString(pair.Value));
			}
		}
		for (const TPair<FName, float>& pair : data->collectedFloatData)
		{
			if (pair.Key == "Damage")
			{
				detailReportWidget->totalDamagedText->SetText(FText::FromString( FString::Printf(TEXT("%.2f"), pair.Value)));
			}
			else if (pair.Key == "GivingItem")
			{
				detailReportWidget->GivingItemCountText->SetText(FText::FromString(FString::FromInt(pair.Value)));
			}
			else if (pair.Key == "Result")
			{
				pair.Value > 0 ? resultText->SetText(FText::FromString(TEXT("승리")))
					: resultText->SetText(FText::FromString(TEXT("패배")));
			}
		}
		
		teamListWidget->SetPlayerNames(data->participantNames);
	}
}

void UEndWidget::OnClickTeamList()
{
	reportSwitcher->SetActiveWidget(teamListWidget);
}

void UEndWidget::OnClickDetailReport()
{
	reportSwitcher->SetActiveWidget(detailReportWidget);
}

void UEndWidget::OnClickExit()
{
	ACitRushPlayerState* cPS = GetOwningPlayerState<ACitRushPlayerState>();
	if (!IsValid(cPS)) {return;}
	cPS->ClientRPC_ExitToMainMenu_Implementation();
}
