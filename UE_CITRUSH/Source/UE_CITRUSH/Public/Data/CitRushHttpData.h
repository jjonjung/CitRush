// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CitRushHttpData.generated.h"

/**
 * AI Agent HTTP API - DataTable Row Structures
 *
 * 이 파일은 HTTP API 엔드포인트 설정과 메타데이터를 DataTable Row로 정의합니다.
 *
 * Protocol 문서:
 * - v1.1: AgentLog/Protocol.md
 * - v1.2.0: AgentLog/[AI 에이전트]언리얼팀 통합 연동 가이드_v1.2.0
 *
 * 관련 스키마 파일:
 * - RequestHttp.h / HttpRequest2.h (Request 구조체)
 * - ResponseHttp.h / HttpResponse2.h (Response 구조체)
 */

// ============================================================================
// Enums
// ============================================================================

/**
 * HTTP 메서드
 */
UENUM(BlueprintType)
enum class EHttpMethod : uint8
{
	GET     UMETA(DisplayName = "GET"),
	POST    UMETA(DisplayName = "POST"),
	PUT     UMETA(DisplayName = "PUT"),
	DELETE  UMETA(DisplayName = "DELETE"),
	PATCH   UMETA(DisplayName = "PATCH")
};

/**
 * API 버전
 */
UENUM(BlueprintType)
enum class EHttpApiVersion : uint8
{
	V1_0    UMETA(DisplayName = "v1.0"),
	V1_1    UMETA(DisplayName = "v1.1"),
	V1_2    UMETA(DisplayName = "v1.2.0")
};

/**
 * API 카테고리
 */
UENUM(BlueprintType)
enum class EHttpApiCategory : uint8
{
	/** 헬스체크 */
	Health      UMETA(DisplayName = "Health"),

	/** 매치 관리 (시작/종료) */
	Match       UMETA(DisplayName = "Match"),

	/** AI 의사결정 */
	Decision    UMETA(DisplayName = "Decision"),

	/** Overseer TTS */
	Overseer    UMETA(DisplayName = "Overseer"),

	/** 지휘관 리포트 */
	Commander   UMETA(DisplayName = "Commander")
};

// ============================================================================
// HTTP API Endpoint DataTable Row (v1.1 & v1.2.0)
// ============================================================================

