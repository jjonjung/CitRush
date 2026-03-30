// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/CitRushGameInstance.h"

#include "MoviePlayer.h"
#include "Camera/CameraActor.h"
#include "Network/SteamSubsystem.h"

DEFINE_UCLASS_LOG(CitRushGameInstance);

void UCitRushGameInstance::Init()
{
	Super::Init();
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UCitRushGameInstance::OnNetworkFailure);
	}
	//FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UCitRushGameInstance::OnPostWorldInit);
}

void UCitRushGameInstance::ResetMoviePlayerSettings()
{
	GetMoviePlayer()->SetupLoadingScreenFromIni();
}

void UCitRushGameInstance::SetCustomMoviePlayerSettings()
{
	FLoadingScreenAttributes loadingScreen;
	loadingScreen.PlaybackType = EMoviePlaybackType::MT_Looped;
	loadingScreen.bAutoCompleteWhenLoadingCompletes = false;
	loadingScreen.bWaitForManualStop = true;
	loadingScreen.bMoviesAreSkippable = false;
	loadingScreen.MoviePaths = {"LoadingVideo"};
	GetMoviePlayer()->SetupLoadingScreen(loadingScreen);
}

void UCitRushGameInstance::ReturnToMainMenu()
{
	Super::ReturnToMainMenu();
	if (USteamSubsystem* steamSubsystem = GetSubsystem<USteamSubsystem>())
	{
		steamSubsystem->DestroySession();
	}
}

void UCitRushGameInstance::PlayLoadingMoviePlayer()
{
	/*
	if (GetMoviePlayer()->IsMovieCurrentlyPlaying()) {return;}
	
	SetCustomMoviePlayerSettings();

	GetMoviePlayer()->PlayMovie();
	*/
}

void UCitRushGameInstance::StopLoadingMoviePlayer()
{
	/*
	if (!GetMoviePlayer()->IsMovieCurrentlyPlaying()) {return;}
	
	GetMoviePlayer()->StopMovie();

	ResetMoviePlayerSettings();*/
}

void UCitRushGameInstance::OnStartInGame(const UWorld* world)
{
	/*
	StopLoadingMoviePlayer();
	
	for (FConstCameraActorIterator camIter = world->GetAutoActivateCameraIterator(); camIter; ++camIter)
	{
		if (!camIter->IsValid(false)) {return;}

		TStrongObjectPtr<ACameraActor> camActor = camIter->Pin(false);
		if (!camActor.IsValid()) {return;}

		if (const APlayerController* netController = Cast<APlayerController>(camActor->GetNetOwner()))
		{
			if (IsValid(netController) && netController->IsLocalController() && IsValid(netController->PlayerCameraManager))
			{
				netController->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.5f, FColor::Black, false, false);
			}
		}
		
	}*/
}

void UCitRushGameInstance::OnPostWorldInit(UWorld* world, UWorld::InitializationValues IVS)
{
	/*if (!world->IsInSeamlessTravel()) {return;}
	if (!GetMoviePlayer()->IsMovieCurrentlyPlaying())
	{
		GetMoviePlayer()->PlayMovie();
	}
	for (FConstCameraActorIterator camIter = world->GetAutoActivateCameraIterator(); camIter; ++camIter)
	{
		if (!camIter->IsValid(false)) {return;}

		TStrongObjectPtr<ACameraActor> camActor = camIter->Pin(false);
		if (!camActor.IsValid()) {return;}

		if (const APlayerController* netController = Cast<APlayerController>(camActor->GetNetOwner()))
		{
			if (IsValid(netController) && netController->IsLocalController() && IsValid(netController->PlayerCameraManager))
			{
				netController->PlayerCameraManager->StartCameraFade(1.f, 1.f, 0.f, FColor::Black, true, true);
			}
		}
		
	}*/
}

void UCitRushGameInstance::OnNetworkFailure(UWorld* world, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
                                            const FString& ErrorString)
{
	LogError(TEXT("%s"), *ErrorString);
	ReturnToMainMenu();
}
