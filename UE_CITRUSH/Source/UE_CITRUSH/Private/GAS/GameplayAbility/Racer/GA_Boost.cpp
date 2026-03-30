// Boost Gameplay Ability 구현

#include "GAS/GameplayAbility/Racer/GA_Boost.h"
#include "Item/ItemData.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSet/ASRacer.h"
#include "GAS/GameplayTags/GTRacer.h"
#include "GAS/GameplayTags/GTNative.h"
#include "ChaosVehicleMovementComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Player/AbstractRacer.h"

UGA_Boost::UGA_Boost()
{
	// Ability 기본 설정
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Ability Tags 설정
	AbilityTags.AddTag(CitRushTags::Racer::Ability::Boost);

	// Activation Owned Tags - 부스트 활성화 중 추가되는 태그
	ActivationOwnedTags.AddTag(CitRushTags::Racer::State::Boosting);

	// Blocking Tags - 이 태그가 있으면 부스트 사용 불가
	BlockAbilitiesWithTag.AddTag(CitRushTags::State::Dead);

	// Input ID는 Blueprint에서 설정
}

bool UGA_Boost::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// ASC 가져오기
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return false;
	}

	// AttributeSet 가져오기
	const UASRacer* RacerAS = ASC->GetSet<UASRacer>();
	if (!RacerAS)
	{
		return false;
	}

	// Fuel 체크
	if (RacerAS->GetFuel() <= 0.f)
	{
		return false;
	}

	// 차량 Movement 컴포넌트 가져오기
	APawn* AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!AvatarPawn)
	{
		return false;
	}

	UChaosVehicleMovementComponent* VehMovement = AvatarPawn->FindComponentByClass<UChaosVehicleMovementComponent>();
	if (!VehMovement)
	{
		return false;
	}

	// 최소 속도 체크
	/*float CurrentSpeedKmH = FMath::Abs(VehMovement->GetForwardSpeed()) * 0.036f; // cm/s to km/h
	if (CurrentSpeedKmH < MinSpeedForBoost)
	{
		return false;
	}

	// 공중 체크
	if (!bAllowInAir && VehicleMovement)
	{
		FVector vec = VehicleMovement->GetOwner()->GetVelocity();
		if (FMath::Abs(vec.Z) > 100.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Boost: Cannot use in air"));
			return false;
		}
	}*/

	return true;
}

void UGA_Boost::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// ItemData 연동: SourceObject가 ItemData라면 속성 오버라이드
	if (UObject* SourceObj = GetCurrentSourceObject())
	{
		if (UItemData* ItemData = Cast<UItemData>(SourceObj))
		{
			// Effect Class 교체
			if (ItemData->durationEffectClass)
			{
				FuelDrainEffect = ItemData->durationEffectClass;
			}
			if (ItemData->coolDownEffectClass)
			{
				CooldownEffect = ItemData->coolDownEffectClass;
			}

			// PowerValue가 설정되어 있다면 BoostForce로 사용
			if (ItemData->PowerValue > 0.0f)
			{
				BoostForce = ItemData->PowerValue;
			}
		}
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 차량 Movement 컴포넌트 캐싱
	APawn* AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (AvatarPawn)
	{
		VehicleMovement = AvatarPawn->FindComponentByClass<UChaosVehicleMovementComponent>();
	}

	if (!VehicleMovement)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Fuel 드레인 Effect 적용 (지속적으로 Fuel 소모)
	if (FuelDrainEffect)
	{
		FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
			FuelDrainEffect, GetAbilityLevel(), EffectContext);

		if (SpecHandle.IsValid())
		{
			ActiveFuelDrainHandle = GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// Boost Tick 타이머 시작 (매 프레임 힘 적용)
	GetWorld()->GetTimerManager().SetTimer(
		BoostTickTimerHandle,
		this,
		&UGA_Boost::OnBoostTick,
		0.016f, // ~60fps
		true
	);

	// GameplayCue 실행 (FOV, 모션 블러, 포스트 프로세스 효과)
	FGameplayCueParameters CueParams;
	CueParams.SourceObject = GetAvatarActorFromActorInfo();
	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(CitRushTags::Racer::Cue::Effect::SpeedBoost, CueParams);
}

void UGA_Boost::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 입력 해제 시 Ability 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Boost::CancelAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateCancelAbility)
{
	// 타이머 정리
	if (BoostTickTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(BoostTickTimerHandle);
	}

	// Fuel Drain Effect 제거
	if (ActiveFuelDrainHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(ActiveFuelDrainHandle);
	}

	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void UGA_Boost::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 타이머 정리
	if (BoostTickTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(BoostTickTimerHandle);
	}

	// Fuel Drain Effect 제거
	if (ActiveFuelDrainHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(ActiveFuelDrainHandle);
		ActiveFuelDrainHandle.Invalidate();
	}

	// GameplayCue 제거
	FGameplayCueParameters CueParams;
	CueParams.SourceObject = GetAvatarActorFromActorInfo();
	GetAbilitySystemComponentFromActorInfo()->RemoveGameplayCue(CitRushTags::Racer::Cue::Effect::SpeedBoost);

	// 아이템 소비 (UI 아이콘 제거를 위해 슬롯 비우기)
	if (ActorInfo->AvatarActor.IsValid())
	{
		if (AAbstractRacer* Racer = Cast<AAbstractRacer>(ActorInfo->AvatarActor.Get()))
		{
			if (UObject* SourceObj = GetCurrentSourceObject())
			{
				if (UItemData* ItemData = Cast<UItemData>(SourceObj))
				{
					Racer->ConsumeItem(ItemData);
					UE_LOG(LogTemp, Log, TEXT("[GA_Boost] Boost finished, consuming item: %s"), *ItemData->GetName());
				}
			}
		}
	}

	// 쿨다운 Effect 적용
	if (CooldownEffect && !bWasCancelled)
	{
		FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
			CooldownEffect, GetAbilityLevel(), EffectContext);

		if (SpecHandle.IsValid())
		{
			GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Boost::OnBoostTick()
{
	// Fuel 체크
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	const UASRacer* RacerAS = ASC->GetSet<UASRacer>();
	if (!RacerAS || RacerAS->GetFuel() <= 0.f)
	{
		// Fuel 소진 시 Ability 종료
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 부스트 힘 적용
	ApplyBoostForce();
}

void UGA_Boost::ApplyBoostForce()
{
	if (!VehicleMovement)
	{
		return;
	}

	// 차량의 전진 방향으로 힘 적용
	AActor* VehicleActor = VehicleMovement->GetOwner();
	if (!VehicleActor)
	{
		return;
	}

	FVector ForwardVector = VehicleActor->GetActorForwardVector();
	FVector BoostForceVector = ForwardVector * BoostForce;

	// Chaos Vehicle에서는 Updated Component에 직접 힘을 적용
	UPrimitiveComponent* UpdatedPrimitive = Cast<UPrimitiveComponent>(VehicleMovement->UpdatedComponent);
	if (UpdatedPrimitive)
	{
		UpdatedPrimitive->AddForce(BoostForceVector, NAME_None, false);
	}
}
