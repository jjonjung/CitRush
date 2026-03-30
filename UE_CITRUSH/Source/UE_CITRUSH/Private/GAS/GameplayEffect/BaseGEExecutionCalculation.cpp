// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GameplayEffect/BaseGEExecutionCalculation.h"

#include "AbilitySystemComponent.h"

void UBaseGEExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& spec = ExecutionParams.GetOwningSpec();
	UAbilitySystemComponent* targetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	AActor* targetActor = targetASC ? targetASC->GetOwner() : nullptr;

	//OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData)
	return;
}
