// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BaseAISubsystem.generated.h"

// Brain Cam 데이터 업데이트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBrainCamDataReceived, const FString&, Summary, const FString&, FinalChoice, const TArray<FString>&, ReasoningDocs);

// AI 서버 연결 상태 변경 델리게이트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAIConnectionChanged, bool /*bConnected*/);

/**
 * AI Subsystem 기본 클래스
 * HTTP/Socket 연결 및 공통 통신 기능 제공
 */
UCLASS(Abstract, Config=Game)
class UE_CITRUSH_API UBaseAISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(LogBaseAI, Log, All)

public:
	UBaseAISubsystem();

	// Subsystem 라이프사이클
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//AI 서버 연결 상태 확인
	UFUNCTION(BlueprintPure, Category = "AI|Subsystem")
	bool IsConnected() const { return bIsConnected; }

	//AI 서버 URL 가져오기
	UFUNCTION(BlueprintPure, Category = "AI|Subsystem")
	FString GetServerURL() const;

	/** Brain Cam 데이터 수신 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "AI|Events")
	FOnBrainCamDataReceived OnBrainCamDataReceived;

	/** AI 서버 연결 상태 변경 이벤트 */
	FOnAIConnectionChanged OnConnectionChanged;

	/**
	 * 런타임에 서버 설정 변경
	 * @param NewServerURL - 새로운 서버 URL
	 * @param NewServerPort - 새로운 서버 포트
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Config")
	void SetServerSettings(const FString& NewServerURL, int32 NewServerPort);

	/**
	 * 서버 설정 초기화 (Config 파일 기본값으로)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Config")
	void ResetServerSettings();

	/**
	 * 현재 서버 설정 가져오기
	 */
	UFUNCTION(BlueprintPure, Category = "AI|Config")
	void GetServerSettings(FString& OutServerURL, int32& OutServerPort) const
	{
		OutServerURL = ServerURL;
		OutServerPort = ServerPort;
	}

protected:
	/**
	 * HTTP POST 요청 전송
	 * @param Endpoint - API 엔드포인트 경로 (예: "/api/v1/get_decision")
	 * @param JsonBody - JSON 형식의 요청 본문
	 * @param OnComplete - 완료 콜백
	 */
	void SendPostRequest(const FString& Endpoint, const FString& JsonBody,
	                     TFunction<void(FHttpResponsePtr, bool)> OnComplete);

	/**
	 * HTTP GET 요청 전송
	 * @param Endpoint - API 엔드포인트 경로
	 * @param OnComplete - 완료 콜백
	 */
	void SendGetRequest(const FString& Endpoint,
	                    TFunction<void(FHttpResponsePtr, bool)> OnComplete);

	/**
	 * 응답 처리 헬퍼
	 * @param Response - HTTP 응답
	 * @param bWasSuccessful - 성공 여부
	 * @param OnSuccess - 성공 시 호출할 람다 (Content, StatusCode)
	 */
	void HandleResponse(FHttpResponsePtr Response, bool bWasSuccessful,
	                    TFunction<void(const FString&, int32)> OnSuccess);

	/**
	 * 서버 전송 데이터 로그 저장
	 * @param Endpoint - API 엔드포인트
	 * @param URL - 전체 URL
	 * @param JsonBody - 전송할 JSON 데이터
	 */
	void SaveRequestLog(const FString& Endpoint, const FString& URL, const FString& JsonBody);

	/**
	 * 오래된 로그 파일 정리
	 * @param LogDirectory - 로그 디렉토리 경로
	 * @param MaxLogCount - 유지할 최대 로그 파일 개수
	 */
	void CleanupOldLogFiles(const FString& LogDirectory, int32 MaxLogCount);

	//서버 연결 시도
	virtual void ConnectToServer();

	//서버 연결 해제
	virtual void DisconnectFromServer();

	//재연결 시도
	void AttemptReconnect();

	/** 연결 상태 변경 (델리게이트 브로드캐스트 포함) */
	void SetConnected(bool bNewConnected);

// Configuration
	/** AI 서버 URL (Blueprint/에디터에서 수정 가능) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	FString ServerURL;

	/** AI 서버 포트 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	int32 ServerPort;

	/** 요청 타임아웃 (초) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	float RequestTimeout;

	/** 재연결 시도 간격 (초) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	float ReconnectInterval;

	/** 최대 재연결 시도 횟수 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	int32 MaxReconnectAttempts;

protected:
	/** 연결 상태 */
	bool bIsConnected;

private:
	/** 재연결 타이머 */
	FTimerHandle ReconnectTimer;

	/** 재연결 시도 횟수 */
	int32 ReconnectAttemptCount;
};
