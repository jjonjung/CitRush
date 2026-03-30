// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CaptureGaugeComponent.generated.h"

class AAbstractRacer;

/**
 * 개별 Driver(추격자)에 대한 위협도 데이터
 */
USTRUCT(BlueprintType)
struct FDriverThreatData
{
	GENERATED_BODY()

	/** Driver 액터 참조 */
	UPROPERTY()
	TObjectPtr<AAbstractRacer> Driver;

	/** NavMesh 경로 비용 (-1 = 경로 없음) */
	UPROPERTY(BlueprintReadOnly, Category = "Threat Data")
	float PathCost;

	/** NavMesh 경로 거리 (cm, -1 = 경로 없음) */
	UPROPERTY(BlueprintReadOnly, Category = "Threat Data")
	float PathDistance;

	/** 직선 거리 (cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Threat Data")
	float StraightDistance;

	/** Pacman 기준 상대 각도 (-180 ~ 180) */
	UPROPERTY(BlueprintReadOnly, Category = "Threat Data")
	float RelativeAngle;

	/** 백어택 각도 내에 있는지 (뒤쪽 120도) */
	UPROPERTY(BlueprintReadOnly, Category = "Threat Data")
	bool bIsInBackAttackZone;

	/** 접근 속도 (양수 = 접근 중, 음수 = 멀어지는 중) */
	UPROPERTY(BlueprintReadOnly, Category = "Threat Data")
	float ApproachSpeed;

	/** 부스터 사용 중 */
	UPROPERTY(BlueprintReadOnly, Category = "Threat Data")
	bool bIsBoosting;

	/** 계산된 개별 위협 점수 */
	UPROPERTY(BlueprintReadOnly, Category = "Threat Data")
	float ThreatScore;

	FDriverThreatData()
		: Driver(nullptr)
		, PathCost(-1.0f)
		, PathDistance(-1.0f)
		, StraightDistance(0.0f)
		, RelativeAngle(0.0f)
		, bIsInBackAttackZone(false)
		, ApproachSpeed(0.0f)
		, bIsBoosting(false)
		, ThreatScore(0.0f)
	{
	}

	bool IsValid() const
	{
		// NavMesh 경로가 실패해도 직선 거리로 위협도 계산 가능
		return Driver != nullptr;
	}
};

/**
 * Capture Gauge 계산 설정
 *
 * PDF 문서 기준:
 * - 0~20: 거의 안전 (LOW)
 * - 20~50: 주의 필요 (MEDIUM)
 * - 50~80: 상당히 위험 (HIGH)
 * - 80~100: 포획 직전 (CRITICAL)
 */
USTRUCT(BlueprintType)
struct FCaptureGaugeConfig
{
	GENERATED_BODY()

	// ========== 탐지 범위 ==========
	/** 위협도 계산 범위 (cm) - 맵 전체 커버를 위해 500m로 설정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	float DetectionRange = 50000.0f;

	// ========== NavMesh 가중치 ==========
	/** NavMesh 경로 비용 기반 위협 점수 (거리 점수와 균형을 맞춤) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavMesh")
	float NavCostThreatMultiplier = 0.0003f;

	// ========== 거리 기반 점수 ==========
	/** 초근접 거리 (cm) - 20m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float VeryCloseRange = 2000.0f;

	/** 근접 거리 (cm) - 25m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float CloseRange = 2500.0f;

	/** 중간 거리 (cm) - 40m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float MidRange = 4000.0f;

	/** 멀리 떨어진 거리 (cm) - 100m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float FarRange = 10000.0f;

	/** 초근접 위협 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float VeryCloseThreatScore = 30.0f;

	/** 근접 위협 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float CloseThreatScore = 20.0f;

	/** 중간 거리 위협 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float MidThreatScore = 10.0f;

	/** 멀리 떨어진 거리 위협 점수 (4000-10000cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float FarThreatScore = 8.0f;

	/** 매우 멀리 떨어진 거리 위협 점수 (10000cm 이상) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	float VeryFarThreatScore = 5.0f;

	// ========== 백어택 ==========
	/** 백어택 가능 각도 (도) - 120도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackAttack")
	float BackAttackAngle = 120.0f;

	/** 백어택 위치 추가 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BackAttack")
	float BackAttackBonusScore = 15.0f;

	// ========== 속도 ==========
	/** 빠른 접근 속도 기준 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float FastApproachSpeed = 3000.0f;

	/** 중간 접근 속도 기준 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float MediumApproachSpeed = 1500.0f;

	/** 빠른 접근 추가 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float FastApproachScore = 15.0f;

	/** 중간 접근 추가 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed")
	float MediumApproachScore = 8.0f;

	// ========== 부스터 ==========
	/** 부스터 사용 중 추가 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost")
	float BoostingBonusScore = 10.0f;

	// ========== 다중 추격 보정 ==========
	/** 3명 이상 추격 시 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiChaser")
	float ThreeChaserMultiplier = 1.5f;

	/** 2명 추격 시 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiChaser")
	float TwoChaserMultiplier = 1.3f;

	/** 백어택 포위 추가 점수 (뒤쪽 120도 내 2명 이상) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiChaser")
	float BackAttackSurroundScore = 20.0f;

	// ========== 지형 보정 ==========
	/** 장애물 체크 거리 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float ObstacleCheckDistance = 500.0f;

	/** 막다른 골목 추가 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float BlockedPathScore = 15.0f;

	// ========== P-Pellet 무적 ==========
	/** 무적 상태일 때 capture_gauge 상한 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Invulnerability")
	float InvulnerableMaxGauge = 20.0f;
};

/**
 * Capture Gauge Component
 *
 * Pacman(Enemy) 액터에 부착하여 포획 위험도를 계산하는 컴포넌트
 * NavSystemDataComponent의 NavMesh 계산 로직을 재사용하여
 * Pacman 관점에서 Driver들까지의 위협도를 산출
 *
 * 주요 기능:
 * - 2초마다 자동 계산 (Timer)
 * - NavMesh 기반 추격 난이도
 * - 추격자 수와 위치 (백어택 120도)
 * - 상대 속도/부스터 사용
 * - P-Pellet 무적 상태 처리
 * - 최종 0~100 정규화
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UCaptureGaugeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCaptureGaugeComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ========== Main Methods ==========

	/**
	 * Capture Gauge 계산 (수동 호출용)
	 * Timer에서 자동 호출되지만 수동으로도 호출 가능
	 */
	UFUNCTION(BlueprintCallable, Category = "Capture Gauge")
	void CalculateCaptureGauge();

