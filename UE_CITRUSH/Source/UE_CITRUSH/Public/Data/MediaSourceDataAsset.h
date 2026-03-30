// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MediaSource.h"
#include "MediaSourceDataAsset.generated.h"

class UMediaSource;
/**
 * MediaSource들을 Key-Value로 관리하는 Primary Data Asset
 */
UCLASS()
class UE_CITRUSH_API UMediaSourceDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Primary Asset ID 반환 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Key로 MediaSource 조회 */
	UFUNCTION(BlueprintCallable)
	UMediaSource* GetMediaSource(const FName& key) const;

protected:
	/** MediaSource Dictionary (Key: 미디어 이름, Value: MediaSource) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Maps")
	TMap<FName, TObjectPtr<UMediaSource>> sources;
};
