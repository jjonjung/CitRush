// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/WidgetDataAsset.h"
#include "Blueprint/UserWidget.h"

FPrimaryAssetId UWidgetDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId("WidgetDataAsset", GetFName());
}
