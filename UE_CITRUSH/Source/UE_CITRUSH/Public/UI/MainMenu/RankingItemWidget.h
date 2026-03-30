// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "RankingItemWidget.generated.h"

class UButton;
class UTextBlock;
/**
 * 랭킹 항목 Widget. ListView 아이템으로 팀 이름 표시
 */
UCLASS()
class UE_CITRUSH_API URankingItemWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	/** Widget 생성 시 호출. 버튼 클릭 델리게이트 바인딩 */
	virtual void NativeConstruct() override;

	/** ListView 아이템 설정 시 호출. 팀 이름 표시 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject);

private:
	/** 아이템 클릭 처리 */
	UFUNCTION()
	void OnClickItem();

protected:
	/** 배경 버튼 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UButton> backgroundButton;

	/** 팀 이름 텍스트 */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> teamNameText;
};
