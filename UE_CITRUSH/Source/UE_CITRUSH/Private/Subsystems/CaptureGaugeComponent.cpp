// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystems/CaptureGaugeComponent.h"
#include "Network/NavSystemDataComponent.h"
#include "Player/AbstractRacer.h"
#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"
#include "Enemy/PixelEnemy.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

UCaptureGaugeComponent::UCaptureGaugeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	CaptureGauge = 0.0f;
}

void UCaptureGaugeComponent::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 계산 (클라이언트는 리플리케이션으로 받음)
	AActor* Owner = GetOwner();
	if (Owner && Owner->HasAuthority())
	{
		StartAutoCalculation(2.0f);
		//UE_LOG(LogTemp, Log, TEXT("[CaptureGaugeComponent] Auto calculation started (Server only)"));
	}
	else
	{
		//UE_LOG(LogTemp, Log, TEXT("[CaptureGaugeComponent] Skipping auto calculation (Client)"));
	}
}

void UCaptureGaugeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAutoCalculation();

	Super::EndPlay(EndPlayReason);
}

void UCaptureGaugeComponent::StartAutoCalculation(float Interval)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 기존 타이머 정리
	if (CalculationTimerHandle.IsValid())
	{
		World->GetTimerManager().ClearTimer(CalculationTimerHandle);
	}

	// 즉시 한 번 계산 (타이머 대기 없이)
	CalculateCaptureGauge();

	// 새 타이머 시작
	World->GetTimerManager().SetTimer(
		CalculationTimerHandle,
		this,
		&UCaptureGaugeComponent::CalculateCaptureGauge,
		Interval,
		true // 반복
	);

	//UE_LOG(LogTemp, Log, TEXT("[CaptureGaugeComponent] Auto calculation started (%.1fs interval, immediate first calculation)"), Interval);
}

void UCaptureGaugeComponent::StopAutoCalculation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (CalculationTimerHandle.IsValid())
	{
		World->GetTimerManager().ClearTimer(CalculationTimerHandle);
		//UE_LOG(LogTemp, Log, TEXT("[CaptureGaugeComponent] Auto calculation stopped"));
	}
}

void UCaptureGaugeComponent::CalculateCaptureGauge()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		CaptureGauge = 0.0f;
		//UE_LOG(LogTemp, Error, TEXT("[CaptureGauge] Owner is null!"));
		return;
	}

	//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] === Calculation Start ==="));

	// P-Pellet 무적 상태 체크
	if (IsInvulnerable())
	{
		CaptureGauge = FMath::Min(CaptureGauge, Config.InvulnerableMaxGauge);
		//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] Invulnerable - Gauge capped at %.1f"), CaptureGauge);
		return;
	}

	// 모든 Driver 위협 데이터 수집
	CurrentThreatData = GatherDriverThreats();
	//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] Gathered %d drivers (DetectionRange: %.1f)"),CurrentThreatData.Num(), Config.DetectionRange);

	// 위협 점수 합산 및 보정
	float TotalThreat = AggregateThreats(CurrentThreatData);
	//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] TotalThreat before terrain: %.1f"), TotalThreat);

	// 지형 보정 (전방 장애물)
	FVector PacmanLocation = Owner->GetActorLocation();
	FVector PacmanForward = Owner->GetActorForwardVector();

	if (CheckForwardObstacle(PacmanLocation, PacmanForward))
	{
		TotalThreat += Config.BlockedPathScore;
		//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] Forward obstacle detected - Added %.1f"), Config.BlockedPathScore);
	}

	// 0~100으로 정규화
	CaptureGauge = FMath::Clamp(TotalThreat, 0.0f, 100.0f);

	/*UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] FINAL: %.1f (Drivers: %d, TotalThreat: %.1f)"),
		CaptureGauge, CurrentThreatData.Num(), TotalThreat);*/
}

