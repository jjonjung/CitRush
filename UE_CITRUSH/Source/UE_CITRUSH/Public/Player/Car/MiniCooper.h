// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/AbstractRacer.h"
#include "InputActionValue.h"
#include "MiniCooper.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UCurveFloat;
class UBoostComponent;
struct FInputMappingData;

DECLARE_LOG_CATEGORY_EXTERN(MiniCooper, Log, All);

/**
 * 바퀴 데이터 구조체
 * 각 바퀴의 본 이름, 위치, 구동/조향 여부 등을 저장
 */
USTRUCT(BlueprintType)
struct FWheelData
{
	GENERATED_BODY()

	/** 바퀴 본 이름 (예: Phys_Wheel_FL) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	FName BoneName = NAME_None;

	/** 바퀴 로컬 오프셋 (차체 중심 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	FVector LocalOffset = FVector::ZeroVector;

	/** 조향 가능 바퀴인지 (앞바퀴만 true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	bool bIsSteered = false;

	/** 구동 바퀴인지 (FWD의 경우 앞바퀴만 true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	bool bIsDriven = false;

	// Runtime data (Blueprint에 노출하지 않음)
	float CurrentRotation = 0.f;
	float SuspensionCompression = 0.f;
	FVector ContactPoint = FVector::ZeroVector;
	bool bIsGrounded = false;
};

/**
 * Mini Cooper 차량 클래스
 * AAbstractRacer를 상속받아 GAS, Voice, Item 시스템 통합
 * Tick()에서 직접 커스텀 물리 시뮬레이션 수행

* 물리 시뮬레이션:
 * - Raycast 기반 서스펜션 (Hooke's Law + Damping)
 * - Pacejka Tire Model (Curve Table 또는 간단한 공식)
 * - 바퀴별 힘 적용 (AddForceAtLocation)
 */
UCLASS(abstract)
class UE_CITRUSH_API AMiniCooper : public AAbstractRacer
{
	GENERATED_BODY()

public:
	AMiniCooper();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

////////camera
	////** Spring Arm for the front camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* FrontSpringArm;

	/** Front Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCamera;

	/** Spring Arm for the back camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* BackSpringArm;

	/** Back Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCamera;

	/** Spring Arm for the left side camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* LeftSpringArm;

	/** Left Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* LeftCamera;

	/** Spring Arm for the right side camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* RightSpringArm;

	/** Right Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* RightCamera;

	/** Spring Arm for the back side camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* BackSideSpringArm;

	/** Back Side Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackSideCamera;

	// ========== Components ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* CarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoostComponent* BoostComponent;

	// ========== 바퀴 설정 ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheels")
	TArray<FWheelData> Wheels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheels")
	float WheelRadius = 31.f; // cm (16인치 휠)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheels")
	float WheelMass = 15.f; // kg (바퀴 + 타이어 질량)

	// ========== 서스펜션 설정 ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
	float SpringStiffness = 30000.f; // N/m (스프링 강성)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
	float SpringDamping = 2000.f; // Ns/m (댐핑 계수)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
	float MaxSuspensionLength = 50.f; // cm (최대 서스펜션 길이)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
	float SuspensionRestLength = 35.f; // cm (서스펜션 기본 길이)

	// ========== 엔진 설정 ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine")
	float MaxTorque = 1000.f; // Nm (최대 토크)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine")
	float MaxRPM = 7000.f; // 최대 RPM

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine")
	float BrakeTorque = 3000.f; // Nm (브레이크 토크)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine")
	float IdleRPM = 900.f; // 아이들 RPM

	// ========== 스티어링 설정 ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float MaxSteeringAngle = 30.f; // 도 (최대 조향 각도)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float SteeringSpeed = 2.5f; // 조향 속도

	// ========== 타이어 모델 (Pacejka) ==========
	/** Slip Ratio → Longitudinal Force 커브 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Model")
	UCurveFloat* LongitudinalForceCurve;

	/** Slip Angle → Lateral Force 커브 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Model")
	UCurveFloat* LateralForceCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Model")
	float TireGripMultiplier = 1.0f; // 타이어 그립 배율

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Model")
	float MaxLateralForce = 8000.f; // N (최대 횡방향 힘)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tire Model")
	float MaxLongitudinalForce = 6000.f; // N (최대 종방향 힘)

	// ========== 공기 저항 ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
	float DragCoefficient = 0.32f; // 공기 저항 계수 (Cd)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
	float FrontalArea = 2.3f; // m^2 (전면 면적)

	// ========== 다운포스 ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics")
	float DownforceCoefficient = 0.0f; // 다운포스 계수 (일반 차량은 0에 가까움)

	// ========== Enhanced Input Actions ==========
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SteeringAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ThrottleAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* BrakeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ResetVehicleAction;

	/** Handbrake Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* HandbrakeAction;

	/** Look Around Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAroundAction;

	/** Toggle Camera Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ToggleCameraAction;

	/** Keeps track of which camera is active */
	bool bFrontCameraActive = false;

