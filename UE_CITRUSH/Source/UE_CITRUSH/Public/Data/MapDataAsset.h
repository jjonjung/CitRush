// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MapDataAsset.generated.h"

/**
 * 맵 정보 (이름, 에셋 경로, 최대 플레이어 수)
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FMapInfo
{
	GENERATED_BODY()

protected:
	/** 맵 표시 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	FText displayName;

	/** 맵 에셋 소프트 참조 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo", meta=(AllowedClasses="/Script/Engine.World"))
	TSoftObjectPtr<UWorld> mapAsset;

	/** 최대 플레이어 수 (1~8) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo", meta=(ClampMin=1, ClampMax=8))
	int32 maxPlayers;

public:
	/** 기본 생성자 */
	FMapInfo()
		:displayName(FText::GetEmpty())
		, maxPlayers(1)
	{}

	/** 맵 표시 이름 반환 */
	FText GetDisplayName() const {return displayName;}

	/** 최대 플레이어 수 반환 */
	int32 GetMaxPlayers() const {return maxPlayers;}

	/** 맵 에셋 소프트 참조 반환 */
	TSoftObjectPtr<UWorld> GetMapAsset() const {return mapAsset;}
};

/**
 * 맵 데이터를 Key-Value로 관리하는 Primary Data Asset
 */
UCLASS(BlueprintType)
class UE_CITRUSH_API UMapDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Primary Asset ID 반환 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Key로 맵 정보 조회 */
	UFUNCTION(BlueprintCallable)
	bool GetMapInfoByKey(const FName& inMapKey, FMapInfo& outMapInfo) const;

	/** 전체 맵 Dictionary 반환 */
	TMap<FName, FMapInfo> GetMapDictionary() const {return maps;}

protected:
	/** 맵 정보 Dictionary (Key: 맵 이름, Value: 맵 정보) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Maps")
	TMap<FName, FMapInfo> maps;

};
