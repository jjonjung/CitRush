// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DetailReportWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class UE_CITRUSH_API UDetailReportWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> elapsedTimeText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> totalDamagedText;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> GivingItemCountText;
};
