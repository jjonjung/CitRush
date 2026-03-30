// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseAS.h"
#include "ASRacer.generated.h"


/**
 * Racer AttributeSet. 체력/연료/공격력 관리
 */
UCLASS()
class UE_CITRUSH_API UASRacer : public UBaseAS
{
	GENERATED_BODY()

public:
	/** 생성자 */
	UASRacer();

	/** 리플리케이션 프로퍼티 등록 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Attribute 변경 전 호출. 값 클램핑 처리 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** GameplayEffect 적용 후 호출. 사망 처리 등 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

public:
	/** 현재 체력 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="GAS|Attribute")
	FGameplayAttributeData Health;
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& oldHealth) const;
	
	/** 최대 체력 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="GAS|Attribute")
	FGameplayAttributeData MaxHealth;
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& oldMaxHealth) const;

	/** 현재 연료 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Fuel, Category="GAS|Attribute")
	FGameplayAttributeData Fuel;
	UFUNCTION()
	void OnRep_Fuel(const FGameplayAttributeData& oldFuel) const;

	/** 최대 연료 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxFuel, Category="GAS|Attribute")
	FGameplayAttributeData MaxFuel;
	UFUNCTION()
	void OnRep_MaxFuel(const FGameplayAttributeData& oldMaxFuel) const;
	
	/** 공격력 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_AttackPower, Category="GAS|Attribute")
	FGameplayAttributeData AttackPower;
	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& oldAttackPower) const;
	
	DECLARE_ATTRIBUTE(UASRacer, Health)
	DECLARE_ATTRIBUTE(UASRacer, MaxHealth)
	DECLARE_ATTRIBUTE(UASRacer, Fuel)
	DECLARE_ATTRIBUTE(UASRacer, MaxFuel)
	DECLARE_ATTRIBUTE(UASRacer, AttackPower)


};
