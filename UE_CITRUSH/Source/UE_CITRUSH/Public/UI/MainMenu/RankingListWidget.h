// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RankingListWidget.generated.h"

class UTreeView;
/**
 * 랭킹 목록 Widget. TreeView로 팀 랭킹 표시
 */
UCLASS()
class UE_CITRUSH_API URankingListWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Widget 생성 시 호출. TreeView 초기화 */
	virtual void NativeConstruct() override;

protected:
	/** 랭킹 TreeView */
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
	TObjectPtr<UTreeView> rankingEntry;
};
