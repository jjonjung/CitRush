// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/VideoPlayerWidget.h"

#include "MediaPlayer.h"
#include "MediaPlaylist.h"
#include "MediaSoundComponent.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/MainMenu/MainMenuWidget.h"
#include "Utility/DebugHelper.h"


void UVideoPlayerWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (!mediaPlayer)
	{
		mediaPlayer = NewObject<UMediaPlayer>(this);
	}
	if (!mediaTexture)
	{
		mediaTexture = NewObject<UMediaTexture>(this);
	}

	if (!mediaSoundComponent && GetOwningPlayer())
	{
		mediaSoundComponent = NewObject<UMediaSoundComponent>(GetOwningPlayer());
		mediaSoundComponent->RegisterComponent();
	}
}

void UVideoPlayerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!IsValid(resourceViewer)) {return;}
	if (IsValid(mediaPlayer) && IsValid(mediaTexture))
	{
		mediaTexture->SetMediaPlayer(mediaPlayer);
		mediaTexture->UpdateResource();

		if (mediaSoundComponent)
		{
			mediaSoundComponent->SetMediaPlayer(mediaPlayer);
		}
		
		mediaPlayer->OnMediaOpened.AddDynamic(this, &UVideoPlayerWidget::OnMediaOpened);
		mediaPlayer->OnMediaEvent().AddUObject(this, &UVideoPlayerWidget::OnMediaEvent);
				
	} else {CITRUSH_LOG("media Fail")}
}

void UVideoPlayerWidget::NativeDestruct()
{
	if (IsValid(mediaPlayer)) 
	{
		mediaPlayer->Close();
		mediaPlayer->CleanUpBeforeDestroy();
	}
	
	Super::NativeDestruct();
}

FReply UVideoPlayerWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() != EKeys::Escape || !bCanSkip) {return Super::NativeOnKeyDown(InGeometry, InKeyEvent);}

	mediaPlayer->Close();
	
	return FReply::Handled();
}

void UVideoPlayerWidget::PlayVideo(UMediaSource* video)
{
	if (IsValid(mediaPlayer)) 
	{
		mediaPlayer->OpenSource(video);
		//mediaPlayer->SetNativeVolume(20.f);
	}
}

void UVideoPlayerWidget::StopVideo()
{
	if (IsValid(mediaPlayer)) 
	{
		mediaPlayer->Close();
	}
}

void UVideoPlayerWidget::PauseVideo()
{
	if (IsValid(mediaPlayer)) 
	{
		mediaPlayer->Pause();
	}
}

void UVideoPlayerWidget::EnableLoop(bool bEnableLoop) const
{
	if (IsValid(mediaPlayer))
	{
		mediaPlayer->SetLooping(bEnableLoop);
	}
}

void UVideoPlayerWidget::OnMediaOpened(FString OpenedUrl)
{
	if (IsValid(mediaPlayer))
	{
		FSlateBrush brush;
		brush.SetResourceObject(mediaTexture);
		FVector2D imageSize = brush.GetImageSize();
		resourceViewer->SetBrush(brush);
		resourceViewer->SetDesiredSizeOverride(imageSize);
		
		mediaPlayer->Play();
		UE_LOG(LogTemp, Warning, TEXT("Number of Media in Playlist : %d"), mediaPlayer->GetPlaylist()->Num());
	}
}

void UVideoPlayerWidget::OnMediaEvent(EMediaEvent EventType)
{
	OnMediaEventCalled.Broadcast(static_cast<EMediaEventType>(EventType));
}