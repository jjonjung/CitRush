// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "PixelAIController.generated.h"

/**
 * AI Controller for Pixel Enemy
 * Integrates Behavior Tree with GAS system
 * AI 제어 및 Perception 처리 
*/
UCLASS()
class UE_CITRUSH_API APixelAIController : public AAIController
{
	GENERATED_BODY()
	

public:
	APixelAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;

#pragma region Perception

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

protected:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// Perception Settings
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float SightRadius = 4000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float LoseSightRadius = 4100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	float PeripheralVisionAngleDegrees = 180.0f;

	// Combat Settings
	UPROPERTY(EditDefaultsOnly, Category = "AI|Combat")
	float RacerAttackRange = 5000.0f;

#pragma endregion

#pragma region Blackboard Helpers

public:
	// Blackboard 업데이트 헬퍼 함수들
	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	void UpdateBlackboardTarget(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	void UpdateHealthPercentage();

	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	AActor* GetBlackboardTarget() const;

	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	float GetBlackboardHealthPercentage() const;

#pragma endregion
};
