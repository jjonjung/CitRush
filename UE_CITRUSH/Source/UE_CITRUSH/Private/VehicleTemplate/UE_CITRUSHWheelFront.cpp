// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHWheelFront.h"
#include "UObject/ConstructorHelpers.h"

UUE_CITRUSHWheelFront::UUE_CITRUSHWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}