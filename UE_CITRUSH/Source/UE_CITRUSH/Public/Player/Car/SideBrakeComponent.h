// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SideBrakeComponent.generated.h"

class UChaosWheeledVehicleMovementComponent;

/**
 * 드리프트용 사이드 브레이크 컴포넌트
 * - 후륜만 브레이크 적용하여 드리프트 유도
 * - 일반 Handbrake와 별도 동작
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API USideBrakeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USideBrakeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 후륜 핸드브레이크 토크 (Nm) - 스포츠카: 강력한 제동력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SideBrake")
	float HandbrakeTorque = 5500.0f;

	// 드리프트 시 후륜 마찰 감소 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SideBrake")
	bool bReduceRearFrictionOnBrake = true;

	// 드리프트 시 후륜 마찰 배율 (스포츠카: 민첩한 반응)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SideBrake", meta = (EditCondition = "bReduceRearFrictionOnBrake"))
	float DriftFrictionMultiplier = 0.6f;

	// 사이드 브레이크 시 엔진 토크 차단 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SideBrake")
	bool bCutThrottleOnBrake = true;

	// 사이드 브레이크 활성화/비활성화
	UFUNCTION(BlueprintCallable, Category = "SideBrake")
	void SetSideBrakeApplied(bool bApplied);

	// 현재 사이드 브레이크 상태 확인
	UFUNCTION(BlueprintPure, Category = "SideBrake")
	bool IsSideBrakeApplied() const { return bIsSideBrakeApplied; }

	// ========== 네트워크 RPC ==========
	/** 서버에서 사이드 브레이크 설정 (클라이언트 → 서버) */
	UFUNCTION(Server, Reliable)
	void ServerRPC_SetSideBrake(bool bApplied);

protected:
	virtual void BeginPlay() override;

private:
	// 차량 MovementComponent 참조
	UPROPERTY()
	UChaosWheeledVehicleMovementComponent* VehicleMovement;

	// 사이드 브레이크 활성화 상태 (네트워크 복제)
	UPROPERTY(Replicated)
	bool bIsSideBrakeApplied = false;

	// 원래 후륜 마찰 계수 (복원용)
	float OriginalRearLeftFriction = 1.0f;
	float OriginalRearRightFriction = 1.0f;

	// 차량 MovementComponent 찾기
	void FindVehicleMovement();

	// 핸드브레이크 설정 초기화
	void InitializeHandbrakeSettings();

	// 후륜 마찰 조정
	void AdjustRearFriction(bool bReduce);

private:
	float CurrentThrottleInput = 0.0f;
};
