// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GameplayAbility/BaseGA.h"
#include "GA_PixelPatrol.generated.h"

/**
 * Pixel Enemy Patrol Ability
 */
UCLASS()
class UE_CITRUSH_API UGA_PixelPatrol : public UBaseGA
{
	GENERATED_BODY()

public:
	UGA_PixelPatrol();

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

	// Patrol 설정
	UPROPERTY(EditDefaultsOnly, Category = "Patrol")
	float PatrolSpeedMultiplier = 1.0f;
};
