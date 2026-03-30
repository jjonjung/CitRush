// Copyright Epic Games, Inc. All Rights Reserved.


#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHSportsCar.h"
#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHSportsWheelFront.h"
#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHSportsWheelRear.h"
#include "ChaosWheeledVehicleMovementComponent.h"

AUE_CITRUSHSportsCar::AUE_CITRUSHSportsCar()
{
	// Note: for faster iteration times, the vehicle setup can be tweaked in the Blueprint instead

	// Set up the chassis
	GetChaosWheeledVehicleMovement()->ChassisHeight = 144.0f;
	GetChaosWheeledVehicleMovement()->DragCoefficient = 0.31f;

	// Set up the wheels
	GetChaosWheeledVehicleMovement()->bLegacyWheelFrictionPosition = true;
	GetChaosWheeledVehicleMovement()->WheelSetups.SetNum(4);

	GetChaosWheeledVehicleMovement()->WheelSetups[0].WheelClass = UUE_CITRUSHSportsWheelFront::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[0].BoneName = FName("Phys_Wheel_FL");
	GetChaosWheeledVehicleMovement()->WheelSetups[0].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosWheeledVehicleMovement()->WheelSetups[1].WheelClass = UUE_CITRUSHSportsWheelFront::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[1].BoneName = FName("Phys_Wheel_FR");
	GetChaosWheeledVehicleMovement()->WheelSetups[1].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosWheeledVehicleMovement()->WheelSetups[2].WheelClass = UUE_CITRUSHSportsWheelRear::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[2].BoneName = FName("Phys_Wheel_BL");
	GetChaosWheeledVehicleMovement()->WheelSetups[2].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosWheeledVehicleMovement()->WheelSetups[3].WheelClass = UUE_CITRUSHSportsWheelRear::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[3].BoneName = FName("Phys_Wheel_BR");
	GetChaosWheeledVehicleMovement()->WheelSetups[3].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	// Set up the engine
	// NOTE: Check the Blueprint asset for the Torque Curve
	GetChaosWheeledVehicleMovement()->EngineSetup.MaxTorque = 750.0f;
	GetChaosWheeledVehicleMovement()->EngineSetup.MaxRPM = 7000.0f;
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineIdleRPM = 900.0f;
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineBrakeEffect = 0.2f;
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineRevUpMOI = 5.0f;
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineRevDownRate = 600.0f;

	// Set up the transmission
	GetChaosWheeledVehicleMovement()->TransmissionSetup.bUseAutomaticGears = true;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.bUseAutoReverse = true;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.FinalRatio = 2.81f;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ChangeUpRPM = 6000.0f;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ChangeDownRPM = 2000.0f;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.GearChangeTime = 0.2f;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.TransmissionEfficiency = 0.9f;

	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios.SetNum(5);
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[0] = 4.25f;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[1] = 2.52f;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[2] = 1.66f;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[3] = 1.22f;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[4] = 1.0f;

	GetChaosWheeledVehicleMovement()->TransmissionSetup.ReverseGearRatios.SetNum(1);
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ReverseGearRatios[0] = 4.04f;

	// Set up the steering
	// NOTE: Check the Blueprint asset for the Steering Curve
	GetChaosWheeledVehicleMovement()->SteeringSetup.SteeringType = ESteeringType::Ackermann;
	GetChaosWheeledVehicleMovement()->SteeringSetup.AngleRatio = 0.7f;
}
