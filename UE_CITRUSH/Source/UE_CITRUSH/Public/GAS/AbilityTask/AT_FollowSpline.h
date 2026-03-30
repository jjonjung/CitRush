// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/AbilityTask/BaseAT.h"
#include "AT_FollowSpline.generated.h"

class ASplinePathActor;
class USplineComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFollowSplineDelegate);

/**
 * Ability Task for following a Spline path
 * Handles asynchronous movement along a spline with interpolation
 */
UCLASS()
class UE_CITRUSH_API UAT_FollowSpline : public UBaseAT
{
	GENERATED_BODY()

public:
	// Delegates
	UPROPERTY(BlueprintAssignable)
	FFollowSplineDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FFollowSplineDelegate OnInterrupted;

	/**
	 * Follow a spline path actor
	 * @param OwningAbility - The ability that owns this task
	 * @param SplineActor - The spline path actor to follow
	 * @param Speed - Movement speed along the spline
	 * @param bLoop - Should the movement loop?
	 * @param bPingPong - Should the movement reverse at the end?
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (DisplayName = "Follow Spline Path",
		HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAT_FollowSpline* FollowSplinePath(
		UGameplayAbility* OwningAbility,
		ASplinePathActor* SplineActor,
		float Speed = 200.0f,
		bool bLoop = true,
		bool bPingPong = false);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	UPROPERTY()
	TObjectPtr<ASplinePathActor> TargetSplineActor;

	UPROPERTY()
	TObjectPtr<USplineComponent> SplineComponent;

	float MovementSpeed = 200.0f;
	bool bShouldLoop = true;
	bool bShouldPingPong = false;

	// Movement state
	float CurrentDistance = 0.0f;
	bool bMovingForward = true;
	float SplineLength = 0.0f;

	// Helper functions
	void UpdateMovement(float DeltaTime);
	bool HasReachedEnd() const;
	void HandleEndReached();
};
