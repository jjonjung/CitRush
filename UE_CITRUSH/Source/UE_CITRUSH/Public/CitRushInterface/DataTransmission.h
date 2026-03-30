// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DataTransmission.generated.h"

struct FReceiveData {};
struct FSendData {};

// This class does not need to be modified.
UINTERFACE()
class UDataTransmission : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UE_CITRUSH_API IDataTransmission
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void SendData(bool bFinished, const FSendData& data) = 0;
	virtual void ReceiveData(bool bFinished, const FReceiveData& data) = 0;
};
