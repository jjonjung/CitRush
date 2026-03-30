
#include "Player/Car/VehicleDemoCejCar.h"
#include "VehicleTemplate/UE_CITRUSHSportsWheelFront.h"
#include "VehicleTemplate/UE_CITRUSHSportsWheelRear.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Player/Car/VehicleDemoUITest.h"
#include "Player/Car/BoostComponent.h"
#include "Player/Car/SideBrakeComponent.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "GAS/AttributeSet/ASRacer.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"          
#include "NiagaraFunctionLibrary.h"
#include "Misc/OutputDeviceNull.h"
#include "Player/CitRushPlayerState.h"

DEFINE_LOG_CATEGORY_CLASS(AVehicleDemoCejCar, CEJCarLog)

AVehicleDemoCejCar::AVehicleDemoCejCar()
{
	// ========== 멀티플레이 네트워크 최적화 ==========
	// 업데이트 빈도 감소 (기본 100Hz → 20Hz)
	NetUpdateFrequency = 20.0f;      // 초당 20번 업데이트 (네트워크 부하 80% 감소)
	MinNetUpdateFrequency = 10.0f;   // 최소 10Hz

	// 거리별 업데이트 최적화 (먼 거리는 업데이트 안함)
	NetCullDistanceSquared = 10000000.0f; // 1km 이상은 업데이트 생략

	// 컴포넌트 생성
	Boost = CreateDefaultSubobject<UBoostComponent>(TEXT("Boost"));
	SideBrake = CreateDefaultSubobject<USideBrakeComponent>(TEXT("SideBrake"));
	shieldSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
	shieldSphere->SetupAttachment(GetMesh());
	ConstructorHelpers::FObjectFinder<UStaticMesh> mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (mesh.Succeeded())
	{
		shieldSphere->SetStaticMesh(mesh.Object);
	}
	ConstructorHelpers::FObjectFinder<UMaterial> matAsset(TEXT("/Game/CITRUSH/CEJ/M_Shield"));
	if (matAsset.Succeeded())
	{
		shieldMat = matAsset.Object;
	}
	shieldSphere->SetMaterial(0, shieldMat);
	shieldSphere->GetStaticMesh()->SetExtendedBounds(GetMesh()->GetPlacementExtent());

	// ========== 미니 쿠퍼 S 기준 차체 설정 ==========
	GetChaosWheeledVehicleMovement()->ChassisHeight = 144.0f;      // 차체 높이 (미니 쿠퍼: 낮은 중심)
	GetChaosWheeledVehicleMovement()->DragCoefficient = DragCoefficient; // 공기 저항 계수 (0.32)

	// 휠 설정
	GetChaosWheeledVehicleMovement()->bLegacyWheelFrictionPosition = true;  // 기존 방식의 휠 마찰 위치 사용 여부
	GetChaosWheeledVehicleMovement()->WheelSetups.SetNum(4);                // 총 4개의 휠 사용

	// 앞 왼쪽 휠
	GetChaosWheeledVehicleMovement()->WheelSetups[0].WheelClass = UUE_CITRUSHSportsWheelFront::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[0].BoneName = FName("Phys_Wheel_FL");
	GetChaosWheeledVehicleMovement()->WheelSetups[0].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	// 앞 오른쪽 휠
	GetChaosWheeledVehicleMovement()->WheelSetups[1].WheelClass = UUE_CITRUSHSportsWheelFront::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[1].BoneName = FName("Phys_Wheel_FR");
	GetChaosWheeledVehicleMovement()->WheelSetups[1].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	// 뒷 왼쪽 휠
	GetChaosWheeledVehicleMovement()->WheelSetups[2].WheelClass = UUE_CITRUSHSportsWheelRear::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[2].BoneName = FName("Phys_Wheel_BL");
	GetChaosWheeledVehicleMovement()->WheelSetups[2].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	// 뒷 오른쪽 휠
	GetChaosWheeledVehicleMovement()->WheelSetups[3].WheelClass = UUE_CITRUSHSportsWheelRear::StaticClass();
	GetChaosWheeledVehicleMovement()->WheelSetups[3].BoneName = FName("Phys_Wheel_BR");
	GetChaosWheeledVehicleMovement()->WheelSetups[3].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	// ========== 미니 쿠퍼 S 엔진 설정 ==========
	// 고속 주행(210km/h)을 위해 토크 상향 조정
	GetChaosWheeledVehicleMovement()->EngineSetup.MaxTorque = 5000.0f;     // 최대 토크 대폭 상향 (기존 1000)
	GetChaosWheeledVehicleMovement()->EngineSetup.MaxRPM = 18000.0f;          // 최대 RPM 상향
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineIdleRPM = 900.0f;    // 아이들 RPM
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineBrakeEffect = 0.2f; // 엔진 브레이크 완화 (고속 유지)
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineRevUpMOI = 1.5f;     // 더 빠른 RPM 상승
	GetChaosWheeledVehicleMovement()->EngineSetup.EngineRevDownRate = 400.0f;// RPM 감소 속도 완화

	// ========== 미니 쿠퍼 S 변속기 설정 (6단 자동) ==========
	GetChaosWheeledVehicleMovement()->TransmissionSetup.bUseAutomaticGears = true;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.bUseAutoReverse = true;
	GetChaosWheeledVehicleMovement()->TransmissionSetup.FinalRatio = 3.65f;        // 최종 기어비 (미니 쿠퍼)
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ChangeUpRPM = 6500.0f;     // 업쉬프트 (고회전)
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ChangeDownRPM = 2500.0f;   // 다운쉬프트
	GetChaosWheeledVehicleMovement()->TransmissionSetup.GearChangeTime = 0.15f;    // 빠른 변속 (스포츠카)
	GetChaosWheeledVehicleMovement()->TransmissionSetup.TransmissionEfficiency = 0.92f; // 효율

	// 전진 기어비 (6단) - 미니 쿠퍼 S 기준
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios.SetNum(6);
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[0] = 3.31f; // 1단
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[1] = 2.05f; // 2단
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[2] = 1.41f; // 3단
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[3] = 1.03f; // 4단
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[4] = 0.81f; // 5단
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ForwardGearRatios[5] = 0.67f; // 6단 (고속 주행)

	// 후진 기어비
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ReverseGearRatios.SetNum(1);
	GetChaosWheeledVehicleMovement()->TransmissionSetup.ReverseGearRatios[0] = 3.20f;

	// ========== 미니 쿠퍼 S 스티어링 설정 ==========
	// 민첩한 핸들링 특화
	GetChaosWheeledVehicleMovement()->SteeringSetup.SteeringType = ESteeringType::Ackermann;
	GetChaosWheeledVehicleMovement()->SteeringSetup.AngleRatio = 0.5f; // 민첩한 조향 (미니 쿠퍼 특성)
	
}