TArray<FDriverThreatData> UCaptureGaugeComponent::GatherDriverThreats()
{
	TArray<FDriverThreatData> Threats;

	UWorld* World = GetWorld();
	if (!World)
	{
		return Threats;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return Threats;
	}

	FVector PacmanLocation = Owner->GetActorLocation();
	FVector PacmanForward = Owner->GetActorForwardVector();

	// GameState에서 모든 Driver 수집
	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return Threats;
	}

	const TArray<TObjectPtr<APlayerState>>& PlayerArray = GameState->PlayerArray;

	//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] Total PlayerStates: %d"), PlayerArray.Num());

	// PlayerArray 순회 (Driver들)
	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		APlayerState* PlayerState = PlayerArray[i];
		if (!PlayerState)
		{
			//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] PlayerState[%d] is null"), i);
			continue;
		}

		// PlayerRole 체크 (Racer만)
		ACitRushPlayerState* CitRushPS = Cast<ACitRushPlayerState>(PlayerState);
		if (CitRushPS && CitRushPS->GetPlayerRole() != EPlayerRole::Racer)
		{
			//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] PlayerState[%d] is not Racer (Role: %d), skipping"), i, (int32)CitRushPS->GetPlayerRole());
			continue;
		}

		APawn* PlayerPawn = PlayerState->GetPawn();
		if (!PlayerPawn)
		{
			//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] PlayerState[%d] has no Pawn"), i);
			continue;
		}

		// AAbstractRacer로 캐스팅 (Driver만)
		AAbstractRacer* Driver = Cast<AAbstractRacer>(PlayerPawn);
		if (!Driver)
		{
			//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] PlayerPawn[%d] is not AAbstractRacer"), i);
			continue;
		}

		// 탐지 범위 체크
		float Distance = FVector::Dist(PacmanLocation, Driver->GetActorLocation());
		//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] Driver[%d] distance: %.1f (DetectionRange: %.1f)"), i, Distance, Config.DetectionRange);

		if (Distance > Config.DetectionRange)
		{
			//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] Driver[%d] too far, skipping"), i);
			continue;
		}

		// 위협 데이터 계산
		FDriverThreatData ThreatData = CalculateDriverThreat(Driver, PacmanLocation, PacmanForward);

		if (ThreatData.IsValid())
		{
			/*UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] Driver[%d] threat score: %.1f (PathCost: %.1f, Distance: %.1f)"),
				i, ThreatData.ThreatScore, ThreatData.PathCost, ThreatData.StraightDistance);*/
			Threats.Add(ThreatData);
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] Driver[%d] threat data invalid (PathCost: %.1f)"), i, ThreatData.PathCost);
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("[CaptureGauge] Final threat count: %d"), Threats.Num());

	return Threats;
}

FDriverThreatData UCaptureGaugeComponent::CalculateDriverThreat(AAbstractRacer* Driver, const FVector& PacmanLocation, const FVector& PacmanForward)
{
	FDriverThreatData Data;
	Data.Driver = Driver;

	if (!Driver)
	{
		return Data;
	}

	FVector DriverLocation = Driver->GetActorLocation();

	// 직선 거리
	Data.StraightDistance = FVector::Dist(PacmanLocation, DriverLocation);

	// NavMesh 경로 계산 (NavSystemDataComponent 로직 재사용)
	UNavSystemDataComponent::CalculateNavPathCost(this, PacmanLocation, DriverLocation, Data.PathCost, Data.PathDistance, GetOwner());

	// 상대 각도 계산 (NavSystemDataComponent 로직 재사용)
	Data.RelativeAngle = UNavSystemDataComponent::CalculateRelativeAngle(PacmanLocation, PacmanForward, DriverLocation);

	// 백어택 체크
	Data.bIsInBackAttackZone = IsInBackAttackZone(Data.RelativeAngle);

	// 접근 속도 계산
	Data.ApproachSpeed = CalculateApproachSpeed(Driver, PacmanLocation);

	// 부스터 사용 체크
	Data.bIsBoosting = Driver->IsBoosting();

	// 개별 위협 점수 계산
	Data.ThreatScore = CalculateIndividualThreatScore(Data);

	return Data;
}

bool UCaptureGaugeComponent::IsInBackAttackZone(float RelativeAngle)
{
	// 뒤쪽 120도 = -120 ~ -60 또는 60 ~ 120
	float AbsAngle = FMath::Abs(RelativeAngle);
	float HalfBackAttackAngle = Config.BackAttackAngle / 2.0f; // 60도

	// 120도보다 크면 백어택 존
	return AbsAngle >= (180.0f - HalfBackAttackAngle);
}

float UCaptureGaugeComponent::CalculateApproachSpeed(AAbstractRacer* Driver, const FVector& PacmanLocation)
{
	if (!Driver)
	{
		return 0.0f;
	}

	FVector DriverLocation = Driver->GetActorLocation();
	FVector DriverVelocity = Driver->GetVelocity();

	// Driver에서 Pacman으로의 방향
	FVector ToPacman = (PacmanLocation - DriverLocation).GetSafeNormal();

	// 속도를 Pacman 방향으로 투영
	float ApproachSpeed = FVector::DotProduct(DriverVelocity, ToPacman);

	return ApproachSpeed;
}