	/**
	 * 현재 Capture Gauge 값 반환 (0~100)
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capture Gauge")
	float GetCaptureGauge() const { return CaptureGauge; }

	/**
	 * 자동 계산 시작
	 * @param Interval 계산 주기 (초) - 기본 2초
	 */
	UFUNCTION(BlueprintCallable, Category = "Capture Gauge")
	void StartAutoCalculation(float Interval = 2.0f);

	/**
	 * 자동 계산 중지
	 */
	UFUNCTION(BlueprintCallable, Category = "Capture Gauge")
	void StopAutoCalculation();

	/**
	 * 계산 설정 가져오기
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capture Gauge")
	FCaptureGaugeConfig GetConfig() const { return Config; }

	/**
	 * 계산 설정 변경
	 */
	UFUNCTION(BlueprintCallable, Category = "Capture Gauge")
	void SetConfig(const FCaptureGaugeConfig& NewConfig) { Config = NewConfig; }

private:
	// ========== Internal Data ==========

	/** 현재 Capture Gauge 값 (0~100) */
	UPROPERTY(VisibleAnywhere, Category = "Capture Gauge")
	float CaptureGauge;

	/** 계산 설정 */
	UPROPERTY(EditAnywhere, Category = "Capture Gauge|Config")
	FCaptureGaugeConfig Config;

	/** 자동 계산 타이머 핸들 */
	FTimerHandle CalculationTimerHandle;

	/** 현재 프레임 Driver 위협 데이터 */
	UPROPERTY()
	TArray<FDriverThreatData> CurrentThreatData;

	// ========== Internal Calculation Methods ==========

	/**
	 * 모든 Driver에 대한 위협 데이터 수집
	 * @return Driver 위협 데이터 배열
	 */
	TArray<FDriverThreatData> GatherDriverThreats();

	/**
	 * 특정 Driver에 대한 위협 데이터 계산
	 * @param Driver 추격자
	 * @param PacmanLocation Pacman 위치
	 * @param PacmanForward Pacman 전방 벡터
	 * @return Driver 위협 데이터
	 */
	FDriverThreatData CalculateDriverThreat(AAbstractRacer* Driver, const FVector& PacmanLocation, const FVector& PacmanForward);

	/**
	 * 백어택 각도 내에 있는지 확인
	 * @param RelativeAngle 상대 각도
	 * @return 백어택 가능 각도이면 true
	 */
	bool IsInBackAttackZone(float RelativeAngle);

	/**
	 * Driver의 접근 속도 계산
	 * @param Driver 추격자
	 * @param PacmanLocation Pacman 위치
	 * @return 접근 속도 (cm/s, 양수 = 접근, 음수 = 멀어짐)
	 */
	float CalculateApproachSpeed(AAbstractRacer* Driver, const FVector& PacmanLocation);

	/**
	 * 개별 Driver 위협 점수 계산
	 * @param ThreatData Driver 위협 데이터
	 * @return 위협 점수
	 */
	float CalculateIndividualThreatScore(const FDriverThreatData& ThreatData);

	/**
	 * 전체 위협 점수 합산 및 보정
	 * @param ThreatDataArray Driver 위협 데이터 배열
	 * @return 합산된 위협 점수
	 */
	float AggregateThreats(const TArray<FDriverThreatData>& ThreatDataArray);

	/**
	 * 전방 장애물 체크
	 * @param PacmanLocation Pacman 위치
	 * @param PacmanForward Pacman 전방 벡터
	 * @return 장애물이 있으면 true
	 */
	bool CheckForwardObstacle(const FVector& PacmanLocation, const FVector& PacmanForward);

	/**
	 * P-Pellet 무적 상태 확인
	 * @return 무적 상태이면 true
	 */
	bool IsInvulnerable();
};
