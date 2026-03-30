// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetRacerSelectionWidget.generated.h"

class AItemInputMachine;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UWidgetAnimation;

/**
 * 레이서 선택 UI 위젯
 * ItemInputMachine에서 레이서를 선택할 때 사용
 */
UCLASS()
class UE_CITRUSH_API UTargetRacerSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// ItemInputMachine 설정
	UFUNCTION(BlueprintCallable, Category="RacerSelection")
	void SetItemInputMachine(AItemInputMachine* InMachine);

	// 레이서 선택 (0, 1, 2 등)
	UFUNCTION(BlueprintCallable, Category="RacerSelection")
	void SelectRacer(int32 RacerIndex);

	// 위젯 닫기
	UFUNCTION(BlueprintCallable, Category="RacerSelection")
	void CloseWidget();

	// 레이서 이미지 설정
	UFUNCTION(BlueprintCallable, Category="RacerSelection")
	void UpdateRacerImages();

	// 레이서 텍스트 설정 (닉네임 등)
	UFUNCTION(BlueprintCallable, Category="RacerSelection")
	void SetRacerText(int32 RacerIndex, const FText& Text);

	// 레이서 닉네임 설정 (편의 함수)
	UFUNCTION(BlueprintCallable, Category="RacerSelection")
	void SetRacerName(int32 RacerIndex, const FString& Name);

private:
	// 버튼 Content를 HorizontalBox로 설정하여 Image와 Text를 나란히 표시
	void SetupButtonContent(UButton* Button, int32 RacerIndex);

	// 헬퍼 함수들
	UImage* GetRacerImage(int32 RacerIndex) const;
	UTextBlock* GetRacerText(int32 RacerIndex) const;
	UWidgetAnimation* GetRacerAnimation(int32 RacerIndex) const;
	void SetRacerImage(int32 RacerIndex, UImage* Image);
	void SetRacerText(int32 RacerIndex, UTextBlock* TextBlock);

	// 통합된 이벤트 핸들러
	UFUNCTION()
	void OnRacerClicked(int32 RacerIndex);

	UFUNCTION()
	void OnRacerHovered(int32 RacerIndex);

	UFUNCTION()
	void OnRacerUnhovered(int32 RacerIndex);

protected:
	// ItemInputMachine 참조
	UPROPERTY(BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<AItemInputMachine> ItemInputMachine;

	// 레이서 선택 버튼들 (블루프린트에서 바인딩)
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<UButton> Racer1Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<UButton> Racer2Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<UButton> Racer3Button;

	// 레이서 이미지 위젯들 (블루프린트에서 바인딩 또는 자동 생성)
	UPROPERTY(meta=(BindWidget, OptionalWidget=true), BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<UImage> Racer1Image;

	UPROPERTY(meta=(BindWidget, OptionalWidget=true), BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<UImage> Racer2Image;

	UPROPERTY(meta=(BindWidget, OptionalWidget=true), BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<UImage> Racer3Image;

	// 레이서 텍스트 위젯들 (자동 생성, 블루프린트에서 접근 가능)
	UPROPERTY(BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<UTextBlock> Racer1Text;

	UPROPERTY(BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<UTextBlock> Racer2Text;

	UPROPERTY(BlueprintReadOnly, Category="RacerSelection")
	TObjectPtr<UTextBlock> Racer3Text;

public:
	// 레이서 이미지 배열 (블루프린트에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RacerSelection")
	TArray<TObjectPtr<UTexture2D>> RacerImages;

protected:

	// 레이서 Hovered 애니메이션 (블루프린트에서 바인딩)
	UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnim), Category="RacerSelection")
	TObjectPtr<UWidgetAnimation> Racer1HoveredAnimation;

	UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnim), Category="RacerSelection")
	TObjectPtr<UWidgetAnimation> Racer2HoveredAnimation;

	UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnim), Category="RacerSelection")
	TObjectPtr<UWidgetAnimation> Racer3HoveredAnimation;

	// 버튼 클릭 이벤트 핸들러 (람다로 래핑)
	UFUNCTION()
	void OnRacer1Clicked() { OnRacerClicked(0); }

	UFUNCTION()
	void OnRacer2Clicked() { OnRacerClicked(1); }

	UFUNCTION()
	void OnRacer3Clicked() { OnRacerClicked(2); }

	// 버튼 Hovered 이벤트 핸들러 (람다로 래핑)
	UFUNCTION()
	void OnRacer1Hovered() { OnRacerHovered(0); }

	UFUNCTION()
	void OnRacer1Unhovered() { OnRacerUnhovered(0); }

	UFUNCTION()
	void OnRacer2Hovered() { OnRacerHovered(1); }

	UFUNCTION()
	void OnRacer2Unhovered() { OnRacerUnhovered(1); }

	UFUNCTION()
	void OnRacer3Hovered() { OnRacerHovered(2); }

	UFUNCTION()
	void OnRacer3Unhovered() { OnRacerUnhovered(2); }
};

