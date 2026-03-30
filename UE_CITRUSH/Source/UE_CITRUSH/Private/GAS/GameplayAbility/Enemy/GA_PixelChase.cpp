// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/GameplayAbility/Enemy/GA_PixelChase.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayTags/GTEnemy.h"

UGA_PixelChase::UGA_PixelChase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Ability Tag 설정
	AbilityTags.AddTag(CitRushTags::Enemy::Ability::Chase);

	// Activation 조건
	ActivationOwnedTags.AddTag(CitRushTags::Enemy::State::Chasing);

	// 공격 또는 도주 중에는 추적 불가
	ActivationBlockedTags.AddTag(CitRushTags::Enemy::State::Attacking);
	ActivationBlockedTags.AddTag(CitRushTags::Enemy::State::Escaping);
}

void UGA_PixelChase::ActivateAbility(
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

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		// Chasing State Tag 추가
		ASC->AddLooseGameplayTag(CitRushTags::Enemy::State::Chasing);

		UE_LOG(LogTemp, Log, TEXT("GA_PixelChase: Activated"));

		// Speed Boost Effect 적용
		if (ChaseSpeedBoostEffect)
		{
			FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
			EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ChaseSpeedBoostEffect, 1, EffectContext);
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void UGA_PixelChase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		// Chasing State Tag 제거
		ASC->RemoveLooseGameplayTag(CitRushTags::Enemy::State::Chasing);

		UE_LOG(LogTemp, Log, TEXT("GA_PixelChase: Ended (Cancelled: %s)"), bWasCancelled ? TEXT("Yes") : TEXT("No"));
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
