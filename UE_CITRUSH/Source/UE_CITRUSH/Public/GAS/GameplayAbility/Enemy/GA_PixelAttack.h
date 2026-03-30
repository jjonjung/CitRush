// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GameplayAbility/BaseGA.h"
#include "GA_PixelAttack.generated.h"

/**
 * Pixel Enemy Attack Ability
 */
UCLASS()
class UE_CITRUSH_API UGA_PixelAttack : public UBaseGA
{
	GENERATED_BODY()

public:
	UGA_PixelAttack();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// Attack 설정
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> AttackDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackCooldown = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRange = 300.0f;
};
