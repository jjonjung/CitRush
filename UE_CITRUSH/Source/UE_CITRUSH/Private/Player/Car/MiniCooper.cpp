// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Car/MiniCooper.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Curves/CurveFloat.h"
#include "Data/InputMappingsSettings.h"
#include "DrawDebugHelpers.h"

// 물리 상수
static constexpr float AIR_DENSITY = 1.225f; // kg/m³ (해수면 기준)
static constexpr float CM_TO_M = 0.01f; // cm → m 변환
static constexpr float M_TO_CM = 100.0f; // m → cm 변환

DEFINE_LOG_CATEGORY(MiniCooper);

AMiniCooper::AMiniCooper()
{
	PrimaryActorTick.bCanEverTick = true;

	// CarMesh 생성 (독립적인 메시)
	CarMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CarMesh"));
	RootComponent = CarMesh;

	// 물리 설정
	CarMesh->SetSimulatePhysics(true);
	CarMesh->SetCollisionProfileName(FName("Vehicle"));
	CarMesh->SetMassOverrideInKg(NAME_None, 1200.0f, true); // 미니 쿠퍼 무게

	// 바퀴 데이터 초기화 (미니 쿠퍼 기준 - FWD)
	Wheels.SetNum(4);
	
	//camera
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);
	BackCamera->bAutoActivate = true; // 시작 카메라로 설정

	// construct the right camera boom (우측 사이드뷰)
	RightSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Right Spring Arm"));
	RightSpringArm->SetupAttachment(GetMesh());
	RightSpringArm->TargetArmLength = 0.0f;
	RightSpringArm->bDoCollisionTest = false;
	RightSpringArm->bInheritPitch = false;
	RightSpringArm->bInheritRoll = false;
	RightSpringArm->bInheritYaw = true;
	RightSpringArm->SetRelativeLocation(FVector(-10.f, 100.f, 100.f));
	RightSpringArm->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	RightCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Right Camera"));
	RightCamera->SetupAttachment(RightSpringArm);
	RightCamera->bAutoActivate = false;

	// construct the left camera boom (좌측 사이드뷰)
	LeftSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Left Spring Arm"));
	LeftSpringArm->SetupAttachment(GetMesh());
	LeftSpringArm->TargetArmLength = 0.0f;
	LeftSpringArm->bDoCollisionTest = false;
	LeftSpringArm->bInheritPitch = false;
	LeftSpringArm->bInheritRoll = false;
	LeftSpringArm->bInheritYaw = true;
	LeftSpringArm->SetRelativeLocation(FVector(-10.f, -100.f, 100.f));
	LeftSpringArm->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	LeftCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Left Camera"));
	LeftCamera->SetupAttachment(LeftSpringArm);
	LeftCamera->bAutoActivate = false;

	// construct the back side camera boom (후방 사이드뷰)
	BackSideSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("BackSide Spring Arm"));
	BackSideSpringArm->SetupAttachment(GetMesh());
	BackSideSpringArm->TargetArmLength = 0.0f;
	BackSideSpringArm->bDoCollisionTest = false;
	BackSideSpringArm->bInheritPitch = false;
	BackSideSpringArm->bInheritRoll = false;
	BackSideSpringArm->bInheritYaw = true;
	BackSideSpringArm->SetRelativeLocation(FVector(-100.0f, 0.0f, 150.0f));
	BackSideSpringArm->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	BackSideCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BackSide Camera"));
	BackSideCamera->SetupAttachment(BackSideSpringArm);
	BackSideCamera->bAutoActivate = false;

	// Front Left
	Wheels[0].BoneName = FName("Phys_Wheel_FL");
	Wheels[0].LocalOffset = FVector(140.0f, -70.0f, -45.0f);
	Wheels[0].bIsSteered = true;
	Wheels[0].bIsDriven = true; // FWD

	// Front Right
	Wheels[1].BoneName = FName("Phys_Wheel_FR");
	Wheels[1].LocalOffset = FVector(140.0f, 70.0f, -45.0f);
	Wheels[1].bIsSteered = true;
	Wheels[1].bIsDriven = true; // FWD

	// Rear Left
	Wheels[2].BoneName = FName("Phys_Wheel_BL");
	Wheels[2].LocalOffset = FVector(-140.0f, -70.0f, -45.0f);
	Wheels[2].bIsSteered = false;
	Wheels[2].bIsDriven = false;

	// Rear Right
	Wheels[3].BoneName = FName("Phys_Wheel_BR");
	Wheels[3].LocalOffset = FVector(-140.0f, 70.0f, -45.0f);
	Wheels[3].bIsSteered = false;
	Wheels[3].bIsDriven = false;
	
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	const UInputMappingsSettings* inputLoader = UInputMappingsSettings::Get();
	inputData = inputLoader->inputMappings.Find(TEXT("IMC_Vehicle"));
}

