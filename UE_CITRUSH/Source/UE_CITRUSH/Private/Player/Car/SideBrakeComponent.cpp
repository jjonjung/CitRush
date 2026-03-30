
#include "Player/Car/SideBrakeComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

USideBrakeComponent::USideBrakeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true); // 네트워크 복제 활성화
}

void USideBrakeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 복제할 변수 등록
	DOREPLIFETIME(USideBrakeComponent, bIsSideBrakeApplied);
}

void USideBrakeComponent::BeginPlay()
{
	Super::BeginPlay();

	FindVehicleMovement();

	if (VehicleMovement)
	{
		InitializeHandbrakeSettings();
	}
}

void USideBrakeComponent::FindVehicleMovement()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (OwnerPawn)
	{
		VehicleMovement = OwnerPawn->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();

		if (!VehicleMovement)
		{
			UE_LOG(LogTemp, Error, TEXT("SideBrakeComponent: VehicleMovementComponent not found on %s"),
				*GetOwner()->GetName());
		}
	}
}

void USideBrakeComponent::InitializeHandbrakeSettings()
{
	if (!VehicleMovement)
	{
		return;
	}

	// 후륜에만 강력한 핸드브레이크 토크 설정
	VehicleMovement->SetWheelHandbrakeTorque(2, HandbrakeTorque);  // 좌측 후륜
	VehicleMovement->SetWheelHandbrakeTorque(3, HandbrakeTorque);  // 우측 후륜

	// 전륜은 핸드브레이크 영향 받지 않도록 설정
	VehicleMovement->SetAffectedByHandbrake(0, false);  // 좌측 전륜
	VehicleMovement->SetAffectedByHandbrake(1, false);  // 우측 전륜

	// 후륜만 핸드브레이크 영향 받도록 설정
	VehicleMovement->SetAffectedByHandbrake(2, true);   // 좌측 후륜
	VehicleMovement->SetAffectedByHandbrake(3, true);   // 우측 후륜

	// 원래 후륜 마찰 계수 저장 (복원용)
	OriginalRearLeftFriction = 1.0f;
	OriginalRearRightFriction = 1.0f;

	UE_LOG(LogTemp, Log, TEXT("SideBrakeComponent: Initialized - Handbrake Torque: %.0f Nm, Rear wheels only"),
		HandbrakeTorque);
}

void USideBrakeComponent::SetSideBrakeApplied(bool bApplied)
{
	//Chaos Vehicle에서는 각 휠별로 구동력(Drive Torque)과 브레이크 토크를 설정 가능?
	if (!VehicleMovement) return;
	
	if (bIsSideBrakeApplied == bApplied)
	{
		return; // 중복 호출 방지
	}

	bIsSideBrakeApplied = bApplied;

	// Chaos Vehicle 핸드브레이크 입력,  설정
	VehicleMovement->SetHandbrakeInput(bApplied);
	VehicleMovement->SetThrottleInput(CurrentThrottleInput); //차량의 스로틀 값 전달

	// 엔진 토크 차단
	/*if (bCutThrottleOnBrake)
	{
		if (bApplied)
		{
			VehicleMovement->SetThrottleInput(0.0f);
			UE_LOG(LogTemp, Log, TEXT("SideBrake: Throttle CUT - Engine power disabled"));
		}
	}*/

	// 후륜 마찰 조정
	if (bReduceRearFrictionOnBrake)
	{
		AdjustRearFriction(bApplied);
	}

	/*전륜만 잠그고 싶을 때!!!!!!!!!!!!
	논리적으로는 WheelSetup에서 전륜 휠만 AffectedByHandbrake = true로 설정
	문제는 전륜만 잠그면 후륜은 구동력 때문에 계속 굴러감 → 차량이 회전하지 않고 그냥 브레이크 느낌이 됨
	드리프트/회전용으로는 후륜만 잠금이 더 자연스러움
	즉, 기술적으로 전륜만 핸드브레이크 잠금 가능하지만,
	드리프트용으로는 전륜 잠금은 거의 사용하지 않음
	전륜 잠금 + 후륜 스로틀 입력 → 차량이 이상하게 동작 가능 */

	if (!bApplied)
	{
		UE_LOG(LogTemp, Log, TEXT("[USideBrakeComponent]: RELEASED - Normal driving resumed"));
	}
}

void USideBrakeComponent::ServerRPC_SetSideBrake_Implementation(bool bApplied)
{
	// 서버에서 실행 - 권위적 사이드 브레이크 설정
	SetSideBrakeApplied(bApplied);
}

void USideBrakeComponent::AdjustRearFriction(bool bReduce)
{
	if (!VehicleMovement)
	{
		return;
	}

	if (bReduce)
	{
		// 드리프트 유도: 후륜 마찰 감소
		VehicleMovement->SetWheelFrictionMultiplier(2, DriftFrictionMultiplier);
		VehicleMovement->SetWheelFrictionMultiplier(3, DriftFrictionMultiplier);
		// 토크 증강 효과는 Throttle 유지로 간접 구현
		VehicleMovement->SetThrottleInput(CurrentThrottleInput); 
		UE_LOG(LogTemp, Log, TEXT("SideBrake: Rear friction reduced to %.2f (Drift mode)"), DriftFrictionMultiplier);
	}
	else
	{
		// 정상 주행: 원래 마찰 계수로 복원
		VehicleMovement->SetWheelFrictionMultiplier(2, OriginalRearLeftFriction);
		VehicleMovement->SetWheelFrictionMultiplier(3, OriginalRearRightFriction);
		UE_LOG(LogTemp, Log, TEXT("SideBrake: Rear friction restored to 1.0 (Normal mode)"));
	}
}
