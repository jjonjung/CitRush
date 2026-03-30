// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/MediaSourceDataAsset.h"

FPrimaryAssetId UMediaSourceDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId("UMediaSourceDataAsset", GetFName());
}

UMediaSource* UMediaSourceDataAsset::GetMediaSource(const FName& key) const
{
	if (const TObjectPtr<UMediaSource>* source = sources.Find(key))
	{
		return *source;
	}
	return nullptr;
}