void AMiniCooper::BeginPlay()
{
	Super::BeginPlay(); 

	if (CarMesh && !CarMesh->IsSimulatingPhysics())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MiniCooper] Mesh is not simulating physics! Enable 'Simulate Physics' in Blueprint."));
	}
}

void AMiniCooper::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CarMesh || !CarMesh->IsSimulatingPhysics())
	{
		return;
	}

	// 물리 시뮬레이션 실행 순서
	// 1. 서스펜션 힘 (차량을 지면에 밀착)
	ApplySuspensionForces(DeltaTime);

	// 2. 타이어 힘 (가속, 브레이크, 조향)
	ApplyDriveForces(DeltaTime);

	// 3. 공기 저항 및 다운포스
	ApplyAerodynamicForces(DeltaTime);

	// 4. 바퀴 회전 업데이트
	UpdateWheelRotations(DeltaTime);

	// 5. 공중 각속도 감쇠
	bool bIsGrounded = false;
	for (const FWheelData& Wheel : Wheels)
	{
		if (Wheel.bIsGrounded)
		{
			bIsGrounded = true;
			break;
		}
	}
	CarMesh->SetAngularDamping(bIsGrounded ? 0.0f : 3.0f);

	// 6. 카메라 업데이트
	UpdateCamera(DeltaTime);

	// 7. 조향 각도 부드럽게 보간
	float TargetSteeringAngle = SteeringInput * MaxSteeringAngle;
	CurrentSteeringAngle = FMath::FInterpTo(CurrentSteeringAngle, TargetSteeringAngle, DeltaTime, SteeringSpeed);
}

void AMiniCooper::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (APlayerController* PC = GetController<APlayerController>())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		    ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (inputData && inputData->inputMappingContext)
			{
				Subsystem->AddMappingContext(inputData->inputMappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("[MiniCooper] Input Mapping Context added"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[MiniCooper] Cannot find IMC_Vehicle!"));
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (SteeringAction)
		{
			EnhancedInput->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &AMiniCooper::Steering);
			EnhancedInput->BindAction(SteeringAction, ETriggerEvent::Completed, this, &AMiniCooper::Steering);
		}
		else
		{
			UE_LOG(MiniCooper, Error, TEXT("SteeringAction is NULL!"));
		}
		if (ThrottleAction)
		{
			EnhancedInput->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AMiniCooper::Throttle);
			EnhancedInput->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AMiniCooper::Throttle);
		}
		else
		{
			UE_LOG(MiniCooper, Error, TEXT("ThrottleAction is NULL!"));
		}
		if (BrakeAction)
		{
			EnhancedInput->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &AMiniCooper::Brake);
			EnhancedInput->BindAction(BrakeAction, ETriggerEvent::Started, this, &AMiniCooper::StartBrake);
			EnhancedInput->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AMiniCooper::StopBrake);
		}
		else
		{
			UE_LOG(MiniCooper, Error, TEXT("BrakeAction is NULL!"));
		}
		/*if (HandbrakeAction)
		{
			EnhancedInput->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &AMiniCooper::StartHandbrake);
			EnhancedInput->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AMiniCooper::StopHandbrake);
		}*/
		if (LookAroundAction)
		{
			EnhancedInput->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &AMiniCooper::LookAround);
		}
		EnhancedInput->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &AMiniCooper::ToggleCam);
		if (ToggleCameraAction)
		{
			EnhancedInput->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &AMiniCooper::ToggleCam);
		}
		if (ResetVehicleAction)
		{
			EnhancedInput->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &AMiniCooper::ResetVehicle);
		}
	}
	else
	{
		UE_LOG(MiniCooper, Error, TEXT("[MiniCooper] EnhancedInputComponent not found"));
	}
}

