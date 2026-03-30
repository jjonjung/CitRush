// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TransitionHUD.generated.h"

class UWidgetBlueprintDataAsset;
class ULoadingWidget;
/**
 * 
 */
UCLASS()
class UE_CITRUSH_API ATransitionHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATransitionHUD();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<ULoadingWidget> loadingWidget;
	UPROPERTY()
	TSubclassOf<ULoadingWidget> loadingWidgetClass;

	UPROPERTY()
	TObjectPtr<UWidgetBlueprintDataAsset> PDA_WBP;
};
