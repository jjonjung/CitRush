// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/WidgetBlueprintDataAsset.h"

FPrimaryAssetId UWidgetBlueprintDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId("WidgetBlueprintDataAsset", GetFName());
}

TSubclassOf<UUserWidget> UWidgetBlueprintDataAsset::GetWidgetBlueprintClassByKey(const FName& inWBPKey) const
{
	if (const TSubclassOf<UUserWidget>* outClass = widgetBlueprintClasses.Find(inWBPKey))
	{
		return *outClass;
	}
	return nullptr;
}
