// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "CitRushInterface/CitRushPlayerInterface.h"
#include "AbstractRacer.generated.h"

struct FOnAttributeChangeData;
class UInputAction;
class UItemData;
class UVoiceAcceptorComponent;
class USteamListenComponent;
class IOnlineVoice;
class UAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UASRacer;
class UInputMappingContext;
class UCameraComponent;
class UCCTVCameraComponent;

/**
 * Racer 추상 클래스. Vehicle Pawn + GAS + Voice 시스템 통합
 */
UCLASS(Abstract)
class UE_CITRUSH_API AAbstractRacer : public AWheeledVehiclePawn, public IAbilitySystemInterface, public ICitRushPlayerInterface
{
	GENERATED_BODY()

protected:
	DECLARE_LOG_CATEGORY_CLASS(RacerLog, Warning, All);

public:
	/** 생성자 */
	AAbstractRacer();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/** 서버에서 Controller에 빙의될 때 호출. GAS 초기화 (Server) */
	virtual void PossessedBy(AController* NewController) override;

	/** 클라이언트에서 PlayerState 리플리케이션 시 호출. GAS 초기화 (Client) */
	virtual void OnRep_PlayerState() override;

	/** 입력 바인딩 설정 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** InputMode 초기화 (GameOnly) */
	virtual void InitInputMode(APlayerController* playerController) override;

	/** 게임 종료 시 호출. Voice 정리 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma region voice
public:
	/** Voice 인터페이스 초기화 (Steam P2P 수신) */
	virtual void InitVoiceInterface() override;

protected:
	/** Online Voice 인터페이스 (Steam) */
	TSharedPtr<IOnlineVoice, ESPMode::ThreadSafe> voiceInterface;

	/** Steam P2P 음성 수신 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Listen")
	TObjectPtr<USteamListenComponent> steamListenComponent;

	/** Raw 음성 수신 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Listen")
	TObjectPtr<UVoiceAcceptorComponent> rawListenComponent;

	bool bRegisterVoiceComponents = false;
#pragma endregion
	
#pragma region AbilitySystem
public:
	/** AbilitySystemComponent 반환 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** AttributeSet 반환 */
	virtual UAttributeSet* GetAttributeSet() const override;

	/** 기본 Attribute 값 적용 (GameplayEffect) */
	virtual void InitDefaultAttributes() const override;

	virtual void AcquireItem(UItemData* NewItem);

	/** 아이템 소비 (슬롯 비우기) */
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void ConsumeItem(UItemData* ItemToConsume);

	/** 클라이언트에 아이템 부여 알림 (Client RPC) */
	UFUNCTION(Client, Reliable)
	void ClientNotifyItemAcquired(const FString& ItemName);

	/** 전방 아이템 슬롯 가져오기 */
	UFUNCTION(BlueprintPure, Category = "Item")
	UItemData* GetFrontItemSlot() const { return frontItemSlot; }

	/** 후방 아이템 슬롯 가져오기 */
	UFUNCTION(BlueprintPure, Category = "Item")
	UItemData* GetBackItemSlot() const { return backItemSlot; }

	/** 아이템 슬롯 변경 델리게이트 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemSlotChanged, UItemData*, FrontItem, UItemData*, BackItem);
	
	/** 아이템 슬롯 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Item")
	FOnItemSlotChanged OnItemSlotChanged;

	/** Ability 부여 (클래스 기반) */
	virtual bool ReceiveAbility(const TSubclassOf<UGameplayAbility>& ability) override;

	/** Ability 부여 (Tag 이름 기반) */
	virtual bool ReceiveAbility(const FName& gameplayTagName) override;

	/** Ability 활성화 (클래스 기반) */
	virtual bool ActivateAbility(const TSubclassOf<UGameplayAbility>& ability) override;

	/** Ability 활성화 (Tag 이름 기반) */
	virtual bool ActivateAbility(const FName& gameplayTagName) override;

