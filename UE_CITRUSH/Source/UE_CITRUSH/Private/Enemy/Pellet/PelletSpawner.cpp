// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/Pellet/PelletSpawner.h"
#include "Enemy/Pellet/PelletActor.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Private/Network/Schemas/HttpV1/RequestHttp.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshPath.h"

APelletSpawner::APelletSpawner()
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
	SpawnVolume->ShapeColor = FColor::Red; 
}

void APelletSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bAutoSpawnOnBeginPlay)
	{
		SpawnPellets();
	}
}

#if WITH_EDITOR
void APelletSpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SpawnVolume)
	{
		SpawnVolume->SetHiddenInGame(!bShowDebugVisualization);
	}
}

void APelletSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(APelletSpawner, PelletCount))
	{
		// Pellet 개수 변경 시 처리
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(APelletSpawner, bShowDebugVisualization))
	{
		if (SpawnVolume)
		{
			SpawnVolume->SetHiddenInGame(!bShowDebugVisualization);
		}
	}
}
#endif

void APelletSpawner::SpawnPellets()
{
	if (!PelletClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PelletSpawner] PelletClass is not set"));
		return;
	}

	if (PelletCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PelletSpawner] PelletCount is 0 or negative"));
		return;
	}

	ClearSpawnedPellets();

	// 고정 위치 사용 모드
	if (bUseFixedLocations && FixedSpawnLocations.Num() > 0)
	{
		const int32 SpawnCount = FMath::Min(PelletCount, FixedSpawnLocations.Num());

		for (int32 i = 0; i < SpawnCount; ++i)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			SpawnParams.Owner = this;

			// 고정 위치에 Pellet Spawn
			APelletActor* NewPellet = GetWorld()->SpawnActor<APelletActor>(
				PelletClass,
				FixedSpawnLocations[i],
				FRotator::ZeroRotator,
				SpawnParams
			);

			if (NewPellet)
			{
				SpawnedPellets.Add(NewPellet);

#if !UE_BUILD_SHIPPING
				// Debug Visualization
				if (bShowDebugVisualization)
				{
					DrawDebugSphere(
						GetWorld(),
						FixedSpawnLocations[i],
						80.0f,
						12,
						FColor::Red,
						false,
						5.0f
					);
				}
#endif
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[PelletSpawner] Spawned %d P-Pellets at fixed locations"), SpawnedPellets.Num());
		return;
	}

	// 랜덤 위치 사용 모드
	if (bUseRandomSeed)
	{
		RandomStream.Initialize(RandomSeed);
	}
	else
	{
		RandomStream.Initialize(FMath::Rand());
	}

	TArray<FVector> SpawnedLocations;
	SpawnedLocations.Reserve(PelletCount);

	int32 SpawnAttempts = 0;
	const int32 MaxAttempts = PelletCount * 10;

	for (int32 i = 0; i < PelletCount && SpawnAttempts < MaxAttempts; ++SpawnAttempts)
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
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // 충돌 조정 없이 계산된 위치(Z Offset 포함)에 강제 스폰
		SpawnParams.Owner = this;

		// Pellet Spawn (NavMesh 투영된 위치 사용)
		APelletActor* NewPellet = GetWorld()->SpawnActor<APelletActor>(
			PelletClass,
			ProjectedLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (NewPellet)
		{
			// ID 할당
			FString NewPelletID = FString::Printf(TEXT("%s_pellet_%d"), *GetName(), i);
			NewPellet->SetPelletID(NewPelletID);

			SpawnedPellets.Add(NewPellet);
			SpawnedLocations.Add(RandomLocation);
			++i;

#if !UE_BUILD_SHIPPING
			// Debug Visualization
			if (bShowDebugVisualization)
			{
				DrawDebugSphere(
					GetWorld(),
					RandomLocation,
					MinDistanceBetweenPellets * 0.5f,
					12,
					FColor::Red,
					false,
					5.0f
				);
			}
#endif
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[PelletSpawner] Spawned %d P-Pellets (Attempts: %d)"),
		SpawnedPellets.Num(), SpawnAttempts);
}

void APelletSpawner::ClearSpawnedPellets()
{
	for (APelletActor* Pellet : SpawnedPellets)
	{
		if (IsValid(Pellet))
		{
			Pellet->Destroy();
		}
	}

	SpawnedPellets.Empty();
}

void APelletSpawner::SetPelletCount(int32 NewCount)
{
	PelletCount = FMath::Clamp(NewCount, 0, 100);
	SpawnPellets();
}

FVector APelletSpawner::GetRandomLocationInVolume() const
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

	return BoxCenter + FVector(RandomX, RandomY, 0.0f) + FVector(0.0f, 0.0f, -BoxExtent.Z + HeightOffset);
}

bool APelletSpawner::IsLocationValid(const FVector& Location, const TArray<FVector>& ExistingLocations) const
{
	for (const FVector& ExistingLocation : ExistingLocations)
	{
		const float Distance = FVector::Dist2D(Location, ExistingLocation);
		if (Distance < MinDistanceBetweenPellets)
		{
			return false;
		}
	}

	return true;
}

TArray<FPPelletLocation> APelletSpawner::CollectPPelletData() const
{
	TArray<FPPelletLocation> PPellets;
	PPellets.Reserve(SpawnedPellets.Num());

	for (int32 i = 0; i < SpawnedPellets.Num(); ++i)
	{
		const APelletActor* Pellet = SpawnedPellets[i];

		// 유효하지 않은 Pellet은 스킵
		if (!IsValid(Pellet))
		{
			continue;
		}

		FPPelletLocation PPellet;

		// ID 사용 (저장된 ID가 있으면 사용, 없으면 생성)
		FString StoredID = Pellet->GetPelletID();
		if (!StoredID.IsEmpty())
		{
			PPellet.id = StoredID;
		}
		else
		{
			PPellet.id = FString::Printf(TEXT("%s_pellet_%d"), *GetName(), i);
		}

		// 위치 정보
		const FVector Location = Pellet->GetActorLocation();
		PPellet.x = Location.X;
		PPellet.y = Location.Y;
		PPellet.z = Location.Z;

		// 획득 가능 여부 (쿨타임 체크)
		PPellet.available = Pellet->IsAvailable();

		// 쿨타임 정보 (남은 쿨타임 시간)
		PPellet.cooldown = Pellet->GetRemainingCooldown();

		PPellets.Add(PPellet);
	}

	return PPellets;
}

bool APelletSpawner::ProjectToNavMesh(const FVector& Location, FVector& OutProjectedLocation) const
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
