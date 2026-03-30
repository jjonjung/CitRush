// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/GameplayAbility/BaseGA.h"
#include "GATestBooster.generated.h"

/**
 * Racer 테스트용 부스터 Ability (임시)
 */
UCLASS()
class UE_CITRUSH_API UGATestBooster : public UBaseGA
{
	GENERATED_BODY()

public:
	/** 생성자 */
	UGATestBooster(const FObjectInitializer& ObjectInitializer);
	
	/** Ability 활성화 가능 여부 확인 */
	virtual bool CanActivateAbility(
		 const FGameplayAbilitySpecHandle Handle,
		 const FGameplayAbilityActorInfo* ActorInfo,
		 const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		 OUT FGameplayTagContainer* OptionalRelevantTags = nullptr
		 ) const override;

	/** Ability 활성화. 부스터 효과 적용 */
	virtual void ActivateAbility(
		 const FGameplayAbilitySpecHandle Handle,
		 const FGameplayAbilityActorInfo* OwnerInfo,
		 const FGameplayAbilityActivationInfo ActivationInfo,
		 const FGameplayEventData* TriggerEventData
		 ) override;

	/** 입력 릴리즈 시 호출. 부스터 종료 */
	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
		) override;

	/** Ability 취소 */
	virtual void CancelAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateCancelAbility
		) override;
};
