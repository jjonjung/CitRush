// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IMediaEventSink.h"
#include "StartupWidget.generated.h"

class UVideoPlayerWidget;
class UMediaSourceDataAsset;
enum class EMediaEventType : uint8;

/**
 * 
 */
UCLASS()
class UE_CITRUSH_API UStartupWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnVideoEnd(const EMediaEventType& EventType);
	
protected:
	/** 비디오 플레이어 Widget */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UVideoPlayerWidget> startVideoWidget;
	
	/** 로딩 영상 에셋 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMediaSourceDataAsset> videoAsset;
	
};
