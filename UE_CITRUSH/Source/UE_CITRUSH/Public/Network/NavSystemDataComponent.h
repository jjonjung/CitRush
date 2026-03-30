// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NavSystemDataComponent.generated.h"

/**
 * 개별 레이서 네비게이션 데이터
 *
 * 특정 레이서에 대한 경로 정보, 위치 정보, Delta 정보를 포함
 */
USTRUCT(BlueprintType)
struct FRacerNavigationData
{
	GENERATED_BODY()

	// ============================================================
	// 기본 정보
	// ============================================================

	/**
	 * 플레이어 인덱스 (PlayerArray에서의 인덱스: 1, 2, 3)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	int32 PlayerIndex;

	// ============================================================
	// 경로 정보
	// ============================================================

	/**
	 * NavMesh 경로 비용
	 * -1 = 경로 없음 또는 플레이어 없음
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float PathCost;

	/**
	 * NavMesh 경로 거리 (cm)
	 * -1 = 경로 없음 또는 플레이어 없음
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float PathDistance;

	/**
	 * 보스에서 플레이어까지의 직선 거리 (cm)
	 * -1 = 플레이어 없음
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float StraightDistance;

	// ============================================================
	// 위치 정보
	// ============================================================

	/**
	 * 플레이어의 월드 좌표
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	FVector PlayerWorldLocation;

	// ============================================================
	// 각도 정보
	// ============================================================

	/**
	 * 보스 forward 벡터 기준 플레이어 상대 각도 (도)
	 *
	 * 범위: -180 ~ +180
	 * - 0도: 보스 정면
	 * - ±180도: 보스 뒤
	 * - 양수: 오른쪽
	 * - 음수: 왼쪽
	 *
	 * 백어택 가능 각도 체크에 사용
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float RelativeAngleToBoss;

	// ============================================================
	// Delta 정보 (이전 프레임과의 변화)
	// ============================================================

	/**
	 * 플레이어 위치 변화 벡터 (cm)
	 * 첫 프레임이거나 이전에 invalid였으면 (0, 0, 0)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	FVector DeltaPosition;

	/**
	 * 직선 거리 변화 (cm)
	 * 음수 = 가까워짐, 양수 = 멀어짐
	 * 첫 프레임 = 0
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float DeltaStraightDistance;

	/**
	 * 경로 거리 변화 (cm)
	 * 첫 프레임 = 0
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float DeltaPathDistance;

	/**
	 * 경로 비용 변화
	 * 첫 프레임 = 0
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float DeltaPathCost;

	/**
	 * 이동 방향 변화 (도)
	 * 이전 이동 방향과 현재 이동 방향 간의 각도 차이
	 * 0 = 직진, 180 = 반대 방향
	 * 첫 프레임 = 0
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float MovementDirectionChange;

	/**
	 * 평균 속도 (cm/s)
	 * DeltaPosition 크기 / DeltaTime
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float AverageSpeed;

	/**
	 * 보스와의 상대 방향 변화 (도)
	 * 보스 기준 플레이어 각도의 변화량
	 * 양수 = 시계 방향, 음수 = 반시계 방향
	 * 첫 프레임 = 0
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float RelativeBearingChange;

	// 기본 생성자
	FRacerNavigationData()
		: PlayerIndex(-1)
		, PathCost(-1.0f)
		, PathDistance(-1.0f)
		, StraightDistance(-1.0f)
		, PlayerWorldLocation(FVector::ZeroVector)
		, RelativeAngleToBoss(0.0f)
		, DeltaPosition(FVector::ZeroVector)
		, DeltaStraightDistance(0.0f)
		, DeltaPathDistance(0.0f)
		, DeltaPathCost(0.0f)
		, MovementDirectionChange(0.0f)
		, AverageSpeed(0.0f)
		, RelativeBearingChange(0.0f)
	{
	}

	/**
	 * 이 데이터가 유효한지 확인
	 * @return 플레이어가 존재하고 경로가 있으면 true
	 */
	FORCEINLINE bool IsValid() const
	{
		return PlayerIndex >= 0 && PathCost >= 0.0f && PathDistance >= 0.0f;
	}
};

/**
 * LLM AI 의사결정을 위한 네비게이션 시스템 데이터
 *
 * 보스와 모든 레이서들의 네비게이션 정보를 포함
 * HTTP 통신 컴포넌트로 전달하여 LLM에게 정량적 데이터를 제공
 */
USTRUCT(BlueprintType)
struct FNavSystemLLMData
{
	GENERATED_BODY()

	// ============================================================
	// 보스 정보
	// ============================================================

