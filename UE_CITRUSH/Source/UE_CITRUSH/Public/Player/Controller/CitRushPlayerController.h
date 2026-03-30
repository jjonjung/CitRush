// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/CitRushPlayerTypes.h"
#include "GameFramework/PlayerController.h"
#include "Utility/DebugHelper.h"
#include "CitRushPlayerController.generated.h"

class UMapDataAsset;
class ULoadingWidget;
class UInGameMenuWidget;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class UE_CITRUSH_API ACitRushPlayerController : public APlayerController
{
	GENERATED_BODY()
	DECLARE_CLASS_LOG(CitRushPlayerController);

public:
	ACitRushPlayerController();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostSeamlessTravel() override;

	virtual void NotifyLoadedWorld(FName WorldPackageName, bool bFinalDest) override;

	virtual void OnRep_PlayerState() override;

	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	virtual void ReceivedPlayer() override;

	UFUNCTION(Client, Reliable)
	void ClientRPC_StartCheckWorldPartition(FVector inPlayerStartLocation);
	
	/** 클라이언트에게 상태 메시지 표시 (Client RPC) */
	UFUNCTION(Client, Reliable, Category = "UI")
	void ClientShowStateMessage(const FString& Message);
	
protected:
	void InitSettingsByGameMode();

	UFUNCTION()
	void CheckWorldPartition();

	UFUNCTION()
	void OnGameEnded();

	void LoadInGameSettings();
	void UnloadInGameSettings();

private:
	UFUNCTION()
	void OnInputInGameMenuKey(const FInputActionValue& inputActionValue);
	UFUNCTION()
	void OnLoadingStateChanged(ELoadingState newState);

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ULoadingWidget> loadingWidgetClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<ULoadingWidget> loadingWidget;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UInGameMenuWidget> menuWidgetClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UInGameMenuWidget> menuWidget;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Context")
	TObjectPtr<UInputMappingContext> IMC_InGameUI;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Action")
	TObjectPtr<UInputAction> IA_InGameMenu;

	FTimerHandle endMatchTimer;

private:
	bool bFinalDestination = false;
	bool bInGameDestination = false;

	FTimerHandle checkWorldPartitionTimer;
	int32 retryCount = 100;

	UPROPERTY()
	FVector playerStartLocation;

	UPROPERTY()
	const UMapDataAsset* mapAsset;

	uint32 bindingHandle = MAX_uint32;
};
