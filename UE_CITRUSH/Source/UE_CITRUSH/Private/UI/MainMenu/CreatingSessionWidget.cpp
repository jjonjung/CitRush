// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenu/CreatingSessionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxKey.h"

#include "Data/CustomizeSessionSettings.h"
#include "Data/MapDataAsset.h"

#include "GameFlow/MainMenuGameMode.h"
#include "Player/CitRushPlayerState.h"

#include "Utility/DebugHelper.h"
#include "Utility/MapAssetLoader.h"

void UCreatingSessionWidget::Reset()
{
	if (IsValid(hostNameDisplay))
	{
		if (ACitRushPlayerState* cPS = GetOwningPlayer()->GetPlayerState<ACitRushPlayerState>())
		{
			hostNameDisplay->SetText(FText::FromString(cPS->GetPlayerName()));
		}
	}
	if (IsValid(displayNameText))
	{
		displayNameText->SetText(FText::GetEmpty());
	}
	if (IsValid(mapSelector))
	{
		mapSelector->SetSelectedOption(defaultKey);
	}
	if (IsValid(maxPlayerCounterSlider))
	{
		CITRUSH_LOG("In Init Widgets");
		maxPlayerCounterValue = 0;
		maxPlayerCounterSlider->SetValue(maxPlayerCounterValue);
		maxPlayerCounterSlider->SetStepSize(1.f);
		maxPlayerCounterText->SetText(FText::AsNumber(maxPlayerCounterValue));
		maxPlayerCounterSlider->SetIsEnabled(false);
	}
	if (IsValid(createButton))
	{
		createButton->SetIsEnabled(false);
	}
}

void UCreatingSessionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	const UMapAssetLoader* settings = UMapAssetLoader::Get();
	if (!settings) {return;}
	
	mapList = settings->PDA_Map.LoadSynchronous();

	if (IsValid(displayNameText))
	{
		displayNameText->OnTextChanged.AddDynamic(this, &UCreatingSessionWidget::OnSessionNameInputChanged);
	}
	if (IsValid(createButton))
	{
		createButton->OnClicked.AddDynamic(this, &UCreatingSessionWidget::OnCreateButtonClick);
	}
	if (IsValid(maxPlayerCounterSlider))
	{
		maxPlayerCounterSlider->OnValueChanged.AddDynamic(this, &UCreatingSessionWidget::OnPlayerCounterValueChanged);
	}
	InitializeMapSelector();
}

void UCreatingSessionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(mapSelector) && mapSelector->GetSelectedOption() != defaultKey)
	{
		InitializeMapSelector();
	}
	Reset();
}

void UCreatingSessionWidget::OnCreateButtonClick()
{
	if (!IsValid(mapList) || !IsValid(mapSelector)) {return;}

	FName key = mapSelector->GetSelectedOption();
	FMapInfo mapInfo;
	if (!mapList->GetMapInfoByKey(key, mapInfo)) {return;}
	
	FCustomizeSessionSettings customizedSessionSettings;
	customizedSessionSettings.mapInfo = mapInfo;
	customizedSessionSettings.maxPlayerNum = maxPlayerCounterValue;
	customizedSessionSettings.displayName = displayNameText->GetText().ToString();
	if (AMainMenuGameMode* gm = GetWorld()->GetAuthGameMode<AMainMenuGameMode>())
	{
		if (!gm->CreateLobby(customizedSessionSettings))
		{
			// TODO : Session 생성 실패 UI
		}
	}
}

void UCreatingSessionWidget::OnPlayerCounterValueChanged(float inValue)
{
	maxPlayerCounterValue = FMath::RoundToInt(inValue);
	maxPlayerCounterText->SetText(FText::FromString(FString::FromInt(maxPlayerCounterValue)));
	maxPlayerCounterSlider->SetValue(static_cast<float>(maxPlayerCounterValue));
}

void UCreatingSessionWidget::OnSessionNameInputChanged(const FText& inputText)
{
	createButton->SetIsEnabled(!inputText.IsEmpty());
}

void UCreatingSessionWidget::OnSelectionChanged(FName SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!IsValid(mapList)) {return;}
	mapSelector->SetSelectedOption(SelectedItem);
	
	FMapInfo SelectedMapInfo;
	if (mapList->GetMapInfoByKey(SelectedItem, SelectedMapInfo))
	{
		maxPlayerCounterValue = static_cast<float>(SelectedMapInfo.GetMaxPlayers());
		maxPlayerCounterSlider->SetMinValue(1.f);
		maxPlayerCounterSlider->SetIsEnabled(true);
	}
	else
	{
		maxPlayerCounterValue = 0;
		maxPlayerCounterSlider->SetMinValue(0.f);
		maxPlayerCounterSlider->SetIsEnabled(false);
	}
	
	maxPlayerCounterSlider->SetMaxValue(maxPlayerCounterValue);
	maxPlayerCounterSlider->SetValue(maxPlayerCounterValue);
	maxPlayerCounterText->SetText(FText::FromString(FString::FromInt(maxPlayerCounterValue)));
}

void UCreatingSessionWidget::InitializeMapSelector()
{
	if (!IsValid(mapList) || !IsValid(mapSelector)) {return;}

	mapSelector->ClearOptions();
	mapSelector->OnSelectionChanged.Clear();
	mapSelector->OnGenerateContentWidget.Clear();
	mapSelector->OnGenerateItemWidget.Clear();
	
	mapSelector->OnSelectionChanged.AddDynamic(this, &UCreatingSessionWidget::OnSelectionChanged);
	mapSelector->OnGenerateContentWidget.BindDynamic(this, &UCreatingSessionWidget::OnGenerateSelectorContent);
	mapSelector->OnGenerateItemWidget.BindDynamic(this, &UCreatingSessionWidget::OnGenerateSelectorItem);
	
	defaultKey = NAME_None;
	mapSelector->SetSelectedOption(defaultKey);
	
	TArray<FName> keys;
	mapList->GetMapDictionary().GetKeys(keys);
	if (keys.IsEmpty()) {return;}

	for (const FName& mapKey : keys)
	{
		UE_LOG(LogTemp, Display, TEXT("map name : %s"), *mapKey.ToString())
		if (mapKey.ToString().Contains("Game"))
		{
			mapSelector->AddOption(mapKey);
		}
	}
}

UWidget* UCreatingSessionWidget::OnGenerateSelectorContent(FName Item)
{
	UTextBlock* itemText = WidgetTree->ConstructWidget<UTextBlock>();
	
	if (mapList)
	{
		if (const FMapInfo* foundMap = mapList->GetMapDictionary().Find(Item))
		{
			itemText->SetText(foundMap->GetDisplayName());
		}
		else if (Item == NAME_None)
		{
			itemText->SetText(FText::FromString(TEXT("NONE")));
		}
		else
		{
			itemText->SetText(FText::FromString(TEXT("Unknown Map")));
		}
	}

	return itemText;
}

UWidget* UCreatingSessionWidget::OnGenerateSelectorItem(FName Item)
{
	UTextBlock* itemText = WidgetTree->ConstructWidget<UTextBlock>();
	
	if (mapList)
	{
		if (const FMapInfo* foundMap = mapList->GetMapDictionary().Find(Item))
		{
			FText text = FText::Format(
				FText::FromString(TEXT("PVE : {0}")),
				foundMap->GetDisplayName()
			);
			itemText->SetText(text);
		}
		else if (Item == NAME_None)
		{
			itemText->SetText(FText::FromString(TEXT("NONE")));
		}
		else
		{
			itemText->SetText(FText::FromString(TEXT("Unknown Map")));
		}
	}

	return itemText;
}
