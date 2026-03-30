// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AttributeSet/ASRacer.h"

#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

UASRacer::UASRacer()
{
	/*InitHealth(GetMaxHealth());
	InitFuel(GetMaxFuel());*/
}

void UASRacer::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UASRacer, Health, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASRacer, MaxHealth, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASRacer, Fuel, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASRacer, MaxFuel, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
}

void UASRacer::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetFuelAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.f, GetMaxFuel());
	}

}

void UASRacer::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetFuelAttribute())
	{
		SetFuel(FMath::Clamp<float>(GetFuel(), 0.f, GetMaxFuel()));
	}
	
	/*UAbilitySystemComponent* asc = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
	AActor* ascOwner = asc->AbilityActorInfo->OwnerActor.Get();*/
}

DEFINE_ATTRIBUTE(UASRacer, Health)
DEFINE_ATTRIBUTE(UASRacer, MaxHealth)
DEFINE_ATTRIBUTE(UASRacer, Fuel)
DEFINE_ATTRIBUTE(UASRacer, MaxFuel)
DEFINE_ATTRIBUTE(UASRacer, AttackPower)
