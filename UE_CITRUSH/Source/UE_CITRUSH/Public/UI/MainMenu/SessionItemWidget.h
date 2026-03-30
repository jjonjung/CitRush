// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "SessionItemWidget.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(SessionItemLog, Warning, All);

class UButton;

/**
 * 세션 아이템 데이터. OnlineSessionSearchResult 래핑
 */
UCLASS(BlueprintType)
class UE_CITRUSH_API USessionItemData : public UObject
{
	GENERATED_BODY()

public:
	/** SessionItemData 초기화. SessionSearchResult 생성 */
	UFUNCTION(BlueprintCallable)
	void Initialize();

	/** 세션 데이터 작성. OnlineSessionSearchResult 래핑 */
	void WriteData(const FOnlineSessionSearchResult& session, const int32& inSearchResultIndex);

	/** 검색 결과 인덱스 반환 */
	FORCEINLINE int32 GetSearchResultIndex() const {return searchResultIndex;}

	/** 세션 검색 결과 반환 */
	FORCEINLINE TSharedPtr<FOnlineSessionSearchResult> GetSessionResult() const {return sessionSearchResult;}

	/** 세션 참가 가능 여부 확인 */
	UFUNCTION(BlueprintCallable)
	bool CanJoinInSession();

protected:
	/** 세션 검색 결과 인덱스 */
	UPROPERTY(BlueprintReadOnly, Category="SessionData|JoinSession")
	int32 searchResultIndex = -1;

	/** 세션 검색 결과 (SharedPtr) */
	TSharedPtr<FOnlineSessionSearchResult> sessionSearchResult;
};

class UTextBlock;


/**
 * 세션 항목 Widget. ListView 아이템으로 세션 정보 표시 (이름/호스트/맵/인원)
 */
UCLASS()
class UE_CITRUSH_API USessionItemWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
protected:
	/** Widget 생성 시 호출 */
	virtual void NativeConstruct() override;

	/** ListView 아이템 설정 시 호출. 세션 정보를 텍스트로 표시 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:
	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(BindWidget))
	//TObjectPtr<UButton> backgroundButton;

	/** 세션 표시 이름 텍스트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UTextBlock> displayNameText;

	/** 호스트 이름 텍스트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UTextBlock> hostNameText;

	/** 맵 이름 텍스트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UTextBlock> mapNameText;

	/** 현재 플레이어 수 텍스트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UTextBlock> currentPlayerCounterText;

	/** 최대 플레이어 수 텍스트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UTextBlock> maxPlayerCounterText;

	/** 호스트 핑 텍스트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UTextBlock> hostPingText;

private:

};