// ========================================
// 입력 콜백
// ========================================
void AMiniCooper::Steering(const FInputActionValue& Value)
{
	SteeringInput = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);
}

void AMiniCooper::Throttle(const FInputActionValue& Value)
{
	ThrottleInput = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);
}

void AMiniCooper::Brake(const FInputActionValue& Value)
{
	BrakeInput = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);
}
void AMiniCooper::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}
void AMiniCooper::StopBrake(const FInputActionValue& Value)
{
	BrakeInput = 0.0f;
}

void AMiniCooper::LookAround(const FInputActionValue& Value)
{
	float LookValue = Value.Get<float>();
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void AMiniCooper::ToggleCam(const FInputActionValue& Value)
{
	switch (CurrentCameraIndex)
	{
	case 0:
		BackCamera->SetActive(false);
		break;
	case 1:
		FrontCamera->SetActive(false);
		break;
	case 2:
		LeftCamera->SetActive(false);
		break;
	case 3:
		RightCamera->SetActive(false);
		break;
	case 4:
		BackSideCamera->SetActive(false);
		break;
	}

	CurrentCameraIndex = (CurrentCameraIndex + 1) % 5;

	switch (CurrentCameraIndex)
	{
	case 0:
		BackCamera->SetActive(true);
		bFrontCameraActive = false;
		break;
	case 1:
		FrontCamera->SetActive(true);
		bFrontCameraActive = true;
		break;
	case 2:
		LeftCamera->SetActive(true);
		bFrontCameraActive = false;
		break;
	case 3:
		RightCamera->SetActive(true);
		bFrontCameraActive = false;
		break;
	case 4:
		BackSideCamera->SetActive(true);
		bFrontCameraActive = false;
		break;
	}
}

void AMiniCooper::ResetVehicle(const FInputActionValue& Value)
{
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector),
	                  false, nullptr, ETeleportType::TeleportPhysics);

	// 물리 초기화
	if (CarMesh)
	{
		CarMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		CarMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	}

	UE_LOG(LogTemp, Log, TEXT("[MiniCooper] Vehicle reset"));
}

// 물리 시뮬레이션
void AMiniCooper::ApplySuspensionForces(float DeltaTime)
{
	/*
	 * 서스펜션 물리:
	 * F_spring = k * x  (Hooke's Law)
	 * F_damping = c * v (Damping)
	 * F_total = F_spring + F_damping
	 *
	 * k: SpringStiffness (N/m)
	 * c: SpringDamping (Ns/m)
	 * x: compression (압축 거리)
	 * v: velocity (압축 속도)
	 */

	if (!CarMesh) return;

	for (FWheelData& Wheel : Wheels)
	{
		// 바퀴 월드 위치 계산 (LocalOffset 기준)
		FVector WheelWorldPos = CarMesh->GetComponentLocation() +
		                        CarMesh->GetComponentRotation().RotateVector(Wheel.LocalOffset);

		// Raycast (서스펜션이 완전히 늘어난 상태에서 시작)
		// 시작점: 서스펜션 최대 길이만큼 위
		// 끝점: 바퀴 반경만큼 아래
		FVector TraceStart = WheelWorldPos + FVector(0, 0, MaxSuspensionLength);
		FVector TraceEnd = WheelWorldPos - FVector(0, 0, WheelRadius);

		FHitResult Hit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		bool bHit = GetWorld()->LineTraceSingleByChannel(
			Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams
		);

		if (bHit)
		{
			Wheel.bIsGrounded = true;
			Wheel.ContactPoint = Hit.ImpactPoint;

			// 서스펜션 압축 거리 계산 (cm)
			// TraceStart부터 Hit까지의 거리에서 (Rest + Radius)를 뺀 값이 압축량
			float TraceDistance = TraceStart.Z - Hit.ImpactPoint.Z;
			float CompressionDistance = TraceDistance - (SuspensionRestLength + WheelRadius);
			CompressionDistance = FMath::Clamp(CompressionDistance, 0.0f, MaxSuspensionLength);

			// 압축률 (0~1)
			Wheel.SuspensionCompression = CompressionDistance / MaxSuspensionLength;

			// 스프링 힘 (N) = k * x
			// cm를 m로 변환
			float SpringForce = SpringStiffness * (CompressionDistance * CM_TO_M);

			// 댐핑 힘 (N) = c * v
			FVector WheelVelocity = CarMesh->GetPhysicsLinearVelocityAtPoint(WheelWorldPos);
			float VerticalVelocity = FVector::DotProduct(WheelVelocity, Hit.Normal) * CM_TO_M; // cm/s → m/s
			float DampingForce = SpringDamping * VerticalVelocity;

			// 총 서스펜션 힘 (위쪽 방향)
			float TotalForce = SpringForce - DampingForce;
			FVector SuspensionForce = Hit.Normal * TotalForce;

			// 힘 적용
			CarMesh->AddForceAtLocation(SuspensionForce, WheelWorldPos);

			// 디버그 라인
			if (bShowDebugLines)
			{
				DrawDebugLine(GetWorld(), TraceStart, Hit.ImpactPoint, FColor::Green, false, -1.0f, 0, 2.0f);
				DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.0f, 8, FColor::Yellow, false, -1.0f);
			}
		}
		else
		{
			Wheel.bIsGrounded = false;
			Wheel.SuspensionCompression = 0.0f;

			// 디버그 라인
			if (bShowDebugLines)
			{
				DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, -1.0f, 0, 2.0f);
			}
		}
	}
}

