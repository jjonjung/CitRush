// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoinWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;
class UPanelWidget;

/**
 * 코인 표시 위젯 - 재사용 가능한 코인 UI
 * CommanderHUD, 상점 UI, 결과 화면 등에서 재사용 가능
 */
UCLASS()
class UE_CITRUSH_API UCoinWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 코인 값 설정 (애니메이션 포함)
	UFUNCTION(BlueprintCallable, Category="Coin")
	void SetCoinValue(int32 NewValue);

	// 코인 변경 팝업 표시 (증가: +100, 차감: -100)
	UFUNCTION(BlueprintCallable, Category="Coin")
	void ShowCoinChange(int32 ChangeAmount);

	// 경고 상태 설정 (0 근처에서 빨간색으로 깜빡임)
	UFUNCTION(BlueprintCallable, Category="Coin")
	void SetWarningState(bool bIsWarning);

protected:
	// 루트 컨테이너 (CanvasPanel 또는 Overlay) - 필수
	// CanvasPanel과 Overlay 모두 PanelWidget을 상속받으므로 PanelWidget으로 선언
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UPanelWidget> Root_Container;

	// 코인 표시 텍스트 (BindWidget으로 자동 연결)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Coin;

	// 코인 증가 팝업 텍스트 (BindWidget으로 자동 연결, 선택사항)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_CoinGain;

	// 코인 바운스 애니메이션 (BindWidgetAnim으로 자동 연결, 선택사항)
	UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_Bounce;

	// 경고 깜빡임 애니메이션 (BindWidgetAnim으로 자동 연결, 선택사항)
	UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_Warning;

	// 현재 코인 값
	UPROPERTY()
	int32 CurrentCoinValue = 0;

	// 경고 임계값 (이 값 이하면 경고 상태)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin")
	int32 WarningThreshold = 100;
};

