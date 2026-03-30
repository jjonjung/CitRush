// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BaseGA.generated.h"

/**
 * 기본 Gameplay Ability 클래스. ActivateAbility 로깅 추가
 */
UCLASS()
class UE_CITRUSH_API UBaseGA : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Ability 활성화 시 호출. 로깅 추가 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
