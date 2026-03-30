// Copyright Epic Games, Inc. All Rights Reserved.


#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHPlayerController.h"
#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHPawn.h"
//#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHUI.h"
#include "Player/Car/VehicleDemoUITest.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"

void AUE_CITRUSHPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// VehicleUIClass가 설정되어 있는지 확인
	if (!VehicleUIClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[UE_CITRUSHPlayerController] VehicleUIClass is not set in Blueprint!"));
		return;
	}

	// spawn the UI widget and add it to the viewport
	VehicleUI = CreateWidget<UVehicleDemoUITest>(this, VehicleUIClass);

	if (!IsValid(VehicleUI))
	{
		UE_LOG(LogTemp, Error, TEXT("[UE_CITRUSHPlayerController] Failed to create VehicleUI widget!"));
		return;
	}

	VehicleUI->AddToViewport();
	UE_LOG(LogTemp, Log, TEXT("[UE_CITRUSHPlayerController] VehicleUI created and added to viewport"));
}

void AUE_CITRUSHPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}
}

void AUE_CITRUSHPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);
}

void AUE_CITRUSHPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<AUE_CITRUSHPawn>(InPawn);
}

void AUE_CITRUSHPlayerController::ClientShowStateMessage_Implementation(const FString& Message)
{
	if (UVehicleDemoUITest* VehicleUIWidget = GetVehicleUI())
	{
		VehicleUIWidget->ShowStateMessage(Message);
		UE_LOG(LogTemp, Log, TEXT("[UE_CITRUSHPlayerController] ClientShowStateMessage 호출: %s"), *Message);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[UE_CITRUSHPlayerController] VehicleUI가 없어서 메시지를 표시할 수 없습니다: %s"), *Message);
	}
}