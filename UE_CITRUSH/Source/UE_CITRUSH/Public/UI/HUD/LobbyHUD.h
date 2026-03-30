// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LobbyHUD.generated.h"

class ACitRushPlayerState;
class ULoadingWidget;
class ULobbyWidget;
/**
 * 
 */
UCLASS()
class UE_CITRUSH_API ALobbyHUD : public AHUD
{
	GENERATED_BODY()

public:
	ALobbyHUD();

	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnAddedPlayer(ACitRushPlayerState* ps);
	UFUNCTION()
	void OnRemovedPlayer(ACitRushPlayerState* ps);
	UFUNCTION()
	void OnUpdatePlayerList(const TArray<ACitRushPlayerState*>& players);
	
	UFUNCTION()
	void OnTransition();
protected:

private:
	/** Lobby Widget */
	UPROPERTY()
	TSubclassOf<ULobbyWidget> lobbyWidgetClass;
	UPROPERTY()
	TObjectPtr<ULobbyWidget> lobbyWidget;

	/** Loading Widget */
	UPROPERTY()
	TSubclassOf<ULoadingWidget> loadingWidgetClass;
	UPROPERTY()
	TObjectPtr<ULoadingWidget> loadingWidget;
	
	bool bIsInit = false;
};
