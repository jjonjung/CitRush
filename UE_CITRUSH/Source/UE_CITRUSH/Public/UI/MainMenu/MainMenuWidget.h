// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UMatchMakingWidget;
class UWidgetDataAsset;
class UHomeWidget;
class UMediaSource;
class UVideoPlayerWidget;
class URankingListWidget;
class USettingWidget;
class UButton;
class USearchingSessionsWidget;
class UCreatingSessionWidget;
class UWidgetSwitcher;
class UMediaSourceDataAsset;

/**
 * 메인메뉴 Widget. WidgetSwitcher로 Home/MatchMaking/Setting/Ranking 화면 전환
 */
UCLASS()
class UE_CITRUSH_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Widget 생성 시 호출. 배경 비디오 재생 및 Home 버튼 바인딩 */
	virtual void NativeConstruct() override;

	/** Widget 초기화 시 호출. WidgetDataAsset에서 Widget들 로드 */
	virtual void NativeOnInitialized() override;

private:
	/** Home 버튼 클릭 처리. Home 화면으로 전환 */
	UFUNCTION()
	void OnClickHomeButton();

	/** 화면 전환 버튼 클릭 처리 (인덱스 기반) */
	UFUNCTION()
	void OnClickChangeScreenButton(const int32& inScreenIndex);

	/** 화면 전환 버튼 클릭 처리 (Widget 기반) */
	UFUNCTION()
	void OnClickChangeScreenButtonByWidget(UWidget* inWidgetObject);

protected:
	/** Home 버튼 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UButton> homeButton;

	/** 배경 비디오 플레이어 Widget */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVideoPlayerWidget> backgroundWidget;

	/** 배경 비디오 에셋 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMediaSourceDataAsset> videoAsset;

	/** 화면 전환 WidgetSwitcher */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> screenSwitcher;

	/** Widget 목록 DataAsset */
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UWidgetDataAsset> widgetDataAsset;

	#pragma region SwitchingWidgets
	/** Home 화면 Widget */
	UPROPERTY()
	TObjectPtr<UHomeWidget> homeWidget;

	/** 설정 화면 Widget */
	UPROPERTY()
	TObjectPtr<USettingWidget> settingWidget;

	/** 세션 생성 화면 Widget */
	UPROPERTY()
	TObjectPtr<UCreatingSessionWidget> creatingSessionWidget;

	/** 세션 검색 화면 Widget */
	UPROPERTY()
	TObjectPtr<USearchingSessionsWidget> searchingSessionsWidget;

	/** 매치메이킹 화면 Widget */
	UPROPERTY()
	TObjectPtr<UMatchMakingWidget> matchMakingWidget;

	/** 랭킹 목록 화면 Widget */
	UPROPERTY()
	TObjectPtr<URankingListWidget> rankingListWidget;
	#pragma endregion SwitchingWidgets
};
