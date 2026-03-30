// EnemyPelletComponent.h
// P-Pellet (Power Pellet) System for Enemy
// Handles invulnerability, cooldown, and visual effects

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnemyPelletComponent.generated.h"

class UStaticMeshComponent;

/** Delegate: P-Pellet 상태 변경 시 브로드캐스트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPelletStateChanged, bool, bIsActive);

/** Delegate: P-Pellet 쿨다운 변경 시 브로드캐스트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPelletCooldownChanged, float, RemainingCooldown);

/**
 * P-Pellet (파워 펠릿) 시스템 컴포넌트
 *
 * 기능:
 * - 펠릿 섭취 시 무적 상태 활성화
 * - 쿨다운 관리
 * - 시각 효과 (Shield Mesh)
 * - 멀티플레이 동기화
 */
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UEnemyPelletComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyPelletComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ========== Core Functions ==========

	/**
	 * P-Pellet 섭취 처리
	 * @param Duration 무적 지속 시간 (초)
	 */
	UFUNCTION(BlueprintCallable, Category = "P-Pellet")
	void OnPelletCollected(float Duration);

	/** P-Pellet 무적 상태 여부 */
	UFUNCTION(BlueprintPure, Category = "P-Pellet")
	bool IsPowerPelletActive() const { return bPowerPellet; }

	/** P-Pellet 쿨다운 남은 시간 */
	UFUNCTION(BlueprintPure, Category = "P-Pellet")
	float GetCooldownRemaining() const { return Cooldown; }

	/** P-Pellet 쿨다운 중인지 여부 */
	UFUNCTION(BlueprintPure, Category = "P-Pellet")
	bool IsOnCooldown() const { return Cooldown > 0.0f; }

	/** 마지막 섭취 시각 (ISO 8601) */
	UFUNCTION(BlueprintPure, Category = "P-Pellet")
	FString GetLastConsumedTime() const { return LastConsumedAt; }

	// ========== Configuration ==========

	/** 쿨다운 지속 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "P-Pellet|Config")
	float CooldownDuration = 30.0f;

	/** Shield Mesh 참조 (시각 효과용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "P-Pellet|Visual")
	TObjectPtr<UStaticMeshComponent> ShieldMesh;

	// ========== Events ==========

	/** P-Pellet 상태 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "P-Pellet|Events")
	FOnPelletStateChanged OnPelletStateChanged;

	/** P-Pellet 쿨다운 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "P-Pellet|Events")
	FOnPelletCooldownChanged OnPelletCooldownChanged;

protected:
	// ========== Internal State (Replicated) ==========

	/** 무적 상태 여부 */
	UPROPERTY(ReplicatedUsing = OnRep_PowerPellet, BlueprintReadOnly, Category = "P-Pellet|State")
	bool bPowerPellet = false;

	/** 현재 쿨다운 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "P-Pellet|State")
	float Cooldown = 0.0f;

	/** 마지막 섭취 시각 (ISO 8601) */
	UPROPERTY(BlueprintReadOnly, Category = "P-Pellet|State")
	FString LastConsumedAt;

	// ========== Timer Handles ==========

	FTimerHandle InvulnerabilityTimer;
	FTimerHandle CooldownTimer;

	// ========== Internal Functions ==========

	/** 무적 종료 콜백 */
	UFUNCTION()
	void OnInvulnerabilityEnd();

	/** 쿨다운 감소 콜백 (1초마다) */
	UFUNCTION()
	void DecreaseCooldown();

	/** OnRep: 클라이언트에서 무적 상태 변경 시 */
	UFUNCTION()
	void OnRep_PowerPellet();

	/** Shield 시각 효과 업데이트 */
	void UpdateShieldVisual(bool bShow);
};