	/**
	 * 보스의 월드 좌표
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	FVector BossWorldLocation;

	// ============================================================
	// 시간 정보
	// ============================================================

	/**
	 * 게임 시작 후 경과 시간 (초)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float Timestamp;

	/**
	 * 이전 계산과의 시간 차이 (초)
	 * 첫 프레임 = 0
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float DeltaTime;

	// ============================================================
	// 레이서 데이터
	// ============================================================

	/**
	 * 모든 레이서의 네비게이션 데이터 (최대 3개)
	 * PlayerIndex 1, 2, 3에 해당
	 * 플레이어가 없거나 경로가 없으면 IsValid() == false
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	TArray<FRacerNavigationData> RacerData;

	// ============================================================
	// 전체 분산도 정보
	// ============================================================

	/**
	 * 플레이어 간 평균 거리 (cm)
	 * 모든 유효한 레이서 쌍 간 거리의 평균
	 * 유효한 레이서가 2명 미만이면 -1
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float InterPlayerAvgDistance;

	/**
	 * 플레이어 간 평균 거리의 변화 (cm)
	 * 음수 = 모여듦, 양수 = 흩어짐
	 * 첫 프레임 또는 이전에 유효한 레이서 2명 미만이었으면 0
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Navigation Data")
	float DeltaInterPlayerDistance;

	// 기본 생성자
	FNavSystemLLMData()
		: BossWorldLocation(FVector::ZeroVector)
		, Timestamp(0.0f)
		, DeltaTime(0.0f)
		, InterPlayerAvgDistance(-1.0f)
		, DeltaInterPlayerDistance(0.0f)
	{
	}
};

/**
 * 네비게이션 시스템 데이터 컴포넌트
 *
 * 보스 액터에 부착하여 레이서들까지의 NavMesh 경로를 계산하고
 * LLM AI 의사결정을 위한 정량적 데이터를 생성.
 *
 * 주요 기능:
 * - GameState의 PlayerArray[1~3] 레이서 자동 추적
 * - NavMesh 경로 비용, 거리, 직선 거리 계산
 * - Delta 정보 계산 (위치, 속도, 각도 변화 등)
 * - 플레이어 간 분산도 계산
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UNavSystemDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNavSystemDataComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ============================================================
	// 메인 메서드
	// ============================================================

	/**
	 * 모든 레이서들에 대한 네비게이션 데이터 계산
	 *
	 * GameState의 PlayerArray[1~3] 레이서들에 대해:
	 * - NavMesh 경로 계산
	 * - 위치 및 각도 계산
	 * - Delta 정보 계산 (이전 프레임과 비교)
	 * - 플레이어 간 분산도 계산
	 *
	 * 주의: 동기 방식이므로 매 프레임 호출하지 마세요
	 */
	UFUNCTION(BlueprintCallable, Category = "NavSystem|Data")
	void CalculateNavigationData();

	const FRacerNavigationData* FindPreviousData(int32 PlayerIndex) const;
	/**
	 * LLM 전달용 네비게이션 데이터 반환
	 *
	 * CalculateNavigationData() 호출 후 이 함수로 결과를 가져옵니다.
	 * 반환된 데이터를 HTTP 통신 컴포넌트로 LLM에 전달하세요.
	 *
	 * @return LLM 의사결정을 위한 종합 네비게이션 데이터
	 */
	UFUNCTION(BlueprintCallable, Category = "NavSystem|Data")
	FNavSystemLLMData GetLLMData() const;

private:
	// ============================================================
	// 내부 데이터 저장
	// ============================================================

	/**
	 * 최신 계산 결과
	 */
	UPROPERTY()
	FNavSystemLLMData CurrentData;

	/**
	 * 이전 프레임 데이터 (Delta 계산용)
	 */
	UPROPERTY()
	TArray<FRacerNavigationData> PreviousRacerData;

	/**
	 * 이전 프레임의 플레이어 간 평균 거리
	 */
	float PreviousInterPlayerDistance;

	/**
	 * 이전 계산 시간
	 */
	float PreviousTimestamp;

	// ============================================================
	// 내부 계산 메서드
	// ============================================================

	/**
	 * 특정 레이서에 대한 네비게이션 데이터 계산
	 * @param PlayerIndex PlayerArray의 인덱스 (1, 2, 3)
	 * @param PlayerPawn 플레이어 Pawn
	 * @param DeltaTime 이전 계산과의 시간 차이
	 * @return 계산된 레이서 네비게이션 데이터
	 */
	FRacerNavigationData CalculateRacerData(int32 PlayerIndex, APawn* PlayerPawn, float DeltaTime);

public:
	// ============================================================
	// Shared Utility Methods (Static)
	// ============================================================

	/**
	 * NavMesh 경로 비용 및 거리 계산 (Static Shared)
	 * @param WorldContextObject 월드 컨텍스트 (GetWorld() 호출용)
	 * @param StartLocation 시작 위치
	 * @param EndLocation 목표 위치
	 * @param OutCost 경로 비용 (-1 = 실패)
	 * @param OutDistance 경로 거리 cm (-1 = 실패)
	 * @param PathOwner 경로 쿼리 소유자 (옵션, Query Filter용)
	 * @return 경로 계산 성공 여부
	 */
	static bool CalculateNavPathCost(const UObject* WorldContextObject, const FVector& StartLocation, const FVector& EndLocation, float& OutCost, float& OutDistance, AActor* PathOwner = nullptr);

	/**
	 * 보스 forward 벡터 기준 플레이어 상대 각도 계산 (Static Shared)
	 * @param BossLocation 보스 위치
	 * @param BossForward 보스 forward 벡터
	 * @param PlayerLocation 플레이어 위치
	 * @return -180 ~ +180도 (0 = 정면, ±180 = 뒤)
	 */
	static float CalculateRelativeAngle(const FVector& BossLocation, const FVector& BossForward, const FVector& PlayerLocation);

private:

private:
	// ============================================================
	// 내부 데이터 저장
	// ============================================================

	/**
	 * 모든 유효한 레이서 간 평균 거리 계산
	 * @param RacerDataArray 레이서 데이터 배열
	 * @return 평균 거리 cm (유효한 레이서 2명 미만이면 -1)
	 */
	float CalculateInterPlayerDistance(const TArray<FRacerNavigationData>& RacerDataArray);

	/**
	 * 두 방향 벡터 간의 각도 차이 계산
	 * @param Dir1 방향 벡터 1
	 * @param Dir2 방향 벡터 2
	 * @return 각도 차이 (도, 0~180)
	 */
	float CalculateDirectionChange(const FVector& Dir1, const FVector& Dir2);
};
