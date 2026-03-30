// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Controller/CitRushPlayerController.h"

#include "Data/InputMappingsSettings.h"
#include "Data/MapDataAsset.h"
#include "GameFlow/CitRushGameMode.h"
#include "GameFlow/CitRushGameState.h"
#include "GameFlow/LocalDataFlowSubsystem.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CitRushPlayerState.h"
#include "UI/InGameMenu/InGameMenuWidget.h"
#include "UI/LoadingScreen/LoadingWidget.h"
#include "Utility/MapAssetLoader.h"
#include "Utility/WidgetBlueprintLoader.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionLevelStreamingPolicy.h"
#include "Player/Car/VehicleDemoUITest.h"
#include "VehicleTemplate/UE_CITRUSHPlayerController.h"
#include "Player/AbstractRacer.h"
#include "Player/CommenderCharacter.h"
#include "UI/CommenderHUDWidget.h"
#include "UI/CommanderMessageType.h"

DEFINE_ACLASS_LOG(CitRushPlayerController)

ACitRushPlayerController::ACitRushPlayerController()
{
	if (const FInputMappingData* inputMappings = UInputMappingsSettings::Get()->inputMappings.Find("IMC_InGameUI"))
	{
		IMC_InGameUI = inputMappings->inputMappingContext;
		IA_InGameMenu = inputMappings->inputActions["IA_InGameMenu"];
	}
	UWidgetBlueprintDataAsset* PDA_WBP = UWidgetBlueprintLoader::Get()->PDA_WBP.LoadSynchronous();
	if (PDA_WBP)
	{
		menuWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey(TEXT("InGameUI"));
		loadingWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey(TEXT("Loading"));
	}
	const UMapAssetLoader* mapAssetLoader = UMapAssetLoader::Get();
	mapAsset = mapAssetLoader->PDA_Map.LoadSynchronous();
}

void ACitRushPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		InitSettingsByGameMode();
	}
	
}

void ACitRushPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	Super::EndPlay(EndPlayReason);
}

void ACitRushPlayerController::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	//CITRUSH_TIME("CitRush_Loading");
}

void ACitRushPlayerController::NotifyLoadedWorld(FName WorldPackageName, bool bFinalDest)
{
	Super::NotifyLoadedWorld(WorldPackageName, bFinalDest);
	
	//CITRUSH_TIME("CitRush_Loading");
	if (bFinalDest || GIsPlayInEditorWorld)
	{
		UE_LOG(CitRushPlayerController, Log, TEXT("[NotifyLoadedWorld] ReceivedPlayer - Final Level loaded"));
		if (ACitRushGameMode* cGM = GetWorld()->GetAuthGameMode<ACitRushGameMode>())
		{
			if (ACitRushGameState* cGS = cGM->GetGameState<ACitRushGameState>())
			{
				cGS->NotifyLoadWorld(true);
			}
		}

		if (ACitRushPlayerState* cPS = GetPlayerState<ACitRushPlayerState>();
			IsLocalController())
		{
			
		}
	}
}

void ACitRushPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
}

void ACitRushPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);
	bFinalDestination = false;

	//CITRUSH_TIME("CitRush_Loading");
	if (ACitRushPlayerState* cPS = GetPlayerState<ACitRushPlayerState>();
		IsLocalController())
	{
		UE_LOG(CitRushPlayerController, Log, TEXT("[LocalPC] PreClientTravel - Starting travel to: %s"), *PendingURL);

		cPS->ServerRPC_SetLoadingState(ELoadingState::Traveling);
	}
}

void ACitRushPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	//CITRUSH_TIME("CitRush_Loading");

}

void ACitRushPlayerController::ClientRPC_StartCheckWorldPartition_Implementation(FVector inPlayerStartLocation)
{
	CITRUSH_TIME("Loading");
	playerStartLocation = inPlayerStartLocation;
	if (checkWorldPartitionTimer.IsValid()) {return;}
	/*if (GetWorld())
	{
		GetWorld()->BlockTillLevelStreamingCompleted();
	}*/
	GetGameInstance()->GetTimerManager().SetTimer(checkWorldPartitionTimer
		, this, &ACitRushPlayerController::CheckWorldPartition
		, 0.5f, true, 0.2f
	);
}

void ACitRushPlayerController::CheckWorldPartition()
{
	if (!checkWorldPartitionTimer.IsValid()) {return;}
	FWorldPartitionStreamingQuerySource query(playerStartLocation);
	query.Radius = 20000.f;
	query.bSpatialQuery = true;
	if (UWorld* world = GetWorld())
	{
		UWorldPartition* worldPartition = world->GetWorldPartition();
		if (!IsValid(worldPartition))
		{
			--retryCount;
			if (retryCount <= 0)
			{
				GetGameInstance()->GetTimerManager().ClearTimer(checkWorldPartitionTimer);
				LogError(TEXT("Spend All Retry Chance for Checking World Partition"));
				GetPlayerState<ACitRushPlayerState>()->ServerRPC_SetLoadingState(ELoadingState::Ready);
			}
			return;
		}
	    if (worldPartition->IsStreamingCompleted(EWorldPartitionRuntimeCellState::Activated, {query}, true))
   		{
   			GetGameInstance()->GetTimerManager().ClearTimer(checkWorldPartitionTimer);
   			GetPlayerState<ACitRushPlayerState>()->ServerRPC_SetLoadingState(ELoadingState::Ready);
   			CITRUSH_TIME("Loading");
   		}
	}
}

