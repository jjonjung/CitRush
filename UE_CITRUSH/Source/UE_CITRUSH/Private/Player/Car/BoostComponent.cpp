
	#include "Player/Car/BoostComponent.h"
	#include "ChaosVehicleMovementComponent.h"
	#include "GameFramework/Pawn.h"
	#include "Net/UnrealNetwork.h"

	DEFINE_LOG_CATEGORY_CLASS(UBoostComponent, Boost)

	UBoostComponent::UBoostComponent()
	{
		PrimaryComponentTick.bCanEverTick = true;
		PrimaryComponentTick.bStartWithTickEnabled = false; // 부스트 미사용 시 Tick 비활성화
		SetIsReplicatedByDefault(true); // 네트워크 복제 활성화
	}

	void UBoostComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

		// 복제할 변수 등록
		DOREPLIFETIME(UBoostComponent, bIsBoostActive);
		DOREPLIFETIME(UBoostComponent, BoostFuel);
		DOREPLIFETIME(UBoostComponent, BoostState);
	}

	void UBoostComponent::BeginPlay()
	{
		Super::BeginPlay();

		AActor* Owner = GetOwner();
		if (!Owner)
		{
			UE_LOG(Boost, Error, TEXT("Owner Null"));
			return;
		}
		VehicleMovement = Owner->FindComponentByClass<UChaosVehicleMovementComponent>();

		Camera = Owner->FindComponentByClass<UCameraComponent>();
		if (!Camera)
		{
			UE_LOG(Boost, Error, TEXT("Owner Camera Null"));
			return;
		}

		//모션 블러 값 백업
		OriginalMotionBlurAmount = Camera->PostProcessSettings.MotionBlurAmount;

		// 공기 저항 값 백업
		if (VehicleMovement)
		{
			OriginalDrag = VehicleMovement->DragCoefficient;
		}

		if (BoostPostProcessMaterial)
		{
			PostProcessMID = UMaterialInstanceDynamic::Create(BoostPostProcessMaterial, this);
			if (PostProcessMID)
			{
				TArray<UCameraComponent*> CameraComponents;
				Owner->GetComponents<UCameraComponent>(CameraComponents);

				for (UCameraComponent* Cam : CameraComponents)
				{
					if (!Cam) continue;

					// 각 카메라의 WeightedBlendables에 PostProcessMID 추가
					bool bAlreadyAdded = false;
					for (int32 i = 0; i < Cam->PostProcessSettings.WeightedBlendables.Array.Num(); ++i)
					{
						if (Cam->PostProcessSettings.WeightedBlendables.Array[i].Object == PostProcessMID)
						{
							bAlreadyAdded = true;
							break;
						}
					}

					if (!bAlreadyAdded)
					{
						FWeightedBlendable NewBlendable;
						NewBlendable.Object = PostProcessMID;
						NewBlendable.Weight = 0.0f;

						// 배열에 추가
						Cam->PostProcessSettings.WeightedBlendables.Array.Add(NewBlendable);
					}
				}
			}
			else
			{
					UE_LOG(Boost, Error, TEXT("MaterialInstanceDynamic 생성 실패"));
			}
		}
		else
		{
			UE_LOG(Boost, Error, TEXT("BoostPostProcessMaterial null"));
		}

		//Timeline 
		if (BoostCurve)
		{
			FOnTimelineFloat TimelineUpdateDelegate;
			TimelineUpdateDelegate.BindUFunction(this, FName("OnTimelineUpdate"));
			BoostTimeline.AddInterpFloat(BoostCurve, TimelineUpdateDelegate);

			// 완료
			FOnTimelineEvent TimelineFinishedDelegate;
			TimelineFinishedDelegate.BindUFunction(this, FName("OnTimelineFinished"));
			BoostTimeline.SetTimelineFinishedFunc(TimelineFinishedDelegate);

			// 설정
			BoostTimeline.SetLooping(false);
			BoostTimeline.SetTimelineLength(TimelineLength);

		}
		else
		{
			UE_LOG(Boost, Warning, TEXT("UBoosterComponent: BoostCurve가 할당되지 않았습니다. Timeline을 사용할 수 없습니다."));
		}
		
		BoostFuel = MaxBoostFuel;
	}

	void UBoostComponent::OnTimelineUpdate(float Value)
	{
		// 현재 활성화된 카메라 동적 검색
		UCameraComponent* ActiveCamera = GetActiveCamera();
		if (!ActiveCamera)
		{
			return;
		}

		CurrentBoostWeight = Value;

		// FOV 계산 및 최대값 BoostFOV 제한
		const float NewFOV = FMath::Min(FMath::Lerp(NormalFOV, BoostFOV, CurrentBoostWeight), BoostFOV);
		ActiveCamera->SetFieldOfView(NewFOV);

		ActiveCamera->PostProcessSettings.bOverride_MotionBlurAmount = true;
		ActiveCamera->PostProcessSettings.MotionBlurAmount = FMath::Lerp(OriginalMotionBlurAmount, BoostMotionBlurAmount, CurrentBoostWeight);

		const float ScaledWeight = CurrentBoostWeight * MaxPostProcessWeight;
		UpdatePostProcessWeight(ScaledWeight);
	}

	  void UBoostComponent::OnTimelineFinished()
	  {
	        UCameraComponent* ActiveCamera = GetActiveCamera();
	        if (!ActiveCamera)
	        {
	                return;
	        }

	        // 부스터가 비활성화 상태이고 Weight가 0에 도달했다면 정리 작업 수행
			// Motion Blur 오버라이드 해제 및 원본 값 복원
			// 0.0에서 0.5로 점진적 증가 후 원상복구(고속 주행 시 역동적인 시각 효과)
	        if (!bIsBoosting && FMath::IsNearlyZero(CurrentBoostWeight, 0.01f))
	        {
	                
	                ActiveCamera->PostProcessSettings.bOverride_MotionBlurAmount = false;
	                ActiveCamera->PostProcessSettings.MotionBlurAmount = OriginalMotionBlurAmount;
	        }
	  }

	  void UBoostComponent::UpdatePostProcessWeight(float Weight)
	  {
	        UCameraComponent* ActiveCamera = GetActiveCamera();
	        if (!ActiveCamera || !PostProcessMID)
	        {
	                return;
	        }

	        for (int32 i = 0; i < ActiveCamera->PostProcessSettings.WeightedBlendables.Array.Num(); ++i)
	        {
	                if (ActiveCamera->PostProcessSettings.WeightedBlendables.Array[i].Object == PostProcessMID)
	                {
	                        ActiveCamera->PostProcessSettings.WeightedBlendables.Array[i].Weight = Weight;
	                        return;
	                }
	        }

	  }

	void UBoostComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

		// Timeline을 Tick하지 않으면 FOV/모션블러/PostProcess 효과가 업데이트되지 않음
		BoostTimeline.TickTimeline(DeltaTime);
		UpdateBoostState(DeltaTime);

		if (bIsBoostActive && BoostState == EBoostState::Active)
		{
			ApplyBoostForce();
		}
	}

	// 부스터(start)
	void UBoostComponent::ActivateBoost()
	{
		if (BoostState != EBoostState::Ready || BoostFuel <= 0.f)
		{
			UE_LOG(Boost, Warning, TEXT("[ActivateBoost FAILED] State:%d (Need Ready=0), Fuel:%.1f (Need >0)"),
				(int32)BoostState, BoostFuel);
			return;
		}

		if (!bAllowInAir && VehicleMovement)
		{
			FVector Velocity = VehicleMovement->GetOwner()->GetVelocity();
			if (FMath::Abs(Velocity.Z) > 100.f)
			{
				UE_LOG(Boost, Warning, TEXT("[ActivateBoost FAILED] Cannot use in air - Z velocity:%.1f"), Velocity.Z);
				return;
			}
		}

		bIsBoostActive = true;
		BoostState = EBoostState::Active;
		SetComponentTickEnabled(true); // 부스트 활성화 시 Tick 시작

		if (!BoostCurve)
		{
			UE_LOG(Boost, Warning, TEXT("[ActivateBoost FAILED] BoostCurve is NULL - Set in Blueprint!"));
			return;
		}

		bIsBoosting = true;
		BoostTimeline.PlayFromStart();

		// 공기 저항 감소 (감속을 느리게 함)
		if (VehicleMovement)
		{
			VehicleMovement->DragCoefficient = OriginalDrag * 0.01f;
		}

		//UE_LOG(Boost, Log, TEXT("[ActivateBoost SUCCESS] Speed:%.1f km/h, Fuel:%.1f%%"), Speed, BoostFuel);
	}

	void UBoostComponent::DeactivateBoost()
	{
		if (bIsBoostActive)
		{
			bIsBoostActive = false;
			BoostState = EBoostState::Cooldown;
			CooldownTimer = BoostCooldown;

			// 공기 저항 원상 복구
			if (VehicleMovement)
			{
				VehicleMovement->DragCoefficient = OriginalDrag;
			}

			UE_LOG(Boost, Warning, TEXT("UBoosterComponent: 부스터 비활성화 시작"));
		}

		if (!BoostCurve)
		{
			UE_LOG(Boost, Warning, TEXT("[UBoosterComponent] DeactivateBoost: BoostCurve가 없습니다."));
			return;
		}

		bIsBoosting = false;
		BoostTimeline.Stop(); // Timeline을 즉시 중단하고 원래 상태로 복귀

		UCameraComponent* ActiveCamera = GetActiveCamera();
		if (ActiveCamera)
		{
			ActiveCamera->SetFieldOfView(NormalFOV);
			ActiveCamera->PostProcessSettings.bOverride_MotionBlurAmount = false;
			ActiveCamera->PostProcessSettings.MotionBlurAmount = OriginalMotionBlurAmount;

			UpdatePostProcessWeight(0.0f);

		}

		CurrentBoostWeight = 0.0f;
		SetComponentTickEnabled(false); // 부스트 종료 시 Tick 비활성화
	}

	void UBoostComponent::ServerRPC_ActivateBoost_Implementation()
	{
		// 서버에서 실행 - 권위적 부스터 활성화
		ActivateBoost();
	}

	void UBoostComponent::ServerRPC_DeactivateBoost_Implementation()
	{
		// 서버에서 실행 - 권위적 부스터 비활성화
		DeactivateBoost();
	}

	void UBoostComponent::RefillBoost(float Amount)
	{
		BoostFuel = FMath::Clamp(BoostFuel + Amount, 0.f, MaxBoostFuel);
	}

	void UBoostComponent::ApplyBoostForce()
	{
		AActor* Owner = GetOwner();
		if (!Owner) return;

		// 클라이언트 예측: 서버 OR 로컬 제어 클라이언트만 물리 힘 적용
		bool bIsServer = Owner->HasAuthority();
		bool bIsLocallyControlled = false;

		if (APawn* OwnerPawn = Cast<APawn>(Owner))
		{
			bIsLocallyControlled = OwnerPawn->IsLocallyControlled();
		}

		// 서버 또는 로컬 제어 클라이언트만 물리 힘 적용 (클라이언트 예측)
		// 다른 플레이어의 차량은 서버 복제 결과만 따름
		if (!bIsServer && !bIsLocallyControlled)
		{
			return;
		}

		if (!VehicleMovement) return;

		// 210km/h 초과 시 부스트 힘 적용 중단
		float CurrentSpeedKmh = FMath::Abs(VehicleMovement->GetForwardSpeed()) * 0.036f;
		if (CurrentSpeedKmh >= 210.0f) return;

		FVector ForwardVector = Owner->GetActorForwardVector();

		// 속도가 낮을수록 더 강한 힘을 가해 가속감 부여
		float BoostMultiplier = FMath::Lerp(1.5f, 0.5f, CurrentSpeedKmh / 210.0f);
		FVector BoostImpulse = ForwardVector * BoostForce * BoostMultiplier;

		UPrimitiveComponent* UpdatedPrimitive = Cast<UPrimitiveComponent>(VehicleMovement->UpdatedComponent);
		if (UpdatedPrimitive && UpdatedPrimitive->IsSimulatingPhysics())
		{
			UpdatedPrimitive->AddForce(BoostImpulse, NAME_None, true);
		}
	}

	void UBoostComponent::UpdateBoostState(float DeltaTime)
	{
		switch (BoostState)
		{
			case EBoostState::Active:
				BoostFuel -= BoostConsumptionRate * DeltaTime;
				if (BoostFuel <= 0.f)
				{
					BoostFuel = 0.f;
					DeactivateBoost();
					BoostState = EBoostState::Depleted;
				}
				break;

			case EBoostState::Cooldown:
				CooldownTimer -= DeltaTime;
				if (CooldownTimer <= 0.f)
				{
					BoostState = (BoostFuel > 0.f) ? EBoostState::Ready : EBoostState::Depleted;
					if (BoostState == EBoostState::Ready)
					{
						SetComponentTickEnabled(false);
					}
				}
				break;

			case EBoostState::Depleted:
				if (BoostFuel >= MaxBoostFuel * 0.1f) // 10% 이상 충전 시 Ready
				{
					BoostState = EBoostState::Ready;
					SetComponentTickEnabled(false);
				}
				break;

			default:
				break;
		}
	}

	UCameraComponent* UBoostComponent::GetActiveCamera()
	{
		AActor* Owner = GetOwner();
		if (!Owner)
		{
			return nullptr;
		}

		TArray<UCameraComponent*> CameraComponents;
		Owner->GetComponents<UCameraComponent>(CameraComponents);

		for (UCameraComponent* Cam : CameraComponents)
		{
			if (Cam && Cam->IsActive())
			{
				return Cam;
			}
		}

		return nullptr;
	}

