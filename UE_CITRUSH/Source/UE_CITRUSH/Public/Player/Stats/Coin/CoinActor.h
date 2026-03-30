// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoinActor.generated.h"

class AAbstractRacer;
class ACommenderCharacter;
class APixelEnemy;

UCLASS()
class UE_CITRUSH_API ACoinActor : public AActor
{
	GENERATED_BODY()

public:
	ACoinActor();

protected:
	virtual void BeginPlay() override;

	/** Root component for the coin actor */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin|Components")
	TObjectPtr<USceneComponent> RootComp;

	/** Coin Mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin|Components")
	TObjectPtr<UStaticMeshComponent> CoinMesh;

	/** Overlap Detection Sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin|Components")
	TObjectPtr<class USphereComponent> OverlapSphere;

	/** Rotating Movement Component (optional) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin|Components")
	TObjectPtr<class URotatingMovementComponent> RotatingMovement;

	/** Coin ID for AI system reporting */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin|Settings")
	FString CoinID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin|Settings", meta = (ClampMin = "1"))
	int32 CoinValue = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin|Settings")
	FRotator RotationRate = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin|Settings", meta = (ClampMin = "10.0"))
	float OverlapRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin|Effects")
	TObjectPtr<class USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin|Effects")
	TObjectPtr<class UParticleSystem> PickupParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coin|Effects")
	TObjectPtr<class UNiagaraSystem> PickupNiagaraEffect;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

public:
	UFUNCTION(BlueprintCallable, Category = "Coin")
	void PlayPickupEffects();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Coin")
	int32 GetCoinValue() const { return CoinValue; }

	UFUNCTION(BlueprintCallable, Category = "Coin")
	void SetCoinValue(int32 NewValue);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Coin")
	FString GetCoinID() const { return CoinID; }

	UFUNCTION(BlueprintCallable, Category = "Coin")
	void SetCoinID(const FString& NewID) { CoinID = NewID; }
};