void AVehicleDemoCejCar::BeginPlay()
{
	Super::BeginPlay();

	// ========== 멀티플레이 물리 최적화 ==========
	// 클라이언트는 간소화된 물리 시뮬레이션 (서버 위치만 따라감)
	if (!HasAuthority() && !IsLocallyControlled())
	{
		// 다른 플레이어의 차량: 키네마틱 모드
		if (GetMesh())
		{
			GetMesh()->SetSimulatePhysics(false);
			GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}

		// ChaosVehicleMovement 비활성화 (CPU 사용량 감소)
		if (UChaosWheeledVehicleMovementComponent* WheeledMovement = GetChaosWheeledVehicleMovement())
		{
			WheeledMovement->SetComponentTickEnabled(false);
		}
	}

	// 충돌 감지를 위해 Mesh의 Overlap 이벤트 활성화 (CoinActor 획득을 위해 필수)
	if (GetMesh())
	{
		GetMesh()->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
		GetMesh()->SetGenerateOverlapEvents(true);
		GetMesh()->SetNotifyRigidBodyCollision(true); // Hit Event 발생 (코인 획득용)
	}

	// 드라이브 타입 설정 (FWD/RWD/AWD)
	SetDriveType(DriveType);

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		RootPrimitive->SetMassOverrideInKg(NAME_None, 2500.0f, true);
		RootPrimitive->SetAngularDamping(0.5f);
	}

	// 카메라 활성화 확인 및 강제 활성화
	TArray<UCameraComponent*> AllCameras;
	GetComponents<UCameraComponent>(AllCameras);

	bool bHasActiveCamera = false;
	UCameraComponent* BackCam = nullptr;

	for (UCameraComponent* Cam : AllCameras)
	{
		if (Cam)
		{
			if (Cam->IsActive())
			{
				bHasActiveCamera = true;
			}

			if (Cam->GetName().Contains(TEXT("Back")) && !Cam->GetName().Contains(TEXT("Side")))
			{
				BackCam = Cam;
			}
		}
	}

	if (!bHasActiveCamera && BackCam)
	{
		BackCam->SetActive(true);
	}

	// HUD 생성 시도 (BeginPlay)
	TryCreateHUD();
	
	shieldSphere->SetCastShadow(false);
	shieldSphere->bVisibleInRayTracing = false;
	shieldSphere->SetVisibility(true, true);
	shieldSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	shieldSphere->SetSimulatePhysics(false);

	// 아이템 슬롯 변경 감지하여 부스트 아이템 처리
	OnItemSlotChanged.AddDynamic(this, &AVehicleDemoCejCar::HandleItemSlotChanged);
}

