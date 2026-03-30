// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/DataTypeObjectManager.h"

UDataTable* UDataTypeObjectManager::GetTableAsset(FName key) const
{
	if (const TSoftObjectPtr<UDataTable>* table = DataTableDict.Find(key))
	{
		return table->IsNull() ? nullptr : table->LoadSynchronous();
	}
	
	return nullptr;
}

UCurveFloat* UDataTypeObjectManager::GetCurveFloatAsset(FName key) const
{
	if (const TSoftObjectPtr<UCurveFloat>* curve = CurveFloatDict.Find(key))
	{
		return curve->IsNull() ? nullptr : curve->LoadSynchronous();
	}
	return nullptr;
}
