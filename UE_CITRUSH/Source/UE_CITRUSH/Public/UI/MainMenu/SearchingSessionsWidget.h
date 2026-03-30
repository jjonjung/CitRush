// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnlineSessionSettings.h"
#include "SearchingSessionsWidget.generated.h"

class UEditableTextBox;
class USessionItemData;
class UTextBlock;
class UButton;
class UListView;
class FOnlineSessionSearch;
class FOnlineSessionSearchResult;

/**
 * 세션 검색 Widget. ListView로 세션 목록 표시 및 참가 처리 (페이징 지원)
 */
UCLASS()
class UE_CITRUSH_API USearchingSessionsWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Widget 초기화 시 호출. SessionSubsystem 세션 검색 완료 델리게이트 바인딩 */
	virtual void NativeOnInitialized() override;

	/** Widget 생성 시 호출. 버튼 클릭 델리게이트 바인딩 및 자동 새로고침 타이머 시작 */
	virtual void NativeConstruct() override;

public:
	/** 페이지 설정. 현재 페이지 변경 및 목록 갱신 */
	UFUNCTION()
	void SetPage(const int32& newPage);

	/** 세션 목록 리셋. 페이지 0으로 초기화 */
	UFUNCTION()
	void ResetSessionsList();

private:
	/** 세션 검색 완료 콜백. SessionSubsystem에서 호출 */
	void OnSearchSessionCompleted(const TSharedPtr<FOnlineSessionSearch>& search);
	void OnSearchSessionByCodeCompleted(const FOnlineSessionSearchResult& SearchResult);

	/** 목록 새로고침. SessionSubsystem에 세션 검색 요청 */
	UFUNCTION()
	void RefreshList();

	/** 세션 목록 채우기. 현재 페이지 기준 SessionItemData 생성 */
	UFUNCTION()
	void PopulateSessionsList();

	/** 다음 페이지 버튼 클릭 처리 */
	UFUNCTION()
	void OnClickNextButton();

	/** 이전 페이지 버튼 클릭 처리 */
	UFUNCTION()
	void OnClickPrevButton();

	/** 세션 아이템 클릭 처리. ListView 선택 */
	UFUNCTION()
	void OnClickSessionItem(UObject* clickedItem);

	/** 참가 버튼 클릭 처리. 선택된 세션에 참가 */
	UFUNCTION()
	void OnClickJoinButton();

	/** 코드로 세션 찾기 버튼 클릭 처리 */
	UFUNCTION()
	void OnClickFindSessionByCodeButton();
	
	UFUNCTION()
	void OnCommitInputCode(const FText& Text, ETextCommit::Type CommitMethod);

protected:
	/** 세션 목록 ListView */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UListView> sessionsEntry;

	/** 세션 참가 버튼 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UButton> joinSessionButton;

	/** 목록 새로고침 버튼 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UButton> refreshSessionButton;

	/** 코드로 찾기 버튼 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UButton> findSessionByCodeButton;
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> sessionSearchByCodeInputText;

	/** 페이지당 표시할 아이템 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 itemCountInEntry = 5;

	/** 현재 페이지 번호 */
	UPROPERTY(EditDefaultsOnly)
	int32 currentPage = -1;

	/** 페이지 카운터 텍스트 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> pageCounter;

	/** 이전 페이지 버튼 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UButton> prevButton;

	/** 다음 페이지 버튼 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UButton> nextButton;

	/** 세션 아이템 데이터 배열 (ListView 데이터) */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TArray<USessionItemData*> sessionItemDataArray;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	USessionItemData* focussingSessionItemData;
	
	/** 세션 검색 결과 배열 (전체 결과 저장) */
	TArray<FOnlineSessionSearchResult> sessionSearchResults;

private:
	/** 자동 새로고침 타이머 핸들 */
	FTimerHandle refreshListTimer;

	/** 마지막 페이지 번호 */
	int32 endPage = -1;

	/** 시작 페이지 번호 */
	int32 startPage = -1;
	
};
