// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchMakingWidget.generated.h"

class UMapDataAsset;
class UBackgroundBlur;
class UTextBlock;
class UButton;
/**
 * 매치메이킹 Widget. 세션 생성/검색 메뉴 표시
 */
UCLASS()
class UE_CITRUSH_API UMatchMakingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Widget 생성 시 호출 */
	virtual void NativeConstruct() override;

private:

	UFUNCTION()
	void OnSelectCommander();
	UFUNCTION()
	void OnSelectRacer();

private:
	/** 매치메이킹 처리 */
	UFUNCTION()
	void OnMatchMaking();
	UFUNCTION()
	void OnCancelMatchMaking(); 

protected:
	/** 지휘관 Role 선택 버튼 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> selectCommanderButton;

	/** 레이서 Role 선택 버튼 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> selectRacerButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> cancelButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBackgroundBlur> matchBlurPanel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> matchMakingInfoText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Map")
	TObjectPtr<UMapDataAsset> mapList;
};
