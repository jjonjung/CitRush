// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/MapDataAsset.h"

FPrimaryAssetId UMapDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId("MapDataAsset", GetFName());
}

bool UMapDataAsset::GetMapInfoByKey(const FName& inMapKey, FMapInfo& outMapInfo) const
{
	if (const FMapInfo* mapInfo = maps.Find(inMapKey);
		mapInfo != nullptr)
	{
		outMapInfo = *mapInfo;
		return true;
	}
	return false;
}