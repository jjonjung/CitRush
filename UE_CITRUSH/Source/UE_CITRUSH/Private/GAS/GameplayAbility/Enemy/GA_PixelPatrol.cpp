// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/GameplayAbility/Enemy/GA_PixelPatrol.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayTags/GTEnemy.h"

UGA_PixelPatrol::UGA_PixelPatrol()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Ability Tag 설정
	AbilityTags.AddTag(CitRushTags::Enemy::Ability::Patrol);

	// Activation 조건: Idle 또는 Patrolling 상태에서만
	ActivationOwnedTags.AddTag(CitRushTags::Enemy::State::Patrolling);
	
	// 다른 공격적인 행동 중에는 활성화 불가
	ActivationBlockedTags.AddTag(CitRushTags::Enemy::State::Chasing);
	ActivationBlockedTags.AddTag(CitRushTags::Enemy::State::Attacking);
	ActivationBlockedTags.AddTag(CitRushTags::Enemy::State::Escaping);
}

void UGA_PixelPatrol::ActivateAbility(
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
		// Patrolling State Tag 추가
		ASC->AddLooseGameplayTag(CitRushTags::Enemy::State::Patrolling);
		
		UE_LOG(LogTemp, Log, TEXT("GA_PixelPatrol: Activated"));
	}

	// 속도 조절 GameplayEffect 적용 (선택사항)
	// TODO: 필요시 Patrol Speed Effect 추가
}

void UGA_PixelPatrol::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		// Patrolling State Tag 제거
		ASC->RemoveLooseGameplayTag(CitRushTags::Enemy::State::Patrolling);
		
		UE_LOG(LogTemp, Log, TEXT("GA_PixelPatrol: Ended (Cancelled: %s)"), bWasCancelled ? TEXT("Yes") : TEXT("No"));
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
