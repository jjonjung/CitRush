#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VendingSlotUIData.generated.h"

class UTexture2D;

// 슬롯 UI 데이터
UCLASS(BlueprintType)
class UE_CITRUSH_API UVendingSlotUIData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Vending")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Vending")
	FText ItemName;

	UPROPERTY(BlueprintReadOnly, Category="Vending")
	int32 Coin = 0;

	UPROPERTY(BlueprintReadOnly, Category="Vending")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 슬롯이 비어있는지 여부
	UPROPERTY(BlueprintReadWrite, Category="Vending")
	bool bIsEmpty = false;

	// 남은 아이템 개수
	UPROPERTY(BlueprintReadWrite, Category="Vending")
	int32 RemainingCount = 0;
};