// Fill out your copyright notice in the Description page of Project Settings.


#include "RatingELO.h"

#include "Data/CitRushPlayerTypes.h"
#include "Player/CitRushPlayerState.h"

const float URatingELO::CommanderWeight = 0.9f;
const float URatingELO::RacerWeight = 1.1f;
const float URatingELO::PerfCorrectionFactor = 5.0f;

float URatingELO::CalculateEnvironmentRating(const FMissionParams& Params)
{
	return Params.BaseEnvRating + (Params.DifficultyLevel * 100.0f);
}

float URatingELO::CalculateExpectedScore(float RatingA, float RatingB)
{
	
	return 1.0f / (1.0f + FMath::Pow(10.0f, (RatingB - RatingA) / 400.0f));
}

TMap<FPlayerResult*, float> URatingELO::CalculateFinalRatingChanges(TArray<FPlayerResult*>& TeamResults,
	const FMissionParams& Params, bool bTeamWin)
{
	TMap<FPlayerResult*, float> RatingChanges;

    // 1. 팀 평균 레이팅 계산 (R_Team_Avg)
    float TotalRating = 0.0f;
    for (const auto& Result : TeamResults)
    {
        TotalRating += Result->CurrentRatings.Rating;
    }
    float TeamAverageRating = TotalRating / TeamResults.Num();

    // 2. R_Env 계산
    float EnvironmentRating = CalculateEnvironmentRating(Params);

    // 3. 팀 예상 승률 (E_Team) 계산
    float ExpectedTeamScore = CalculateExpectedScore(TeamAverageRating, EnvironmentRating);

    // 4. 기본 팀 점수 변화량 (G_Team) 계산
    // G_Team = K * (S_Team - E_Team)
    float ActualTeamScore = bTeamWin ? 1.0f : 0.0f;
    int32 KFactor = TeamResults.IsEmpty() ? 32 : TeamResults[0]->CurrentRatings.KFactor; // 임시 K-Factor
    float BaseTeamGain = KFactor * (ActualTeamScore - ExpectedTeamScore);

    // 5. 역할별, 시간별, 성과별 보정 및 최종 점수 변화 계산
    for (auto& Result : TeamResults)
    {
    	// TODO : 하드 코딩
    	ACitRushPlayerState* cPS = Cast<ACitRushPlayerState>(Result->Player);
    	if (IsValid(cPS)) { return {}; }
        // 역할 가중치 (W_R)
        float RoleWeight = (cPS->GetPlayerRole() == EPlayerRole::Commander) 
                             ? CommanderWeight 
                             : RacerWeight;

        // 시간 효율 보너스 (C_Time)
        float TimeBonusFactor = 1.0f;
        if (bTeamWin && Params.TimeLimitMinutes > 0)
        {
            // 실제 클리어 시간이 제한 시간을 넘지 않았을 때만 보정
            float ActualTime = FMath::Min(Result->ActualClearTimeMinutes, Params.TimeLimitMinutes);
            // C_Time = 1 + ((N - T_Actual) / N) * 0.2
            TimeBonusFactor = 1.0f + ((Params.TimeLimitMinutes - ActualTime) / Params.TimeLimitMinutes) * 0.2f;
        }

        // 1) 승패/역할/시간 보정 점수: (G_Team * W_R) * C_Time
        float WinLossScore = (BaseTeamGain * RoleWeight) * TimeBonusFactor;

        // 2) 개인 성과 보정 점수 (Delta R_Perf)
        // Delta R_Perf = P * K_Perf
        float PerformanceScore = Result->IndividualPerformanceScore * URatingELO::PerfCorrectionFactor;

        // 3) 최종 점수 변화: Delta R_Final = WinLossScore + PerformanceScore
        float FinalChange = WinLossScore + PerformanceScore;

        RatingChanges.Add(Result, FinalChange);

        // 레이팅 업데이트 (선택적)
        Result->CurrentRatings.Rating += FinalChange; 
    }

    return RatingChanges;
}
