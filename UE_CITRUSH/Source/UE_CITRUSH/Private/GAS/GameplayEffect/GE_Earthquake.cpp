// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GameplayEffect/GE_Earthquake.h"

UGE_Earthquake::UGE_Earthquake()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(10.0f);
	
	FGameplayEffectCue CueToAdd;
	CueToAdd.GameplayCueTags.AddTag(
		FGameplayTag::RequestGameplayTag(FName("Enemy.Cue.Earthquake"))
	);
	GameplayCues.Add(CueToAdd);
}