void AVehicleDemoCejCar::HandleItemSlotChanged(UItemData* FrontItem, UItemData* BackItem)
{
	// 부스트 아이템이 있는지 확인
	if (FrontItem) // && FrontItem->ID == FName("boost") // ID 체크는 필요 시 주석 해제
	{
		// 텍스처 로드 (Visible = true 효과)
		UTexture2D* BoostTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/CITRUSH/Commender/Tex/Boost.Boost"));
		if (!BoostTexture)
		{
			BoostTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/CITRUSH/Commender/Tex/Boost"));
		}

		if (BoostTexture)
		{
			UE_LOG(CEJCarLog, Log, TEXT("[VehicleDemoCejCar] Boost Item Received! Texture Loaded: %s"), *BoostTexture->GetName());
		}
	}
	else
	{
		// 아이템이 없으면 (Visible = hidden 효과)
		UE_LOG(CEJCarLog, Log, TEXT("[VehicleDemoCejCar] Front Item is empty."));
	}
}

void AVehicleDemoCejCar::SetDriveType(EDriveType NewDriveType)
{
	DriveType = NewDriveType;

	if (UChaosWheeledVehicleMovementComponent* WheeledMovement = Cast<UChaosWheeledVehicleMovementComponent>(GetChaosWheeledVehicleMovement()))
	{
		switch (DriveType)
		{
		case EDriveType::FWD: // 전륜 구동
			WheeledMovement->DifferentialSetup.DifferentialType = EVehicleDifferential::FrontWheelDrive;
			break;

		case EDriveType::RWD: // 후륜 구동
			WheeledMovement->DifferentialSetup.DifferentialType = EVehicleDifferential::RearWheelDrive;
			break;

		case EDriveType::AWD: // 사륜 구동
			WheeledMovement->DifferentialSetup.DifferentialType = EVehicleDifferential::AllWheelDrive;
			WheeledMovement->DifferentialSetup.FrontRearSplit = 0.5f;// AWD 토크 분배 (전륜:후륜 = 50:50)
			break;
		}

		if (GetMesh() && GetMesh()->IsSimulatingPhysics())
		{
			WheeledMovement->RecreatePhysicsState();
		}

	}
}

void AVehicleDemoCejCar::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (BoostAction)
		{
			EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Started, this, &AVehicleDemoCejCar::OnBoostPressed);
			EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Completed, this, &AVehicleDemoCejCar::OnBoostReleased);
		}

		if (SideBrakeAction)
		{
			EnhancedInputComponent->BindAction(SideBrakeAction, ETriggerEvent::Started, this, &AVehicleDemoCejCar::OnSideBrakePressed);
			EnhancedInputComponent->BindAction(SideBrakeAction, ETriggerEvent::Completed, this, &AVehicleDemoCejCar::OnSideBrakeReleased);
		}
		
	}
}

void AVehicleDemoCejCar::UpdateCompass()
{
	UCameraComponent* CurrentBackCamera = GetBackCamera();

	if (CarHUDWidget && CurrentBackCamera)
	{
		FRotator CameraRotation = CurrentBackCamera->GetComponentRotation();
		float ZRotation = CameraRotation.Yaw;
		FOutputDeviceNull ar;
		FString Command = FString::Printf(TEXT("RotateCompass %f"), ZRotation);
		CarHUDWidget->CallFunctionByNameWithArguments(*Command, ar, NULL, true);
	}
}

void AVehicleDemoCejCar::NetMulticastRPC_Damaged_Implementation(FVector repelLocation, FVector repelDirection)
{
	if (untouchableTime > 0.f) {return;}
	
	Super::NetMulticastRPC_Damaged_Implementation(repelLocation, repelDirection);
}

void AVehicleDemoCejCar::ApplyShield()
{
	shieldSphere->SetVisibility(true, true);
}

