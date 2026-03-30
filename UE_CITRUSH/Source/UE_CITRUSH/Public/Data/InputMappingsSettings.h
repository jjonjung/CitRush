// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "InputMappingsSettings.generated.h"

/**
 * Enhanced Input Mapping Context 및 Action 데이터
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FInputMappingData
{
	GENERATED_BODY()

	/** Enhanced Input Mapping Context */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UInputMappingContext> inputMappingContext;

	/** Input Action 맵 (Key: Action 이름, Value: InputAction) */
	UPROPERTY(EditAnywhere)
	TMap<FName, UInputAction*> inputActions;
};

/**
 * Enhanced Input Mapping 캐싱 DeveloperSettings. Context 및 Action 관리
 */
UCLASS(Config=Game, meta=(DisplayName="CachedInputContexts"))
class UE_CITRUSH_API UInputMappingsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

protected:
	/** 프로퍼티 초기화 후 호출. InputMappings 초기 설정 */
	virtual void PostInitProperties() override;

public:
	/** Input Mapping 데이터 맵 (Config 저장) */
	UPROPERTY(Config, EditAnywhere, Category="InputMappingData", meta=(ToolTip="/Script/Data/InputMappingSettings"))
	TMap<FName, FInputMappingData> inputMappings;

	/** 이전 Mapping 데이터 (변경 감지용) */
	TMap<FName, FInputMappingData> prevMappings;

	/** 에디터에서 프로퍼티 변경 시 호출. InputMappings 변경 처리 */
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;

	/** Settings 싱글톤 인스턴스 반환 */
	static const UInputMappingsSettings* Get()
	{
		return GetDefault<UInputMappingsSettings>();
	}
};
