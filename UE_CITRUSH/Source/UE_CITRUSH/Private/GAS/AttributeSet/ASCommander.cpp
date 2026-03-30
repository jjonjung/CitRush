// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AttributeSet/ASCommander.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UASCommander::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UASCommander, CommandPoints, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always)
 	DOREPLIFETIME_CONDITION_NOTIFY(UASCommander, MaxCommandPoints, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always)
 	DOREPLIFETIME_CONDITION_NOTIFY(UASCommander, ScanRange, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_OnChanged)
}

void UASCommander::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCommandPointsAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.f, GetMaxCommandPoints());
	}
	else if (Attribute == GetScanRangeAttribute())
	{
		// TODO: Max Scan. Or Leveling (to CSV)
		NewValue = FMath::Clamp<float>(NewValue, 0.f, 5.0f);
	}
}

void UASCommander::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCommandPointsAttribute())
	{
		SetCommandPoints(FMath::Clamp(GetCommandPoints(), 0.f, GetMaxCommandPoints()));
	}
	else if (Data.EvaluatedData.Attribute == GetScanRangeAttribute())
	{
		// TODO: Max Scan. Or Leveling (to CSV)
		SetScanRange(FMath::Clamp(GetScanRange(), 0.f, 5.f));
	}
}

DEFINE_ATTRIBUTE(UASCommander, CommandPoints)
DEFINE_ATTRIBUTE(UASCommander, MaxCommandPoints)
DEFINE_ATTRIBUTE(UASCommander, ScanRange)
