// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommenderSystem/AVendingMachineBase.h"
#include "FirstPersonTemplate/TP_FirstPersonCharacter.h"
#include "Data/PingTypes.h"
#include "CommenderCharacter.generated.h"

class AItemInputMachine;
class AVendingItemActor;
class UCommenderHUDWidget;

UCLASS()
class UE_CITRUSH_API ACommenderCharacter : public ATP_FirstPersonCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACommenderCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// HUD 위젯 인스턴스
	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<UCommenderHUDWidget> CommenderHUDWidget;

	// 현재 상호작용 중인 자판기 (PickupBox 영역 안에 있을 때)
	UPROPERTY(BlueprintReadWrite)
	AVendingMachineBase* FocusedVending = nullptr;

	// 아이템을 들고 있는 자판기 참조 (영역 밖으로 나가도 유지)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	AVendingMachineBase* HoldingVendingMachine = nullptr;

	// 현재 조준 중인 아이템
	UPROPERTY(BlueprintReadOnly, Category="Item")
	class AVendingItemActor* AimedItem = nullptr;

	// 현재 들고 있는 아이템 (디스크로 사용)
	UPROPERTY(BlueprintReadWrite, Category="Item")
	class AVendingItemActor* HeldDisc = nullptr;

	// 지금 손에 들고 있는 아이템
	UPROPERTY(BlueprintReadOnly, Category="Vending|Item")
	TObjectPtr<class AVendingItemActor> GrabbedItem = nullptr;

	// 현재 크로스헤어가 보고 있는 머신 (Interactable로부터 세팅)
	UPROPERTY(BlueprintReadOnly, Category="Vending|Item")
	TObjectPtr<class AItemInputMachine> FocusedInputMachine = nullptr;

	// 아이템 잡기 최대 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	float GrabDistance = 300.f;

	// 3D 위젯 상호작용 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Widget")
	class UWidgetInteractionComponent* WidgetInteraction = nullptr;

	// Physics Handle (물리 오브젝트 잡기용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	class UPhysicsHandleComponent* PhysicsHandle = nullptr;

	// 아이템 홀드용 소켓 (카메라에 부착)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	class USceneComponent* ItemHoldSocket = nullptr;

	// 소켓 위치 (카메라 기준 상대 위치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FVector ItemHoldSocketOffset = FVector(80.0f, 0.0f, -10.0f);

	// 아이템 충돌 감지 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	float ItemCollisionCheckDistance = 20.0f;

	// 아이템이 충돌 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category="Item")
	bool bItemColliding = false;

	// 충돌 방향 (정규화된 벡터)
	UPROPERTY(BlueprintReadOnly, Category="Item")
	FVector CollisionNormal = FVector::ZeroVector;

	// Grab Input Action
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	class UInputAction* GrabAction = nullptr;

	// UseItem Input Action (F키)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	class UInputAction* UseItemAction = nullptr;

	// 마우스 왼클릭 입력 처리 (Grab/Drop 전용)
	UFUNCTION()
	void OnGrabItemStarted(const struct FInputActionValue& Value);

	UFUNCTION()
	void OnGrabItemCompleted(const struct FInputActionValue& Value);

	// LMB 입력 처리 (Grab/Drop 전용)
	void OnLeftClickPressed();

	// F키 입력 처리 (아이템 사용)
	UFUNCTION()
	void OnUseItemPressed(const struct FInputActionValue& Value);

	// 서버에 아이템 지급 요청 (Server RPC)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Item")
	void Server_RequestGiveItemToRacer(class AItemInputMachine* InputMachine, class AVendingItemActor* Item);
	void Server_RequestGiveItemToRacer_Implementation(class AItemInputMachine* InputMachine, class AVendingItemActor* Item);

	// Interactable 시스템에서 불러 줄 함수
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void SetFocusedInteractable(class UInteractableComponent* NewInteractable);

	// PhysicsHandle Release 헬퍼 함수
	void ReleaseGrabPhysics();

	// 아이템 충돌 체크
	UFUNCTION()
	void CheckItemCollision();

	// 이동 입력 필터링 (충돌 방향으로 이동 제한)
	UFUNCTION()
	FVector FilterMovementInput(const FVector& InputVector);

	// 회전 입력 필터링 (충돌 시 제한)
	UFUNCTION()
	FVector2D FilterRotationInput(const FVector2D& InputVector);

	// 아이템을 들고 있는지 확인 (자판기 아이템 또는 월드 아이템)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Item")
	bool IsHoldingItem() const;

	// GRAB된 VendingItemActor 가져오기 (없으면 nullptr)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Item")
	class AVendingItemActor* GetGrabbedVendingItem() const;

	// 아이템 Grab 시도
	void TryGrabItem();

	// 아이템 Drop
	void DropGrabbedItem();

	// 아이템을 조준하고 있는지 확인 (크로스헤어용)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Item")
	bool IsAimingAtItem() const;

	// ==================== 코인 시스템 ====================
	// 현재 보유 코인
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coin")
	int32 CurrentCoin = 1000;

	// 코인 추가
	UFUNCTION(BlueprintCallable, Category="Coin")
	void AddCoin(int32 Amount);

	// 코인 차감
	UFUNCTION(BlueprintCallable, Category="Coin")
	bool SpendCoin(int32 Amount);

	// 코인 보유량 확인
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Coin")
	bool HasEnoughCoin(int32 Amount) const;

	// 코인 변경 이벤트 (UI 업데이트용)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCoinChanged, int32, NewCoin);
	UPROPERTY(BlueprintAssignable, Category="Coin")
	FOnCoinChanged OnCoinChanged;

	// ==================== 맵 시스템 ====================
	// 맵 열기 (입력 모드 변경 및 이동 차단)
	UFUNCTION(BlueprintCallable, Category="Map")
	void OpenMap();

	// 맵 닫기 (입력 모드 복원 및 이동 활성화)
	UFUNCTION(BlueprintCallable, Category="Map")
	void CloseMap();

	// 맵이 열려있는지 확인
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Map")
	bool IsMapOpen() const { return bMapOpen; }

	// ==================== 핑 시스템 ====================
	// 서버에 핑 배치 요청 (Server RPC)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Ping")
	void ServerPlacePing(const FVector& WorldLocation, ECommanderPingType Type);

	// 클라이언트에 핑 쿨다운 알림 (Client RPC)
	UFUNCTION(Client, Reliable)
	void ClientNotifyPingCooldown(float RemainingTime);

	// 핑이 글로벌 쿨다운 중인지 확인
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ping")
	bool IsPingOnGlobalCooldown(float& OutRemainingTime) const;

	// 핑 쿨타임 초기화 (레이서가 핑에 부딪혔을 때 호출)
	UFUNCTION(BlueprintCallable, Category="Ping")
	void ResetPingCooldown();

