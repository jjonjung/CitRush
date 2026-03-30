// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "SplinePathActor.generated.h"

/**
 * Spline Path Actor - Enemy or AI can follow this path
 * This is a reusable path that can be placed in the level
 */
UCLASS()
class UE_CITRUSH_API ASplinePathActor : public AActor
{
	GENERATED_BODY()

public:
	ASplinePathActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineComponent> SplineComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Settings")
	bool bClosedLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Settings")
	bool bDrawDebugPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Settings")
	FLinearColor DebugPathColor = FLinearColor::Green;

	// Spline Points 수동 저장/로드	
	UPROPERTY(EditAnywhere, Category = "Path Settings|Advanced")
	TArray<FVector> SavedSplinePoints;

protected:
	// Editor에서 Spline 변경 시 자동 저장
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

public:
	// Spline 정보 가져오기 헬퍼 함수
	UFUNCTION(BlueprintCallable, Category = "Spline Path")
	USplineComponent* GetSplineComponent() const { return SplineComponent; }

	UFUNCTION(BlueprintCallable, Category = "Spline Path")
	float GetSplineLength() const;

	UFUNCTION(BlueprintCallable, Category = "Spline Path")
	FVector GetLocationAtDistanceAlongSpline(float Distance) const;

	UFUNCTION(BlueprintCallable, Category = "Spline Path")
	FRotator GetRotationAtDistanceAlongSpline(float Distance) const;

	UFUNCTION(BlueprintCallable, Category = "Spline Path")
	int32 GetNumberOfSplinePoints() const;

#if WITH_EDITOR
	// Spline 포인트를 배열에 저장
	UFUNCTION(CallInEditor, Category = "Path Settings|Advanced")
	void SaveSplinePointsToArray();

	// 배열에서 Spline 포인트 복원
	UFUNCTION(CallInEditor, Category = "Path Settings|Advanced")
	void LoadSplinePointsFromArray();
#endif
};
