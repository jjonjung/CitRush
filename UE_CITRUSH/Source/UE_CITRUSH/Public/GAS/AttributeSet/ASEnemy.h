// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseAS.h"
#include "ASEnemy.generated.h"
/**
 * Enemy AttributeSet. 체력/감지범위/속도/공격력 관리
 */
UCLASS()
class UE_CITRUSH_API UASEnemy : public UBaseAS
{
	GENERATED_BODY()

public:
	/** 리플리케이션 프로퍼티 등록 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Attribute 변경 전 호출. 값 클램핑 처리 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** GameplayEffect 적용 후 호출. 사망 처리 등 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	/** 이전 HP 값 (HP 감소 감지용) */
	float PreviousHealth = -1.0f;

protected:
	/** 현재 체력 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="GAS|Attribute")
	FGameplayAttributeData Health;
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& oldHealth) const;
	/** 최대 체력 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="GAS|Attribute")
	FGameplayAttributeData MaxHealth;
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& oldMaxHealth) const;

	/** 현재 감지 범위 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_DetectionRange, Category="GAS|Attribute")
	FGameplayAttributeData DetectionRange;
	UFUNCTION()
	void OnRep_DetectionRange(const FGameplayAttributeData& oldDetectionRange) const;
	/** 기본 감지 범위 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_DefaultDetectionRange, Category="GAS|Attribute")
	FGameplayAttributeData DefaultDetectionRange;
	UFUNCTION()
	void OnRep_DefaultDetectionRange(const FGameplayAttributeData& oldDefaultDetectionRange) const;

	/** 현재 이동 속도 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Speed, Category="GAS|Attribute")
	FGameplayAttributeData Speed;
	UFUNCTION()
	void OnRep_Speed(const FGameplayAttributeData& oldSpeed) const;
	/** 기본 이동 속도 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_DefaultSpeed, Category="GAS|Attribute")
	FGameplayAttributeData DefaultSpeed;
	UFUNCTION()
	void OnRep_DefaultSpeed(const FGameplayAttributeData& oldDefaultSpeed) const;
	
	/** 현재 공격력 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_AttackPower, Category="GAS|Attribute")
	FGameplayAttributeData AttackPower;
	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& oldAttackPower) const;
	
	/** 기본 공격력 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_DefaultAttackPower, Category="GAS|Attribute")
	FGameplayAttributeData DefaultAttackPower;
	UFUNCTION()
	void OnRep_DefaultAttackPower(const FGameplayAttributeData& oldDefaultAttackPower) const;
	
	DECLARE_ATTRIBUTE(UASEnemy, Health)
	DECLARE_ATTRIBUTE(UASEnemy, MaxHealth)
	DECLARE_ATTRIBUTE(UASEnemy, DetectionRange)
	DECLARE_ATTRIBUTE(UASEnemy, DefaultDetectionRange)
	DECLARE_ATTRIBUTE(UASEnemy, Speed)
	DECLARE_ATTRIBUTE(UASEnemy, DefaultSpeed)
	DECLARE_ATTRIBUTE(UASEnemy, AttackPower)
	DECLARE_ATTRIBUTE(UASEnemy, DefaultAttackPower)
	
};
