// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Base64Converter.generated.h"

/**
 * Base64 인코딩/디코딩 Blueprint Function Library
 */
UCLASS()
class UE_CITRUSH_API UBase64Converter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** FString을 Base64로 인코딩 */
	UFUNCTION(BlueprintCallable, Category = "Base64|Encoding")
	static FString StringToBase64(const FString& inString);
	/** Base64를 FString으로 디코딩 */
	UFUNCTION(BlueprintCallable, Category = "Base64|Decoding")
	static FString Base64ToString(const FString& inBase64);
};
