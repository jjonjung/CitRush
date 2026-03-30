// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/PlayerInfoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxKey.h"
#include "Components/Image.h"

#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"
#include "Utility/DebugHelper.h"

void UPlayerInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	const UEnum* enumPtr = StaticEnum<EPlayerRole>();
	if (!enumPtr) {return;}
	
	for (int32 i = 0; i < enumPtr->NumEnums(); ++i)
	{
		FName enumKey = enumPtr->GetNameByIndex(i);
		playerRoleComboBox->AddOption(enumKey);
	}

	playerRoleComboBox->OnGenerateContentWidget.BindDynamic(this, &UPlayerInfoWidget::OnGenerateSelector);
	playerRoleComboBox->OnGenerateItemWidget.BindDynamic(this, &UPlayerInfoWidget::OnGenerateSelector);
	playerRoleComboBox->OnSelectionChanged.AddDynamic(this, &UPlayerInfoWidget::SelectRole);
	SetRole(EPlayerRole::None);
	SetVisibility(ESlateVisibility::Hidden);
	SetIsFocusable(false);
	
	CITRUSH_TIME("Lobby");
}

void UPlayerInfoWidget::InitializePlayerInfo(ACitRushPlayerState* inPlayerState, const int32& index)
{
	ownerPS = inPlayerState;
	CITRUSH_TIME("Lobby");
	if (inPlayerState == nullptr || !IsValid(inPlayerState) || index == INDEX_NONE)
	{
		playerID = -1;
		playerNameText->SetText(FText::GetEmpty());
		SetRole(EPlayerRole::None);
		SetReady(false);
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	
	FPlayerInfo info = ownerPS->GetPlayerInfo();
	playerID = ownerPS->GetUniqueId()->GetTypeHash();
	playerNameText->SetText(FText::FromString(info.playerName));
	ownerPS->OnPlayerReadyChangedDelegate.AddUObject(this, &UPlayerInfoWidget::SetReady);
	ownerPS->OnPlayerRoleChangedDelegate.AddUObject(this, &UPlayerInfoWidget::SetRole);
	if (index == 0)
	{
		ownerPS->ServerRPC_RoleChange(EPlayerRole::Commander);
		SetRole(EPlayerRole::Commander);
	}
	else
	{
		ownerPS->ServerRPC_RoleChange(EPlayerRole::Racer);
		SetRole(EPlayerRole::Racer);
	}
	
	
	SetReady(info.bIsReady);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPlayerInfoWidget::SetReady(bool bReady)
{
	playerReadyStateImage->SetBrushFromTexture(bReady ? readyImage : notReadyImage);
}

void UPlayerInfoWidget::SelectRole(FName SelectedItem, ESelectInfo::Type SelectionType)
{	
	if (SelectionType != ESelectInfo::OnNavigation || SelectionType != ESelectInfo::Direct)
	{
		if (!IsValid(ownerPS)) {return;}
			
		const UEnum* enumPtr = StaticEnum<EPlayerRole>();
		if (!enumPtr) {return;}
		
		EPlayerRole role = static_cast<EPlayerRole>(enumPtr->GetValueByName(SelectedItem));
		ownerPS->ServerRPC_RoleChange(role);
	}
}

void UPlayerInfoWidget::SetRole(const EPlayerRole& directlyRole)
{
	playerRoleComboBox->SetSelectedOption(UEnum::GetValueAsName(directlyRole));
}

UWidget* UPlayerInfoWidget::OnGenerateSelector(FName slotName)
{
	const UEnum* enumPtr = StaticEnum<EPlayerRole>();
	if (!enumPtr) {return nullptr;}
	FText enumText = enumPtr->GetDisplayNameTextByValue(enumPtr->GetValueByName(slotName));
		
	UTextBlock* itemText = WidgetTree->ConstructWidget<UTextBlock>();
	itemText->SetText(enumText);
	return itemText;
}

bool UPlayerInfoWidget::ComparePlayer(const uint32& comparedID)
{
	return playerID == comparedID;
}

void UPlayerInfoWidget::SetAuthority(bool bAuth)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetIsFocusable(false);
	/*if (bAuth) // 추후 Dedicated server로 리팩토링 할 시
	{
		SetVisibility(ESlateVisibility::Visible);
		SetIsFocusable(true);
		return;
	}*/
	
}
