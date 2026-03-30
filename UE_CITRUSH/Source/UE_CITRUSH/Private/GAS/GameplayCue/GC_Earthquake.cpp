// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GameplayCue/GC_Earthquake.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GAS/Effects/EarthquakeCameraShake.h"

AGC_Earthquake::AGC_Earthquake()
{
	bAutoDestroyOnRemove = true;
	AutoDestroyDelay = 0.1f;

	
	earthquakeCameraShake = UEarthquakeCameraShake::StaticClass();
}

bool AGC_Earthquake::OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	if (!Target || !GetWorld())
	{
		return false;
	}

	targetActor = Target;
	FVector earthquakeLocation = Target->GetActorLocation();
	
	if (earthquakeSound)
	{
		earthquakeSoundComponent = UGameplayStatics::SpawnSoundAtLocation(
			GetWorld()
			, earthquakeSound, earthquakeLocation, FRotator::ZeroRotator
			, 1.0f, 1.0f, 0.0f, soundAttenuation
			, nullptr, true
		);
	}
	if (groundShakeVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),groundShakeVFX
			, earthquakeLocation,FRotator::ZeroRotator,FVector(1.0f)
			,true,true, ENCPoolMethod::AutoRelease
		);
	}

	shakeCounter = maxShakeCount;
	GetWorld()->GetTimerManager().SetTimer(shakeTimer,
		this,	&AGC_Earthquake::TriggerEarthquake,
		shakeInterval, true, 0.1f
	);
	
	return true;
}

bool AGC_Earthquake::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	if (!GetWorld()) {return false;}
	GetWorld()->GetTimerManager().ClearTimer(shakeTimer);

	if (earthquakeSoundComponent && earthquakeSoundComponent->IsPlaying())
	{
		earthquakeSoundComponent->FadeOut(0.5f, 0.0f);
	}

	return true;
}

void AGC_Earthquake::TriggerEarthquake()
{
	if (!targetActor.IsValid() || !earthquakeCameraShake || !shakeTimer.IsValid())
	{
		return;
	}
	if (--shakeCounter <= 0)
	{
		GetWorldTimerManager().ClearTimer(shakeTimer);
	}
	UGameplayStatics::PlayWorldCameraShake(GetWorld()
		, earthquakeCameraShake,
		targetActor->GetActorLocation()
		, shakeInnerRadius, shakeOuterRadius, shakeFalloff
		, true  // Orient Towards Epicenter
	);
}
