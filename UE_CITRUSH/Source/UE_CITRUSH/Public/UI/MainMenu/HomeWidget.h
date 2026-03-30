// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HomeWidget.generated.h"

class UWidgetDataAsset;
class UButton;
class UVerticalBox;
class UIndexedButton;


/**
 * Widget 추가 매크로 (WidgetDataAsset 사용). 추가 순서가 중요함
 * - WidgetDataAsset에서 Widget 클래스 로드
 * - Widget 생성 후 Dictionary에 추가
 */
#define ADD_WIDGET_CITRUSH(WidgetClass, DA_Widget, WidgetDict, Value) \
	if (TSubclassOf<WidgetClass> factory = DA_Widget->GetWidgetClass<WidgetClass>(WidgetClass::StaticClass()))\
	{\
		if ( UWidget* widget = CreateWidget<WidgetClass>(this, factory) )\
		{\
			WidgetDict.Add(widget, Value);\
		}\
	}\

/**
 * Widget 추가 매크로 (직접 클래스 지정). 추가 순서가 중요함
 * - WBPClass가 있으면 해당 클래스 사용, 없으면 기본 클래스 사용
 * - Widget 생성 후 Dictionary에 추가
 */
#define ADD_WIDGET(WidgetClass, WBPClass, WidgetProperty, WidgetDict, Value) \
	if (WBPClass)\
	{\
		if ( (WidgetProperty = CreateWidget<WidgetClass>(this, WBPClass)) )\
		{\
			WidgetDict.Add(WidgetProperty, Value);\
		}\
	}\
	else\
	{\
		if ( (WidgetProperty = CreateWidget<WidgetClass>(this, WidgetClass::StaticClass())) )\
		{\
			WidgetDict.Add(WidgetProperty, Value);\
		}\
	}\

/**
 * 홈 화면 Widget. 동적 버튼 생성으로 메뉴 항목 표시 (MatchMaking/Settings/Ranking)
 */
UCLASS()
class UE_CITRUSH_API UHomeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Widget 초기화 시 호출. WidgetDataAsset에서 Widget 목록 로드 */
	virtual void NativeOnInitialized() override;

	/** Widget 생성 시 호출. 동적 버튼 생성 및 Exit 버튼 바인딩 */
	virtual void NativeConstruct() override;

public:
	/** 버튼 초기화 (인덱스 기반). 각 인덱스에 대한 버튼 생성 */
	void InitializeButtons(const TMap<int32, FText>& dictTextPerIndex);

	/** 버튼 초기화 (Widget 기반). 각 Widget에 대한 버튼 생성 */
	void InitializeButtons(const TMap<UWidget*, FText>& dictTextPerWidget);

	/** Widget 선택 델리게이트 (Widget 포인터 전달) */
	DECLARE_DELEGATE_OneParam(FOnWidgetSelected, UWidget*);
	FOnWidgetSelected OnWidgetSelected;

	/** 화면 변경 델리게이트 (인덱스 전달) */
	DECLARE_DELEGATE_OneParam(FOnScreenChange, const int32&);
	FOnScreenChange OnScreenButtonClicked;


private:
	/** 버튼 클릭 콜백 (인덱스 기반). OnScreenButtonClicked 델리게이트 브로드캐스트 */
	UFUNCTION()
	void OnButtonClickedIndex(const int32& index);

	/** 버튼 클릭 콜백 (Widget 기반). OnWidgetSelected 델리게이트 브로드캐스트 */
	UFUNCTION()
	void OnButtonClickedWidget(UWidget* widget);

	/** 종료 버튼 클릭 처리. 게임 종료 */
	UFUNCTION()
	void OnClickExitButton();

	/** 버튼 마진 변경 콜백. 모든 버튼의 Padding 갱신 */
	UFUNCTION()
	void OnMarginChanged();

protected:
	/** Widget DataAsset */
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UWidgetDataAsset> widgetDataAsset;

	/** 버튼 컨테이너 VerticalBox. BeginPlay에서 자식 클리어 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget), meta=(ToolTip="Clear Children when Begin Play. You Can Add Widget for Example"))
	TObjectPtr<UVerticalBox> buttonContainer;

	/** 버튼 마진 (Padding) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(
			ToolTip="Buttons Padding",
			EditCondition = true,
			OnPropertyValueChanged = OnMarginChanged
		)
	)
	FMargin buttonMargin = FMargin(0.f, 0.f, 0.f, 40.f);

	/** 종료 버튼 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UButton> exitButton;

	/** 동적 버튼 클래스. NULL이면 기본 클래스 사용 */
	UPROPERTY(EditDefaultsOnly, meta=(ToolTip="Can Be Empty. Set Default Class If Variable is NULL"))
	TSubclassOf<UIndexedButton> dynamicButtonFactory = nullptr;

};
