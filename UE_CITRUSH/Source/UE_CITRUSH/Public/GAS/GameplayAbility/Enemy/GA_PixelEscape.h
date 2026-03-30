// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GameplayAbility/BaseGA.h"
#include "GA_PixelEscape.generated.h"

/**
 * Pixel Enemy Escape Ability
 */
UCLASS()
class UE_CITRUSH_API UGA_PixelEscape : public UBaseGA
{
	GENERATED_BODY()

public:
	UGA_PixelEscape();

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

	// Escape 설정
	UPROPERTY(EditDefaultsOnly, Category = "Escape")
	float EscapeSpeedMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Escape")
	TSubclassOf<UGameplayEffect> EscapeSpeedBoostEffect;
};
