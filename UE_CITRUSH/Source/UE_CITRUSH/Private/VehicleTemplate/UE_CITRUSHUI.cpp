// Copyright Epic Games, Inc. All Rights Reserved.


#include "UE_CITRUSH/Public/VehicleTemplate/UE_CITRUSHUI.h"

void UUE_CITRUSHUI::UpdateSpeed(float NewSpeed)
{
	// format the speed to KPH or MPH
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	// call the Blueprint handler
	OnSpeedUpdate(FormattedSpeed);
}

void UUE_CITRUSHUI::UpdateGear(int32 NewGear)
{
	// call the Blueprint handler
	OnGearUpdate(NewGear);
}