// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IndexedButton.generated.h"

class UButton;
class UTextBlock;
/**
 * 인덱스 기반 버튼 Widget. 버튼 클릭 시 인덱스 또는 바운드 위젯 전달
 */
UCLASS()
class UE_CITRUSH_API UIndexedButton : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Widget 생성 시 호출. 버튼 클릭 델리게이트 바인딩 */
	virtual void NativeConstruct() override;

public:
	/** 버튼 텍스트 설정 */
	void SetText(const FText& newText);

	/** 버튼 인덱스 설정 */
	FORCEINLINE void SetIndex(const int32& newIndex) {widgetIndex = newIndex;}

	/** 바운드 위젯 설정 */
	FORCEINLINE void SetWidget(UWidget* newWidget) {boundWidget = MakeWeakObjectPtr(newWidget);}

	/** 인덱스 버튼 클릭 델리게이트 (인덱스 전달) */
	DECLARE_DELEGATE_OneParam(FOnIndexedButtonClicked, const int32&)
	FOnIndexedButtonClicked OnIndexedButtonClicked;

	/** 바운드 위젯 버튼 클릭 델리게이트 (위젯 포인터 전달) */
	DECLARE_DELEGATE_OneParam(FOnBoundWidgetButtonClicked, UWidget*)
	FOnBoundWidgetButtonClicked OnBoundWidgetButtonClicked;

	/** 내부 버튼 반환 */
	TObjectPtr<UButton> GetButton() const {return indexedButton;}

private:
	/** 버튼 클릭 시 호출. 델리게이트 브로드캐스트 */
	UFUNCTION()
	void OnDynamicButtonClicked();

protected:
	/** 버튼 Widget */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> indexedButton;

	/** 버튼 텍스트 블록 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> buttonText;

	/** 버튼 인덱스 */
	UPROPERTY()
	int32 widgetIndex = -1;

	/** 바운드된 위젯 (약한 포인터) */
	UPROPERTY()
	TWeakObjectPtr<UWidget> boundWidget = nullptr;
};
