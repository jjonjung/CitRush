// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WidgetBlueprintDataAsset.generated.h"

class UUserWidget;

/**
 * Widget Blueprint 클래스들을 Key-Value로 관리하는 Primary Data Asset
 */
UCLASS()
class UE_CITRUSH_API UWidgetBlueprintDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	/** Primary Asset ID 반환 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Key로 Widget Blueprint 클래스 조회 */
	UFUNCTION(BlueprintCallable)
	TSubclassOf<UUserWidget> GetWidgetBlueprintClassByKey(const FName& inWBPKey) const;

protected:
	/** Widget Blueprint 클래스 Dictionary (Key: Widget 이름, Value: Widget 클래스) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WidgetBlueprintDataAsset")
	TMap<FName, TSubclassOf<UUserWidget>> widgetBlueprintClasses;
	
};
