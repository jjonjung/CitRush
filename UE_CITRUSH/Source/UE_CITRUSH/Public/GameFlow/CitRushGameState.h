// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/CitRushPlayerTypes.h"
#include "GameFramework/GameStateBase.h"
#include "Data/PingTypes.h"
#include "CitRushGameState.generated.h"

class ACitRushPlayerState;
class UComponentCamera;
class AMapBoundsActor;
class APingObjectiveSphere;
class UPingMarkerManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRacerAliveChanged, TArray<bool>, ChangedArray);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameEnded);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPingUpdated, const FPingData&);

/**
 * InGame Map용 GameState
 * - 게임 진행 상태 관리
 * - Commander/Racer 추적
 * - 생존자 수 관리
 *
 * Match Start Timing Catch
 // UrClass::BeginPlay 에서 호출하면 보통 될거임
 if (ACitRushGameState* cGS = GetWorld()->GetGameState<ACitRushGameState>())
 {
	cGS->OnMatchStarted.AddDynamic(this, &UrClass::UrFunc);
 }

 // UrClass::UrFunc
 if (ACitRushGameState* cGS = GetWorld()->GetGameState<ACitRushGameState>())
 {
	TArray<ACitRushPlayerState*> cPSs = cGS->GetCitRushPlayers();
	// ... 구현 ...
 }
 */
UCLASS()
class UE_CITRUSH_API ACitRushGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/** 생성자 */
	ACitRushGameState();

protected:
	/** 게임 시작 시 호출. 게임 타이머 시작 및 Racer 생존자 초기화 */
	virtual void BeginPlay() override;

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	/** 리플리케이션 프로퍼티 등록 */
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

#pragma region PlayerTracking
public:
	/** 특정 역할의 플레이어 반환 */
	UFUNCTION(BlueprintCallable, Category = "PlayerTracking")
	TArray<ACitRushPlayerState*> GetPlayerStatesByRole(EPlayerRole TargetRole) const;

	UFUNCTION(BlueprintCallable, Category = "PlayerTracking")
	TArray<ACitRushPlayerState*> GetCitRushPlayers() const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerArrayChanged, TArray<ACitRushPlayerState*>, CitRushPlayerStates);
	FOnPlayerArrayChanged OnPlayerArrayChanged;

	void NotifyLoadWorld(bool bWorldIsLoaded) {bLoadWorld = bWorldIsLoaded;};

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastRPC_CollectStringData(FName key, const FString& value);
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastRPC_CollectFloatData(FName key, float value);

private:
	UFUNCTION()
	void OnRep_CitRushPlayerArray();
	
protected:
	UPROPERTY(ReplicatedUsing=OnRep_CitRushPlayerArray, BlueprintReadOnly, Category = "PlayerTracking")
	TArray<ACitRushPlayerState*> CitRushPSs;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "PlayerTracking")
	bool bLoadWorld = false;

#pragma endregion

#pragma region GameProgress
public:
	UFUNCTION()
	void OnMatchStart();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStarted);
	FOnGameStarted OnMatchStarted;
	
	UFUNCTION()
	void Lose();

	/** 첫 번째 플레이어 입력 알림 (서버에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Game Progress")
	void NotifyFirstPlayerInput();

	/** 첫 번째 플레이어 입력 델리게이트 (PixelEnemy 활성화 등) */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFirstPlayerInput);
	FOnFirstPlayerInput OnFirstPlayerInput;

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastRPC_QuitMatch(ACitRushPlayerState* ps);
	
	/** 게임 경과 시간 반환 */
	UFUNCTION(BlueprintPure, Category = "Game Progress")
	float GetElapsedTime() const;
	/** 게임 남은 시간 반환 */
	UFUNCTION(BlueprintPure, Category = "Game Progress")
	float GetRemainingTime() const;

	/** 플레이어 승리 처리 */
	UFUNCTION(BlueprintCallable, Category = "Game Progress")
	void Victory();

	/** 게임 종료 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Game Progress")
	FOnGameEnded OnGameEndedDelegate;

private:
	UFUNCTION()
	void AddCustomPlayerArray(ACitRushPlayerState* CitRushPlayerState);

	UFUNCTION()
	void OnRep_MatchStartTime();
	
	/** 게임 종료 RepNotify */
	UFUNCTION()
	void OnRep_MatchEnded();
	
	/** 시간 초과 시 호출. 플레이어 패배 처리 */
	UFUNCTION(BlueprintCallable, Category = "Game Progress")
	void TimeUp();

