// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "BaseGEExecutionCalculation.generated.h"

/**
 * GameplayEffect Execution Calculation 기본 클래스. 복잡한 데미지/효과 계산용
 */
UCLASS()
class UE_CITRUSH_API UBaseGEExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
/*
public:
	UBaseGEExecutionCalculation();
*/

protected:
	/** 복잡한 계산 수행. 데미지 계산 등에 사용 */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