void ACitRushPlayerController::InitSettingsByGameMode()
{
	if (AGameStateBase* gs = GetWorld()->GetGameState<AGameStateBase>())
	{
		CITRUSH_TIME("PlayerController");
		if (ACitRushGameState* cGS = Cast<ACitRushGameState>(gs))
		{
			SetInputMode(FInputModeGameOnly());
			SetShowMouseCursor(false);
			bInGameDestination = true;
			cGS->OnGameEndedDelegate.AddDynamic(this, &ACitRushPlayerController::OnGameEnded);
			LoadInGameSettings();
		}
		else
		{
			SetInputMode(FInputModeUIOnly());
			SetShowMouseCursor(true);
			bInGameDestination = false;
			UnloadInGameSettings();
		}
	}
}

void ACitRushPlayerController::OnGameEnded()
{
	if (endMatchTimer.IsValid()) {return;}
	CITRUSH_TIME("PlayerController");
	GetWorldTimerManager().SetTimer(endMatchTimer, [&]()
	{
		FMapInfo mapInfo;
		if (mapAsset->GetMapInfoByKey("End", mapInfo))
		{
			UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Traveling to EndGame map"));
			UGameplayStatics::OpenLevel(GetWorld(), FName(mapInfo.GetMapAsset().GetAssetName()));
		}
	}, 3.0f, false);
}

void ACitRushPlayerController::LoadInGameSettings()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(IMC_InGameUI, 1);
	}
	if (UEnhancedInputComponent* eic = Cast<UEnhancedInputComponent>(InputComponent))
	{
		FEnhancedInputActionEventBinding& binding = eic->BindAction(IA_InGameMenu, ETriggerEvent::Started, this, &ACitRushPlayerController::OnInputInGameMenuKey);
		bindingHandle = binding.GetHandle();		
	}

	if (!IsValid(menuWidget))
	{
		menuWidget = CreateWidget<UInGameMenuWidget>(this, menuWidgetClass);
		menuWidget->AddToViewport(5);
		menuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (GIsPlayInEditorWorld) {return;}
	
	if (ACitRushPlayerState* cPS = GetPlayerState<ACitRushPlayerState>())
	{
		cPS->OnLoadingStateChanged.AddUObject(this, &ACitRushPlayerController::OnLoadingStateChanged);
		loadingWidget = CreateWidget<ULoadingWidget>(this, loadingWidgetClass);
		loadingWidget->AddToViewport(10);
	}
}

void ACitRushPlayerController::UnloadInGameSettings()
{
	if (bindingHandle == MAX_uint32) {return;}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(IMC_InGameUI);
	}
	if (UEnhancedInputComponent* eic = Cast<UEnhancedInputComponent>(InputComponent))
	{
		eic->RemoveBindingByHandle(bindingHandle);
		bindingHandle = MAX_uint32;
	}
	if (IsValid(menuWidget))
	{
		menuWidget->RemoveFromParent();
	}
	if (ACitRushPlayerState* cPS = GetPlayerState<ACitRushPlayerState>())
	{
		cPS->OnLoadingStateChanged.RemoveAll(this);
	}
}

void ACitRushPlayerController::OnInputInGameMenuKey(const FInputActionValue& inputActionValue)
{
	if (menuWidget->IsVisible())
	{
		menuWidget->SetVisibility(ESlateVisibility::Collapsed);
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		return;
	}
	
	menuWidget->SetVisibility(ESlateVisibility::Visible);
	SetInputMode(FInputModeGameAndUI());
	SetShowMouseCursor(true);
}

void ACitRushPlayerController::OnLoadingStateChanged(ELoadingState newState)
{
	if (newState == ELoadingState::MatchStarted)
	{
		SetInputMode(FInputModeGameOnly());
		if (IsValid(loadingWidget))
		{
			loadingWidget->RemoveFromParent();
		}
	}
}

void ACitRushPlayerController::ClientShowStateMessage_Implementation(const FString& Message)
{
	// Pawn에서 UI 찾기
	if (APawn* ControlledPawn = GetPawn())
	{
		// 레이서인 경우 VehicleUI 찾기
		if (AAbstractRacer* Racer = Cast<AAbstractRacer>(ControlledPawn))
		{
			// Racer의 VehicleDemoUITest 위젯 찾기
			if (APlayerController* RacerPC = Racer->GetController<APlayerController>())
			{
				if (AUE_CITRUSHPlayerController* VehiclePC = Cast<AUE_CITRUSHPlayerController>(RacerPC))
				{
					if (UVehicleDemoUITest* VehicleUI = VehiclePC->GetVehicleUI())
					{
						VehicleUI->ShowStateMessage(Message);
						UE_LOG(CitRushPlayerController, Log, TEXT("[CitRushPlayerController] ClientShowStateMessage 호출 (레이서): %s"), *Message);
						return;
					}
				}
			}
		}
		// 커맨더인 경우 CommenderHUDWidget 찾기
		else if (ACommenderCharacter* Commander = Cast<ACommenderCharacter>(ControlledPawn))
		{
			if (UCommenderHUDWidget* CommanderHUD = Commander->CommenderHUDWidget)
			{
				CommanderHUD->ShowMessage(FText::FromString(Message), ECommanderMessageType::Info, 3.0f);
				UE_LOG(CitRushPlayerController, Log, TEXT("[CitRushPlayerController] ClientShowStateMessage 호출 (커맨더): %s"), *Message);
				return;
			}
		}
	}
	
	UE_LOG(CitRushPlayerController, Warning, TEXT("[CitRushPlayerController] UI를 찾을 수 없어서 메시지를 표시할 수 없습니다: %s"), *Message);
}