	/** Current camera index for cycling (0=Back, 1=Front, 2=Left, 3=Right, 4=BackSide) */
	int32 CurrentCameraIndex = 0;
	
	const FInputMappingData* inputData;

	// ========== 디버그 ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugLines = false;

protected:
	// ========== 입력 값 ==========
	float ThrottleInput = 0.f;
	float SteeringInput = 0.f;
	float BrakeInput = 0.f;
	float CurrentSteeringAngle = 0.f; // 현재 조향 각도 (부드러운 조향을 위함)

	/** Input Mapping Data from InputMappingsSettings */
	//const FInputMappingData* inputData;

	// ========== 입력 콜백 함수 ==========
	void Steering(const FInputActionValue& Value);
	void Throttle(const FInputActionValue& Value);
	void Brake(const FInputActionValue& Value);
	//void StopBrake(const FInputActionValue& Value);
	void ResetVehicle(const FInputActionValue& Value);

	/** Handles brake start/stop inputs */
	void StartBrake(const FInputActionValue& Value);
	void StopBrake(const FInputActionValue& Value);
	UFUNCTION(BlueprintImplementableEvent)
	void BrakeLights(bool bBraking);

	/** Handles handbrake start/stop inputs */
	/*void StartHandbrake(const FInputActionValue& Value);
	void StopHandbrake(const FInputActionValue& Value);*/

	/** Handles look around input */
	void LookAround(const FInputActionValue& Value);

	/** Handles toggle camera input */
	void ToggleCam(const FInputActionValue& Value);

public:

	// ========== 물리 시뮬레이션 ==========
	/** 서스펜션 물리 적용 (Raycast + Hooke's Law + Damping) */
	void ApplySuspensionForces(float DeltaTime);

	/** 구동력 및 타이어 힘 적용 (Pacejka Model) */
	void ApplyDriveForces(float DeltaTime);

	/** 공기 저항 적용 (Drag Force = 0.5 * ρ * Cd * A * v²) */
	void ApplyAerodynamicForces(float DeltaTime);

	/** 바퀴 회전 업데이트 */
	void UpdateWheelRotations(float DeltaTime);

	/** 카메라 각도 감쇠 (공중에서 카메라 흔들림 방지) */
	void UpdateCamera(float DeltaTime);

	// ========== Pacejka 타이어 모델 ==========
	/** 종방향 힘 계산 (Slip Ratio → Force) */
	float CalculateLongitudinalForce(float SlipRatio) const;

	/** 횡방향 힘 계산 (Slip Angle → Force) */
	float CalculateLateralForce(float SlipAngle) const;

	/** Slip Ratio 계산 (간단 버전) */
	float CalculateSlipRatio(float WheelSpeed, float VehicleSpeed, bool bIsBraking) const;

public:
	/** 현재 속도 (km/h) */
	UFUNCTION(BlueprintPure, Category = "Vehicle")
	float GetSpeedKmh() const;

	/** 현재 RPM (근사값) */
	UFUNCTION(BlueprintPure, Category = "Vehicle")
	float GetCurrentRPM() const;
};