void AMiniCooper::ApplyDriveForces(float DeltaTime)
{
	/*
	 * 타이어 힘 계산:
	 * 1. Slip Ratio 계산 (종방향 미끄러짐)
	 * 2. Slip Angle 계산 (횡방향 미끄러짐)
	 * 3. Pacejka Tire Model로 힘 계산
	 * 4. 바퀴 위치에 힘 적용
	 */

	if (!CarMesh) return;

	FVector ForwardVector = CarMesh->GetForwardVector();
	FVector RightVector = CarMesh->GetRightVector();

	for (FWheelData& Wheel : Wheels)
	{
		if (!Wheel.bIsGrounded) continue;

		// 바퀴 월드 위치 (LocalOffset 기준)
		FVector WheelWorldPos = CarMesh->GetComponentLocation() +
		                        CarMesh->GetComponentRotation().RotateVector(Wheel.LocalOffset);

		// 바퀴 속도
		FVector WheelVelocity = CarMesh->GetPhysicsLinearVelocityAtPoint(WheelWorldPos);

		// 조향 적용 (앞바퀴만)
		FVector WheelForward = ForwardVector;
		FVector WheelRight = RightVector;
		if (Wheel.bIsSteered)
		{
			FRotator SteerRotation(0, CurrentSteeringAngle, 0);
			WheelForward = CarMesh->GetComponentRotation().RotateVector(SteerRotation.RotateVector(FVector::ForwardVector));
			WheelRight = FVector::CrossProduct(FVector::UpVector, WheelForward).GetSafeNormal();
		}

		// 종방향 및 횡방향 속도 분해
		float LongitudinalVelocity = FVector::DotProduct(WheelVelocity, WheelForward);
		float LateralVelocity = FVector::DotProduct(WheelVelocity, WheelRight);

		// ========== 종방향 힘 (Longitudinal Force) ==========
		float LongitudinalForce = 0.0f;

		if (Wheel.bIsDriven && FMath::Abs(ThrottleInput) > 0.01f)
		{
			// 목표 속도 (cm/s)
			float TargetSpeed = ThrottleInput * 2000.0f; // 약 72 km/h

			// Slip Ratio 계산
			float SlipRatio = CalculateSlipRatio(TargetSpeed, LongitudinalVelocity, false);

			// Pacejka 모델로 힘 계산 (N)
			LongitudinalForce = CalculateLongitudinalForce(SlipRatio);
		}

		// 브레이크 힘
		if (BrakeInput > 0.01f)
		{
			float BrakeForce = -FMath::Sign(LongitudinalVelocity) * BrakeTorque * BrakeInput;
			LongitudinalForce += BrakeForce;
		}

		// ========== 횡방향 힘 (Lateral Force) ==========
		// Slip Angle 계산 (라디안)
		float SlipAngle = 0.0f;
		if (FMath::Abs(LongitudinalVelocity) > 10.0f) // 10 cm/s 이상일 때만 계산
		{
			SlipAngle = FMath::Atan2(LateralVelocity, FMath::Abs(LongitudinalVelocity));
		}

		// Pacejka 모델로 횡방향 힘 계산 (N)
		float LateralForce = CalculateLateralForce(SlipAngle);

		// ========== 힘 적용 ==========
		FVector TotalForce = WheelForward * LongitudinalForce + WheelRight * LateralForce;
		CarMesh->AddForceAtLocation(TotalForce, WheelWorldPos);

		// 디버그 화살표
		if (bShowDebugLines)
		{
			DrawDebugDirectionalArrow(GetWorld(), WheelWorldPos, WheelWorldPos + TotalForce * 0.01f,
			                          5.0f, FColor::Cyan, false, -1.0f, 0, 2.0f);
		}
	}
}

