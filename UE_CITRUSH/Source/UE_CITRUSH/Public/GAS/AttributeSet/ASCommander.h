// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseAS.h"
#include "ASCommander.generated.h"

/**
 * Commander AttributeSet. 지휘 포인트 및 스캔 범위 관리
 */
UCLASS()
class UE_CITRUSH_API UASCommander : public UBaseAS
{
	GENERATED_BODY()

public:
	/** 리플리케이션 프로퍼티 등록 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Attribute 변경 전 호출. 값 클램핑 처리 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** GameplayEffect 적용 후 호출. 최종 값 처리 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/** 현재 지휘 포인트 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CommandPoints, Category="GAS|Attribute")
	FGameplayAttributeData CommandPoints;
	UFUNCTION()
	void OnRep_CommandPoints(const FGameplayAttributeData& oldCommandPoints) const;

protected:
	/** 최대 지휘 포인트 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxCommandPoints, Category="GAS|Attribute")
	FGameplayAttributeData MaxCommandPoints;
	UFUNCTION()
	void OnRep_MaxCommandPoints(const FGameplayAttributeData& oldMaxCommandPoints) const;

	/** 스캔 범위 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ScanRange, Category="GAS|Attribute")
	FGameplayAttributeData ScanRange;
	UFUNCTION()
	void OnRep_ScanRange(const FGameplayAttributeData& oldScanRange) const;
	
	DECLARE_ATTRIBUTE(UASCommander, CommandPoints)
	DECLARE_ATTRIBUTE(UASCommander, MaxCommandPoints)
	DECLARE_ATTRIBUTE(UASCommander, ScanRange)

};
