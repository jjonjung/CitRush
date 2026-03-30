// SideBrake Gameplay Ability - GAS 시스템으로 구현
#pragma once

#include "CoreMinimal.h"
#include "GAS/GameplayAbility/BaseGA.h"
#include "GA_SideBrake.generated.h"

class UChaosVehicleMovementComponent;

/**
 * SideBrake (Handbrake) Gameplay Ability
 *
 * 기존 SideBrakeComponent 기능을 GAS로 이주:
 * - 후륜 브레이크 적용
 * - 드리프트 효과
 * - 입력 지속 시간 동안 활성화
 */
UCLASS()
class UE_CITRUSH_API UGA_SideBrake : public UBaseGA
{
	GENERATED_BODY()

public:
	UGA_SideBrake();

	// Ability 활성화
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 입력 해제 시
	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	// Ability 종료
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 사이드 브레이크 적용
	void ApplySideBrake(bool bApply);

	// 차량 Movement 컴포넌트
	UPROPERTY()
	TObjectPtr<UChaosVehicleMovementComponent> VehicleMovement;

	//=================================================================
	// 튜닝 파라미터
	//=================================================================

	/** 사이드 브레이크 강도 (0.0 ~ 1.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SideBrake|Tuning")
	float SideBrakeStrength = 1.0f;

	//=================================================================
	// Gameplay Effect 클래스
	//=================================================================

	/** 드리프트 중 마찰력 감소 Effect (Duration) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SideBrake|Effects")
	TSubclassOf<UGameplayEffect> DriftEffect;

	/** 활성 드리프트 Effect 핸들 */
	FActiveGameplayEffectHandle ActiveDriftHandle;
};
