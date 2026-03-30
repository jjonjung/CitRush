// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VendingItemActor.generated.h"

class AVendingMachineBase;

// 커맨더 아이템 효과 타입 (디스크에 저장된 아이템이 레이서용인지 커맨더용인지)
UENUM(BlueprintType)
enum class ECommanderItemEffect : uint8
{
	None = 0,
	RacerItem,      // 레이서에게 줄 아이템
	CommanderItem   // 커맨더가 직접 사용할 아이템
};

// 자판기 아이템 액터
UCLASS()
class UE_CITRUSH_API AVendingItemActor : public AActor
{
	GENERATED_BODY()

public:
	AVendingItemActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending|Item")
	TObjectPtr<USceneComponent> MeshRoot;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Item")
	TObjectPtr<class UStaticMeshComponent> MeshComp;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Item")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Item")
	int32 Coin = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Item")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Item")
	int32 StackIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Item")
	TObjectPtr<class UTexture2D> ItemIcon = nullptr;

	// ==================== 아이템 타입 설정 ====================
	// 이 아이템이 레이서에게 줄 아이템인지, 커맨더가 직접 사용할 아이템인지 설정
	// - RacerItem: 레이서에게 전달되는 아이템 (CD 플레이어에서 레이서 선택 필요)
	// - CommanderItem: 커맨더가 직접 사용하는 아이템 (CD 플레이어에서 사용 결정 필요)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Item|Type", meta=(DisplayName="아이템 타입", Tooltip="레이서 아이템: 레이서에게 전달 | 커맨더 아이템: 커맨더가 직접 사용"))
	ECommanderItemEffect EffectType = ECommanderItemEffect::RacerItem;

	UFUNCTION(BlueprintCallable, Category = "Vending|Item")
	void InitItem(FName InItemId, int32 InCoin, int32 InSlotIndex, int32 InStackIndex);

	// 아이템 효과 타입 가져오기
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vending|Item")
	ECommanderItemEffect GetEffectType() const { return EffectType; }

	// 레이서 아이템인지 확인
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vending|Item")
	bool IsRacerItem() const { return EffectType == ECommanderItemEffect::RacerItem; }

	// 커맨더 아이템인지 확인
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vending|Item")
	bool IsCommanderItem() const { return EffectType == ECommanderItemEffect::CommanderItem; }

	// 아이템 배출 (물리 활성화)
	UFUNCTION(BlueprintCallable, Category="Vending|Item")
	void StartDispense(const FVector& DropPointWorld);

	// 바닥 충돌 감지
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	bool bHasHitGround = false;
	UPROPERTY()
	AVendingMachineBase* VendingMachine = nullptr;
	
	void SetVendingMachine(AVendingMachineBase* InVendingMachine) { VendingMachine = InVendingMachine; }
	
};
