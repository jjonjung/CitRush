// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenu/MainMenuWidget.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

#include "Data/MediaSourceDataAsset.h"
#include "UI/VideoPlayerWidget.h"

#include "UI/MainMenu/HomeWidget.h"
#include "UI/MainMenu/SearchingSessionsWidget.h"
#include "UI/MainMenu/CreatingSessionWidget.h"
#include "UI/MainMenu/SettingWidget.h"
#include "UI/MainMenu/RankingListWidget.h"
#include "UI/MainMenu/MatchMakingWidget.h"
#include "Data/WidgetDataAsset.h"

#include "Utility/DebugHelper.h"

void UMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	TMap<UWidget*, FText> widgets;
	//TMap<int32, FText> indices;
	
	if (TSubclassOf<UHomeWidget> factory = widgetDataAsset->GetWidgetClass<UHomeWidget>(UHomeWidget::StaticClass()))
	{
		if ( (homeWidget = CreateWidget<UHomeWidget>(this, factory)) )
		{
			screenSwitcher->AddChild(homeWidget);
		}
	}
	
	if (TSubclassOf<UMatchMakingWidget> factory = widgetDataAsset->GetWidgetClass<UMatchMakingWidget>(UMatchMakingWidget::StaticClass()))
	{
		ADD_WIDGET(UMatchMakingWidget, factory, matchMakingWidget, widgets, FText::FromString(TEXT("랭크 게임")));
		screenSwitcher->AddChild(matchMakingWidget);
	}
	if (TSubclassOf<UCreatingSessionWidget> factory = widgetDataAsset->GetWidgetClass<UCreatingSessionWidget>(UCreatingSessionWidget::StaticClass()))
	{
		ADD_WIDGET(UCreatingSessionWidget, factory, creatingSessionWidget, widgets, FText::FromString(TEXT("방 만들기")));
		screenSwitcher->AddChild(creatingSessionWidget);
	}
	if (TSubclassOf<USearchingSessionsWidget> factory = widgetDataAsset->GetWidgetClass<USearchingSessionsWidget>(USearchingSessionsWidget::StaticClass()))
	{
		ADD_WIDGET(USearchingSessionsWidget, factory, searchingSessionsWidget, widgets, FText::FromString(TEXT("방 찾기")));
		screenSwitcher->AddChild(searchingSessionsWidget);
	}
	if (TSubclassOf<URankingListWidget> factory = widgetDataAsset->GetWidgetClass<URankingListWidget>(URankingListWidget::StaticClass()))
	{
		ADD_WIDGET(URankingListWidget, factory, rankingListWidget, widgets, FText::FromString(TEXT("랭킹 보기")));
		screenSwitcher->AddChild(rankingListWidget);
	}
	if (TSubclassOf<USettingWidget> factory = widgetDataAsset->GetWidgetClass<USettingWidget>(USettingWidget::StaticClass()))
	{
		ADD_WIDGET(USettingWidget, factory, settingWidget, widgets, FText::FromString(TEXT("설정")));
		screenSwitcher->AddChild(settingWidget);
	}
	
	if (IsValid(homeWidget))
	{
		CITRUSH_LOG("Init MainMenu Buttons");
		homeWidget->InitializeButtons(widgets);
		//homeWidget->InitializeButtons(indices);
	}
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(videoAsset) && IsValid(backgroundWidget))
	{
		if (UMediaSource* source = videoAsset->GetMediaSource("MainMenu"))
		{
			backgroundWidget->PlayVideo(source);
			backgroundWidget->EnableLoop(true);
		}
	}
	else {CITRUSH_LOG("video file Fail")}

	if (IsValid(homeButton))
	{
		homeButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnClickHomeButton);
		homeButton->SetVisibility(ESlateVisibility::Hidden);
		homeButton->SetIsEnabled(false);
	}

	if (IsValid(homeWidget))
	{
		homeWidget->OnScreenButtonClicked.BindUObject(this, &UMainMenuWidget::OnClickChangeScreenButton);
		homeWidget->OnWidgetSelected.BindUObject(this, &UMainMenuWidget::OnClickChangeScreenButtonByWidget);
		OnClickHomeButton();
	}
}

void UMainMenuWidget::OnClickHomeButton()
{
	if (!IsValid(homeWidget)) {return;}
	screenSwitcher->SetActiveWidget(homeWidget);
	homeButton->SetVisibility(ESlateVisibility::Hidden);
	homeButton->SetIsEnabled(false);
	searchingSessionsWidget->ResetSessionsList();
	creatingSessionWidget->Reset();
}

void UMainMenuWidget::OnClickChangeScreenButton(const int32& inScreenIndex)
{
	const bool bActive = inScreenIndex > 0;
	screenSwitcher->SetActiveWidgetIndex(inScreenIndex);
	homeButton->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	homeButton->SetIsEnabled(bActive);
}

void UMainMenuWidget::OnClickChangeScreenButtonByWidget(UWidget* inWidgetObject)
{
	const bool bActive = inWidgetObject != homeWidget;
	screenSwitcher->SetActiveWidget(inWidgetObject);
	homeButton->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	homeButton->SetIsEnabled(bActive);
}

