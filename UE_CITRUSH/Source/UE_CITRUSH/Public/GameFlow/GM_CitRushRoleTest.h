// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GM_CitRushRoleTest.generated.h"

class ACitRushPlayerState;
class AAbstractCommander;
class AAbstractRacer;

/**
 * 역할 할당 테스트용 GameMode
 * PostLogin에서 서버가 역할을 결정하고 Pawn 스폰
 */
UCLASS()
class UE_CITRUSH_API AGM_CitRushRoleTest : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGM_CitRushRoleTest();

	/** 플레이어 로그인 시 호출 - 역할 할당 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 플레이어 시작 시 호출 - Pawn 스폰 */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

protected:
	/** GameState의 PlayerArray에서 CitRushPlayerState 목록 가져오기 */
	TArray<ACitRushPlayerState*> GetCitRushPlayers() const;

	/** Commander Pawn 클래스 (블루프린트에서 BP_Commander로 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "Pawn")
	TSubclassOf<AAbstractCommander> CommanderPawnClass;

	/** Racer Pawn 클래스 (블루프린트에서 BP_EJCar로 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "Pawn")
	TSubclassOf<AAbstractRacer> RacerPawnClass;
};

