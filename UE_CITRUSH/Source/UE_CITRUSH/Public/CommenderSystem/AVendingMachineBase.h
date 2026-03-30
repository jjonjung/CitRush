// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AVendingMachineBase.generated.h"

class ACommenderCharacter;
class UWidgetComponent;
class UVendingItemListWidget;
class UVendingSlotUIData;
class AVendingItemActor;
class UInteractableComponent;

// 슬롯 아이템 스택
USTRUCT(BlueprintType)
struct FVendingSlotStack
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<AVendingItemActor>> Items;
};

// 슬롯별 설정
USTRUCT(BlueprintType)
struct FVendingSlotConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vending|Items")
	TSubclassOf<AVendingItemActor> ItemClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vending|Items")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vending|Items")
	FVector Scale = FVector(1.f, 1.f, 1.f);

};

// 자판기 베이스 액터
UCLASS()
class UE_CITRUSH_API AVendingMachineBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AVendingMachineBase();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// 슬롯으로 이동하여 아이템 배출
	UFUNCTION(BlueprintCallable, Category="Vending|Bar")
	void MoveToSlot(int32 SlotIndex);

	// 아이템 바닥 충돌 처리
	void OnItemHitGround(AVendingItemActor* Item);

	// 테스트용: 레벨에 배치된 아이템을 픽업 가능하게 설정
	UFUNCTION(BlueprintCallable, Category="Vending|Test")
	void SetTestItem(AVendingItemActor* TestItem);

	// 테스트용: 현재 상태 출력
	UFUNCTION(BlueprintCallable, Category="Vending|Test")
	void DebugPrintStatus();

	// 테스트 모드 활성화 (BeginPlay에서 자동으로 아이템 찾기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Test")
	bool bEnableTestMode = false;

	// PickupBox Overlap 이벤트
	UFUNCTION()
	void OnPickupBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnPickupBoxEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	// 테스트 아이템 자동 설정 (타이머 콜백)
	void AutoSetupTestItem();

public:
	// 플레이어가 아이템 픽업 (내부 호출용, InteractableComponent에서 호출)
	UFUNCTION(BlueprintCallable, Category="Vending|Pickup")
	void PickupItem(APlayerController* PlayerController);

protected:
	virtual void BeginPlay() override;

	// ==================== UI 관련 ====================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending|UI")
	TObjectPtr<UWidgetComponent> ItemListWidgetComp;
	
	UFUNCTION()
	void OnUISlotSelected(int32 SlotIndex);

	UPROPERTY()
	ACommenderCharacter* CachedCommander;

	// ==================== 메시 컴포넌트 ====================
	// 자판기 본체 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending")
	TObjectPtr<class UStaticMeshComponent> BodyMesh;

	// 자판기 유리 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending")
	TObjectPtr<class UStaticMeshComponent> GlassMesh;
	
	// ==================== Bar & Hand 메커니즘 ====================
	// Bar의 루트 컴포넌트 (Z축 이동용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending")
	TObjectPtr<USceneComponent> BarRoot;

	// Bar 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending")
	TObjectPtr<class UStaticMeshComponent> BarMesh;
	
	// Hand 메시 컴포넌트 (Bar의 자식, Y축 이동)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending")
	TObjectPtr<class UStaticMeshComponent> HandMesh;

	// Hand에 아이템을 부착할 지점 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending")
	TObjectPtr<class USceneComponent> HandAttachPoint;

	// 픽업 도어
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending")
	TObjectPtr<class UStaticMeshComponent> DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending")
	TObjectPtr<class USceneComponent> PickupDoorRoot;

	// PickupBox
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending|Pickup")
	TObjectPtr<class UBoxComponent> PickupBox;

	// DropPoint 영역 체크용 Box (이 안에 있으면 자판기 아이템)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending|Pickup")
	TObjectPtr<class UBoxComponent> DropPointCheckBox;

	// 상호작용 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending|Pickup")
	TObjectPtr<UInteractableComponent> InteractableComponent;

	// InteractionSphere 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Pickup")
	FVector InteractionSphereOffset = FVector::ZeroVector;

	// UI 위젯 스케일 (크기 조정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Pickup", meta=(ClampMin="0.1", ClampMax="2.0"))
	float InteractionUIScale = 0.5f;

	

	// 이동 속도 - Bar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float BarMoveSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float BarMoveTolerance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float BarMoveNearDistance = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float BarMoveNearSpeedMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float ImmediateMoveDistance = 5.0f;

	// 이동 속도 - Hand
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float HandMoveSpeed = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float HandMoveDecelerationSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float HandMoveDecelerationDistance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float HandMoveTolerance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float HandMoveFinalDistanceMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vending|Speed")
	float HandMoveFinalSpeedMultiplier = 5.0f;

	// 이동 상태
	FVector BarTargetLocation;
	FVector BarBaseLocation;
	bool bMoveBar = false;

	FVector HandBaseRelativeLocation;
	FVector HandTargetRelativeLocation;
	bool bMoveHand = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending")
	TObjectPtr<USceneComponent> DropPoint;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending|Slot")
	TArray<TObjectPtr<USceneComponent>> SlotTargets;

	// 드랍된 아이템 (바닥에 떨어진 아이템)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending|Pickup")
	TObjectPtr<AVendingItemActor> DroppedItem = nullptr;

protected:
	// 아이템 관리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending|Items")
	TArray<FVendingSlotStack> SlotItemStacks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vending|Items")
	TSubclassOf<AVendingItemActor> DefaultItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vending|Items")
	TArray<FVendingSlotConfig> SlotConfigs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vending|Items")
	int32 ItemsPerSlot = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vending|Items")
	float ItemSpacing = 8.f;

	int32 CurrentSlotIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vending|Items")
	TObjectPtr<AVendingItemActor> GrabbedItem = nullptr;

	// 상태 플래그
	bool bIsProcessing = false;
	bool bWaitingToGrabItem = false;
	bool bDroppingItem = false;

	// 내부 함수
	void GrabItemFromSlot();
	void ReleaseItemAtDropPoint();
	void SpawnItemAtDropPoint();
	
	// DropPoint 박스 안에서 아이템 찾기
	AVendingItemActor* FindItemInDropPointBox() const;
	
	// InteractionSphere 위치 업데이트 (중복 코드 통합)
	void UpdateInteractionSphereLocation();
	
	// CachedCommander 업데이트 (Commander 스폰 시점 문제 해결)
	void UpdateCachedCommander();
};