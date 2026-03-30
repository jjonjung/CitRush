// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreatingSessionWidget.generated.h"

class UMapDataAsset;
class UComboBoxKey;
class UEditableTextBox;
class USlider;
class UTextBlock;
class UButton;
/**
 * 세션 생성 Widget. 맵/세션명/최대 플레이어 수 설정 후 세션 생성
 */
UCLASS()
class UE_CITRUSH_API UCreatingSessionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Widget 초기화. 모든 입력 필드 리셋 */
	void Reset();

protected:
	/** Widget 초기화 시 호출. 맵 선택기 초기화 */
	virtual void NativeOnInitialized() override;

	/** Widget 생성 시 호출. 델리게이트 바인딩 및 호스트 이름 설정 */
	virtual void NativeConstruct() override;

private:
	/** 맵 선택기 초기화. MapList에서 맵 목록 로드 */
	void InitializeMapSelector();

	/** 생성 버튼 클릭 처리. 세션 생성 요청 */
	UFUNCTION()
	void OnCreateButtonClick();

	/** 플레이어 수 슬라이더 변경 콜백. 텍스트 갱신 */
	UFUNCTION()
	void OnPlayerCounterValueChanged(float inValue);

	/** 세션 이름 입력 변경 콜백. 생성 버튼 활성화 조건 체크 */
	UFUNCTION()
	void OnSessionNameInputChanged(const FText& inputText);

	/** 맵 선택 변경 콜백 */
	UFUNCTION()
	void OnSelectionChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

	/** ComboBox 선택된 아이템 컨텐츠 생성 */
	UFUNCTION()
	UWidget* OnGenerateSelectorContent(FName Item);

	/** ComboBox 드롭다운 아이템 생성 */
	UFUNCTION()
	UWidget* OnGenerateSelectorItem(FName Item);

protected:
	/** 맵 선택 ComboBox */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UComboBoxKey> mapSelector;

	/** 세션 이름 입력 필드 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> displayNameText;

	/** 최대 플레이어 수 슬라이더 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USlider> maxPlayerCounterSlider;

	/** 최대 플레이어 수 텍스트 표시 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> maxPlayerCounterText;

	/** 최대 플레이어 수 값 */
	int32 maxPlayerCounterValue;

	/** 호스트 이름 표시 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> hostNameDisplay;

	/** 세션 생성 버튼 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> createButton;

	/** 맵 목록 DataAsset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Map")
	TObjectPtr<UMapDataAsset> mapList;

private:
	/** 기본 선택 맵 키 */
	FName defaultKey = NAME_None;
};
