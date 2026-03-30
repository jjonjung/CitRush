// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ItemData.generated.h"

class UGameplayEffect;
class UGameplayAbility;
/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API UItemData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> icon;

	/** 레이서 UI에 표시할 아이콘 (Commander 아이콘과 다를 수 있음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> racerIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayAbility> grantedAbilityClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> coolDownEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> durationEffectClass;

	/** 어빌리티에 전달할 강도 값 (예: 부스트 힘, 데미지 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PowerValue = 0.0f;

};

USTRUCT(BlueprintType)
struct FItemTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> icon;

	/** 레이서 UI에 표시할 아이콘 (Commander 아이콘과 다를 수 있음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> racerIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag tag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UItemData> itemData;

};