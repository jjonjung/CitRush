// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Startup/StartupWidget.h"

#include "MediaSource.h"
#include "Data/MediaSourceDataAsset.h"
#include "GameFlow/StartupGameMode.h"
#include "UI/VideoPlayerWidget.h"

void UStartupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(videoAsset))
	{
		if (UMediaSource* source = videoAsset->GetMediaSource("Intro"))
		{
			startVideoWidget->PlayVideo(source);
			startVideoWidget->EnableLoop(false);
			
			startVideoWidget->OnMediaEventCalled.AddDynamic(this, &UStartupWidget::OnVideoEnd);
		}
	}
}

void UStartupWidget::OnVideoEnd(const EMediaEventType& EventType)
{
	if (EventType == EMediaEvent::PlaybackEndReached
		|| EventType == EMediaEvent::MediaClosed)
	{
		if (AStartupGameMode* sGM = GetWorld()->GetAuthGameMode<AStartupGameMode>())
		{
			sGM->EnterToMainMenu();
		}
	}
}