void AVehicleDemoCejCar::ApplyReducedVision_Implementation()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	UCameraComponent* TargetCam = nullptr;
	
	// 활성화된 카메라 찾기
	TArray<UCameraComponent*> AllCameras;
	GetComponents<UCameraComponent>(AllCameras);
	for (UCameraComponent* Cam : AllCameras)
	{
		if (Cam && Cam->IsActive())
		{
			TargetCam = Cam;
			break;
		}
	}

	if (TargetCam)
	{
		// 원래 FOV 저장
	    float OriginalFOV = 90.0f; 
	    TargetCam->SetFieldOfView(20.0f);

	    // 나이아가라 이펙트 로드
	    UNiagaraSystem* WaveEffect = Cast<UNiagaraSystem>(StaticLoadObject(
	        UNiagaraSystem::StaticClass(), 
	        nullptr, 
	        TEXT("/Script/Niagara.NiagaraSystem'/Game/CITRUSH/CEJ/Car/N_Wave.N_Wave'")));

	    UNiagaraComponent* SpawnedWaveComponent = nullptr;

	    if (WaveEffect)
	    {
	        // 카메라 위치에서 전방 150 유닛 앞에 스폰
	        FVector SpawnLocation = TargetCam->GetComponentLocation() + (TargetCam->GetForwardVector() * 150.0f);
	        
	        // 회전은 카메라의 회전 사용 (GetForwardRotation() → GetComponentRotation()으로 수정)
	        FRotator SpawnRotation = TargetCam->GetComponentRotation();

	        SpawnedWaveComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
	            GetWorld(),
	            WaveEffect,
	            SpawnLocation,
	            SpawnRotation,
	            FVector(2.0f),   // 스케일
	            false,           // bAutoDestroy = false (수동으로 Deactivate 하기 위해)
	            true             // bAutoActivate
	        );
	    }

	    // UI 메시지
	    if (CarHUDWidget)
	    {
	        CarHUDWidget->ShowStateMessage(TEXT("이빨이 point 획득, 시야 감소"));
	    }

	    // Throttle 차단 플래그 활성화 (클래스에 bool bIsThrottleBlocked; 선언 필요)
	    bIsThrottleBlocked = true;

	    // 2.5초 후 복구
	    FTimerHandle RestoreTimer;
	    GetWorld()->GetTimerManager().SetTimer(RestoreTimer, [this, TargetCam, OriginalFOV, SpawnedWaveComponent]()
	    {
	        // FOV 복원
	        if (IsValid(TargetCam))
	        {
	            TargetCam->SetFieldOfView(OriginalFOV);
	            UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoCejCar] Vision Restored! FOV: %.1f"), OriginalFOV);
	        }

	        // Throttle 입력 복구
	        bIsThrottleBlocked = false;

	        // 나이아가라 이펙트 자연스럽게 사라지게
	        if (IsValid(SpawnedWaveComponent))
	        {
	            SpawnedWaveComponent->Deactivate();  // 파티클이 서서히 사라짐
	            // 만약 즉시 사라지게 하고 싶다면:
	            // SpawnedWaveComponent->DestroyComponent();
	        }

	    }, 2.5f, false);  // 2.5초 후 실행
	}
}


void AVehicleDemoCejCar::Throttle(const FInputActionValue& Value)
{
	// 시야 감소 디버프 중 스로틀 차단
	if (bIsThrottleBlocked)
	{
		return;
	}

	// 부모 클래스의 정상적인 Throttle 로직 실행
	Super::Throttle(Value);
}

void AVehicleDemoCejCar::CreateCarHUDWidget()
{
	if (CarHUDWidget)
	{
		return;
	}

	APlayerController* PC = GetController<APlayerController>();
	if (!PC)
	{
		return;
	}

	UClass* WidgetClass = LoadClass<UVehicleDemoUITest>(nullptr, TEXT("/Game/CITRUSH/CEJ/Car/WBP_CarHUD.WBP_CarHUD_C"));

	if (WidgetClass)
	{
		CarHUDWidget = CreateWidget<UVehicleDemoUITest>(PC, WidgetClass);
	}
	else
	{
		//UE_LOG(CEJCarLog, Warning, TEXT("[CreateCarHUDWidget] Failed to load WBP_CarHUD. Falling back to UVehicleDemoUITestclass."));
		if (UVehicleDemoUITestclass)
		{
			CarHUDWidget = CreateWidget<UVehicleDemoUITest>(PC, UVehicleDemoUITestclass);
		}
	}

	if (CarHUDWidget)
	{
		CarHUDWidget->AddToViewport();
	}
}

