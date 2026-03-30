// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Stats/Coin/CoinSpawner.h"
#include "Player/Stats/Coin//CoinActor.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Private/Network/Schemas/HttpV1/HttpRequest2.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshPath.h"

ACoinSpawner::ACoinSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	bReplicates = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	SetRootComponent(RootComp);

	SpawnVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnVolume"));
	SpawnVolume->SetupAttachment(RootComp);
	SpawnVolume->SetBoxExtent(FVector(500.0f, 500.0f, 100.0f));
	SpawnVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnVolume->SetHiddenInGame(false);
	SpawnVolume->ShapeColor = FColor::Yellow;
}

void ACoinSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bAutoSpawnOnBeginPlay)
	{
		SpawnCoins();
	}
}

#if WITH_EDITOR
void ACoinSpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SpawnVolume)
	{
		SpawnVolume->SetHiddenInGame(!bShowDebugVisualization);
	}
}

void ACoinSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACoinSpawner, CoinCount))
	{
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACoinSpawner, bShowDebugVisualization))
	{
		if (SpawnVolume)
		{
			SpawnVolume->SetHiddenInGame(!bShowDebugVisualization);
		}
	}
}
#endif

void ACoinSpawner::SpawnCoins()
{
	if (!CoinClass)
	{
		return;
	}

	if (CoinCount <= 0)
	{
		return;
	}

	ClearSpawnedCoins();

	// 고정 위치 사용 모드
	if (bUseFixedLocations && FixedSpawnLocations.Num() > 0)
	{
		const int32 SpawnCount = FMath::Min(CoinCount, FixedSpawnLocations.Num());

		for (int32 i = 0; i < SpawnCount; ++i)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Owner = this;

			// 고정 위치에 Coin Spawn
			ACoinActor* NewCoin = GetWorld()->SpawnActor<ACoinActor>(
				CoinClass,
				FixedSpawnLocations[i],
				FRotator::ZeroRotator,
				SpawnParams
			);

			if (NewCoin)
			{
				NewCoin->SetCoinValue(CoinValue);
				SpawnedCoins.Add(NewCoin);

#if !UE_BUILD_SHIPPING
				// Debug Visualization
				if (bShowDebugVisualization)
				{
					DrawDebugSphere(
						GetWorld(),
						FixedSpawnLocations[i],
						50.0f,
						12,
						FColor::Yellow,
						false,
						5.0f
					);
				}
#endif
			}
		}

		//UE_LOG(LogTemp, Log, TEXT("[CoinSpawner] Spawned %d coins at fixed locations"), SpawnedCoins.Num());
		return;
	}

	// 랜덤 위치 사용 모드 (기존 로직)
	if (bUseRandomSeed)
	{
		RandomStream.Initialize(RandomSeed);
	}
	else
	{
		RandomStream.Initialize(FMath::Rand());
	}

	TArray<FVector> SpawnedLocations;
	SpawnedLocations.Reserve(CoinCount);

	int32 SpawnAttempts = 0;
	const int32 MaxAttempts = CoinCount * 10;

	for (int32 i = 0; i < CoinCount && SpawnAttempts < MaxAttempts; ++SpawnAttempts)
	{
		FVector RandomLocation = GetRandomLocationInVolume();

		// NavMesh 위에 투영
		FVector ProjectedLocation;
		if (!ProjectToNavMesh(RandomLocation, ProjectedLocation))
		{
			// NavMesh 위에 없으면 스킵
			continue;
		}

		// 바닥 위 z + 위치 보정
		ProjectedLocation.Z += HeightOffset;

		if (!IsLocationValid(ProjectedLocation, SpawnedLocations))
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;

		// Coin Spawn (NavMesh 투영된 위치 사용)
		ACoinActor* NewCoin = GetWorld()->SpawnActor<ACoinActor>(
			CoinClass,
			ProjectedLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (NewCoin)
		{
			NewCoin->SetCoinValue(CoinValue);

			SpawnedCoins.Add(NewCoin);
			SpawnedLocations.Add(RandomLocation);
			++i;

#if !UE_BUILD_SHIPPING
			// Debug Visualization
			if (bShowDebugVisualization)
			{
				DrawDebugSphere(
					GetWorld(),
					RandomLocation,
					MinDistanceBetweenCoins * 0.5f,
					12,
					FColor::Green,
					false,
					5.0f
				);
			}
#endif
		}
	}


	/*UE_LOG(LogTemp, Log, TEXT("[CoinSpawner] Spawned %d coins (Attempts: %d)"),
		SpawnedCoins.Num(), SpawnAttempts);*/
}

