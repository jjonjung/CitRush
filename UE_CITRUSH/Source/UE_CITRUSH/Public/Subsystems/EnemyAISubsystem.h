// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/PixelEnemy.h"
#include "Subsystems/BaseAISubsystem.h"
#include "Enemy/Interface/AIDecisionReceiver.h"
#include "Player/Car/VehicleDemoUITest.h"
#include "Private/Network/Schemas/HttpV1/HttpRequest2.h"
#include "Private/Network/Schemas/HttpV1/HttpResponse2.h"
#include "EnemyAISubsystem.generated.h"

class AActor;
class AVehicleDemoCejCar;
class APelletActor;
class ACoinActor;
class APelletSpawner;

/**
 * Enemy AI 전용 Subsystem
 * - Enemy들을 등록/관리
 * - AI 서버와 통신 (일괄 또는 개별 요청)
 * - 결정 결과를 Enemy들에게 배포
 */
UCLASS()
class UE_CITRUSH_API UEnemyAISubsystem : public UBaseAISubsystem
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(LogEnemyAI, Log, All)

public:
	UEnemyAISubsystem();

	// Subsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	FVector GetPixelLocation();

	UFUNCTION()
	void OnActorSpawned(AActor* SpawnedActor);

	UFUNCTION()
	void OnActorDestroyed(AActor* DestroyedActor);

	// ========== 액터 캐시 (GetAllActorsOfClass 대체) ==========

	/** 캐시된 Vehicle 목록 반환 (stale 참조 자동 정리) */
	const TArray<TWeakObjectPtr<AVehicleDemoCejCar>>& GetCachedVehicles();

	/** 캐시된 Pellet 목록 반환 */
	const TArray<TWeakObjectPtr<APelletActor>>& GetCachedPellets();

	/** 캐시된 Coin 목록 반환 */
	const TArray<TWeakObjectPtr<ACoinActor>>& GetCachedCoins();

	/** 캐시된 PelletSpawner 목록 반환 */
	const TArray<TWeakObjectPtr<APelletSpawner>>& GetCachedPelletSpawners();

	// Enemy 관리
	/**
	 * Enemy 등록
	 * @param Enemy - IAIDecisionReceiver 인터페이스를 구현한 Enemy
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Enemy")
	void RegisterEnemy(TScriptInterface<IAIDecisionReceiver> Enemy);

	/**
	 * Enemy 등록 해제
	 * @param Enemy - 등록 해제할 Enemy
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Enemy")
	void UnregisterEnemy(TScriptInterface<IAIDecisionReceiver> Enemy);

	/**
	 * 등록된 Enemy 수
	 */
	UFUNCTION(BlueprintPure, Category = "AI|Enemy")
	int32 GetRegisteredEnemyCount() const { return RegisteredEnemies.Num(); }

	// AI 결정 요청
	/**
	 * 특정 Enemy에 대한 AI 결정 요청
	 * @param EnemyID - Enemy 고유 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Request")
	void RequestDecisionForEnemy(const FString& EnemyID);

	/**
	 * 모든 등록된 Enemy에 대한 AI 결정 일괄 요청
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Request")
	void RequestBatchDecision();

	/**
	 * 자동 AI 결정 요청 시작
	 * @param IntervalSeconds - 요청 간격 (초)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Request")
	void StartAutoDecisionRequests(float IntervalSeconds = 3.0f);

	/**
	 * 매치 시작 시 호출 - AI 서버에 시작 시간 전송 및 자동 요청 시작
	 * @param MatchStartTime - 매치 시작 시간 (ISO 8601)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Request")
	void StartMatch(const FString& InMatchStartTime);

	//자동 AI 결정 요청 중지
	UFUNCTION(BlueprintCallable, Category = "AI|Request")
	void StopAutoDecisionRequests();

	/**
	 * 매치 종료 시 AI 서버에 /api/v1/match/end 전송
	 * @param Result - "PLAYERS_WIN" | "PACMAN_WIN" | "DRAW" | "ABORTED"
	 * @param EndReason - "PACMAN_DEAD" | "TIME_OVER" | "SURRENDER" 등
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Request")
	void SendMatchEndToServer(const FString& Result = TEXT("PLAYERS_WIN"), const FString& EndReason = TEXT("PACMAN_DEAD"));

	//자동 요청 활성화 여부
	UFUNCTION(BlueprintPure, Category = "AI|Request")
	bool IsAutoRequestActive() const { return DecisionTimer.IsValid(); }

	// P-Point (Coin) 관리
	/**
	 * 코인 획득 시 호출 - 해당 ID의 P-Point를 available = false로 설정
	 * @param CoinID - 획득한 코인 ID (예: "p_point_1")
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|PPoint")
	void MarkPPointAsCollected(const FString& CoinID);

protected:
	UPROPERTY()
	TWeakObjectPtr<APixelEnemy> PixelEnemy;

public:
	
	UPROPERTY()
	FVector PixelLocation;
	
	// Delegates
	/** AI 결정 수신 이벤트 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDecisionReceived, const FString& /*EnemyID*/, const FGetDecisionResponse2& /*Response*/);
	FOnDecisionReceived OnDecisionReceived;

	/** 에러 발생 이벤트 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAIError, const FString& /*EnemyID*/, const FString& /*ErrorMessage*/);
	FOnAIError OnAIError;

	/** Brain Cam 데이터 수신 이벤트 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBrainCamDataReceived, const FString&, Summary, const FString&, FinalChoice, const TArray<FRetrievedDoc>&, ReasoningDocs);
	/*UPROPERTY(BlueprintAssignable, Category = "AI|Event")
	FOnBrainCamDataReceived OnBrainCamDataReceived;*/

	/**
	 * 일괄 요청 전송
	 */
	void SendBatchDecisionRequest();

	/**
	 * 매치 시작 시 AI 서버에 /api/v1/match/start 전송
	 */
	void SendMatchStartToServer();

	/**
	 * 현재 세션의 Room ID 반환
	 */
	FString GetCurrentRoomId() const;

	/**
	 * AI 결정 응답 처리
	 * @param Response - AI 서버 응답
	 */
	void HandleDecisionResponse(const FGetDecisionResponse2& Response);

	/**
	 * Enemy ID로 Enemy 찾기
	 * @param EnemyID - 찾을 Enemy ID
	 * @return Enemy 인터페이스 (없으면 nullptr)
	 */
	TScriptInterface<IAIDecisionReceiver> FindEnemyByID(const FString& EnemyID) const;

	/**
	 * Capture Gauge 계산 (포획 위험도 게이지)
	 * @param PacmanLocation - 팩맨 위치
	 * @param PacmanForward - 팩맨 전방 벡터
	 * @param bIsInvulnerable - 무적 상태 여부
	 * @param PlayerTeam - 플레이어 팀 정보 (드라이버들)
	 * @param NavSys - NavMesh 시스템
	 * @return 포획 게이지 값 (0.0 ~ 100.0)
	 */

	UPROPERTY()
	FVector CachedPacmanLocation;
	float CalculateCaptureGauge(
		const FVector& PacmanLocation,
		const FVector& PacmanForward,
		bool bIsInvulnerable,
		const TArray<struct FPlayerTeamContext2>& PlayerTeam,
		class UNavigationSystemV1* NavSys) const;

