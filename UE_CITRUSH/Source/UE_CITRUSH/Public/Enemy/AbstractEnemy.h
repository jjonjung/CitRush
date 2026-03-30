// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbstractEnemy.generated.h"

struct FGameplayTag;
class UGameplayAbility;
class UGameplayEffect;
class AAbstractRacer;
class UAttributeSet;
class UASEnemy;

/** 적 Character 추상 클래스. GAS 사용, 충돌 처리 (Front/Back) */
UCLASS(Abstract)
class UE_CITRUSH_API AAbstractEnemy : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/** ASC, AttributeSet 생성 */
	AAbstractEnemy();

protected:
	/** InitAbilityActorInfo 호출 */
	virtual void PossessedBy(AController* NewController) override;
	virtual void BeginPlay() override;

	/** 충돌 방향에 따라 데미지 처리 (Front: Racer 데미지, Back: Enemy 데미지) */
	virtual void OnCollisionHit(const AAbstractRacer* racer, const FVector& hitLocation);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
#pragma region AbilitySystem
public:
	/** AbilitySystemComponent 반환 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	/** AttributeSet 반환 */
	UAttributeSet* GetAttributeSet() const;
	/** 기본 Attribute 초기화 (defaultAttributeEffect 적용) */
	virtual void InitDefaultAttributes() const;
	/** Ability 활성화 (TSubclassOf) */
	virtual bool ActivateAbility(const TSubclassOf<UGameplayAbility>& ability);
	/** Ability 활성화 (GameplayTag) */
	virtual bool ActivateAbility(const FGameplayTag& gameplayTag);
	/** Ability 부여 (TSubclassOf) */
	virtual bool ReceiveAbility(const TSubclassOf<UGameplayAbility>& ability);
	/** Ability 부여 (GameplayTag) */
	virtual bool ReceiveAbility(const FGameplayTag& gameplayTag);

	/** Enemy AttributeSet */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UASEnemy> attributeSet;
protected:
	/** AbilitySystemComponent */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> abilitySystemComponent;

	/** Gameplay Abilities Dictionary */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Ability")
	TMap<FName, TSubclassOf<UGameplayAbility>> gameplayAbilities;
	/** 기본 Attribute 설정 GameplayEffect */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Effect")
	TSubclassOf<UGameplayEffect> defaultAttributeEffect;

	/** 충돌 쿨다운 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float collisionCooldown = 0.5f;
	/** 마지막 충돌 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float lastCollisionTime = -1.f;
	/** 마지막 충돌 Racer */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	AAbstractRacer* lastCollisionRacer = nullptr;

#pragma endregion
};
