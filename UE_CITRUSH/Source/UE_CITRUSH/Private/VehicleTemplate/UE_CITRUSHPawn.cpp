// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHPawn.h"
#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHWheelFront.h"
#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Data/InputMappingsSettings.h"
#include "Player/CCTV/CCTVCameraComponent.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

AUE_CITRUSHPawn::AUE_CITRUSHPawn()
{
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);
	BackCamera->bAutoActivate = true; // 시작 카메라로 설정

	// construct the right camera boom (우측 사이드뷰)
	RightSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Right Spring Arm"));
	RightSpringArm->SetupAttachment(GetMesh());
	RightSpringArm->TargetArmLength = 0.0f;
	RightSpringArm->bDoCollisionTest = false;
	RightSpringArm->bInheritPitch = false;
	RightSpringArm->bInheritRoll = false;
	RightSpringArm->bInheritYaw = true;
	RightSpringArm->SetRelativeLocation(FVector(-10.f, 100.f, 100.f));
	RightSpringArm->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	RightCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Right Camera"));
	RightCamera->SetupAttachment(RightSpringArm);
	RightCamera->bAutoActivate = false;

	// construct the left camera boom (좌측 사이드뷰)
	LeftSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Left Spring Arm"));
	LeftSpringArm->SetupAttachment(GetMesh());
	LeftSpringArm->TargetArmLength = 0.0f;
	LeftSpringArm->bDoCollisionTest = false;
	LeftSpringArm->bInheritPitch = false;
	LeftSpringArm->bInheritRoll = false;
	LeftSpringArm->bInheritYaw = true;
	LeftSpringArm->SetRelativeLocation(FVector(-10.f, -100.f, 100.f));
	LeftSpringArm->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	LeftCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Left Camera"));
	LeftCamera->SetupAttachment(LeftSpringArm);
	LeftCamera->bAutoActivate = false;

	// construct the back side camera boom (후방 사이드뷰)
	BackSideSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("BackSide Spring Arm"));
	BackSideSpringArm->SetupAttachment(GetMesh());
	BackSideSpringArm->TargetArmLength = 0.0f;
	BackSideSpringArm->bDoCollisionTest = false;
	BackSideSpringArm->bInheritPitch = false;
	BackSideSpringArm->bInheritRoll = false;
	BackSideSpringArm->bInheritYaw = true;
	BackSideSpringArm->SetRelativeLocation(FVector(-100.0f, 0.0f, 150.0f));
	BackSideSpringArm->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	BackSideCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BackSide Camera"));
	BackSideCamera->SetupAttachment(BackSideSpringArm);
	BackSideCamera->bAutoActivate = false;

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	ChaosWheeledVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

	const UInputMappingsSettings* inputLoader = UInputMappingsSettings::Get();
	inputData = inputLoader->inputMappings.Find(TEXT("IMC_Vehicle"));

}

void AUE_CITRUSHPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (APlayerController* pc = this->GetController<APlayerController>())
	{
		if (UEnhancedInputLocalPlayerSubsystem* inputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer()))
		{
			inputSubsystem->AddMappingContext(inputData->inputMappingContext, 0);
		}
	}
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering
		if (SteeringAction)
		{
			EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &AUE_CITRUSHPawn::Steering);
			EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &AUE_CITRUSHPawn::Steering);
			UE_LOG(LogTemplateVehicle, Warning, TEXT("SteeringAction bound: %s"), *SteeringAction->GetName());
		}
		else
		{
			UE_LOG(LogTemplateVehicle, Error, TEXT("SteeringAction is NULL!"));
		}

		// throttle
		if (ThrottleAction)
		{
			EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AUE_CITRUSHPawn::Throttle);
			EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AUE_CITRUSHPawn::Throttle);
			UE_LOG(LogTemplateVehicle, Warning, TEXT("ThrottleAction bound: %s"), *ThrottleAction->GetName());
		}
		else
		{
			UE_LOG(LogTemplateVehicle, Error, TEXT("ThrottleAction is NULL!"));
		}

		// break
		if (BrakeAction)
		{
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &AUE_CITRUSHPawn::Brake);
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &AUE_CITRUSHPawn::StartBrake);
			EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AUE_CITRUSHPawn::StopBrake);
			UE_LOG(LogTemplateVehicle, Warning, TEXT("BrakeAction bound: %s"), *BrakeAction->GetName());
		}
		else
		{
			UE_LOG(LogTemplateVehicle, Error, TEXT("BrakeAction is NULL!"));
		}

		// handbrake
		if (HandbrakeAction)
		{
			EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &AUE_CITRUSHPawn::StartHandbrake);
			EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AUE_CITRUSHPawn::StopHandbrake);
		}

		// look around
		if (LookAroundAction)
		{
			EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &AUE_CITRUSHPawn::LookAround);
		}

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &AUE_CITRUSHPawn::ToggleCam);
		// toggle camera
		if (ToggleCameraAction)
		{
			EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &AUE_CITRUSHPawn::ToggleCam);
		}

		// reset the vehicle
		if (ResetVehicleAction)
		{
			EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &AUE_CITRUSHPawn::ResetVehicle);
		}
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AUE_CITRUSHPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = GetVehicleMovementComponent()->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));

	if (resetVehicleCooldown > 0.f)
	{
		resetVehicleCooldown -= Delta;
	}

	if (untouchableTime > 0.f)
	{
		untouchableTime -= Delta;
	}
}

