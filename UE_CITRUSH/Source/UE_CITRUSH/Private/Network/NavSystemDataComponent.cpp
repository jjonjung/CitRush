// Fill out your copyright notice in the Description page of Project Settings.

#include "Network/NavSystemDataComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavMesh/NavMeshPath.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Player/CitRushPlayerState.h"
#include "AIController.h"
#include "Enemy/PixelEnemy.h"

UNavSystemDataComponent::UNavSystemDataComponent()
{
	// Tick 비활성화 (수동으로 CalculateNavigationData 호출)
	PrimaryComponentTick.bCanEverTick = false;

	// 초기화
	PreviousInterPlayerDistance = -1.0f;
	PreviousTimestamp = 0.0f;
}

void UNavSystemDataComponent::BeginPlay()
{
	Super::BeginPlay();

	// 초기화
	PreviousRacerData.Empty();
	PreviousInterPlayerDistance = -1.0f;
	PreviousTimestamp = 0.0f;
}

void UNavSystemDataComponent::CalculateNavigationData()
{
	APixelEnemy* OwnerActor = Cast<APixelEnemy>(GetOwner());
	if (!OwnerActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("NavSystemDataComponent: 소유자 액터를 찾을 수 없습니다"));
		return;
	}

	// 월드 가져오기
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("NavSystemDataComponent: 월드가 유효하지 않습니다"));
		return;
	}

	// GameState 가져오기
	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("NavSystemDataComponent: 게임스테이트를 찾을 수 없습니다"));
		return;
	}

	// 시간 정보 계산
	float CurrentTime = World->GetTimeSeconds();
	float DeltaTime = (PreviousTimestamp > 0.0f) ? (CurrentTime - PreviousTimestamp) : 0.0f;

	// 현재 데이터 초기화
	CurrentData = FNavSystemLLMData();
	CurrentData.Timestamp = CurrentTime;
	CurrentData.DeltaTime = DeltaTime;

	// 보스 정보 설정
	CurrentData.BossWorldLocation = OwnerActor->GetActorLocation();

	// PlayerArray 가져오기
	const TArray<TObjectPtr<APlayerState>>& PlayerArray = GameState->PlayerArray;

	UE_LOG(LogTemp, Log, TEXT("[NavSystemDataComponent] PlayerArray.Num(): %d"), PlayerArray.Num());

	// PlayerArray의 모든 플레이어 순회 (Commander 제외, Racer만 처리)
	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		if (!PlayerArray.IsValidIndex(i) || !PlayerArray[i])
		{
			continue;
		}

		APlayerState* PS = PlayerArray[i];
		ACitRushPlayerState* CPS = Cast<ACitRushPlayerState>(PS);

		// CitRushPlayerState가 아니거나 Racer가 아니면 스킵
		if (!CPS)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[NavSystemDataComponent] PlayerArray[%d] is not CitRushPlayerState"), i);
			continue;
		}

		EPlayerRole Role = CPS->GetPlayerRole();
		if (Role != EPlayerRole::Racer)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[NavSystemDataComponent] PlayerArray[%d] is not Racer (Role: %d)"), i, (int32)Role);
			continue;
		}

		// Racer인 경우 데이터 계산
		APawn* PlayerPawn = PS->GetPawn();
		if (PlayerPawn)
		{
			FRacerNavigationData RacerData = CalculateRacerData(i, PlayerPawn, DeltaTime);
			CurrentData.RacerData.Add(RacerData);

			UE_LOG(LogTemp, Log, TEXT("[NavSystemDataComponent] Added Racer data for PlayerArray[%d], PathValid: %s, StraightDist: %.1f"),
				i, RacerData.IsValid() ? TEXT("true") : TEXT("false"), RacerData.StraightDistance);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[NavSystemDataComponent] PlayerArray[%d] is Racer but has no Pawn"), i);
		}
	}

	// 플레이어 간 분산도 계산
	float CurrentInterPlayerDistance = CalculateInterPlayerDistance(CurrentData.RacerData);
	CurrentData.InterPlayerAvgDistance = CurrentInterPlayerDistance;

	UE_LOG(LogTemp, Log, TEXT("[NavSystemDataComponent] RacerData.Num: %d, InterPlayerAvgDistance: %.1f"),
		CurrentData.RacerData.Num(), CurrentInterPlayerDistance);

	if (PreviousInterPlayerDistance >= 0.0f && CurrentInterPlayerDistance >= 0.0f)
	{
		CurrentData.DeltaInterPlayerDistance = CurrentInterPlayerDistance - PreviousInterPlayerDistance;
	}
	else
	{
		CurrentData.DeltaInterPlayerDistance = 0.0f;
	}

	UE_LOG(LogTemp, Log, TEXT("[NavSystemDataComponent] DeltaInterPlayerDistance: %.1f (Prev: %.1f, Current: %.1f)"),
		CurrentData.DeltaInterPlayerDistance, PreviousInterPlayerDistance, CurrentInterPlayerDistance);

	// 다음 계산을 위해 현재 데이터 저장
	PreviousRacerData = CurrentData.RacerData;
	PreviousInterPlayerDistance = CurrentInterPlayerDistance;
	PreviousTimestamp = CurrentTime;
}

