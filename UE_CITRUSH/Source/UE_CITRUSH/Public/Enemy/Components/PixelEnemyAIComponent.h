// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Components/ActorComponent.h"
#include "Private/Network/Schemas/HttpV1/HttpRequest2.h"
#include "Private/Network/Schemas/HttpV1/HttpResponse2.h"
#include "PixelEnemyAIComponent.generated.h"

/**
 * PixelEnemy 전용 AI 서버 통신 컴포넌트
 *
 * AIDataManagerComponent를 기반으로 PixelEnemy에 최적화
 * - GetDecision: AI 서버로부터 행동 결정 받기
 * - 자동 요청 타이머
 */

// ============================================================================
// Delegates
// ============================================================================

/** 의사결정 응답 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPixelDecisionResponse, bool, bSuccess, FGetDecisionResponse2, Response);

/** HTTP 에러 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPixelHttpError, int32, StatusCode, FString, ErrorMessage, FFailedResponse, ErrorResponse);

// ============================================================================
// Component
// ============================================================================

UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UPixelEnemyAIComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(PixelEnemyAILog, Log, All)

public:
	UPixelEnemyAIComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ========================================================================
	// Public API Functions
	// ========================================================================

	/**
	 * 의사결정 요청 (PixelEnemy 행동 결정)
	 * POST /api/v1/get_decision
	 */
	UFUNCTION(BlueprintCallable, Category="PixelEnemy|AI")
	void RequestDecision(const FGetDecisionRequest2& Request);

	/**
	 * 자동 의사결정 요청 시작
	 * @param IntervalSeconds - 요청 간격 (초, 기본 3.0초)
	 */
	UFUNCTION(BlueprintCallable, Category="PixelEnemy|AI")
	void StartAutoDecisionRequests(float IntervalSeconds = 3.0f);

	/**
	 * 자동 의사결정 요청 중지
	 */
	UFUNCTION(BlueprintCallable, Category="PixelEnemy|AI")
	void StopAutoDecisionRequests();

	/**
	 * 현재 요청 중인지 확인
	 */
	UFUNCTION(BlueprintPure, Category="PixelEnemy|AI")
	bool IsRequestPending() const { return bIsRequestPending; }

	// ========================================================================
	// Delegates (Blueprint에서 바인딩 가능)
	// ========================================================================

	/** 의사결정 응답 */
	UPROPERTY(BlueprintAssignable, Category="PixelEnemy|Events")
	FOnPixelDecisionResponse OnDecisionResponse;

	/** HTTP 에러 */
	UPROPERTY(BlueprintAssignable, Category="PixelEnemy|Events")
	FOnPixelHttpError OnHttpError;

	// ========================================================================
	// Configuration
	// ========================================================================

	/** AI 서버 URL (예: http://34.64.186.153) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PixelEnemy|Config")
	FString ServerURL;

	/** AI 서버 포트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PixelEnemy|Config")
	int32 ServerPort;

	/** 요청 타임아웃 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PixelEnemy|Config")
	float RequestTimeout;

protected:
	// ========================================================================
	// Internal Functions
	// ========================================================================

	/** HTTP POST 요청 전송 */
	void SendPostRequest(const FString& JsonBody, TFunction<void(FHttpResponsePtr, bool)> OnComplete);

	/** 응답 처리 */
	void HandleResponse(FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(const FString&, int32)> OnSuccess);

	/** 에러 응답 처리 */
	void HandleErrorResponse(int32 StatusCode, const FString& ResponseContent);

private:
	/** 자동 의사결정 타이머 핸들 */
	FTimerHandle AutoDecisionTimer;

	/** 마지막 의사결정 요청 (자동 요청용) */
	FGetDecisionRequest2 LastDecisionRequest;

	/** 요청 진행 중 플래그 */
	bool bIsRequestPending;

	/** GetDecision 엔드포인트 경로 */
	static constexpr const TCHAR* DecisionEndpoint = TEXT("/api/v1/get_decision");
};