void AUE_CITRUSHPawn::Steering(const FInputActionValue& Value)
{
	// get the input magnitude for steering
	float SteeringValue = Value.Get<float>();

	// add the input
	GetVehicleMovementComponent()->SetSteeringInput(SteeringValue);
}

void AUE_CITRUSHPawn::Throttle(const FInputActionValue& Value)
{
	// get the input magnitude for the throttle
	float ThrottleValue = Value.Get<float>();

	// add the input
	GetVehicleMovementComponent()->SetThrottleInput(ThrottleValue);
}

void AUE_CITRUSHPawn::Brake(const FInputActionValue& Value)
{
	// get the input magnitude for the brakes
	float BreakValue = Value.Get<float>();

	// add the input
	GetVehicleMovementComponent()->SetBrakeInput(BreakValue);
}

void AUE_CITRUSHPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void AUE_CITRUSHPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	GetVehicleMovementComponent()->SetBrakeInput(0.0f);
}

void AUE_CITRUSHPawn::StartHandbrake(const FInputActionValue& Value)
{
	// add the input
	GetVehicleMovementComponent()->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void AUE_CITRUSHPawn::StopHandbrake(const FInputActionValue& Value)
{
	// add the input
	GetVehicleMovementComponent()->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void AUE_CITRUSHPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input 
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

/*
void AUE_CITRUSHPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}
*/

void AUE_CITRUSHPawn::ToggleCam(const FInputActionValue& Value)
{
	// Deactivate current camera
	switch (CurrentCameraIndex)
	{
	case 0:
		BackCamera->SetActive(false);
		break;
	case 1:
		FrontCamera->SetActive(false);
		break;
	case 2:
		LeftCamera->SetActive(false);
		break;
	case 3:
		RightCamera->SetActive(false);
		break;
	case 4:
		BackSideCamera->SetActive(false);
		break;
	}

	CurrentCameraIndex = (CurrentCameraIndex + 1) % 5;

	switch (CurrentCameraIndex)
	{
	case 0:
		BackCamera->SetActive(true);
		bFrontCameraActive = false;
		break;
	case 1:
		FrontCamera->SetActive(true);
		bFrontCameraActive = true;
		break;
	case 2:
		LeftCamera->SetActive(true);
		bFrontCameraActive = false;
		break;
	case 3:
		RightCamera->SetActive(true);
		bFrontCameraActive = false;
		break;
	case 4:
		BackSideCamera->SetActive(true);
		bFrontCameraActive = false;
		break;
	}
}

void AUE_CITRUSHPawn::ServerRPC_ResetVehicle_Implementation()
{
	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle _CurLocation : %s / _CurRotation : %s"), *GetActorLocation().ToString(), *GetActorRotation().ToString());
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, GetMesh()->GetLocalBounds().BoxExtent.Z + 30.f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;
	
	// teleport the actor to the reset spot and reset physics
	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
	GetMesh()->SetWorldLocationAndRotation(ResetLocation, ResetRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AUE_CITRUSHPawn::ResetVehicle(const FInputActionValue& Value)
{
	if (resetVehicleCooldown > 0.f) {return;}
	resetVehicleCooldown = 5.f;
	ServerRPC_ResetVehicle();
	
	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle _CurLocation : %s / _CurRotation : %s"), *GetActorLocation().ToString(), *GetActorRotation().ToString());
}

UCameraComponent* AUE_CITRUSHPawn::GetCCTVCamera_Implementation(int32 SlotIndex)
{
	return CCTVCameraComponent ? CCTVCameraComponent->GetCCTVCamera(SlotIndex) : nullptr;
}

bool AUE_CITRUSHPawn::IsCCTVCameraInUse_Implementation(int32 SlotIndex)
{
	UCameraComponent* CCTVCamera = GetCCTVCamera(SlotIndex);
	if (!CCTVCamera || !CCTVCamera->IsActive())
	{
		return false;
	}

	// CurrentCameraIndex: 0=Back, 1=Front, 2=Left, 3=Right, 4=BackSide
	if (SlotIndex == 0)
	{
		return CurrentCameraIndex == 0;
	}
	if (SlotIndex == 1)
	{
		return CurrentCameraIndex == 1;
	}
	if (SlotIndex == 2)
	{
		return CurrentCameraIndex >= 2 && CurrentCameraIndex <= 4;
	}

	return false;
}


#undef LOCTEXT_NAMESPACE