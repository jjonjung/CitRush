// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingWidget.generated.h"

class UMediaSourceDataAsset;
class UVideoPlayerWidget;
/**
 * 로딩 화면 Widget. VideoPlayerWidget으로 로딩 영상 표시
 */
UCLASS()
class UE_CITRUSH_API ULoadingWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	/** Widget 생성 시 호출. 로딩 영상 재생 시작 */
	virtual void NativeConstruct() override;

protected:
	/** 비디오 플레이어 Widget */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVideoPlayerWidget> loadingVideo;

	/** 로딩 영상 에셋 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMediaSourceDataAsset> videoAsset;
};
