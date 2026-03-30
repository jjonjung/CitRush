// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetRacerDisplayWidget.generated.h"

class UImage;
class UTextBlock;
class UPanelWidget;

/**
 * 타겟 레이서 표시 위젯
 * ItemInputMachine 위에 현재 타겟 레이서를 표시
 */
UCLASS()
class UE_CITRUSH_API UTargetRacerDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 타겟 레이서 설정 (레이서 인덱스, 레이서 이미지, 색상)
	UFUNCTION(BlueprintCallable, Category="TargetRacer")
	void SetTargetRacer(int32 RacerIndex, UTexture2D* RacerImage = nullptr, FLinearColor RacerColor = FLinearColor::White);

	// 타겟 레이서 초기화 (타겟 없음)
	UFUNCTION(BlueprintCallable, Category="TargetRacer")
	void ClearTargetRacer();

protected:
	// 루트 컨테이너 (CanvasPanel 또는 Overlay) - 필수
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UPanelWidget> Root_Container;

	// 레이서 초상화 이미지
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_RacerPortrait;

	// 레이서 이름 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_RacerName;

	// 현재 타겟 레이서 인덱스
	UPROPERTY(BlueprintReadOnly, Category="TargetRacer")
	int32 CurrentRacerIndex = -1;

	// 레이서 정보 업데이트 (블루프린트에서 구현 가능)
	UFUNCTION(BlueprintImplementableEvent, Category="TargetRacer")
	void OnTargetRacerUpdated(int32 RacerIndex);
};

