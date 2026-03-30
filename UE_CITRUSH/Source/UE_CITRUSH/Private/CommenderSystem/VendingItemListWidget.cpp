// Fill out your copyright notice in the Description page of Project Settings.


#include "CommenderSystem/VendingItemListWidget.h"
#include "CommenderSystem/VendingItemPartWidget.h"
#include "Components/TileView.h"
#include "CommenderSystem/VendingSlotUIData.h"


void UVendingItemListWidget::InitializeSlots(const TArray<UVendingSlotUIData*>& SlotDatas)
{
	if (!ItemTileView) return;

	ItemTileView->ClearListItems();

	for (UVendingSlotUIData* Data : SlotDatas)
	{
		if (Data)
			ItemTileView->AddItem(Data);
	}
}

void UVendingItemListWidget::HandleTileItemClicked(UVendingSlotUIData* ItemData)
{
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingItemListWidget] HandleTileItemClicked: ItemData가 nullptr입니다"));
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[VendingItemListWidget] HandleTileItemClicked: SlotIndex=%d, OnSlotSelected.Broadcast 호출"), ItemData->SlotIndex);
	OnSlotSelected.Broadcast(ItemData->SlotIndex);
}

void UVendingItemListWidget::OnItemEntryClicked(UVendingSlotUIData* ItemData)
{
	UE_LOG(LogTemp, Log, TEXT("[VendingItemListWidget] OnItemEntryClicked 호출: ItemData=%p"), ItemData);
	
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingItemListWidget] ItemData가 nullptr입니다"));
		return;
	}
	
	if (ItemData->bIsEmpty)
	{
		UE_LOG(LogTemp, Log, TEXT("[VendingItemListWidget] 슬롯 %d가 비어있습니다"), ItemData->SlotIndex);
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[VendingItemListWidget] HandleTileItemClicked 호출: SlotIndex=%d"), ItemData->SlotIndex);
	HandleTileItemClicked(ItemData);
}

void UVendingItemListWidget::UpdateSlotStatus(int32 SlotIndex, bool bIsEmpty, int32 RemainingCount)
{
	if (!ItemTileView) return;

	// TileView의 모든 아이템을 순회하며 해당 슬롯 찾기
	TArray<UObject*> AllItems = ItemTileView->GetListItems();
	for (UObject* Item : AllItems)
	{
		if (UVendingSlotUIData* SlotData = Cast<UVendingSlotUIData>(Item))
		{
			if (SlotData->SlotIndex == SlotIndex)
			{
				SlotData->bIsEmpty = bIsEmpty;
				SlotData->RemainingCount = RemainingCount;
				ItemTileView->RequestRefresh();
				break;
			}
		}
	}
}

void UVendingItemListWidget::RefreshAllSlots()
{
	if (ItemTileView)
	{
		ItemTileView->RequestRefresh();
	}
}

void UVendingItemListWidget::NativeConstruct()
{
	Super::NativeConstruct();
}