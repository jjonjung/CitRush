// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/GameplayAbility/Enemy/GA_PixelAttack.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayTags/GTEnemy.h"
#include "Enemy/PixelEnemy.h"
#include "Enemy/PixelAIController.h"

UGA_PixelAttack::UGA_PixelAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Ability Tag 설정
	AbilityTags.AddTag(CitRushTags::Enemy::Ability::Attack);

	// Activation 조건
	ActivationOwnedTags.AddTag(CitRushTags::Enemy::State::Attacking);

	// 도주 중에는 공격 불가
	ActivationBlockedTags.AddTag(CitRushTags::Enemy::State::Escaping);
}

void UGA_PixelAttack::ActivateAbility(
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
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Attacking State Tag 추가
	ASC->AddLooseGameplayTag(CitRushTags::Enemy::State::Attacking);

	UE_LOG(LogTemp, Log, TEXT("GA_PixelAttack: Activated"));

	// 타겟 가져오기 (AI Controller의 Blackboard에서)
	APixelEnemy* PixelEnemy = Cast<APixelEnemy>(GetAvatarActorFromActorInfo());
	if (!PixelEnemy)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APixelAIController* AIController = Cast<APixelAIController>(PixelEnemy->GetController());
	if (!AIController)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Target = AIController->GetBlackboardTarget();
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_PixelAttack: No target found"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 거리 체크
	float Distance = FVector::Dist(PixelEnemy->GetActorLocation(), Target->GetActorLocation());
	if (Distance > AttackRange)
	{
		UE_LOG(LogTemp, Warning, TEXT("GA_PixelAttack: Target out of range"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공격 데미지 Effect 적용
	if (AttackDamageEffect)
	{
		// 타겟의 ASC 가져오기
		IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Target);
		if (TargetASI)
		{
			UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
			if (TargetASC)
			{
				FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
				EffectContext.AddSourceObject(PixelEnemy);
				EffectContext.AddInstigator(PixelEnemy, PixelEnemy);

				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(AttackDamageEffect, 1, EffectContext);
				if (SpecHandle.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
					UE_LOG(LogTemp, Log, TEXT("GA_PixelAttack: Applied damage to %s"), *Target->GetName());
				}
			}
		}
	}

	// Attack Cue 발생
	FGameplayEventData EventData;
	EventData.Instigator = PixelEnemy;
	EventData.Target = Target;
	ASC->HandleGameplayEvent(CitRushTags::Enemy::Cue::Attack, &EventData);

	// 공격 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_PixelAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		// Attacking State Tag 제거
		ASC->RemoveLooseGameplayTag(CitRushTags::Enemy::State::Attacking);

		UE_LOG(LogTemp, Log, TEXT("GA_PixelAttack: Ended (Cancelled: %s)"), bWasCancelled ? TEXT("Yes") : TEXT("No"));
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
