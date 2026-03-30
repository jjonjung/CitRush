// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UE_CITRUSHPlayerController.generated.h"

class UInputMappingContext;
class AUE_CITRUSHPawn;
//class UUE_CITRUSHUI;
class UVehicleDemoUITest;

/**
 *  Vehicle Player Controller class
 *  Handles input mapping and user interface
 */
UCLASS(abstract)
class UE_CITRUSH_API AUE_CITRUSHPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Input Mapping Context to be used for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext;

	/** Pointer to the controlled vehicle pawn */
	TObjectPtr<AUE_CITRUSHPawn> VehiclePawn;

	/** Type of the UI to spawn */
	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UUE_CITRUSHUI> VehicleUIClass;

	/** Pointer to the UI widget #1#
	TObjectPtr<UUE_CITRUSHUI> VehicleUI;*/

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UVehicleDemoUITest> VehicleUIClass;

	/** Pointer to the UI widget #1# **/
	TObjectPtr<UVehicleDemoUITest> VehicleUI;
	

	// Begin Actor interface
protected:

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

public:

	virtual void Tick(float Delta) override;

	/** VehicleUI 위젯 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI")
	UVehicleDemoUITest* GetVehicleUI() const { return VehicleUI; }

	/** 클라이언트에게 상태 메시지 표시 (Client RPC) */
	UFUNCTION(Client, Reliable, Category = "UI")
	void ClientShowStateMessage(const FString& Message);

	// End Actor interface

	// Begin PlayerController interface
protected:

	virtual void OnPossess(APawn* InPawn) override;

	// End PlayerController interface
};