protected:
	/** 게임 진행 타이머 핸들 */
	FTimerHandle MatchProgressTimer;
	/** 게임 시작 시간 (서버 타임) */
	UPROPERTY(ReplicatedUsing=OnRep_MatchStartTime, BlueprintReadOnly, Category = "Game Progress")
	float matchStartTime = 0.f;

	/** 게임 제한 시간 (초) */
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "Game Progress")
	float matchDuration = 180.f;
	
	/** 게임 종료 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_MatchEnded, BlueprintReadOnly, Category = "Game Progress")
	float gameEndedTime = -1.f;
	/** 플레이어 승리 여부 (true: 승리, false: 패배) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game Progress")
	bool bPlayerVictory = false;

	/** 첫 번째 플레이어 입력이 들어왔는지 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "Game Progress")
	bool bFirstInputReceived = false;
#pragma endregion

#pragma region SurvivorTracking
public:
	/** Racer 사망 처리 */
	UFUNCTION(Category = "Survivor")
	void OnRacerDied(ACitRushPlayerState* DiedRacer);
	/** 생존 Racer 수 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Racer")
	FOnRacerAliveChanged OnRacerAliveChangedDelegate;	

protected:
	/** 생존 Racer 수 초기화 */
	void InitializeAliveRacer();

private:
	/** 생존 Racer 배열 변경 RepNotify */
	UFUNCTION()
	void OnRep_AliveRacerArrayChanged() const;

protected:
	/** 생존 중인 Racer 배열 (인덱스별 생존 여부) */
	UPROPERTY(ReplicatedUsing = OnRep_AliveRacerArrayChanged, BlueprintReadOnly, Category = "Survivor")
	TArray<bool> AliveRacerArray;
#pragma endregion

#pragma region PingSystem
public:
	/** 핑 업데이트 이벤트 (모든 클라이언트에서 구독 가능) */
	FOnPingUpdated OnPingUpdated;

	/** 현재 활성 핑 데이터 반환 */
	UFUNCTION(BlueprintPure, Category = "Ping")
	const FPingData& GetActivePing() const { return ActivePing; }

	/** 활성 핑이 있는지 확인 */
	UFUNCTION(BlueprintPure, Category = "Ping")
	bool HasActivePing() const { return bHasActivePing; }

	/** 핑 설정 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Ping")
	void SetActivePing(const FVector& WorldLocation, ECommanderPingType Type, float Duration, APlayerState* OwnerPS);

	/** 핑 제거 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Ping")
	void ClearActivePing();

	/** 위치 기반 핑 제거 (서버 전용, 레이서 충돌 시 사용) */
	UFUNCTION(BlueprintCallable, Category = "Ping")
	void RemovePingByLocation(const FVector& WorldLocation, float Tolerance = 100.0f);

	/** 활성 핑 목록 (타입별 최대 3개) 반환 */
	UFUNCTION(BlueprintPure, Category = "Ping")
	const TArray<FPingData>& GetActivePings() const { return ActivePings; }

	/** MapBounds 액터 반환 */
	UFUNCTION(BlueprintPure, Category = "Ping")
	AMapBoundsActor* GetMapBounds() const { return MapBoundsActor; }

	/** 기본 핑 지속 시간(초) 가져오기 */
	UFUNCTION(BlueprintPure, Category = "Ping")
	float GetDefaultPingDurationSeconds() const { return DefaultPingDurationSeconds; }

protected:
	/** MapBounds 액터 찾기 및 저장 (서버 전용) */
	void FindAndCacheMapBounds();

private:
	/** 활성 핑 변경 RepNotify */
	UFUNCTION()
	void OnRep_ActivePing();

	/** 활성 핑 존재 여부 변경 RepNotify */
	UFUNCTION()
	void OnRep_HasActivePing();

	/** 핑 만료 타이머 */
	void CheckPingExpiration();

protected:
	/** 현재 활성 핑 데이터 */
	UPROPERTY(ReplicatedUsing = OnRep_ActivePing, BlueprintReadOnly, Category = "Ping")
	FPingData ActivePing;

	/** 활성 핑 존재 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_HasActivePing, BlueprintReadOnly, Category = "Ping")
	bool bHasActivePing = false;

	/** 활성 핑 목록 (타입별 최대 3개씩) */
	UPROPERTY(ReplicatedUsing = OnRep_ActivePings, BlueprintReadOnly, Category = "Ping")
	TArray<FPingData> ActivePings;

	/** 활성 핑 목록 변경 RepNotify */
	UFUNCTION()
	void OnRep_ActivePings();

	/** PingMarkerManager 업데이트 (OnRep_ActivePings에서 호출) */
	void UpdatePingMarkerManager();

	/** MapBounds 액터 캐시 (서버 전용) */
	UPROPERTY()
	AMapBoundsActor* MapBoundsActor = nullptr;

	/** Objective Sphere 액터 (서버 전용) */
	UPROPERTY()
	APingObjectiveSphere* ObjectiveSphere = nullptr;

	/** 핑 만료 체크 타이머 핸들 */
	FTimerHandle PingExpirationTimer;

	/** 핑 마커를 관리할 매니저 클래스 (Blueprint에서 BP_PingMarkerManager 로 지정 가능) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ping")
	TSubclassOf<UPingMarkerManager> PingMarkerManagerClass;

	/** 기본 핑 지속 시간(초) - BP/GameState 에서 설정 가능 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ping")
	float DefaultPingDurationSeconds = 15.f;
#pragma endregion
};