private:
	/** 등록된 Enemy들 */
	UPROPERTY()
	TArray<TScriptInterface<IAIDecisionReceiver>> RegisteredEnemies;

	//자동 결정 타이머
	FTimerHandle DecisionTimer;

	/** 마지막 요청 (재시도용) */
	FGetDecisionRequest2 LastDecisionRequest;

	/** 요청 진행 중 플래그 */
	bool bIsRequestPending;

	/** Coin ID -> ACoinActor 매핑 (획득 상태 추적용) */
	UPROPERTY()
	TMap<FString, TObjectPtr<class ACoinActor>> CoinActorMap;

	/** 획득된 코인 ID와 위치 정보 (Destroy 후에도 유지) */
	UPROPERTY()
	TMap<FString, FVector> CollectedCoinLocations;

	/** GetDecision 엔드포인트 경로 */
	static constexpr const TCHAR* DecisionEndpoint = TEXT("/api/v1/get_decision");

	// Debug Logging
	/**
	 * JSON 로그 파일 저장
	 * @param Prefix - 파일명 접두사 (Request/Response)
	 * @param JsonContent - 저장할 JSON 문자열
	 */
	void SaveJsonLog(const FString& Prefix, const FString& JsonContent);

	/** 로그 저장 경로 */
	FString LogDirectoryPath;

	/** 매치 시작 시간 (ISO 8601) */
	FString MatchStartTime;

	UPROPERTY()
	TWeakObjectPtr<UVehicleDemoUITest> RemainingTime;

	/** Actor Spawned 델리게이트 핸들 */
	FDelegateHandle ActorSpawnedHandle;

	// ========== 액터 캐시 배열 ==========
	TArray<TWeakObjectPtr<AVehicleDemoCejCar>> CachedVehicleArray;
	TArray<TWeakObjectPtr<APelletActor>> CachedPelletArray;
	TArray<TWeakObjectPtr<ACoinActor>> CachedCoinArray;
	TArray<TWeakObjectPtr<APelletSpawner>> CachedPelletSpawnerArray;

	/** stale 참조 정리 헬퍼 */
	template<typename T>
	static void CleanupWeakArray(TArray<TWeakObjectPtr<T>>& Array)
	{
		Array.RemoveAll([](const TWeakObjectPtr<T>& Ptr) { return !Ptr.IsValid(); });
	}

	// ========== v1.5.0: Player ID 누적 카운터 ==========

	/** 플레이어 순번 카운터 (매치마다 0으로 리셋, 첫 배정 시 ++1) */
	int32 PlayerIdCounter = 0;

	/** UE PlayerState ID → 누적 순번(1~4) 매핑 */
	TMap<int32, int32> PlayerIdAssignmentMap;

	/** UE PlayerState ID → 마지막 지휘관 지시 시각(ISO 8601) */
	TMap<int32, FString> PlayerLastCommandTimeMap;

	// ========== v1.4.0: Request 순서 보장 ==========

	/** Request 순서 번호 (증가 카운터) */
	int32 RequestSequenceNumber = 0;

	/** 마지막으로 보낸 request_num (Request-Response 매칭용) */
	FString LastSentRequestNum;

	/** 마지막으로 처리한 request_num (순서 검증용) */
	FString LastProcessedRequestNum;

	/**
	 * request_num 생성
	 * 형식: {room_id}_{timestamp}_{sequence}
	 * @param RoomID - 게임 방 ID
	 * @return 생성된 request_num 문자열
	 */
	FString GenerateRequestNum(const FString& RoomID);

	/**
	 * Response의 request_num이 유효한지 검증
	 * - LastSentRequestNum과 같을 때만 허용 (Request-Response 매칭)
	 * - LastSentRequestNum보다 작으면 무시 (오래된 응답)
	 * @param IncomingRequestNum - 검증할 request_num
	 * @return true = 유효한 응답 (처리 가능), false = 무효한 응답 (무시)
	 */
	bool ValidateResponseOrder(const FString& IncomingRequestNum);

	// ========== HTTP 422 Fallback ==========
	/** 마지막으로 성공한 AI 결정 응답 (422 에러 시 재사용) */
	FGetDecisionResponse2 LastSuccessfulResponse;

	/** 유효한 성공 응답이 있는지 여부 */
	bool bHasSuccessfulResponse = false;

	/** 연속 실패 카운터 (연결 상태 추적) */
	int32 ConsecutiveFailureCount = 0;

	/** 연결 끊김 판정 임계값 */
	static constexpr int32 DisconnectThreshold = 2;

	/**
	 * 서버 연결 실패 시 기본 폴백 응답 생성 (Racer 추격/공격)
	 * @return 생성된 폴백 응답 (ATTACK_TARGET directive)
	 */
	FGetDecisionResponse2 CreateFallbackAttackResponse();

};
