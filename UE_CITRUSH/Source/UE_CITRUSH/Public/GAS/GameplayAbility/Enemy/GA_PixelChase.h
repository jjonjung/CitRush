// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GameplayAbility/BaseGA.h"
#include "GA_PixelChase.generated.h"

/**
 * Pixel Enemy Chase Ability
 */
UCLASS()
class UE_CITRUSH_API UGA_PixelChase : public UBaseGA
{
	GENERATED_BODY()

public:
	UGA_PixelChase();

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

	// Chase 설정
	UPROPERTY(EditDefaultsOnly, Category = "Chase")
	float ChaseSpeedMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Chase")
	TSubclassOf<UGameplayEffect> ChaseSpeedBoostEffect;
};
