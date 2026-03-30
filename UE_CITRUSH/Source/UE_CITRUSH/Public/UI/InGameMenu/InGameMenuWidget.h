// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InGameMenuWidget.generated.h"

class UImage;
class UButton;
/**
 * 
 */
UCLASS()
class UE_CITRUSH_API UInGameMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnClickedContinue();
	UFUNCTION()
	void OnClickedExit();

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UButton> continueButton;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget, ShortTooltip="Exit Game"))
	TObjectPtr<UButton> exitButton;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UImage> commanderGuideImage;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UImage> racerGuideImage;
};
