// SideBrake Gameplay Ability 구현

#include "GAS/GameplayAbility/Racer/GA_SideBrake.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayTags/GTRacer.h"
#include "GAS/GameplayTags/GTNative.h"
#include "ChaosVehicleMovementComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/Pawn.h"

UGA_SideBrake::UGA_SideBrake()
{
	// Ability 기본 설정
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Ability Tags 설정
	AbilityTags.AddTag(CitRushTags::Racer::Ability::SideBrake);

	// Activation Owned Tags - 사이드 브레이크 활성화 중 추가되는 태그
	ActivationOwnedTags.AddTag(CitRushTags::Racer::State::SideBraking);

	// Blocking Tags
	BlockAbilitiesWithTag.AddTag(CitRushTags::State::Dead);
}

void UGA_SideBrake::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 차량 Movement 컴포넌트 가져오기
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

	// 사이드 브레이크 적용
	ApplySideBrake(true);

	// 드리프트 Effect 적용
	if (DriftEffect)
	{
		FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
			DriftEffect, GetAbilityLevel(), EffectContext);

		if (SpecHandle.IsValid())
		{
			ActiveDriftHandle = GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// GameplayCue 실행 (드리프트 이펙트, 타이어 흔적 등)
	FGameplayCueParameters CueParams;
	CueParams.SourceObject = GetAvatarActorFromActorInfo();
	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(CitRushTags::Racer::Cue::Effect::Drift, CueParams);
}

void UGA_SideBrake::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 입력 해제 시 Ability 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_SideBrake::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 사이드 브레이크 해제
	ApplySideBrake(false);

	// 드리프트 Effect 제거
	if (ActiveDriftHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(ActiveDriftHandle);
		ActiveDriftHandle.Invalidate();
	}

	// GameplayCue 제거
	GetAbilitySystemComponentFromActorInfo()->RemoveGameplayCue(CitRushTags::Racer::Cue::Effect::Drift);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_SideBrake::ApplySideBrake(bool bApply)
{
	if (!VehicleMovement)
	{
		return;
	}

	// ChaosWheeledVehicleMovementComponent 캐스팅
	UChaosWheeledVehicleMovementComponent* WheeledMovement = Cast<UChaosWheeledVehicleMovementComponent>(VehicleMovement);
	if (!WheeledMovement)
	{
		return;
	}

	// 핸드브레이크 적용/해제
	if (bApply)
	{
		WheeledMovement->SetHandbrakeInput(true);
		UE_LOG(LogTemp, Log, TEXT("GA_SideBrake: Handbrake Applied"));
	}
	else
	{
		WheeledMovement->SetHandbrakeInput(false);
		UE_LOG(LogTemp, Log, TEXT("GA_SideBrake: Handbrake Released"));
	}
}
