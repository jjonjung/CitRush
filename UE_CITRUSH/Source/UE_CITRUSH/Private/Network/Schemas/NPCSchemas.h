#pragma once

#include "CoreMinimal.h"
#include "NPCSchemas.generated.h"

USTRUCT()
struct UE_CITRUSH_API FNPCConditionRequest
{
	GENERATED_BODY()
};

USTRUCT()
struct UE_CITRUSH_API FAICommandResponse
{
	GENERATED_BODY()
};


/**
 * 중첩 객체 테스트용 구조체
 * JSON Example:
 * {
 *   "nested_field": "nested_value",
 *   "nested_number": 123.45
 * }
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FTestObject
{
	GENERATED_BODY()

	UPROPERTY()
	FString nested_field;

	UPROPERTY()
	double nested_number = 0.0;
};

/**
 * HTTP 요청 테스트용 구조체
 * Endpoint: POST http://localhost:8000/test
 * JSON Example: Schemas/JsonExamples/request_example.json
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FTestRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FString test_message;

	UPROPERTY()
	int32 test_number = 0;

	UPROPERTY()
	TArray<FString> test_array;

	UPROPERTY()
	FTestObject test_object;
};

/**
 * HTTP 응답 테스트용 구조체
 * JSON Example: Schemas/JsonExamples/response_example.json
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FTestResponse
{
	GENERATED_BODY()

	UPROPERTY()
	FString method;

	UPROPERTY()
	FString status;

	UPROPERTY()
	FTestRequest received;

	UPROPERTY()
	FString timestamp;
};