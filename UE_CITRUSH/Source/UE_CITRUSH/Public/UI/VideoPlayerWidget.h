// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IMediaEventSink.h"
#include "VideoPlayerWidget.generated.h"

class UMediaSoundComponent;
class UMediaPlayer;
class UMediaPlaylist;
class UMediaSource;
class UMediaTexture;
class UImage;

UENUM(BlueprintType)
enum class EMediaEventType : uint8
{
	MediaBuffering,
	MediaBufferingComplete,
	MediaClosed,
	MediaConnecting,
	MediaOpened,
	MediaOpenFailed,
	PlaybackEndReached,
	PlaybackResumed,
	PlaybackSuspended,
	SeekCompleted,
	TracksChanged,
	MetadataChanged,

	// - - - - - - - - - - - - - - - - - - - - - - - -
	Internal_Start,

	Internal_PurgeVideoSamplesHint = Internal_Start,

	Internal_VideoSamplesAvailable,
	Internal_VideoSamplesUnavailable,
	Internal_AudioSamplesAvailable,
	Internal_AudioSamplesUnavailable
};
FORCEINLINE bool operator==(EMediaEventType lEnumType, EMediaEvent rEnum)
{
	return static_cast<uint8>(lEnumType) == static_cast<uint8>(rEnum);
}

/**
 * 비디오 재생 Widget. MediaPlayer/MediaTexture로 동영상 재생 및 제어
 */
UCLASS(BlueprintType)
class UE_CITRUSH_API UVideoPlayerWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativePreConstruct() override;
	
	/** Widget 생성 시 호출. MediaPlayer/MediaTexture 생성 및 Image에 바인딩 */
	virtual void NativeConstruct() override;

	/** Widget 소멸 시 호출. MediaPlayer 정리 */
	virtual void NativeDestruct() override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

public:	
	/** 비디오 재생 시작 */
	UFUNCTION(BlueprintCallable, Category = "Video")
	void PlayVideo(UMediaSource* video);

	/** 비디오 재생 정지 */
	UFUNCTION(BlueprintCallable, Category = "Video")
	void StopVideo();

	/** 비디오 일시정지 */
	UFUNCTION(BlueprintCallable, Category = "Video")
	void PauseVideo();

	/** 루프 재생 활성화/비활성화 */
	UFUNCTION(BlueprintCallable, Category = "Video")
	void EnableLoop(bool bEnableLoop) const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMediaEventCalled, const EMediaEventType&, EventType);
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable, Category = "Video|Event")
	FOnMediaEventCalled OnMediaEventCalled;

private:
	/** 미디어 열림 콜백. 재생 시작 */
	UFUNCTION()
	void OnMediaOpened(FString OpenedUrl);

	void OnMediaEvent(EMediaEvent EventEnum);

protected:
	/** 비디오 표시 Image Widget */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> resourceViewer;

	/** MediaPlayer (동영상 재생) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UMediaPlayer* mediaPlayer;

	/** MediaTexture (MediaPlayer 출력 텍스처) */
	UPROPERTY(EditDefaultsOnly)
	UMediaTexture* mediaTexture;

	UPROPERTY()
	TObjectPtr<UMediaSoundComponent> mediaSoundComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(BindWidgetOptional))
	bool bCanSkip = false;

};