void AVehicleDemoCejCar::OnBoostPressed()
{
	if (Boost)
	{
		// 서버로 부스터 활성화 요청
		Boost->ServerRPC_ActivateBoost();
		bIsBoosting = true;
	}
}

void AVehicleDemoCejCar::OnBoostReleased()
{
	if (Boost)
	{
		// 서버로 부스터 비활성화 요청
		Boost->ServerRPC_DeactivateBoost();
		bIsBoosting = false;
	}
}

void AVehicleDemoCejCar::OnSideBrakePressed()
{
	if (SideBrake)
	{
		// 서버로 사이드 브레이크 활성화 요청
		SideBrake->ServerRPC_SetSideBrake(true);
	}
}

void AVehicleDemoCejCar::OnSideBrakeReleased()
{
	if (SideBrake)
	{
		// 서버로 사이드 브레이크 비활성화 요청
		SideBrake->ServerRPC_SetSideBrake(false);
	}
}

void AVehicleDemoCejCar::DrainFuel()
{
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		GetWorld()->GetTimerManager().ClearTimer(FuelDrainTimer);
		return;
	}

	UASRacer* RacerAS = Cast<UASRacer>(GetAttributeSet());
	if (!RacerAS)
	{
		GetWorld()->GetTimerManager().ClearTimer(FuelDrainTimer);
		return;
	}

	float CurrentFuel = RacerAS->GetFuel();
	float MaxFuel = RacerAS->GetMaxFuel();

	if (CurrentFuel <= 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(FuelDrainTimer);
		UpdateVehiclePerformanceByFuel();// 차량 성능 제한
		return;
	}

	float DrainAmount = MaxFuel * 0.01f;
	float NewFuel = FMath::Max(CurrentFuel - DrainAmount, 0.0f);

	RacerAS->SetFuel(NewFuel);
	OnFuelChanged.Broadcast(NewFuel, MaxFuel);
	UpdateVehiclePerformanceByFuel();

}

void AVehicleDemoCejCar::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	TryCreateHUD();
}

// 클라이언트에서 PlayerState가 복제되었을 때 HUD 생성 시도
void AVehicleDemoCejCar::TryCreateHUD()
{
	if (IsLocallyControlled())
	{
		ACitRushPlayerState* CitRushPS = GetPlayerState<ACitRushPlayerState>();
		
		if (!CitRushPS)
		{
			//UE_LOG(CEJCarLog, Log, TEXT("[TryCreateHUD] PlayerState not ready yet."));
			return;
		}

		if (CitRushPS->GetPlayerRole() == EPlayerRole::Racer)
		{
			CreateCarHUDWidget();
		}
		else
		{
			//UE_LOG(CEJCarLog, Log, TEXT("[TryCreateHUD] Role is %d, not Racer."), (int32)CitRushPS->GetPlayerRole());
		}
	}
}

void AVehicleDemoCejCar::UpdateVehiclePerformanceByFuel()
{
	UASRacer* RacerAS = Cast<UASRacer>(GetAttributeSet());
	if (!RacerAS)
	{
		return;
	}

	float CurrentFuel = RacerAS->GetFuel();
	float MaxFuel = RacerAS->GetMaxFuel();

	UChaosWheeledVehicleMovementComponent* WheeledMovement = GetChaosWheeledVehicleMovement();
	if (!WheeledMovement)
	{
		return;
	}

	// UI 업데이트는 로컬 플레이어에서만 실행
	if (CarHUDWidget && IsLocallyControlled())
	{
		CarHUDWidget->ShowFuelWarning();
	}

	// 차량 성능 제한은 서버에서만 실행 (권위적 제어)
	if (!HasAuthority())
	{
		return;
	}

	// 연료 10 이하일 때 속도 10km/h 제한
	if (CurrentFuel <= 10.0f)
	{
		float CurrentSpeedKmh = WheeledMovement->GetForwardSpeed() * 0.036f;
		if (CurrentSpeedKmh >= 10.0f)
		{
			WheeledMovement->SetThrottleInput(0.0f);
		}
	}

	/*// 연료 완전 고갈 시
	if (CurrentFuel <= 0.0f)
	{
		WheeledMovement->SetThrottleInput(0.0f);
	}*/
}