void AMiniCooper::ApplyAerodynamicForces(float DeltaTime)
{
	/*
	 * 공기 저항 (Drag Force):
	 * F_drag = 0.5 * ρ * Cd * A * v²
	 *
	 * ρ: 공기 밀도 (1.225 kg/m³)
	 * Cd: 항력 계수 (0.32)
	 * A: 전면 면적 (m²)
	 * v: 속도 (m/s)
	 */

	if (!CarMesh) return;

	FVector Velocity = CarMesh->GetPhysicsLinearVelocity(); // cm/s
	float SpeedCmPerSec = Velocity.Size();
	float SpeedMPerSec = SpeedCmPerSec * CM_TO_M; // m/s

	if (SpeedMPerSec < 0.1f) return;

	// 공기 저항 (N)
	float DragForceMagnitude = 0.5f * AIR_DENSITY * DragCoefficient * FrontalArea * SpeedMPerSec * SpeedMPerSec;

	// 속도 반대 방향으로 힘 적용
	FVector DragForce = -Velocity.GetSafeNormal() * DragForceMagnitude;
	CarMesh->AddForce(DragForce);

	// 다운포스 (일반 차량은 거의 0)
	if (DownforceCoefficient > 0.01f)
	{
		float DownforceMagnitude = 0.5f * AIR_DENSITY * DownforceCoefficient * FrontalArea * SpeedMPerSec * SpeedMPerSec;
		FVector DownforceVector = FVector(0, 0, -DownforceMagnitude);
		CarMesh->AddForce(DownforceVector);
	}
}

void AMiniCooper::UpdateWheelRotations(float DeltaTime)
{
	/*
	 * 바퀴 회전 각속도:
	 * ω = v / r
	 *
	 * v: 선속도 (cm/s)
	 * r: 바퀴 반지름 (cm)
	 * ω: 각속도 (rad/s)
	 */

	if (!CarMesh) return;

	for (FWheelData& Wheel : Wheels)
	{
		// 바퀴 속도 계산 (LocalOffset 기준)
		FVector WheelWorldPos = CarMesh->GetComponentLocation() +
		                        CarMesh->GetComponentRotation().RotateVector(Wheel.LocalOffset);
		FVector WheelVelocity = CarMesh->GetPhysicsLinearVelocityAtPoint(WheelWorldPos);
		FVector ForwardVector = CarMesh->GetForwardVector();
		float LongitudinalVelocity = FVector::DotProduct(WheelVelocity, ForwardVector);

		// 회전 각속도 (rad/s)
		float AngularVelocity = LongitudinalVelocity / WheelRadius;

		// 회전 누적 (도 단위)
		float RotationDelta = FMath::RadiansToDegrees(AngularVelocity * DeltaTime);
		Wheel.CurrentRotation += RotationDelta;

		// 본 회전은 Animation Blueprint에서 처리하는 것을 권장
		// 또는 직접 SetBoneRotationByName() 사용 가능
	}
}

