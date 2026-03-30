// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Components/ActorComponent.h"
#include "Private/Network/Schemas/HttpV1/HttpRequest2.h"
#include "Private/Network/Schemas/HttpV1/HttpResponse2.h"
#include "AIDataManagerComponent.generated.h"


/**
 * AI Agent HTTP API v1.2.0 통신 컴포넌트
 *
 * 주요 엔드포인트:
 * - GET  /api/v1/health              : 서버 헬스체크 (v1.1 재사용)
 * - POST /api/v1/match/start         : 매치 시작 (v1.2.0 - player_types 추가)
 * - POST /api/v1/get_decision        : 의사결정 요청 (v1.2.0 - NavMesh 필드 추가)
 * - POST /api/v1/match/end           : 매치 종료 (v1.1 재사용)
 * - POST /api/v1/overseer/tts        : Overseer TTS 요청 (v1.2.0 신규)
 * - GET  /api/v1/commander/report    : Commander 리포트 조회 (v1.2.0 신규)
 */
UENUM(BlueprintType)
enum class EHttpEndpoint : uint8
{
	Health         		UMETA(DisplayName = "Health Check"),
	MatchStart     		UMETA(DisplayName = "Match Start"),
	GetDecision    		UMETA(DisplayName = "Get Decision"),
	OverseerTTS    		UMETA(DisplayName = "Overseer TTS"),		// v1.2.0 신규
	CommanderReport		UMETA(DisplayName = "Commander Report"),	// v1.2.0 신규
	MatchEnd       		UMETA(DisplayName = "Match End")
};

// ============================================================================
// Delegates
// ============================================================================

/** 헬스체크 응답 델리게이트 (v1.1 재사용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthCheckResponse, bool, bSuccess, FHealthResponse, Response);

/** 매치 시작 응답 델리게이트 (v1.1 재사용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchStartResponse, bool, bSuccess, FMatchStartResponse, Response);

/** 의사결정 응답 델리게이트 v1.2.0 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDecisionResponse2, bool, bSuccess, FGetDecisionResponse2, Response);

/** 매치 종료 응답 델리게이트 (v1.1 재사용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEndResponse, bool, bSuccess, FMatchEndResponse, Response);

/** Overseer TTS 응답 델리게이트 v1.2.0 (신규) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOverseerTTSResponse, bool, bSuccess, FOverseerTTSResponse2, Response);

/** Commander 리포트 응답 델리게이트 v1.2.0 (신규) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCommanderReportResponse, bool, bSuccess, FCommanderReportResponse2, Response);

/** HTTP 에러 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHttpError, int32, StatusCode, FString, ErrorMessage, FFailedResponse, ErrorResponse);

// ============================================================================
// Component
// ============================================================================

UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UAIDataManagerComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(CitRushHttpLog, Log, All)

public:
	UAIDataManagerComponent();
	virtual void BeginPlay() override;

	// ========================================================================
	// Public API Functions
	// ========================================================================

	/**
	 * 서버 헬스체크
	 * GET /api/v1/health
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Http|API")
	void CheckHealth();

	/**
	 * 매치 시작
	 * POST /api/v1/match/start
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Http|API")
	void StartMatch();
	//void StartMatch(const FMatchStartRequest& Request);

	/**
	 * 의사결정 요청 v1.2.0 (2~3초 주기로 호출)
	 * POST /api/v1/get_decision
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Http|API")
	void GetDecision(const FGetDecisionRequest2& Request);

	/**
	 * 매치 종료 (v1.1 재사용)
	 * POST /api/v1/match/end
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Http|API")
	void EndMatch(/*const FMatchEndRequest& Request*/);

	/**
	 * Overseer TTS 요청 v1.2.0 (신규)
	 * POST /api/v1/overseer/tts
	 */
	UFUNCTION(BlueprintCallable, Category="Http|API")
	void GetOverseerTTS(const FOverseerTTSRequest2& Request);

	/**
	 * Commander 리포트 조회 v1.2.0 (신규)
	 * GET /api/v1/commander/report
	 */
	UFUNCTION(BlueprintCallable, Category="Http|API")
	void GetCommanderReport(const FString& RoomID);

	/**
	 * 자동 의사결정 요청 시작 (타이머 사용)
	 * @param IntervalSeconds - 요청 간격 (초, 기본 2.5초)
	 */
	UFUNCTION(BlueprintCallable, Category="Http|Auto")
	void StartAutoDecisionRequests(float IntervalSeconds = 2.5f);

	/**
	 * 자동 의사결정 요청 중지
	 */
	UFUNCTION(BlueprintCallable, Category="Http|Auto")
	void StopAutoDecisionRequests();

	// ========================================================================
	// Delegates (Blueprint에서 바인딩 가능)
	// ========================================================================

	UPROPERTY(BlueprintAssignable, Category="Http|Events")
	FOnHealthCheckResponse OnHealthCheckResponse;

	UPROPERTY(BlueprintAssignable, Category="Http|Events")
	FOnMatchStartResponse OnMatchStartResponse;

	/** v1.2.0 의사결정 응답 */
	UPROPERTY(BlueprintAssignable, Category="Http|Events")
	FOnDecisionResponse2 OnDecisionResponse;

	UPROPERTY(BlueprintAssignable, Category="Http|Events")
	FOnMatchEndResponse OnMatchEndResponse;

	/** v1.2.0 Overseer TTS 응답 (신규) */
	UPROPERTY(BlueprintAssignable, Category="Http|Events")
	FOnOverseerTTSResponse OnOverseerTTSResponse;

	/** v1.2.0 Commander 리포트 응답 (신규) */
	UPROPERTY(BlueprintAssignable, Category="Http|Events")
	FOnCommanderReportResponse OnCommanderReportResponse;

	UPROPERTY(BlueprintAssignable, Category="Http|Events")
	FOnHttpError OnHttpError;



protected:
	// ========================================================================
	// Internal Functions
	// ========================================================================

	/** HTTP GET 요청 전송 */
	void SendGetRequest(const FHttpEndpointConfig& Config, TFunction<void(FHttpResponsePtr, bool)> OnComplete);

	/** HTTP POST 요청 전송 */
	void SendPostRequest(const FHttpEndpointConfig& Config, const FString& JsonBody, TFunction<void(FHttpResponsePtr, bool)> OnComplete);

	/** 공통 응답 처리 */
	void HandleResponse(FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(const FString&, int32)> OnSuccess);

	/** 에러 응답 파싱 및 델리게이트 호출 */
	void HandleErrorResponse(int32 StatusCode, const FString& ResponseContent);

	// ========================================================================
	// Configuration
	// ========================================================================
public:
	/** AI 서버 URL (예: http://localhost:8000) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Http|Config")
	FString ServerURL;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Http|Config")
	int32 PortURL;

protected:
	/** 엔드포인트 설정 맵 (Blueprint에서 수정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Http|Config")
	TMap<EHttpEndpoint, FHttpEndpointConfig> Endpoints;

private:
	/** 자동 의사결정 타이머 핸들 */
	FTimerHandle AutoDecisionTimer;

	/** 마지막 의사결정 요청 v1.2.0 (자동 요청용) */
	FGetDecisionRequest2 LastDecisionRequest;

	/** 엔드포인트 설정을 가져오는 헬퍼 함수 */
	const FHttpEndpointConfig* GetEndpointConfig(EHttpEndpoint Endpoint) const;
};
