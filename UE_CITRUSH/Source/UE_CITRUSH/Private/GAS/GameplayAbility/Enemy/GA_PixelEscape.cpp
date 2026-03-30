// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/GameplayAbility/Enemy/GA_PixelEscape.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayTags/GTEnemy.h"

UGA_PixelEscape::UGA_PixelEscape()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Ability Tag 설정
	AbilityTags.AddTag(CitRushTags::Enemy::Ability::Escape);

	// Activation 조건
	ActivationOwnedTags.AddTag(CitRushTags::Enemy::State::Escaping);

	// Escape는 다른 모든 행동보다 우선순위가 높음
	CancelAbilitiesWithTag.AddTag(CitRushTags::Enemy::State::Patrolling);
	CancelAbilitiesWithTag.AddTag(CitRushTags::Enemy::State::Chasing);
	CancelAbilitiesWithTag.AddTag(CitRushTags::Enemy::State::Attacking);
}

void UGA_PixelEscape::ActivateAbility(
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
		// Escaping State Tag 추가
		ASC->AddLooseGameplayTag(CitRushTags::Enemy::State::Escaping);

		UE_LOG(LogTemp, Warning, TEXT("GA_PixelEscape: Activated - Low Health!"));

		// Speed Boost Effect 적용
		if (EscapeSpeedBoostEffect)
		{
			FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
			EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EscapeSpeedBoostEffect, 1, EffectContext);
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void UGA_PixelEscape::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		// Escaping State Tag 제거
		ASC->RemoveLooseGameplayTag(CitRushTags::Enemy::State::Escaping);

		UE_LOG(LogTemp, Log, TEXT("GA_PixelEscape: Ended (Cancelled: %s)"), bWasCancelled ? TEXT("Yes") : TEXT("No"));
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
