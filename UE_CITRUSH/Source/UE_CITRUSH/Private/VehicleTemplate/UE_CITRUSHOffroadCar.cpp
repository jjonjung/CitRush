// Copyright Epic Games, Inc. All Rights Reserved.


#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHOffroadCar.h"
#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHOffroadWheelFront.h"
#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHOffroadWheelRear.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AUE_CITRUSHOffroadCar::AUE_CITRUSHOffroadCar()
{
	// construct the mesh components
	Chassis = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chassis"));
	Chassis->SetupAttachment(GetMesh());

	// NOTE: tire sockets are set from the Blueprint class
	TireFrontLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Front Left"));
	TireFrontLeft->SetupAttachment(GetMesh(), FName("VisWheel_FL"));
	TireFrontLeft->SetCollisionProfileName(FName("NoCollision"));

	TireFrontRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Front Right"));
	TireFrontRight->SetupAttachment(GetMesh(), FName("VisWheel_FR"));
	TireFrontRight->SetCollisionProfileName(FName("NoCollision"));
	TireFrontRight->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	TireRearLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Rear Left"));
	TireRearLeft->SetupAttachment(GetMesh(), FName("VisWheel_BL"));
	TireRearLeft->SetCollisionProfileName(FName("NoCollision"));

	TireRearRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tire Rear Right"));
	TireRearRight->SetupAttachment(GetMesh(), FName("VisWheel_BR"));
	TireRearRight->SetCollisionProfileName(FName("NoCollision"));
	TireRearRight->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	// adjust the cameras
	GetFrontSpringArm()->SetRelativeLocation(FVector(-5.0f, -30.0f, 135.0f));
	GetBackSpringArm()->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));

	// Note: for faster iteration times, the vehicle setup can be tweaked in the Blueprint instead

	// Set up the chassis
	GetChaosWheeledVehicleMovement()->ChassisHeight = 160.0f;
	GetChaosWheeledVehicleMovement()->DragCoefficient = 0.1f;
	GetChaosWheeledVehicleMovement()->DownforceCoefficient = 0.1f;
	GetChaosWheeledVehicleMovement()->CenterOfMassOverride = FVector(0.0f, 0.0f, 75.0f);
	GetChaosWheeledVehicleMovement()->bEnableCenterOfMassOverride = true;

	// Set up the wheels
	GetChaosWheeledVehicleMovement()->bLegacyWheelFrictionPosition = true;
	GetChaosWheeledVehicleMovement()->WheelSetups.SetNum(4);

	GetChaosWheeledVehicleMovement()->WheelSetups[0].WheelClass = UUE_CITRUSHOffroadWheelFront::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[0].BoneName = FName("PhysWheel_FL");
	GetChaosWheeledVehicleMovement()->WheelSetups[0].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosWheeledVehicleMovement()->WheelSetups[1].WheelClass = UUE_CITRUSHOffroadWheelFront::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[1].BoneName = FName("PhysWheel_FR");
	GetChaosWheeledVehicleMovement()->WheelSetups[1].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosWheeledVehicleMovement()->WheelSetups[2].WheelClass = UUE_CITRUSHOffroadWheelRear::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[2].BoneName = FName("PhysWheel_BL");
	GetChaosWheeledVehicleMovement()->WheelSetups[2].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosWheeledVehicleMovement()->WheelSetups[3].WheelClass = UUE_CITRUSHOffroadWheelRear::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[3].BoneName = FName("PhysWheel_BR");
	GetChaosWheeledVehicleMovement()->WheelSetups[3].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	// Set up the engine
	// NOTE: Check the Blueprint asset for the Torque Curve
	GetChaosWheeledVehicleMovement()->EngineSetup.MaxTorque = 600.0f;
	GetChaosWheeledVehicleMovement()->EngineSetup.MaxRPM = 5000.0f;
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineIdleRPM = 1200.0f;
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineBrakeEffect = 0.05f;
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineRevUpMOI = 5.0f;
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineRevDownRate = 600.0f;

	// Set up the differential
	GetChaosWheeledVehicleMovement()->DifferentialSetup.DifferentialType = EVehicleDifferential::AllWheelDrive;
	GetChaosWheeledVehicleMovement()->DifferentialSetup.FrontRearSplit = 0.5f;

	// Set up the steering
	// NOTE: Check the Blueprint asset for the Steering Curve
	GetChaosWheeledVehicleMovement()->SteeringSetup.SteeringType = ESteeringType::AngleRatio;
	GetChaosWheeledVehicleMovement()->SteeringSetup.AngleRatio = 0.7f;
}