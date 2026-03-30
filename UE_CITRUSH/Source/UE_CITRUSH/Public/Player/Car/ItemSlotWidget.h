// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Item/ItemData.h"
#include "Engine/DataTable.h"
#include "ItemSlotWidget.generated.h"

/**
 * 레이서 아이템 슬롯 위젯
 * 전방/후방 아이템 슬롯 2개를 표시
 */
UCLASS()
class UE_CITRUSH_API UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** 아이템 슬롯 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "Item Slot")
	void UpdateItemSlots(UItemData* FrontItem, UItemData* BackItem);

	/** 전방 아이템 슬롯 배경 이미지 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* FrontItemSlotImage;

	/** 전방 아이템 아이콘 이미지 (FrontItemSlotImage 하위) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* FrontItemIconImage;

	/** 후방 아이템 슬롯 배경 이미지 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* BackItemSlotImage;

	/** 후방 아이템 아이콘 이미지 (BackItemSlotImage 하위) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* BackItemIconImage;

protected:
	/** 이전 아이템 상태 저장 (애니메이션 트리거용) */
	UPROPERTY()
	UItemData* PreviousFrontItem = nullptr;
	
	UPROPERTY()
	UItemData* PreviousBackItem = nullptr;

	/** 애니메이션 타이머 핸들 */
	FTimerHandle FrontIconAnimationTimerHandle;
	FTimerHandle BackIconAnimationTimerHandle;

	/** 애니메이션 진행 상태 */
	float FrontIconAnimationProgress = 0.0f;
	float BackIconAnimationProgress = 0.0f;
	
	/** 애니메이션 재생 중인지 여부 */
	bool bIsFrontIconAnimating = false;
	bool bIsBackIconAnimating = false;

	/** 아이콘 Scale 애니메이션 업데이트 */
	UFUNCTION()
	void UpdateFrontIconAnimation();
	
	UFUNCTION()
	void UpdateBackIconAnimation();
	
	/** 아이콘 Scale 애니메이션 시작 */
	void StartIconAnimation(UImage* IconImage, bool bIsFront);
	
	/** 아이템 아이콘 가져오기 (racerIcon 우선 사용) */
	UTexture2D* GetItemIcon(UItemData* Item) const;
};

