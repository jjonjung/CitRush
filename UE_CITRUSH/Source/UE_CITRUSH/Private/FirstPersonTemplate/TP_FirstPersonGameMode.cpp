// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonTemplate/TP_FirstPersonGameMode.h"
#include "FirstPersonTemplate/TP_FirstPersonCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATP_FirstPersonGameMode::ATP_FirstPersonGameMode()
	: Super()
{
	DefaultPawnClass = ATP_FirstPersonCharacter::StaticClass();

}
