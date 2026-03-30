// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "EnemyBrainCam.generated.h"

class UEnemyAISubsystem;

/**
 * Enemy Brain Cam UI 위젯
 * AI 서버로부터 받은 brain_cam_data를 표시
 */
UCLASS()
class UE_CITRUSH_API UEnemyBrainCam : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	/** Summary 텍스트 블록 (Blueprint에서 바인딩) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SummaryTextBlock;

	/** Reasoning 텍스트 블록 (Blueprint에서 바인딩) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ReasoningTextBlock;

	/** Final Choice 텍스트 블록 (Blueprint에서 바인딩) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FinalChoiceTextBlock;

	/** 현재 체력 표시 텍스트 블록 (Blueprint에서 바인딩) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TEETHcurrentHP;

	/** Brain Cam 데이터 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "Brain Cam")
	void UpdateBrainCamData(const FString& Summary, const FString& FinalChoice, const TArray<FString>& ReasoningDocs);

	/** 콤팩트 모드 여부 (True면 FinalChoice만 표시) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brain Cam")
	bool bIsCompactMode = false;

private:
	/** AI Subsystem 참조 */
	UPROPERTY()
	TObjectPtr<UEnemyAISubsystem> AISubsystem;

	/** Brain Cam 데이터 수신 콜백 */
	UFUNCTION()
	void OnBrainCamDataReceived(const FString& Summary, const FString& FinalChoice, const TArray<FString>& ReasoningDocs);
};
