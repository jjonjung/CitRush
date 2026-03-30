// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CCTVCameraComponent.generated.h"

class UCameraComponent;

/**
 * CCTV 카메라를 관리하는 컴포넌트
 * 레이서와 Enemy에 추가하여 CCTV 시스템에서 사용할 카메라를 지정할 수 있습니다.
 */
UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UCCTVCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCTVCameraComponent(const FObjectInitializer& ObjectInitializer);

	/** CCTV 카메라 가져오기 (Slot: 0~2) - SpringArm을 자동으로 찾아서 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	UCameraComponent* GetCCTVCamera(int32 SlotIndex) const;

protected:
	virtual void BeginPlay() override;

	/** SpringArm에 Attach된 Camera 찾기 */
	UCameraComponent* FindCameraAttachedToSpringArm(class USpringArmComponent* SpringArm) const;

	/** VehiclePawn의 Slot 2용 SpringArm 찾기 (Player Index에 따라) */
	UCameraComponent* FindSlot2SpringArmForVehiclePawn(class AAbstractRacer* Racer) const;
};

