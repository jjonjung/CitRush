// EnemyCombatComponent.h
// Combat System for Enemy
// Handles damage state, invulnerability, and visual effects

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnemyCombatComponent.generated.h"

class UTimelineComponent;
class UCurveFloat;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UNiagaraSystem;

/** Delegate: 무적 상태 변경 시 브로드캐스트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvulnerabilityChanged, bool, bIsInvulnerable);

/** Delegate: 데미지 받았을 때 브로드캐스트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageReceived, float, DamageAmount, FVector, HitLocation);

/** Delegate: 사망 시 브로드캐스트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDeath);

/**
 * Combat 시스템 컴포넌트
 *
 * 기능:
 * - 데미지 무적 상태 관리
 * - Flash 이펙트 (데미지 표시)
 * - 이벤트 브로드캐스트
 * - 멀티플레이 동기화
 */
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UEnemyCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyCombatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ========== Core Functions ==========

	/**
	 * 데미지 무적 상태 시작
	 * @param Duration 무적 지속 시간 (초)
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartDamageInvulnerability(float Duration);

	/**
	 * 무적 상태 즉시 종료
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EndInvulnerability();

	/**
	 * Flash 이펙트 시작 (데미지 피드백)
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Visual")
	void PlayFlashEffect();

	/**
	 * Flash 이펙트 중지
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Visual")
	void StopFlashEffect();

	// ========== State Queries ==========

	/** 무적 상태 여부 (데미지 무적 또는 P-Pellet 무적) */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInvulnerable() const;

	/** 데미지 무적 상태 여부 */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsUntouchable() const { return bUntouchable; }

	// ========== Configuration ==========

	/** 데미지 무적 지속 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Config")
	float InvulnerabilityDuration = 1.5f;

	/** 기본 데미지량 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Config")
	float BaseDamage = 10.0f;

	/** Flash 커브 (외부에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Visual")
	TObjectPtr<UCurveFloat> FlashCurve;

	/** Skeletal Mesh 참조 (Flash 이펙트용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Visual")
	TObjectPtr<USkeletalMeshComponent> OwnerMesh;

	/** P-Pellet 무적 상태 참조 (외부에서 설정) */
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	bool bPowerPelletActive = false;

	// ========== Events ==========

	/** 무적 상태 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnInvulnerabilityChanged OnInvulnerabilityChanged;

	/** 데미지 수신 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnDamageReceived OnDamageReceived;

	/** 사망 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnEnemyDeath OnEnemyDeath;

protected:
	// ========== Internal State (Replicated) ==========

	/** 데미지 무적 상태 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_Untouchable, BlueprintReadOnly, Category = "Combat|State")
	bool bUntouchable = false;

	// ========== Timeline ==========

	/** Flash Timeline Component */
	UPROPERTY()
	TObjectPtr<UTimelineComponent> FlashTimeline;

	/** Flash Dynamic Material Instance */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FlashMaterial;

	// ========== Timer Handles ==========

	FTimerHandle InvulnerabilityTimer;

	// ========== Internal Functions ==========

	/** 무적 종료 콜백 */
	UFUNCTION()
	void OnInvulnerabilityEnd();

	/** OnRep: 클라이언트에서 무적 상태 변경 시 */
	UFUNCTION()
	void OnRep_Untouchable();

	/** Timeline Progress 콜백 */
	UFUNCTION()
	void OnFlashProgress(float Value);

	/** Flash 타임라인 초기화 */
	void SetupFlashTimeline();
};
