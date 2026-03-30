// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/AbstractEnemy.h"
#include "Enemy/Interface/AIDecisionReceiver.h"
#include "CloneEnemy.generated.h"

class UAIDirectiveComponent;
class UEnemyAISubsystem;
class APixelEnemy;

/** 분신 사망 시 부모에게 알리는 델리게이트 */
DECLARE_DELEGATE_OneParam(FOnCloneDeath, class ACloneEnemy*);

/**
 * 분신(Clone) 액터
 * - P-Pellet 활성화 시 PixelEnemy가 스폰
 * - AI 서버로부터 개별 명령을 받아 행동
 * - P-Pellet 만료 시 제거
 */
UCLASS()
class UE_CITRUSH_API ACloneEnemy : public AAbstractEnemy, public IAIDecisionReceiver
{
	GENERATED_BODY()

public:
	ACloneEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// ========== IAIDecisionReceiver Interface ==========

	virtual void OnAIDecisionReceived(const FUnitCommand& Command) override;
	virtual FString GetEnemyID() const override;
	virtual FEnemyGameState GetCurrentGameState() const override;
	virtual bool IsAIEnabled() const override { return bIsAlive; }

	// ========== Clone Setup ==========

	/** 분신 초기화 (PixelEnemy에서 스폰 후 호출) */
	void InitializeClone(int32 Index, APixelEnemy* InOwner);

	/** 분신 사망 처리 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_CloneDeath();

	/** 분신 사망 알림 델리게이트 */
	FOnCloneDeath OnCloneDeath;

	// ========== Components ==========

	/** AI Directive Component - FSM 관리 담당 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIDirectiveComponent> AIDirectiveComponent;

	// ========== Properties ==========

	/** 분신 인덱스 (1~4) */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Clone")
	int32 CloneIndex = 0;

	/** 생존 여부 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Clone")
	bool bIsAlive = true;

	/** 분신 체력 */
	UPROPERTY(EditDefaultsOnly, Category = "Clone")
	float CloneHP = 1.0f;

	/** 이동 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "Clone")
	float MoveSpeed = 3000.0f;

	/** 부모 PixelEnemy 참조 */
	UPROPERTY()
	TWeakObjectPtr<APixelEnemy> OwnerPixelEnemy;

private:
	/** Racer 충돌 처리 */
	UFUNCTION()
	void OnHitToRacer(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** EnemyAISubsystem 캐시 */
	UPROPERTY()
	TObjectPtr<UEnemyAISubsystem> CachedAISubsystem;
};
