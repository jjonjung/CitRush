// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/CitRushPlayerTypes.h"
#include "Data/MapDataAsset.h"
#include "GameFramework/GameStateBase.h"
#include "Utility/DebugHelper.h"
#include "LobbyGameState.generated.h"

class ACitRushPlayerState;

/**
 * 로비용 GameState. 플레이어 정보 관리 및 게임 시작 조건 체크 (Commander/Racer 수)
 */
UCLASS()
class UE_CITRUSH_API ALobbyGameState : public AGameStateBase
{
	GENERATED_BODY()
	DECLARE_CLASS_LOG(LobbyGameState);

public:
	/** 생성자 */
	ALobbyGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 플레이어 추가 시 호출. LobbyWidget에 플레이어 정보 추가 */
	virtual void AddPlayerState(APlayerState* PlayerState) override;

	/** 플레이어 제거 시 호출. LobbyWidget에서 플레이어 정보 제거 */
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UFUNCTION()
	void StartGame();

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastRPC_QuitLobby(ACitRushPlayerState* requestor);

#pragma region PlayersInfo
public:
	UFUNCTION(BlueprintCallable, Category="PlayersInfo")
	TArray<ACitRushPlayerState*> GetCitRushPlayers() const;
	
	/** 전체 플레이어 정보 배열 반환 */
	UFUNCTION(BlueprintCallable, Category="PlayersInfo")
	TArray<FPlayerInfo> GetAllPlayerInfos() const;

	/** 게임 시작 가능 여부 확인 (Commander/Racer 수 체크) */
	UFUNCTION()
	bool CanStartGame();

	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnUpdatedPlayerArray, const TArray<ACitRushPlayerState*>&);
	FOnUpdatedPlayerArray OnUpdatedPlayerArray;
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAddedPlayer, ACitRushPlayerState*);
	FOnAddedPlayer OnAddedPlayer;
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRemovedPlayer, ACitRushPlayerState*);
	FOnAddedPlayer OnRemovedPlayer;
	DECLARE_MULTICAST_DELEGATE(FOnGameStated);
	FOnGameStated OnGameStarted;


private:
	UFUNCTION()
	void AddCustomPlayerArray(ACitRushPlayerState* cPS);
	
	UFUNCTION()
	void OnRep_CitRushPlayerArray();

protected:
	UPROPERTY(ReplicatedUsing=OnRep_CitRushPlayerArray, BlueprintReadOnly, Category="PlayersInfo")
	TArray<ACitRushPlayerState*> CitRushPSs; 
	
	/** 필요한 Commander 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PlayersInfo")
	int32 requiredCommanderCount;

	/** 필요한 Racer 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PlayersInfo")
	int32 requiredRacerCount;
#pragma endregion

/*private:
	FMapInfo rankMapInfo;*/
};
