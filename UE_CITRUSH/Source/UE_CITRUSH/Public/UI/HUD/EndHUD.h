// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EndHUD.generated.h"

class UEndWidget;
class UEndTransitionWidget;

/**
 * 
 */
UCLASS()
class UE_CITRUSH_API AEndHUD : public AHUD
{
	GENERATED_BODY()

public:
	AEndHUD();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void EndMatchEvent();

private:
	UPROPERTY()
	TSubclassOf<UEndTransitionWidget> transitionWidgetClass;
	UPROPERTY()
	TObjectPtr<UEndTransitionWidget> transitionWidget;
	
	UPROPERTY()
	TSubclassOf<UEndWidget> endWidgetClass;
	UPROPERTY()
	TObjectPtr<UEndWidget> endWidget;
};