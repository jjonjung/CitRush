// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoinSpawner.generated.h"

class ACoinActor;
class UBoxComponent;

UCLASS()
class UE_CITRUSH_API ACoinSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACoinSpawner();

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// ==================== Components ====================

	/** Root Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|Components")
	TObjectPtr<USceneComponent> RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|Components")
	TObjectPtr<UBoxComponent> SpawnVolume;

	// ==================== Spawn Settings ====================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Coin Class"))
	TSubclassOf<ACoinActor> CoinClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (ClampMin = "0", ClampMax = "1000", DisplayName = "Coin Count"))
	int32 CoinCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (ClampMin = "1", DisplayName = "Coin Value"))
	int32 CoinValue = 100;

	//Coin 간 최소 거리	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (ClampMin = "10.0", DisplayName = "Min Distance Between Coins"))
	float MinDistanceBetweenCoins = 200.0f;
	//바닥으로부터의 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Height Offset"))
	float HeightOffset = 150.0f;
	//랜덤 시드 (재현 가능한 배치) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Random Seed"))
	int32 RandomSeed = 0;
	// 시드 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Use Random Seed"))
	bool bUseRandomSeed = false;
	//자동 생성 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Auto Spawn On Begin Play"))
	bool bAutoSpawnOnBeginPlay = false;

	// 고정 위치 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Use Fixed Locations"))
	bool bUseFixedLocations = true;

	// 고정 스폰 위치 배열 (bUseFixedLocations가 true일 때 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Fixed Spawn Locations", EditCondition = "bUseFixedLocations"))
	TArray<FVector> FixedSpawnLocations;

	//디버그 표시
	// ==================== Debug Settings ====================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Debug", meta = (DisplayName = "Show Debug Visualization"))
	bool bShowDebugVisualization = true;

	UPROPERTY(BlueprintReadOnly, Category = "Spawner|Runtime")
	TArray<TObjectPtr<ACoinActor>> SpawnedCoins;

	// ==================== Functions ====================

public:
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SpawnCoins();

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void ClearSpawnedCoins();

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SetCoinCount(int32 NewCount);

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	FVector GetRandomLocationInVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	bool IsLocationValid(const FVector& Location, const TArray<FVector>& ExistingLocations) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spawner")
	int32 GetSpawnedCoinCount() const { return SpawnedCoins.Num(); }

	/**
	 * AI 서버로 전송할 P-Point 데이터 수집
	 * SpawnedCoins 배열을 순회하여 FPPointLocation 배열로 변환
	 * @return AI 서버 전송용 P-Point 위치 데이터 배열
	 */
	UFUNCTION(BlueprintCallable, Category = "Spawner|AI")
	TArray<struct FPPointLocation> CollectPPointData() const;

protected:
	FRandomStream RandomStream;

	/**
	 * 위치를 NavMesh에 투영
	 * @param Location 원본 위치
	 * @param OutProjectedLocation 투영된 위치 (출력)
	 * @return NavMesh 위에 투영 성공 여부
	 */
	bool ProjectToNavMesh(const FVector& Location, FVector& OutProjectedLocation) const;
};