FNavSystemLLMData UNavSystemDataComponent::GetLLMData() const
{
	return CurrentData;
}

FRacerNavigationData UNavSystemDataComponent::CalculateRacerData(int32 PlayerIndex, APawn* PlayerPawn, float DeltaTime)
{
	FRacerNavigationData Data;
	Data.PlayerIndex = PlayerIndex;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !PlayerPawn)
	{
		return Data;
	}

	// 보스 정보
	FVector BossLocation = OwnerActor->GetActorLocation();
	FVector BossForward = OwnerActor->GetActorForwardVector();

	// 플레이어 정보
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	Data.PlayerWorldLocation = PlayerLocation;

	// 직선 거리 계산
	Data.StraightDistance = FVector::Dist(BossLocation, PlayerLocation);

	// NavMesh 경로 계산
	CalculateNavPathCost(this, BossLocation, PlayerLocation, Data.PathCost, Data.PathDistance, OwnerActor);

	// 보스 기준 상대 각도 계산
	Data.RelativeAngleToBoss = CalculateRelativeAngle(BossLocation, BossForward, PlayerLocation);

	// 이전 프레임 데이터 찾기
	const FRacerNavigationData* PrevData = FindPreviousData(PlayerIndex);

	if (PrevData && PrevData->IsValid() && DeltaTime > 0.0f)
	{
		// Delta 위치
		Data.DeltaPosition = PlayerLocation - PrevData->PlayerWorldLocation;

		// Delta 직선 거리
		Data.DeltaStraightDistance = Data.StraightDistance - PrevData->StraightDistance;

		// Delta 경로 거리
		if (Data.PathDistance >= 0.0f && PrevData->PathDistance >= 0.0f)
		{
			Data.DeltaPathDistance = Data.PathDistance - PrevData->PathDistance;
		}

		// Delta 경로 비용
		if (Data.PathCost >= 0.0f && PrevData->PathCost >= 0.0f)
		{
			Data.DeltaPathCost = Data.PathCost - PrevData->PathCost;
		}

		// 평균 속도
		Data.AverageSpeed = Data.DeltaPosition.Size() / DeltaTime;

		// 이동 방향 변화
		FVector PrevDirection = PrevData->DeltaPosition;
		FVector CurrentDirection = Data.DeltaPosition;

		if (!PrevDirection.IsNearlyZero() && !CurrentDirection.IsNearlyZero())
		{
			Data.MovementDirectionChange = CalculateDirectionChange(PrevDirection, CurrentDirection);
		}

		// 보스와의 상대 방향 변화
		Data.RelativeBearingChange = Data.RelativeAngleToBoss - PrevData->RelativeAngleToBoss;

		// 각도를 -180 ~ 180 범위로 정규화
		if (Data.RelativeBearingChange > 180.0f)
		{
			Data.RelativeBearingChange -= 360.0f;
		}
		else if (Data.RelativeBearingChange < -180.0f)
		{
			Data.RelativeBearingChange += 360.0f;
		}
	}
	else
	{
		// 첫 프레임 또는 이전 데이터가 invalid - Delta 값 0으로 설정
		Data.DeltaPosition = FVector::ZeroVector;
		Data.DeltaStraightDistance = 0.0f;
		Data.DeltaPathDistance = 0.0f;
		Data.DeltaPathCost = 0.0f;
		Data.MovementDirectionChange = 0.0f;
		Data.AverageSpeed = 0.0f;
		Data.RelativeBearingChange = 0.0f;
	}

	return Data;
}

