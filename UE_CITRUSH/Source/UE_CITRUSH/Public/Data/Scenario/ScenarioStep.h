#pragma once

#include "Data/CitRushHttpData.h"
#include "ScenarioStep.generated.h"

// ============================================================================
// 사용 예시 (DataTable 생성 가이드)
// ============================================================================

/**
 * DataTable 에셋 생성 방법:
 *
 * 1. DT_HttpApiEndpoints (API 엔드포인트 설정)
 *    - Content Browser → 우클릭 → Data Table
 *    - Row Structure: FHttpApiEndpointRow
 *    - 각 API별 Row 추가:
 *      * HealthCheck_V1
 *      * MatchStart_V1_2
 *      * GetDecision_V1_2
 *      * MatchEnd_V1_2
 *      * OverseerTTS_V1_2
 *      * CommanderReport_V1_2
 *
 * 2. DT_HttpRequestTemplates (테스트용 Request 템플릿)
 *    - Row Structure: FHttpRequestTemplateRow
 *    - 각 시나리오별 샘플 Request 추가
 *
 * 3. DT_HttpResponseSamples (테스트용 Response 샘플)
 *    - Row Structure: FHttpResponseSampleRow
 *    - 각 시나리오별 샘플 Response 추가
 *
 * CSV Import 예시 (DT_HttpApiEndpoints):
 *
 * Name,ApiName,Description,Category,ApiVersion,Method,Path,Timeout,RequestTypeName,ResponseTypeName,bEnabled
 * HealthCheck_V1,"Health Check","서버 상태 확인",Health,V1_1,GET,"/api/v1/health",3.0,"","FHealthResponse",true
 * MatchStart_V1_2,"Match Start","매치 시작",Match,V1_2,POST,"/api/v1/match/start",5.0,"FMatchStartRequest2","FMatchStartResponse",true
 * GetDecision_V1_2,"Get Decision","AI 의사결정 요청",Decision,V1_2,POST,"/api/v1/get_decision",10.0,"FGetDecisionRequest2","FGetDecisionResponse2",true
 * MatchEnd_V1_2,"Match End","매치 종료",Match,V1_2,POST,"/api/v1/match/end",5.0,"FMatchEndRequest2","FMatchEndResponse",true
 * OverseerTTS_V1_2,"Overseer TTS","TTS 조회",Overseer,V1_2,POST,"/api/v1/overseer/tts",5.0,"FOverseerTTSRequest2","FOverseerTTSResponse2",true
 * CommanderReport_V1_2,"Commander Report","지휘관 리포트",Commander,V1_2,GET,"/api/v1/commander/report",5.0,"","FCommanderReportResponse2",true
 */

// ============================================================================
// Scenario DataTable Rows (시나리오 기반 플레이 흐름 테스트)
// ============================================================================

/**
 * 시나리오 정의 DataTable Row
 *
 * 전체 플레이 흐름을 시나리오로 정의합니다.
 * DataTable 에셋: DT_GameScenarios
 *
 * Row Name 규칙: Scenario_[Name] (예: Scenario_EarlyGameNormal)
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FGameScenarioRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 시나리오 이름 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scenario")
	FText ScenarioName;

	/** 시나리오 설명 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scenario")
	FText Description;

	/** 게임 페이즈 (필수): EARLY_GAME, MID_GAME, LATE_GAME */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scenario")
	FString GamePhase = TEXT("EARLY_GAME");

	/** 예상 플레이 시간 (초) (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scenario")
	float ExpectedDurationSeconds = 60.0f;

	/** 시나리오 난이도 (선택): EASY, NORMAL, HARD */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scenario")
	FString Difficulty = TEXT("NORMAL");

	/** 시나리오 스텝 Row Name 목록 (필수)
	 * FScenarioStepRow의 Row Name 배열
	 * 실행 순서대로 나열
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scenario")
	TArray<FName> StepRowNames;

	/** 활성화 여부 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scenario")
	bool bEnabled = true;

	/** 태그 목록 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scenario")
	TArray<FName> Tags;

	FGameScenarioRow()
		: ScenarioName(FText::FromString(TEXT("Unnamed Scenario")))
		, Description(FText::GetEmpty())
		, GamePhase(TEXT("EARLY_GAME"))
		, ExpectedDurationSeconds(60.0f)
		, Difficulty(TEXT("NORMAL"))
		, bEnabled(true)
	{
	}
};


/**
 * 시나리오 스텝 DataTable Row
 *
 * 시나리오의 각 단계별 API 호출을 정의합니다.
 * DataTable 에셋: DT_ScenarioSteps
 *
 * Row Name 규칙: Step_[ScenarioName]_[StepNumber] (예: Step_EarlyGame_01_MatchStart)
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FScenarioStepRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 스텝 이름 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step")
	FText StepName;

	/** 스텝 설명 (선택) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step")
	FText Description;

	/** API 엔드포인트 Row Name (필수)
	 * FHttpApiEndpointRow의 Row Name 참조
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step")
	FName ApiEndpointRowName;

	/** Request 템플릿 Row Name (필수)
	 * FHttpRequestTemplateRow의 Row Name 참조
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step")
	FName RequestTemplateRowName;

	/** Mock Response 샘플 Row Name (필수)
	 * FHttpResponseSampleRow의 Row Name 참조
	 * AI 서버 없을 때 이 응답 사용
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step")
	FName MockResponseSampleRowName;

	/** 대기 시간 (초) (선택)
	 * 이전 스텝 실행 후 다음 스텝까지 대기 시간
	 * 0 = 즉시 실행
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step")
	float DelayBeforeExecutionSeconds = 0.0f;

	/** Mock 모드 강제 사용 (선택)
	 * true: 서버 호출 없이 항상 Mock Response 사용
	 * false: 서버 연결 시 실제 호출, 실패 시 Mock 사용
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step")
	bool bForceMockMode = false;

	/** 실패 시 계속 진행 (선택)
	 * true: 이 스텝 실패해도 다음 스텝 계속
	 * false: 실패 시 시나리오 중단
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step")
	bool bContinueOnFailure = false;

	/** 활성화 여부 (필수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step")
	bool bEnabled = true;

	FScenarioStepRow()
		: StepName(FText::FromString(TEXT("Unnamed Step")))
		, Description(FText::GetEmpty())
		, ApiEndpointRowName(NAME_None)
		, RequestTemplateRowName(NAME_None)
		, MockResponseSampleRowName(NAME_None)
		, DelayBeforeExecutionSeconds(0.0f)
		, bForceMockMode(false)
		, bContinueOnFailure(false)
		, bEnabled(true)
	{
	}
};
