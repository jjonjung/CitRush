// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PelletSpawner.generated.h"

class APelletActor;
class UBoxComponent;

/**
 * P-Pellet Spawner
 * 맵에 P-Pellet을 생성하는 스포너
 * CoinSpawner와 유사하지만 P-Pellet 전용
 */
UCLASS()
class UE_CITRUSH_API APelletSpawner : public AActor
{
	GENERATED_BODY()

public:
	APelletSpawner();

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

	/** Spawn Volume */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|Components")
	TObjectPtr<UBoxComponent> SpawnVolume;

	// ==================== Spawn Settings ====================

	/** P-Pellet Class */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Pellet Class"))
	TSubclassOf<APelletActor> PelletClass;

	/** P-Pellet Count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (ClampMin = "0", ClampMax = "100", DisplayName = "Pellet Count"))
	int32 PelletCount = 4;

	/** P-Pellet 간 최소 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (ClampMin = "100.0", DisplayName = "Min Distance Between Pellets"))
	float MinDistanceBetweenPellets = 450.0f;

	/** 바닥으로부터의 높이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Height Offset"))
	float HeightOffset = 400.0f;

	/** 랜덤 시드 (재현 가능한 배치) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Random Seed"))
	int32 RandomSeed = 0;

	/** 시드 사용 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Use Random Seed"))
	bool bUseRandomSeed = false;

	/** 자동 생성 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Auto Spawn On Begin Play"))
	bool bAutoSpawnOnBeginPlay = true;

	/** 고정 위치 사용 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Use Fixed Locations"))
	bool bUseFixedLocations = true;

	/** 고정 스폰 위치 배열 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Settings", meta = (DisplayName = "Fixed Spawn Locations", EditCondition = "bUseFixedLocations"))
	TArray<FVector> FixedSpawnLocations;

	/** 디버그 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner|Debug", meta = (DisplayName = "Show Debug Visualization"))
	bool bShowDebugVisualization = true;

	/** 생성된 Pellet 배열 */
	UPROPERTY(BlueprintReadOnly, Category = "Spawner|Runtime")
	TArray<TObjectPtr<APelletActor>> SpawnedPellets;

	// ==================== Functions ====================

public:
	/** P-Pellet 생성 */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SpawnPellets();

	/** 생성된 P-Pellet 제거 */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void ClearSpawnedPellets();

	/** P-Pellet 개수 설정 */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SetPelletCount(int32 NewCount);

	/** 랜덤 위치 생성 */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	FVector GetRandomLocationInVolume() const;

	/** 위치 유효성 검사 */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	bool IsLocationValid(const FVector& Location, const TArray<FVector>& ExistingLocations) const;

	/** 생성된 P-Pellet 개수 반환 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spawner")
	int32 GetSpawnedPelletCount() const { return SpawnedPellets.Num(); }

	/**
	 * AI 서버로 전송할 P-Pellet 데이터 수집
	 * @return AI 서버 전송용 P-Pellet 위치 데이터 배열
	 */
	UFUNCTION(BlueprintCallable, Category = "Spawner|AI")
	TArray<struct FPPelletLocation> CollectPPelletData() const;

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
