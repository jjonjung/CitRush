// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PelletActor.generated.h"

class AAbstractEnemy;

/**
 * P-Pellet Actor
 * AbstractEnemy(PixelEnemy)만 획득 가능한 파워업 아이템
 * 획득 시 무적 상태 부여 
 */
UCLASS()
class UE_CITRUSH_API APelletActor : public AActor
{
	GENERATED_BODY()

public:
	APelletActor();

protected:
	virtual void BeginPlay() override;

	// ==================== Components ====================
	/** Root Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pellet|Components")
	TObjectPtr<USceneComponent> RootComp;

	/** Pellet Mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pellet|Components")
	TObjectPtr<UStaticMeshComponent> PelletMesh;

	/** Overlap Detection Sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pellet|Components")
	TObjectPtr<class USphereComponent> OverlapSphere;

	/** Rotating Movement Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pellet|Components")
	TObjectPtr<class URotatingMovementComponent> RotatingMovement;

	// ==================== Properties ====================
	/** P-Pellet 무적 지속 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pellet|Settings", meta = (ClampMin = "1.0"))
	float InvulnerabilityDuration = 5.0f;

	/** 회전 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pellet|Settings")
	FRotator RotationRate = FRotator(0.0f, 120.0f, 0.0f);

	/** 획득 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pellet|Settings", meta = (ClampMin = "10.0"))
	float OverlapRadius = 80.0f;

	/** 획득 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pellet|Effects")
	TObjectPtr<class USoundBase> PickupSound;

	/** 획득 파티클 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pellet|Effects")
	TObjectPtr<class UParticleSystem> PickupParticle;

	/** 획득 Niagara 이펙트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pellet|Effects")
	TObjectPtr<class UNiagaraSystem> PickupNiagaraEffect;

	/** 획득 가능 여부 (쿨타임 관리용) */
	UPROPERTY(BlueprintReadOnly, Category = "Pellet|State")
	bool bIsAvailable = true;

	/** 쿨타임 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pellet|Settings", meta = (ClampMin = "0.0"))
	float CooldownDuration = 5.0f;

public:
	/** Overlap 이벤트 핸들러 */
	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	/** 획득 이펙트 재생 */
	UFUNCTION(BlueprintCallable, Category = "Pellet")
	void PlayPickupEffects();

	/** 쿨타임 만료 후 재활성화 */
	UFUNCTION()
	void ReactivatePellet();

	/** 쿨타임 타이머 핸들 */
	FTimerHandle CooldownTimerHandle;

	/** 획득 가능 여부 확인 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pellet")
	bool IsAvailable() const { return bIsAvailable; }

	/** 무적 지속 시간 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pellet")
	float GetInvulnerabilityDuration() const { return InvulnerabilityDuration; }

	/** 남은 쿨타임 반환 (초) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pellet")
	float GetRemainingCooldown() const;

	/** Pellet ID (AI 서버용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pellet|Settings")
	FString PelletID;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pellet")
	FString GetPelletID() const { return PelletID; }

	UFUNCTION(BlueprintCallable, Category = "Pellet")
	void SetPelletID(const FString& NewID) { PelletID = NewID; }
};
