// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Data/CitRushPlayerTypes.h"
#include "CitRushPlayerState.generated.h"


class UASCommander;
class UASRacer;
class UCitRushDataManagerComponent;

class UPlayerInfoWidget;
class UAttributeSet;
class UGameplayAbility;
/**
 * CitRush PlayerState. PlayerRole, Ready 상태, Ability 선택 관리 + GAS 소유
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API ACitRushPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	friend class FGameFlowUtility;

public:
	/** 생성자 */
	ACitRushPlayerState();

	UFUNCTION(Client, Reliable)
	void ClientRPC_ApplyPlayerNickName();
	virtual void OnRep_PlayerName() override;

	APlayerState* operator()(ACitRushPlayerState* cPS) const {return Cast<APlayerState>(cPS);}

protected:
	/** 게임 시작 시 호출 */
	virtual void BeginPlay() override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	/** 리플리케이션 프로퍼티 등록 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
#pragma region PlayerRole
public:
	/** 메인메뉴 나가기 (세션 퇴장) */
	UFUNCTION(Client, Reliable, Category = "Player")
	void ClientRPC_ExitToMainMenu();

	/** 역할 변경 ServerRPC */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Player")
	void ServerRPC_RoleChange(EPlayerRole newRole);

	/** Ready 상태 설정 ServerRPC */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Player")
	void ServerRPC_SetReady(bool bReady);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Player")
	void ServerRPC_SetRacerIndex(int32 newIndex);
	
	/** 사망 상태 설정 ServerRPC */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Player")
	void ServerRPC_SetDied(bool bDied);
	/** Ready 상태 변경 델리게이트 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerReadyChanged, bool);
	FOnPlayerReadyChanged OnPlayerReadyChangedDelegate;
	
	/** 역할 변경 델리게이트 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerRoleChanged, const EPlayerRole&);
	FOnPlayerRoleChanged OnPlayerRoleChangedDelegate;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTargetIndexChanged, int32);
	FOnTargetIndexChanged OnTargetIndexChanged;

	/** 플레이어 사망 델리게이트 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerDied, const ACitRushPlayerState*);
	FOnPlayerDied OnPlayerDied;

	/** 플레이어 역할 반환 */
	UFUNCTION(BlueprintPure, Category="Player")
	FORCEINLINE EPlayerRole GetPlayerRole() const;

	/** Ready 상태 여부 */
	UFUNCTION(BlueprintPure, Category="Player")
	FORCEINLINE bool IsReady() const {return bIsReady;}

	/** Ready 가능 여부 (역할 선택 필요) */
	UFUNCTION(BlueprintPure, Category="Player")
	FORCEINLINE bool CanReady();

	/** 사망 여부 */
	UFUNCTION(BlueprintPure, Category="Player")
	FORCEINLINE bool IsDied() const {return bIsDied;}

	/** 플레이어 정보 구조체 반환 */
	UFUNCTION(BlueprintPure, Category="Player")
	FORCEINLINE FPlayerInfo GetPlayerInfo() const;

	/** 선택된 Ability 이름 배열 반환 */
	UFUNCTION(BlueprintPure, Category="Player")
	FORCEINLINE TArray<FName> GetSelectedAbilities() {return selectedAbilities;}

protected:
	/** 역할 변경 RepNotify */
	UFUNCTION()
	void OnRep_PlayerRole();
	
	UFUNCTION()
	void OnRep_TargetIndex();
	
	/** Ready 상태 변경 RepNotify */
	UFUNCTION()
	void OnRep_IsReady();

	/** 사망 상태 변경 RepNotify */
	UFUNCTION()
	void OnRep_IsDied();

protected:
	/** 플레이어 역할 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, ReplicatedUsing=OnRep_PlayerRole, Category="Player")
	EPlayerRole playerRole = EPlayerRole::None;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_TargetIndex, Category="Player")
	ETargetRacer targetIndex = ETargetRacer::None;

	/** Ready 상태 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, ReplicatedUsing=OnRep_IsReady, Category="Player")
	bool bIsReady = false;

	/** 사망 상태 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, ReplicatedUsing=OnRep_IsDied, Category="Player")
	bool bIsDied = false;

#pragma endregion

#pragma region LoadingState
public:
	/** 로딩 상태 반환 */
	UFUNCTION(BlueprintPure, Category="Player")
	FORCEINLINE ELoadingState GetLoadingState() const { return loadingState; }

	/** 로딩 진행도 반환 */
	UFUNCTION(BlueprintPure, Category="Player")
	FORCEINLINE float GetLoadingProgress() const { return loadingProgress; }

	UFUNCTION(Server, Reliable)
	void ServerRPC_SetLoadingState(ELoadingState NewState);

	UFUNCTION(Server, Reliable)
	void ServerRPC_SetTravelStartTime(int64 InStartTime);

	UFUNCTION(Server, Reliable)
	void ServerRPC_SetTravelEndTime(int64 InEndTime);

	int64 GetEndTravelTime() const {return travelEndTime;}

	void UpdateLoadingProgress(float Progress);

	/** 로딩 상태 변경 델리게이트 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadingStateChanged, ELoadingState);
	FOnLoadingStateChanged OnLoadingStateChanged;
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTravelEnd, ACitRushPlayerState*);
	FOnTravelEnd OnTravelEnd;

protected:
	
	/** 로딩 상태 변경 RepNotify */
	UFUNCTION()
	void OnRep_LoadingState();
	UFUNCTION()
	void OnRep_TravelEndTime();

protected:
	/** 레벨 전환 로딩 상태 */
	UPROPERTY(Replicated, ReplicatedUsing=OnRep_LoadingState, BlueprintReadOnly, Category="Player")
	ELoadingState loadingState = ELoadingState::NotStarted;

	/** 레벨 전환 시작 시간 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Player")
	int64 travelStartTime = 0;

	/** 레벨 전환 완료 시간 */
	UPROPERTY(ReplicatedUsing=OnRep_TravelEndTime, BlueprintReadOnly, Category="Player")
	int64 travelEndTime = 0;

	/** 로딩 진행도 (0.0 ~ 1.0) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Player")
	float loadingProgress = 0.0f;

#pragma endregion

#pragma region AbilitySystemInterface

public:
	/** AbilitySystemComponent 반환 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** AttributeSet 반환 */
	virtual UAttributeSet* GetAttributeSet(const EPlayerRole& role) const;

	/** Ability 선택 ServerRPC (Tag 이름으로) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Ability")
	void ServerRPC_SelectAbilityByName(FName abilityName, int32 slotIndex);

protected:
	/** AbilitySystemComponent (PlayerState 소유) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> abilitySystemComponent;

	/** AttributeSet (PlayerState 소유) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UASRacer> racerAttributeSet;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UASCommander> commanderAttributeSet;

	/** 선택된 Ability 이름 배열 (Rune 시스템 예정) */
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly)
	TArray<FName> selectedAbilities;
#pragma endregion

};
