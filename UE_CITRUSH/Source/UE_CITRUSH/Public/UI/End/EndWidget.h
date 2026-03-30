// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndWidget.generated.h"

class UTextBlock;
class UButton;
class UWidgetSwitcher;
class UTeamListWidget;
class UDetailReportWidget;

/**
 * 
 */
UCLASS()
class UE_CITRUSH_API UEndWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnClickTeamList();
	UFUNCTION()
	void OnClickDetailReport();
	UFUNCTION()
	void OnClickExit();

public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTeamListWidget> teamListWidget;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UDetailReportWidget> detailReportWidget;
	
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> reportSwitcher;
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> teamListButton; 
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> detailReportButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> resultText;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> exitButton;
};