void AMiniCooper::UpdateCamera(float DeltaTime)
{
	if (!BackSpringArm) return;

	// 카메라 Yaw를 부드럽게 전방으로 복귀
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, DeltaTime, 1.0f);
	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

// Pacejka Tire Model
float AMiniCooper::CalculateLongitudinalForce(float SlipRatio) const
{
	/*
	 * Pacejka Tire Model (간단 버전):
	 * - Curve가 있으면 Curve 사용
	 * - 없으면 간단한 비선형 모델
	 *
	 * Slip Ratio 범위: -1 ~ 1
	 * - 0: 순수 구름 (no slip)
	 * - >0: 가속 (wheel speed > vehicle speed)
	 * - <0: 브레이크 (wheel speed < vehicle speed)
	 */

	if (LongitudinalForceCurve)
	{
		// Curve에서 정규화된 힘 가져오기 (0~1)
		float NormalizedForce = LongitudinalForceCurve->GetFloatValue(SlipRatio);
		return NormalizedForce * MaxLongitudinalForce * TireGripMultiplier;
	}
	else
	{
		// 간단한 비선형 모델 (Sin 기반)
		// Peak at SlipRatio ≈ 0.15
		float NormalizedSlip = FMath::Clamp(SlipRatio / 0.15f, -1.0f, 1.0f);
		float Force = FMath::Sin(NormalizedSlip * PI / 2.0f) * MaxLongitudinalForce;
		return Force * TireGripMultiplier;
	}
}

float AMiniCooper::CalculateLateralForce(float SlipAngle) const
{
	/*
	 * 횡방향 힘 (코너링 포스):
	 * Slip Angle이 클수록 타이어가 옆으로 미끄러짐
	 *
	 * Slip Angle 범위: -π/2 ~ π/2 (라디안)
	 * - 0: 직진
	 * - >0: 우회전
	 * - <0: 좌회전
	 */

	if (LateralForceCurve)
	{
		// Curve에서 정규화된 힘 가져오기 (-1~1)
		float NormalizedForce = LateralForceCurve->GetFloatValue(SlipAngle);
		return NormalizedForce * MaxLateralForce * TireGripMultiplier;
	}
	else
	{
		// 간단한 비선형 모델
		// Peak at SlipAngle ≈ 0.2 rad (약 11도)
		float NormalizedAngle = FMath::Clamp(SlipAngle / 0.2f, -1.0f, 1.0f);
		float Force = -FMath::Sin(NormalizedAngle * PI / 2.0f) * MaxLateralForce;
		return Force * TireGripMultiplier;
	}
}

float AMiniCooper::CalculateSlipRatio(float WheelSpeed, float VehicleSpeed, bool bIsBraking) const
{
	/*
	 * Slip Ratio 공식:
	 * σ = (v_wheel - v_vehicle) / |v_vehicle|
	 *
	 * v_wheel: 바퀴 선속도
	 * v_vehicle: 차량 속도
	 */

	if (FMath::Abs(VehicleSpeed) < 1.0f) // 거의 정지 상태
	{
		return FMath::Sign(WheelSpeed) * 0.1f;
	}

	float SlipRatio = (WheelSpeed - VehicleSpeed) / FMath::Abs(VehicleSpeed);
	return FMath::Clamp(SlipRatio, -1.0f, 1.0f);
}

// Getters
float AMiniCooper::GetSpeedKmh() const
{
	if (!CarMesh) return 0.0f;

	FVector Velocity = CarMesh->GetPhysicsLinearVelocity(); // cm/s
	float SpeedCmPerSec = Velocity.Size();
	float SpeedKmh = SpeedCmPerSec * 0.036f; // cm/s → km/h (= * 3.6 / 100)
	return SpeedKmh;
}
float AMiniCooper::GetCurrentRPM() const
{
	// 근사 RPM 계산 (실제 기어비는 고려 안 함)
	float SpeedKmh = GetSpeedKmh();
	float SpeedRatio = SpeedKmh / 200.0f; // 200 km/h를 기준으로 정규화
	float RPM = IdleRPM + (MaxRPM - IdleRPM) * SpeedRatio;
	return FMath::Clamp(RPM, IdleRPM, MaxRPM);
}