	UFUNCTION(NetMulticast, Reliable, Category = "Collision")
	void NetMulticastRPC_TryAttack(bool bSuccess, FVector repelLocation, FVector repelDirection);
	virtual void NetMulticastRPC_TryAttack_Implementation(bool bSuccess, FVector repelLocation, FVector repelDirection);
	UFUNCTION(NetMulticast, Reliable, Category = "Collision")
	void NetMulticastRPC_Damaged(FVector repelLocation, FVector repelDirection);
	virtual void NetMulticastRPC_Damaged_Implementation(FVector repelLocation, FVector repelDirection);

protected:
	/** GAS 시스템 초기화 */
	virtual void InitAbilitySystem() override;

private:
	UFUNCTION()
	void OnRep_FrontItemSlot();
	
	UFUNCTION()
	void OnRep_BackItemSlot();

	UFUNCTION()
	void OnUseItemKeyInput();
	UFUNCTION(Server, Reliable)
	void ServerRPC_UseItem();

	UFUNCTION()
	void OnToggleItemKeyInput();
	UFUNCTION(Server, Reliable)
	void ServerRPC_ToggleItem();

	void OnCollectDamagedEvent(const FOnAttributeChangeData& changeData);

protected:
	/** AbilitySystemComponent */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> abilitySystemComponent;

	/** Racer AttributeSet */
	UPROPERTY()
	TObjectPtr<UASRacer> attributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="HitReaction")
	float repulsive_SuccessHit = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="HitReaction")
	float repulsive_FailedHit = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="HitReaction")
	float repulsive_Damaged = 200.f;

	UPROPERTY(Replicated)
	float untouchableTime = 0.f;
	

	/** 전방 Gameplay Ability */
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_FrontItemSlot, Category = "GAS|Ability")
	TObjectPtr<UItemData> frontItemSlot;

	/** 후방 Gameplay Ability */
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing=OnRep_BackItemSlot, Category = "GAS|Ability")
	TObjectPtr<UItemData> backItemSlot;

	/** 기본 Attribute 초기화 Effect */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ability")
	TSubclassOf<UGameplayEffect> defaultAttributeEffect;

	/** GameplayTag 컨테이너 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Tag")
	FGameplayTagContainer tagContainer;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input")
	TObjectPtr<UInputMappingContext> IMC_RacerItem;
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input")
	TObjectPtr<UInputAction> IA_UseItem;
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input")
	TObjectPtr<UInputAction> IA_ToggleItem;

	TArray<TPair<FVector, FRotator>> restartTransforms;
#pragma endregion

#pragma region Boost
public:
	/** 부스터 사용 중 여부 확인 */
	UFUNCTION(BlueprintPure, Category = "Vehicle|Boost")
	bool IsBoosting() const { return bIsBoosting; }

protected:
	/** 부스터 사용 중 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Boost")
	bool bIsBoosting = false;

	/** 첫 번째 입력을 GameState에 알렸는지 여부 */
	UPROPERTY()
	bool bFirstInputSent = false;

	/** 움직임 감지 타이머 핸들 */
	FTimerHandle MovementCheckTimer;

public:
	/** 첫 입력 감지 및 GameState에 알림 (서버 전용) */
	void CheckAndNotifyFirstInput();

private:
	/** 타이머로 차량 속도를 체크하여 첫 움직임 감지 (서버 전용) */
	void CheckVehicleMovement();
#pragma endregion

#pragma region CCTV
public:
	/** CCTV 카메라 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<class UCCTVCameraComponent> CCTVCameraComponent;
	
	/** MinimapIconComponent (real ping 표시용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UMinimapIconComponent> MinimapIconComponent;

	/** CCTV 전용 카메라 가져오기 (Slot: 0~2) */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "CCTV")
	UCameraComponent* GetCCTVCamera(int32 SlotIndex);

	/** CCTV 전용 카메라 기본 구현 (하위 클래스에서 오버라이드) */
	virtual UCameraComponent* GetCCTVCamera_Implementation(int32 SlotIndex);

	/** CCTV 카메라가 현재 Racer가 사용 중인지 확인 (Slot: 0~2) */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "CCTV")
	bool IsCCTVCameraInUse(int32 SlotIndex);

	/** CCTV 카메라 사용 상태 확인 기본 구현 (하위 클래스에서 오버라이드) */
	virtual bool IsCCTVCameraInUse_Implementation(int32 SlotIndex);

	/** CCTV SceneCaptureComponent 가져오기 (CCTVFeedComponent에서 사용, PixelEnemy와 동일한 방식) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	class USceneCaptureComponent2D* GetCCTVSceneCaptureComponent() const;
#pragma endregion
};
