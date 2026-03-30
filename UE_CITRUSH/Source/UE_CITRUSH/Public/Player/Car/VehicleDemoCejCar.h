// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VehicleTemplate/UE_CITRUSHPawn.h"
#include "VehicleDemoUITest.h"
#include "VehicleDemoCejCar.generated.h"

class USphereComponent;
class UBoostComponent;
class USideBrakeComponent;
class UWidgetBlueprintDataAsset;

UENUM(BlueprintType)
enum class EDriveType : uint8
{		
	FWD UMETA(DisplayName = "Front-Wheel Drive"),  // 전륜 구동
	RWD UMETA(DisplayName = "Rear-Wheel Drive"),   // 후륜 구동
	AWD UMETA(DisplayName = "All-Wheel Drive")     // 사륜 구동
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FFuelChangedDelegate,
	float, CurrentFuel,
	float, MaxFuel
);

UCLASS(abstract)
class UE_CITRUSH_API AVehicleDemoCejCar : public AUE_CITRUSHPawn
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(CEJCarLog, Log, All);

public:
	AVehicleDemoCejCar();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void OnRep_PlayerState() override;

	// Throttle 입력 오버라이드 (시야 감소 중 차단 처리)
	virtual void Throttle(const FInputActionValue& Value);

	// 구동 방식 설정
	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	void SetDriveType(EDriveType NewDriveType);

	// HUD 생성 시도 (PlayerState 복제 시점 고려)
	void TryCreateHUD();

	UPROPERTY(BlueprintAssignable)
	FFuelChangedDelegate OnFuelChanged;

	// ========== 컴포넌트 ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoostComponent* Boost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USideBrakeComponent* SideBrake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> shieldSphere;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material")
	TObjectPtr<UMaterial> shieldMat;

	// ========== Enhanced Input (추가 기능만) ==========
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* BoostAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SideBrakeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ToggleEffectAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* TogglePixelEnemyCameraAction; 

	// ========== 차량 튜닝 파라미터 (미니 쿠퍼 S 기준) ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tuning")
	EDriveType DriveType = EDriveType::FWD; // 기본: 전륜 구동

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tuning")
	float VehicleMass = 7000.0f; // kg (미니 쿠퍼 S: 약 1200kg)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tuning")
	float MaxTorque = 1000.0f; // Nm (미니 쿠퍼 S: 약 250Nm)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tuning")
	float MaxRPM = 15000.0f; // 최대 RPM

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tuning")
	float MaxSpeed = 225.0f; // km/h (미니 쿠퍼 S: 약 225km/h)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tuning")
	float DragCoefficient = 0.32f; // 공기 저항 계수

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tuning")
	float TireFriction = 3.8f; // 타이어 마찰 계수 (고성능 타이어)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tuning")
	float SuspensionStiffness = 6000.0f; // 서스펜션 강성 (스포츠 세팅)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Tuning")
	float WheelRadius = 31.0f; // cm (16인치 휠)

	// ========== 입력 콜백 ==========
	void OnBoostPressed();
	void OnBoostReleased();
	void OnSideBrakePressed();
	void OnSideBrakeReleased();

	// 에디터에서 할당하거나 생성 시 저장할 HUD 위젯 참조
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UVehicleDemoUITest> CarHUDWidget;

	/** CarHUDWidget 가져오기 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	UVehicleDemoUITest* GetCarHUDWidget() const { return CarHUDWidget; }

	// 컴퍼스 업데이트 함수
	void UpdateCompass();
	
	virtual void NetMulticastRPC_Damaged_Implementation(FVector repelLocation, FVector repelDirection) override;
	UFUNCTION(BlueprintCallable, Category = "GAS|Buff")
	void ApplyShield();
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "GAS|Debuff")
	void ApplyReducedVision();


protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UVehicleDemoUITest> UVehicleDemoUITestclass;

	void CreateCarHUDWidget();

private:
	/** CCTV Widget Focus 활성화 여부 */
	bool bCCTVWidgetFocused = false;
	
public:
	// ========== Getters ==========
	UFUNCTION(BlueprintPure, Category = "Vehicle")
	UBoostComponent* GetBoost() const { return Boost; }

	UFUNCTION(BlueprintPure, Category = "Vehicle")
	USideBrakeComponent* GetSideBrake() const { return SideBrake; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MouseResetVehicleAction;

	// ========== 연료 시스템 ==========
	/** 연료 소진 타이머 핸들 */
	FTimerHandle FuelDrainTimer;
	/** 연료 소진 함수 (0.5초마다 1% 감소) */
	UFUNCTION()
	void DrainFuel();
	/** 연료량에 따른 차량 성능 제한 */
	void UpdateVehiclePerformanceByFuel();

	/** 아이템 슬롯 변경 핸들러 */
	UFUNCTION()
	void HandleItemSlotChanged(class UItemData* FrontItem, class UItemData* BackItem);

private:
	UPROPERTY()
	bool bIsThrottleBlocked = false;
	
	UPROPERTY()
	UWidgetBlueprintDataAsset* widgetData;

	/** 원래 ViewTarget (Racer 자신) */
	UPROPERTY()
	TObjectPtr<AActor> OriginalViewTarget;

};