protected:
	// 입력 함수 오버라이드 (충돌 시 제한 적용)
	virtual void Move(const struct FInputActionValue& Value) override;
	virtual void Look(const struct FInputActionValue& Value) override;

	// HUD 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSubclassOf<UCommenderHUDWidget> CommenderHUDWidgetClass;

private:
	// LineTrace 간격 조정용 (성능 최적화)
	float LineTraceInterval = 0.1f; // 0.1초마다 실행
	float LineTraceTimer = 0.0f;

	// 이전 크로스헤어 상태 (중복 업데이트 방지)
	bool bPreviousCanUseItem = false;
	bool bPreviousCanGrabItem = false;

	// 크로스헤어 업데이트 (이벤트 기반)
	// bCanGrabItem: CheckGrabableItem에서 계산한 결과를 전달 (중복 LineTrace 방지)
	void UpdateCrosshair(bool bCanGrabItem = false);

	// LineTrace로 Grab 가능한 아이템 확인 (타이머 기반)
	FTimerHandle GrabCheckTimerHandle;
	void CheckGrabableItem();

	// PhysicsHandle 업데이트 (타이머 기반, Tick 대신)
	FTimerHandle PhysicsHandleUpdateTimerHandle;
	void UpdatePhysicsHandle();

	// 맵 열림 상태
	UPROPERTY()
	bool bMapOpen = false;

	// 핑 시스템 관련 변수
	// 서버: 글로벌 쿨다운 종료 시각
	float GlobalPingCooldownEndTime = 0.f;
	// 서버: 최근 핑 배치 시간 기록 (스팸 감지용)
	TArray<float> RecentPingTimes;
	// 클라이언트: 글로벌 쿨다운 종료 시각
	float ClientGlobalPingCooldownEndTime = 0.f;
};