bool UNavSystemDataComponent::CalculateNavPathCost(const UObject* WorldContextObject, const FVector& StartLocation, const FVector& EndLocation, float& OutCost, float& OutDistance, AActor* PathOwner)
{
	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	// NavigationSystem 가져오기
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		UE_LOG(LogTemp, Warning, TEXT("NavSystemDataComponent: Navigation System이 존재하지 않습니다"));
		return false;
	}

	// NavMesh 데이터 가져오기
	const ANavigationData* NavData = NavSys->GetDefaultNavDataInstance();
	if (NavData == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("NavSystemDataComponent: NavMesh 데이터를 찾을 수 없습니다"));
		return false;
	}

	// 시작 위치와 끝 위치가 NavMesh 위에 있는지 확인
	// Boss가 공중에 있을 수 있으므로 Z Extent를 크게 설정
	FNavLocation StartNavLoc, EndNavLoc;
	bool bStartOnNavMesh = NavSys->ProjectPointToNavigation(StartLocation, StartNavLoc, FVector(1000.0f, 1000.0f, 5000.0f));
	bool bEndOnNavMesh = NavSys->ProjectPointToNavigation(EndLocation, EndNavLoc, FVector(1000.0f, 1000.0f, 5000.0f));

	if (!bStartOnNavMesh)
	{
		// UE_LOG(LogTemp, Warning, TEXT("NavSystemDataComponent: Start location NOT on NavMesh - (%.1f, %.1f, %.1f)"), StartLocation.X, StartLocation.Y, StartLocation.Z);
		OutCost = -1.0f;
		OutDistance = -1.0f;
		return false;
	}

	if (!bEndOnNavMesh)
	{
		// UE_LOG(LogTemp, Warning, TEXT("NavSystemDataComponent: End location NOT on NavMesh - (%.1f, %.1f, %.1f)"), EndLocation.X, EndLocation.Y, EndLocation.Z);
		OutCost = -1.0f;
		OutDistance = -1.0f;
		return false;
	}

	// NavMesh 위로 투영된 위치 사용
	FPathFindingQuery Query(PathOwner, *NavData, StartNavLoc.Location, EndNavLoc.Location);

	// FindPathSync를 사용하여 즉시 경로 찾기
	FPathFindingResult PathResult = NavSys->FindPathSync(Query);

	// 경로 찾기 실패 검사
	if (!PathResult.IsSuccessful() || !PathResult.Path.IsValid())
	{
		// UE_LOG(LogTemp, Warning, TEXT("NavSystemDataComponent: Path finding FAILED"));
		OutCost = -1.0f;
		OutDistance = -1.0f;
		return false;
	}

	// 경로 비용 가져오기
	OutCost = PathResult.Path->GetCost();

	// 경로 길이 계산 (cm)
	OutDistance = PathResult.Path->GetLength();

	return true;
}

float UNavSystemDataComponent::CalculateRelativeAngle(const FVector& BossLocation, const FVector& BossForward, const FVector& PlayerLocation)
{
	// 보스에서 플레이어로의 방향 벡터
	FVector ToPlayer = (PlayerLocation - BossLocation).GetSafeNormal2D();

	// 보스의 forward 벡터 (2D)
	FVector BossForward2D = BossForward.GetSafeNormal2D();

	if (ToPlayer.IsNearlyZero() || BossForward2D.IsNearlyZero())
	{
		return 0.0f;
	}

	// 내적으로 각도 계산
	float DotProduct = FVector::DotProduct(BossForward2D, ToPlayer);
	float AngleRad = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f));
	float AngleDeg = FMath::RadiansToDegrees(AngleRad);

	// 외적으로 방향 결정 (왼쪽/오른쪽)
	FVector CrossProduct = FVector::CrossProduct(BossForward2D, ToPlayer);

	// Z 값이 음수면 왼쪽 (음수 각도)
	if (CrossProduct.Z < 0.0f)
	{
		AngleDeg = -AngleDeg;
	}

	return AngleDeg;
}

const FRacerNavigationData* UNavSystemDataComponent::FindPreviousData(int32 PlayerIndex) const
{
	for (const FRacerNavigationData& Data : PreviousRacerData)
	{
		if (Data.PlayerIndex == PlayerIndex)
		{
			return &Data;
		}
	}

	return nullptr;
}

float UNavSystemDataComponent::CalculateInterPlayerDistance(const TArray<FRacerNavigationData>& RacerDataArray)
{
	// 유효한 레이서들만 필터링
	TArray<FVector> ValidLocations;

	for (const FRacerNavigationData& Data : RacerDataArray)
	{
		if (Data.IsValid())
		{
			ValidLocations.Add(Data.PlayerWorldLocation);
		}
	}

	// 2명 미만이면 계산 불가
	if (ValidLocations.Num() < 2)
	{
		return -1.0f;
	}

	// 모든 쌍 간 거리 계산
	float TotalDistance = 0.0f;
	int32 PairCount = 0;

	for (int32 i = 0; i < ValidLocations.Num(); ++i)
	{
		for (int32 j = i + 1; j < ValidLocations.Num(); ++j)
		{
			float Distance = FVector::Dist(ValidLocations[i], ValidLocations[j]);
			TotalDistance += Distance;
			PairCount++;
		}
	}

	// 평균 거리 반환
	return (PairCount > 0) ? (TotalDistance / PairCount) : -1.0f;
}

float UNavSystemDataComponent::CalculateDirectionChange(const FVector& Dir1, const FVector& Dir2)
{
	FVector NormDir1 = Dir1.GetSafeNormal();
	FVector NormDir2 = Dir2.GetSafeNormal();

	if (NormDir1.IsNearlyZero() || NormDir2.IsNearlyZero())
	{
		return 0.0f;
	}

	// 내적으로 각도 계산
	float DotProduct = FVector::DotProduct(NormDir1, NormDir2);
	float AngleRad = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f));
	float AngleDeg = FMath::RadiansToDegrees(AngleRad);

	return AngleDeg;
}