void ACoinSpawner::ClearSpawnedCoins()
{
	for (ACoinActor* Coin : SpawnedCoins)
	{
		if (IsValid(Coin))
		{
			Coin->Destroy();
		}
	}

	SpawnedCoins.Empty();
}

void ACoinSpawner::SetCoinCount(int32 NewCount)
{
	CoinCount = FMath::Clamp(NewCount, 0, 100);
	SpawnCoins();
}

FVector ACoinSpawner::GetRandomLocationInVolume() const
{
	if (!SpawnVolume)
	{
		return GetActorLocation();
	}

	const FVector BoxExtent = SpawnVolume->GetScaledBoxExtent();
	const FVector BoxCenter = SpawnVolume->GetComponentLocation();

	float RandomX, RandomY;

	if (bUseRandomSeed)
	{
		RandomX = RandomStream.FRandRange(-BoxExtent.X, BoxExtent.X);
		RandomY = RandomStream.FRandRange(-BoxExtent.Y, BoxExtent.Y);
	}
	else
	{
		RandomX = FMath::FRandRange(-BoxExtent.X, BoxExtent.X);
		RandomY = FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y);
	}

	const float Z = BoxCenter.Z - BoxExtent.Z + HeightOffset;

	return BoxCenter + FVector(RandomX, RandomY, 0.0f) + FVector(0.0f, 0.0f, -BoxExtent.Z + HeightOffset);
}

bool ACoinSpawner::IsLocationValid(const FVector& Location, const TArray<FVector>& ExistingLocations) const
{
	for (const FVector& ExistingLocation : ExistingLocations)
	{
		const float Distance = FVector::Dist2D(Location, ExistingLocation);
		if (Distance < MinDistanceBetweenCoins)
		{
			return false;
		}
	}

	return true;
}

TArray<FPPointLocation> ACoinSpawner::CollectPPointData() const
{
	TArray<FPPointLocation> PPoints;
	PPoints.Reserve(SpawnedCoins.Num());

	for (int32 i = 0; i < SpawnedCoins.Num(); ++i)
	{
		const ACoinActor* Coin = SpawnedCoins[i];

		// 유효하지 않은 코인은 스킵
		if (!IsValid(Coin))
		{
			continue;
		}

		FPPointLocation PPt;

		// ID 생성 (Spawner 이름 + 인덱스)
		PPt.id = FString::Printf(TEXT("%s_coin_%d"), *GetName(), i);

		// 위치 정보
		const FVector Location = Coin->GetActorLocation();
		PPt.x = Location.X;
		PPt.y = Location.Y;
		PPt.z = Location.Z;

		// 획득 가능 여부 (액터가 유효하면 획득 가능)
		PPt.available = true;

		PPoints.Add(PPt);
	}

	return PPoints;
}

bool ACoinSpawner::ProjectToNavMesh(const FVector& Location, FVector& OutProjectedLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return false;
	}

	// NavMesh에 투영
	FNavLocation NavLocation;
	const FVector ProjectExtent(500.0f, 500.0f, 1000.0f);

	if (NavSys->ProjectPointToNavigation(Location, NavLocation, ProjectExtent))
	{
		OutProjectedLocation = NavLocation.Location;
		return true;
	}

	return false;
}
