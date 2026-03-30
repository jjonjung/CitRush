// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RatingELO.generated.h"

struct FRating
{
	float Rating = 1000.0f; // 현재 레이팅
	int32 KFactor = 32;     // K-팩터 (점수 변동 폭)
	// float RatingDeviation = 350.0f; // Glicko-2 사용 시 필요 (선택 사항)
};

/**
 * @brief ELO 계산에 사용되는 미션 및 환경 정보
 */
struct FMissionParams
{
	float BaseEnvRating = 1000.0f; // 기본 가상 상대 레이팅
	int32 DifficultyLevel = 5;     // 미션 난이도 레벨 (L)
	float TimeLimitMinutes = 30.0f; // 제한 시간 N (분)
};

/**
 * @brief 개별 플레이어의 게임 결과 데이터
 */
struct FPlayerResult
{
	FPlayerResult(APlayerState* Player,
FRating CurrentRatings,
float ActualClearTimeMinutes,
float IndividualPerformanceScore);
	
	APlayerState* Player;
	FRating CurrentRatings;
	float ActualClearTimeMinutes; // 실제 클리어 시간
	float IndividualPerformanceScore; // 개인 성과 지수 (P)
};

/**
 * 
 */
UCLASS()
class UE_CITRUSH_API URatingELO : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	static const float CommanderWeight;
	static const float RacerWeight;
	static const float PerfCorrectionFactor;

public:
	/**
	 * @brief 미션 레벨에 따른 가상 환경 레이팅(R_Env)을 계산합니다.
	 */
	static float CalculateEnvironmentRating(const FMissionParams& Params);

	/**
	 * @brief 두 레이팅 간의 예상 승률 (E)을 계산합니다. (PvP ELO 공식)
	 */
	static float CalculateExpectedScore(float RatingA, float RatingB);

	/**
	 * @brief 팀의 승패 결과에 따른 모든 플레이어의 최종 레이팅 변화를 계산합니다.
	 * @param TeamResults 팀의 모든 플레이어 결과 데이터
	 * @param Params 미션 환경 파라미터
	 * @param bTeamWin 팀의 승리 여부 (true = 1, false = 0)
	 * @return 플레이어별 레이팅 변화량 맵
	 */
	static TMap<FPlayerResult*, float> CalculateFinalRatingChanges(TArray<FPlayerResult*>& TeamResults, const FMissionParams& Params, bool bTeamWin);
};

