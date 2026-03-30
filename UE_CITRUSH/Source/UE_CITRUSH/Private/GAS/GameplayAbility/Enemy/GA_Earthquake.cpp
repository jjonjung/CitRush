// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GameplayAbility/Enemy/GA_Earthquake.h"

#include "AbilitySystemComponent.h"
#include "GAS/GameplayEffect/GE_Earthquake.h"

UGA_Earthquake::UGA_Earthquake()
{
}

void UGA_Earthquake::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// GameplayEffect 적용
	FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(
		UGE_Earthquake::StaticClass(),
		GetAbilityLevel(),
		EffectContext
	);

	if (SpecHandle.IsValid())
	{
		// Effect 적용 - 자동으로 5초 후 제거되며, Cue도 함께 제거됨
		FActiveGameplayEffectHandle ActiveHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
			*SpecHandle.Data.Get()
		);
	}

	// Ability는 즉시 종료 (Effect와 Cue가 알아서 관리)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    

}

void UGA_Earthquake::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
