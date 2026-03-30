// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LoadingScreen/LoadingWidget.h"

#include "UI/VideoPlayerWidget.h"
#include "Data/MediaSourceDataAsset.h"

void ULoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (IsValid(videoAsset))
	{
		if (UMediaSource* source = videoAsset->GetMediaSource("Loading"))
		{
			loadingVideo->PlayVideo(source);
			loadingVideo->EnableLoop(true);
		}
	}
}
