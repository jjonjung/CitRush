// Boost Gameplay Ability - GAS 시스템으로 구현
#pragma once

#include "CoreMinimal.h"
#include "GAS/GameplayAbility/BaseGA.h"
#include "GA_Boost.generated.h"

class UChaosVehicleMovementComponent;

/**
 * Boost Gameplay Ability
 *
 * 기존 BoostComponent 기능을 GAS로 이주:
 * - Fuel 소모를 통한 부스트 활성화
 * - 차량에 전진 방향 힘 적용
 * - 부스트 상태 관리 (Ready, Active, Cooldown, Depleted)
 * - FOV, 모션 블러, 포스트 프로세스 효과는 GameplayCue로 처리
 */
UCLASS()
class UE_CITRUSH_API UGA_Boost : public UBaseGA
{
	GENERATED_BODY()

public:
	UGA_Boost();

	// Ability 활성화 가능 여부 체크
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	// Ability 활성화
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 입력 해제 시 (부스트 키를 뗄 때)
	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	// Ability 취소
	virtual void CancelAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateCancelAbility) override;

	// Ability 종료
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 부스트 힘 적용 (Tick에서 호출)
	void ApplyBoostForce();

	// Tick 콜백
	UFUNCTION()
	void OnBoostTick();

	// 차량 Movement 컴포넌트
	UPROPERTY()
	TObjectPtr<UChaosVehicleMovementComponent> VehicleMovement;

	// 타이머 핸들
	FTimerHandle BoostTickTimerHandle;

	//=================================================================
	// 튜닝 파라미터
	//=================================================================

	/** 부스트 전진 힘 (Newtons) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost|Tuning")
	float BoostForce = 500000.f;

	/** 최소 속도 (km/h) - 이 속도 이하에서는 부스트 사용 불가 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost|Tuning")
	float MinSpeedForBoost = 10.f;

	/** 공중에서 부스트 사용 가능 여부 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost|Tuning")
	bool bAllowInAir = false;

	//=================================================================
	// Gameplay Effect 클래스
	//=================================================================

	/** Fuel 소모 Effect (Instant, Fuel 감소) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost|Effects")
	TSubclassOf<UGameplayEffect> FuelCostEffect;

	/** 부스트 활성화 중 Fuel 지속 소모 Effect (Duration, Periodic) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost|Effects")
	TSubclassOf<UGameplayEffect> FuelDrainEffect;

	/** 쿨다운 Effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boost|Effects")
	TSubclassOf<UGameplayEffect> CooldownEffect;

	/** 활성 Fuel 드레인 Effect 핸들 */
	FActiveGameplayEffectHandle ActiveFuelDrainHandle;
};