float UCaptureGaugeComponent::CalculateIndividualThreatScore(const FDriverThreatData& ThreatData)
{
	if (!ThreatData.IsValid())
	{
		return 0.0f;
	}

	float Score = 0.0f;

	// 1. NavMesh 경로 비용 기반
	if (ThreatData.PathCost >= 0.0f)
	{
		// PathCost가 낮을수록 위협 (0-50000cm 범위로 정규화)
		// PathCost가 0이면 max 점수, 50000이면 0점
		float NormalizedCost = FMath::Clamp(ThreatData.PathCost, 0.0f, 50000.0f);
		float NavScore = (50000.0f - NormalizedCost) * Config.NavCostThreatMultiplier;
		Score += FMath::Max(0.0f, NavScore); // 음수 방지
	}

	// 2. 거리 기반
	if (ThreatData.StraightDistance < Config.VeryCloseRange)
	{
		Score += Config.VeryCloseThreatScore;
	}
	else if (ThreatData.StraightDistance < Config.CloseRange)
	{
		Score += Config.CloseThreatScore;
	}
	else if (ThreatData.StraightDistance < Config.MidRange)
	{
		Score += Config.MidThreatScore;
	}
	else if (ThreatData.StraightDistance < Config.FarRange)
	{
		// 6000-10000cm: 멀리 떨어져 있음
		Score += Config.FarThreatScore;
	}
	else
	{
		// 10000cm 이상: 매우 멀리 떨어져 있음
		Score += Config.VeryFarThreatScore;
	}

	// 3. 백어택 보너스
	if (ThreatData.bIsInBackAttackZone)
	{
		Score += Config.BackAttackBonusScore;
	}

	// 4. 접근 속도
	if (ThreatData.ApproachSpeed > Config.FastApproachSpeed)
	{
		Score += Config.FastApproachScore;
	}
	else if (ThreatData.ApproachSpeed > Config.MediumApproachSpeed)
	{
		Score += Config.MediumApproachScore;
	}
	else if (ThreatData.ApproachSpeed < 0.0f)
	{
		// 멀어지는 중이면 위협도 감소
		Score *= 0.5f;
	}

	// 5. 부스터 사용
	if (ThreatData.bIsBoosting)
	{
		Score += Config.BoostingBonusScore;
	}

	return Score;
}

float UCaptureGaugeComponent::AggregateThreats(const TArray<FDriverThreatData>& ThreatDataArray)
{
	if (ThreatDataArray.Num() == 0)
	{
		return 0.0f;
	}

	float TotalScore = 0.0f;
	int32 BackAttackCount = 0;

	// 개별 위협 점수 합산
	for (const FDriverThreatData& Data : ThreatDataArray)
	{
		TotalScore += Data.ThreatScore;

		if (Data.bIsInBackAttackZone)
		{
			BackAttackCount++;
		}
	}

	// 다중 추격 보정
	int32 ChaserCount = ThreatDataArray.Num();

	if (ChaserCount >= 3)
	{
		TotalScore *= Config.ThreeChaserMultiplier;
		//UE_LOG(LogTemp, Verbose, TEXT("[CaptureGauge] 3+ chasers - Multiplier: %.2f"), Config.ThreeChaserMultiplier);
	}
	else if (ChaserCount >= 2)
	{
		TotalScore *= Config.TwoChaserMultiplier;
		//UE_LOG(LogTemp, Verbose, TEXT("[CaptureGauge] 2 chasers - Multiplier: %.2f"), Config.TwoChaserMultiplier);
	}

	// 백어택 포위 보정
	if (BackAttackCount >= 2)
	{
		TotalScore += Config.BackAttackSurroundScore;
		//UE_LOG(LogTemp, Verbose, TEXT("[CaptureGauge] Back attack surround - Added %.1f"), Config.BackAttackSurroundScore);
	}

	return TotalScore;
}

bool UCaptureGaugeComponent::CheckForwardObstacle(const FVector& PacmanLocation, const FVector& PacmanForward)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	FVector EndLocation = PacmanLocation + PacmanForward * Config.ObstacleCheckDistance;

	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		PacmanLocation,
		EndLocation,
		ECC_WorldStatic,
		Params
	);

	return bHit;
}

bool UCaptureGaugeComponent::IsInvulnerable()
{
	APixelEnemy* PixelEnemy = Cast<APixelEnemy>(GetOwner());
	if (!PixelEnemy)
	{
		return false;
	}

	// PixelEnemy의 무적 상태 확인
	// bPowerPellet 또는 bUntouchable은 private이므로 public getter 필요
	// 임시로 GetCurrentGameState()를 통해 확인
	FEnemyGameState State = PixelEnemy->GetCurrentGameState();
	return State.is_invulnerable;
}
