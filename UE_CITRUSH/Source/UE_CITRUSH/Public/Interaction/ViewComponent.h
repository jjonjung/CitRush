// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ViewComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnViewSthByLineTrace, const FHitResult&, hitResult);

/**
 * 카메라 시점 LineTrace Component. Commander가 바라보는 Actor 감지
 */
UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UViewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 생성자 */
	UViewComponent();

protected:
	/** 게임 시작 시 호출. 타이머 시작 */
	virtual void BeginPlay() override;

	/** 게임 종료 시 호출. 타이머 정리 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;;

public:
	/** LineTrace 활성화/비활성화 */
	UFUNCTION(BlueprintCallable)
	void EnableTrace(bool bEnable);

private:
	/** LineTrace 발사. 카메라 시점에서 전방으로 Trace */
	void ShootLineTrace();

	/** 시야각 내에 있는지 확인 */
	bool IsInViewAngle(const AActor* inTarget) const;

public:
	/** LineTrace Hit 이벤트 델리게이트. InteractionComponent에서 구독 */
	UPROPERTY(BlueprintAssignable, BlueprintReadWrite, Category="Event")
	FOnViewSthByLineTrace OnViewSthByLineTrace;

protected:
	/** Owner Pawn */
	UPROPERTY()
	TObjectPtr<APawn> pawnOwner;

	/** Owner의 카메라 컴포넌트 (LineTrace 시작점) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> ownerEye = nullptr;

	/** LineTrace 거리 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Line Trace")
	float traceDistance = 1000.f;

	/** LineTrace 간격 (초 단위, per 3 frames) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Line Trace")
	float traceInterval = 0.05f;  // per 3 frames

	/** 시야각 절반 (도 단위) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="View Angle")
	float halfViewAngle = 60.f;

	/** LineTrace 타이머 핸들 */
	FTimerHandle traceTimer;

private:
	/** Collision Query 파라미터 */
	FCollisionQueryParams queryParams;

	/** Collision Response 파라미터 */
	FCollisionResponseParams responseParams;
};
