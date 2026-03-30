// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GC_Earthquake.generated.h"

class UNiagaraSystem;
/**
 * 
 */
UCLASS()
class UE_CITRUSH_API AGC_Earthquake : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGC_Earthquake();

protected:
	virtual bool OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

private:
	void TriggerEarthquake();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Earthquake")
	TSubclassOf<UCameraShakeBase> earthquakeCameraShake;

	UPROPERTY(EditDefaultsOnly, Category = "Earthquake")
	int32 maxShakeCount = 5;
	
	UPROPERTY(VisibleAnywhere, Category = "Earthquake")
	int32 shakeCounter = 0;
	
	UPROPERTY(EditDefaultsOnly, Category = "Earthquake")
	float shakeInterval = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Earthquake")
	float shakeInnerRadius = 500.0f; 

	UPROPERTY(EditDefaultsOnly, Category = "Earthquake")
	float shakeOuterRadius = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Earthquake")
	float shakeFalloff = 1.0f;  // 감쇠 곡선 (1.0 = 선형)

	UPROPERTY(EditDefaultsOnly, Category = "Earthquake")
	USoundBase* earthquakeSound;

	UPROPERTY(EditDefaultsOnly, Category = "Earthquake")
	USoundAttenuation* soundAttenuation;

	UPROPERTY(EditDefaultsOnly, Category = "Earthquake")
	UNiagaraSystem* groundShakeVFX;
	
private:
	FTimerHandle shakeTimer;
	TWeakObjectPtr<AActor> targetActor;
	UPROPERTY()
	UAudioComponent* earthquakeSoundComponent;
};