/**
 * HTTP API 엔드포인트 DataTable Row
 *
 * 각 API의 메타데이터와 설정을 저장합니다.
 * DataTable 에셋: DT_HttpApiEndpoints
 *
 * Row Name 규칙: [ApiName]_[Version] (예: HealthCheck_V1, GetDecision_V1_2)
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FHttpApiEndpointRow : public FTableRowBase
{
	GENERATED_BODY()

	// ======== API 식별 정보 ========

	/** API 이름 (필수)
	 * 예: "Health Check", "Get Decision", "Match Start"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Identity")
	FText ApiName;

	/** API 설명 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Identity")
	FText Description;

	/** API 카테고리 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Identity")
	EHttpApiCategory Category = EHttpApiCategory::Health;

	/** API 버전 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Identity")
	EHttpApiVersion ApiVersion = EHttpApiVersion::V1_1;

	// ======== 엔드포인트 설정 ========

	/** HTTP 메서드 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Endpoint")
	EHttpMethod Method = EHttpMethod::GET;

	/** 엔드포인트 경로 (필수)
	 * 예: "/api/v1/health", "/api/v1/get_decision"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Endpoint")
	FString Path;

	/** 타임아웃 (초) (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Endpoint")
	float Timeout = 5.0f;

	/** Content-Type (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Endpoint")
	FString ContentType = TEXT("application/json");

	// ======== Request/Response 타입 정보 ========

	/** Request 구조체 타입 이름 (선택)
	 * 예: "FHealthRequest", "FGetDecisionRequest2"
	 * GET 요청은 비어있을 수 있음
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Schema")
	FString RequestTypeName;

	/** Response 구조체 타입 이름 (필수)
	 * 예: "FHealthResponse", "FGetDecisionResponse2"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Schema")
	FString ResponseTypeName;

	/** 실패 응답 타입 (선택)
	 * 기본값: "FFailedResponse"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Schema")
	FString FailedResponseTypeName = TEXT("FFailedResponse");

	// ======== 동작 플래그 ========

	/** 활성화 여부 (필수)
	 * false로 설정하면 해당 API를 비활성화
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Config")
	bool bEnabled = true;

	/** 재시도 허용 여부 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Config")
	bool bAllowRetry = true;

	/** 최대 재시도 횟수 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Config")
	int32 MaxRetryCount = 3;

	/** 로깅 활성화 여부 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Config")
	bool bEnableLogging = true;

	// ======== 메타데이터 ========

	/** 프로토콜 문서 참조 (선택)
	 * 예: "AgentLog/Protocol.md#health-check"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Meta")
	FString DocumentationReference;

	/** 구현 노트 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Meta")
	FText ImplementationNotes;

	/** 마지막 업데이트 날짜 (선택)
	 * 형식: YYMMDD (예: "251217")
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "API|Meta")
	FString LastUpdated;

	// ======== 생성자 ========

	FHttpApiEndpointRow()
		: ApiName(FText::FromString(TEXT("Unnamed API")))
		, Description(FText::GetEmpty())
		, Category(EHttpApiCategory::Health)
		, ApiVersion(EHttpApiVersion::V1_1)
		, Method(EHttpMethod::GET)
		, Path(TEXT("/"))
		, Timeout(5.0f)
		, ContentType(TEXT("application/json"))
		, RequestTypeName(TEXT(""))
		, ResponseTypeName(TEXT(""))
		, FailedResponseTypeName(TEXT("FFailedResponse"))
		, bEnabled(true)
		, bAllowRetry(true)
		, MaxRetryCount(3)
		, bEnableLogging(true)
		, DocumentationReference(TEXT(""))
		, ImplementationNotes(FText::GetEmpty())
		, LastUpdated(TEXT(""))
	{
	}
};

// ============================================================================
// HTTP Request Template DataTable Row (테스트/디버깅용)
// ============================================================================

/**
 * HTTP Request 템플릿 DataTable Row
 *
 * 테스트 및 디버깅을 위한 샘플 Request 데이터를 저장합니다.
 * DataTable 에셋: DT_HttpRequestTemplates
 *
 * Row Name 규칙: [ApiName]_[TemplateName] (예: GetDecision_SampleEarlyGame)
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FHttpRequestTemplateRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 템플릿 이름 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Template")
	FText TemplateName;

	/** 템플릿 설명 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Template")
	FText Description;

	/** API 엔드포인트 Row Name (필수)
	 * FHttpApiEndpointRow의 Row Name 참조
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Template")
	FName ApiEndpointRowName;

	/** Request JSON 템플릿 (필수)
	 * 샘플 Request 데이터를 JSON 문자열로 저장
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Template", meta = (MultiLine = true))
	FString RequestJsonTemplate;

	/** 활성화 여부 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Template")
	bool bEnabled = true;

	/** 태그 목록 (선택)
	 * 예: "EarlyGame", "TestData", "LowHealth"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Template")
	TArray<FName> Tags;

	FHttpRequestTemplateRow()
		: TemplateName(FText::FromString(TEXT("Unnamed Template")))
		, Description(FText::GetEmpty())
		, ApiEndpointRowName(NAME_None)
		, RequestJsonTemplate(TEXT("{}"))
		, bEnabled(true)
	{
	}
};

// ============================================================================
// HTTP Response Sample DataTable Row (테스트/디버깅용)
// ============================================================================

/**
 * HTTP Response 샘플 DataTable Row
 *
 * 테스트 및 디버깅을 위한 샘플 Response 데이터를 저장합니다.
 * DataTable 에셋: DT_HttpResponseSamples
 *
 * Row Name 규칙: [ApiName]_[SampleName] (예: GetDecision_SuccessEarlyGame)
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FHttpResponseSampleRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 샘플 이름 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sample")
	FText SampleName;

	/** 샘플 설명 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sample")
	FText Description;

	/** API 엔드포인트 Row Name (필수)
	 * FHttpApiEndpointRow의 Row Name 참조
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sample")
	FName ApiEndpointRowName;

	/** Response JSON 샘플 (필수)
	 * 샘플 Response 데이터를 JSON 문자열로 저장
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sample", meta = (MultiLine = true))
	FString ResponseJsonSample;

	/** HTTP 상태 코드 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sample")
	int32 HttpStatusCode = 200;

	/** 성공 여부 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sample")
	bool bIsSuccess = true;

	/** 활성화 여부 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sample")
	bool bEnabled = true;

	/** 태그 목록 (선택)
	 * 예: "Success", "Error", "Timeout"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sample")
	TArray<FName> Tags;

	FHttpResponseSampleRow()
		: SampleName(FText::FromString(TEXT("Unnamed Sample")))
		, Description(FText::GetEmpty())
		, ApiEndpointRowName(NAME_None)
		, ResponseJsonSample(TEXT("{}"))
		, HttpStatusCode(200)
		, bIsSuccess(true)
		, bEnabled(true)
	{
	}
};
