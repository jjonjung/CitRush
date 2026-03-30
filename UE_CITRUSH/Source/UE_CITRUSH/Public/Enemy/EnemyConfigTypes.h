// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyConfigTypes.generated.h"

/**
 * 위협도 계산 설정 (CalculateCaptureGauge용)
 * DataTable로 관리하여 난이도별 조정 가능
 */
USTRUCT(BlueprintType)
struct FEnemyThreatConfig : public FTableRowBase
{
	GENERATED_BODY()

	// ========== 거리 임계값 ==========

	/** 위협 탐지 거리 (cm) - 기본 6000.0f (60m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float DetectionRange = 6000.0f;

	/** 근접 거리 (cm) - 기본 2000.0f (20m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float CloseRange = 2000.0f;

	/** 중거리 (cm) - 기본 4000.0f (40m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float MidRange = 4000.0f;

	/** 전방 장애물 탐지 거리 (cm) - 기본 500.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float ObstacleCheckDistance = 500.0f;

	// ========== 각도 임계값 ==========

	/** 백어택 각도 (내적값) - 기본 -0.5f (cos(120/2)) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle")
	float BackAttackAngleDot = -0.5f;

	/** 접근 각도 (내적값) - 기본 0.5f (cos(60)) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle")
	float ApproachAngleDot = 0.5f;

	// ========== 위협 점수 ==========

	/** 백어택 위협 점수 - 기본 20.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float BackAttackScore = 20.0f;

	/** 시야 내 위협 점수 - 기본 10.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float InSightScore = 10.0f;

	/** 근접 거리 위협 점수 - 기본 30.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float CloseRangeScore = 30.0f;

	/** 중거리 위협 점수 - 기본 15.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float MidRangeScore = 15.0f;

	/** 빠른 속도 접근 점수 - 기본 15.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float FastApproachScore = 15.0f;

	/** 중속 접근 점수 - 기본 8.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float MediumApproachScore = 8.0f;

	/** 부스팅 위협 점수 - 기본 20.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float BoostingScore = 20.0f;

	/** 지름길 위협 점수 - 기본 10.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float ShortcutScore = 10.0f;

	/** 전방 막힘 위협 점수 - 기본 20.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float BlockedScore = 20.0f;

	/** 백어택 포위 위협 점수 - 기본 25.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThreatScore")
	float BackAttackSurroundScore = 25.0f;

	// ========== 다중 추격 배수 ==========

	/** 포위 배수 (3명 이상) - 기본 2.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplier")
	float SurroundMultiplier = 2.0f;

	/** 협공 배수 (2명) - 기본 1.5f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplier")
	float CoopMultiplier = 1.5f;

	// ========== 속도 임계값 ==========

	/** 빠른 속도 임계값 (cm/s) - 기본 1000.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float FastSpeed = 1000.0f;

	/** 중속 임계값 (cm/s) - 기본 500.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float MediumSpeed = 500.0f;

	// ========== NavMesh 경로 관련 ==========

	/** 경로 비용 비율 (직선거리 대비) - 기본 0.8f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh")
	float PathCostRatio = 0.8f;

	/** 멀어지는 중일 때 위협도 감소 배수 - 기본 0.5f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplier")
	float RetreatingMultiplier = 0.5f;
};

/**
 * 전투 설정 (데미지, 쿨타임 등)
 */
USTRUCT(BlueprintType)
struct FEnemyCombatConfig : public FTableRowBase
{
	GENERATED_BODY()

	/** 기본 데미지 - 기본 34.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseDamage = 34.0f;

	/** 피격 후 무적 시간 (초) - 기본 2.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float InvulnerabilityDuration = 2.0f;

	/** P-Pellet 쿨타임 (초) - 기본 30.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float PPelletCooldownDuration = 30.0f;

	/** AI 요청 주기 (초) - 기본 5.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AIRequestInterval = 5.0f;

	/** 카메라 SpringArm 길이 (cm) - 기본 600.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float SpringArmLength = 600.0f;

	/** 카메라 Z 오프셋 (cm) - 기본 100.0f */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraZOffset = 100.0f;
};

/**
 * Aggressiveness 레벨 매핑
 */
USTRUCT(BlueprintType)
struct FAggressivenessMapping : public FTableRowBase
{
	GENERATED_BODY()

	/** Aggressiveness 레벨 문자열 ("HIGH", "MEDIUM", "LOW" 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FString AggressivenessLevel;

	/** Aggressiveness 계수 (0.0 ~ 1.0+) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AggressivenessFactor = 1.0f;
};
