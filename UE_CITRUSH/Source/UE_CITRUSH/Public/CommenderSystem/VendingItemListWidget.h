// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VendingItemListWidget.generated.h"

class UTileView;
class UVendingSlotUIData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVendingSlotSelected, int32, SlotIndex);

// 자판기 아이템 리스트 위젯
UCLASS()
class UE_CITRUSH_API UVendingItemListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 아이템 클릭 시 호출되는 델리게이트
	UPROPERTY(BlueprintAssignable, Category="Vending")
	FOnVendingSlotSelected OnSlotSelected;

	// 슬롯 데이터 초기화
	UFUNCTION(BlueprintCallable, Category="Vending")
	void InitializeSlots(const TArray<UVendingSlotUIData*>& SlotDatas);

	// 아이템 클릭 처리
	UFUNCTION(BlueprintCallable, Category="Vending")
	void OnItemEntryClicked(UVendingSlotUIData* ItemData);

	// 슬롯 상태 업데이트 (비어있을 때 비활성화)
	UFUNCTION(BlueprintCallable, Category="Vending")
	void UpdateSlotStatus(int32 SlotIndex, bool bIsEmpty, int32 RemainingCount);

	// 모든 슬롯 새로고침
	UFUNCTION(BlueprintCallable, Category="Vending")
	void RefreshAllSlots();

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTileView> ItemTileView;

	UFUNCTION()
	void HandleTileItemClicked(UVendingSlotUIData* ItemData);

	virtual void NativeConstruct() override;
};
