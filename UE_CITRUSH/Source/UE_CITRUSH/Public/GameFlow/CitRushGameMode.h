// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CitRushGameMode.generated.h"

enum class EPlayerRole : uint8;
class ACitRushPlayerState;
class AAbstractCommander;
class AAbstractRacer;
class UMapDataAsset;

/**
 * InGame Map용 GameMode. PlayerRole에 따라 Commander/Racer Pawn 스폰 및 Ability 부여
 */
UCLASS()
class UE_CITRUSH_API ACitRushGameMode : public AGameModeBase
{
	GENERATED_BODY()
	friend class FGameFlowUtility;

public:
	/** 생성자 */
	ACitRushGameMode();

	virtual void BeginPlay() override;

	/** 플레이어 로그인 시 호출 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void HandleSeamlessTravelPlayer(AController*& C) override;
	virtual void PostSeamlessTravel() override;

	//virtual AActor* ChoosePlayerStart_Implementation(AController* player) override;

	/** 플레이어 로그아웃 시 호출 */
	virtual void Logout(AController* Exiting) override;

	// Seamless Travel 에선 최적화된 방법이지만, 안정성 부족
	//UClass* GetDefaultPawnClassForController_Implementation(APlayerController* NewPlayer);

	/** 모든 플레이어가 로딩 완료되었는지 체크 */
	void CheckAllPlayersReady();
	
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** 플레이어에게 GAS Ability 부여 */
	virtual void GrantAbilitiesToPlayer(APawn* PlayerPawn, ACitRushPlayerState* CitRushPlayerState);

private:


	/** 게임 시작 카운트다운 */
	void StartGameCountdown();

	void StartGame();
	
	EPlayerRole GetPlayerRoleHelper(const ACitRushPlayerState* cPS) const;
/*
	void RegisterVoiceForPlayer(APlayerController* NewPlayer);
	void UnregisterVoiceForPlayer(APlayerController* ExitingPlayerController);*/

protected:
	/** Commander Pawn 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Pawn")
	TSubclassOf<AAbstractCommander> CommanderPawnClass;

	/** Racer Pawn 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Pawn")
	TSubclassOf<AAbstractRacer> RacerPawnClass;

	UPROPERTY(BlueprintReadOnly, Category = "Game Progress")
	int32 playersNum = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Game Progress")
	int32 spawnedPlayersNum = 0;

private:
	int32 retryCount = 100;

	FTimerHandle loadingCheckTimer;
	FTimerHandle matchStartTimer;

	/** 맵 에셋 로더 (DeveloperSettings) */
	UPROPERTY()
	const UMapDataAsset* mapAsset;
	
};

