#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "VendingItemPartWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UVendingSlotUIData;
class UVendingItemListWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVendingItemEntryClicked, UVendingSlotUIData*, ItemData);

// 자판기 아이템 개별 위젯 (TileView 항목)
UCLASS()
class UE_CITRUSH_API UVendingItemPartWidget 
	: public UUserWidget
	, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category="Vending")
	FOnVendingItemEntryClicked OnEntryClicked;

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ItemButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	// Optional: Coin 텍스트 (있으면 사용)
	UPROPERTY(meta=(BindWidget), meta=(OptionalWidget=true))
	TObjectPtr<UTextBlock> CoinText;

	// Optional: 남은 개수 텍스트 (있으면 사용)
	UPROPERTY(meta=(BindWidget), meta=(OptionalWidget=true))
	TObjectPtr<UTextBlock> RemainingCountText;

	UPROPERTY()
	TObjectPtr<UVendingSlotUIData> CurrentData;

	virtual void NativeConstruct() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UFUNCTION()
	void HandleButtonClicked();
};