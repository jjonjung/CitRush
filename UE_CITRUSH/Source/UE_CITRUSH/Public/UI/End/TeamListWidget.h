// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TeamListWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class UE_CITRUSH_API UTeamListWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	void SetPlayerNames(TArray<FString> playerNames);

protected:
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> playerNameValue_0;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> playerNameValue_1;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> playerNameValue_2;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> playerNameValue_3;
	
	
};
