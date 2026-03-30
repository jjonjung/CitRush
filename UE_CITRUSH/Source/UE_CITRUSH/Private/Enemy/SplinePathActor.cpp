// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/SplinePathActor.h"
#include "DrawDebugHelpers.h"

ASplinePathActor::ASplinePathActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	RootComponent = SplineComponent;
	SplineComponent->SetClosedLoop(bClosedLoop);
	SplineComponent->Duration = 1.0f;
}

void ASplinePathActor::BeginPlay()
{
	Super::BeginPlay();

	if (SplineComponent)
	{
		SplineComponent->SetClosedLoop(bClosedLoop);
	}
}

void ASplinePathActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void ASplinePathActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SplineComponent)
	{
		SplineComponent->SetClosedLoop(bClosedLoop);

		if (bDrawDebugPath)
		{
			const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
			const float SplineLength = SplineComponent->GetSplineLength();
			const int32 DrawSegments = FMath::Max(10, NumPoints * 5);

			for (int32 i = 0; i < DrawSegments; ++i)
			{
				const float CurrentDistance = (float)i / DrawSegments * SplineLength;
				const float NextDistance = (float)(i + 1) / DrawSegments * SplineLength;

				const FVector CurrentLocation = SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World);
				const FVector NextLocation = SplineComponent->GetLocationAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::World);

				DrawDebugLine(
					GetWorld(),
					CurrentLocation,
					NextLocation,
					DebugPathColor.ToFColor(true),
					false,
					-1.0f,
					0,
					5.0f
				);
			}
		}
	}
}

//자동저장
void ASplinePathActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (SplineComponent && PropertyChangedEvent.MemberProperty)
	{
		const FName PropertyName = PropertyChangedEvent.MemberProperty->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(USplineComponent, SplineCurves))
		{
			SaveSplinePointsToArray();
		}
	}
}
#endif

float ASplinePathActor::GetSplineLength() const
{
	if (!IsValid(SplineComponent))
	{
		return 0.0f;
	}
	return SplineComponent->GetSplineLength();
}

FVector ASplinePathActor::GetLocationAtDistanceAlongSpline(float Distance) const
{
	if (!IsValid(SplineComponent))
	{
		return FVector::ZeroVector;
	}
	return SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

FRotator ASplinePathActor::GetRotationAtDistanceAlongSpline(float Distance) const
{
	if (!IsValid(SplineComponent))
	{
		return FRotator::ZeroRotator;
	}
	return SplineComponent->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

int32 ASplinePathActor::GetNumberOfSplinePoints() const
{
	if (!IsValid(SplineComponent))
	{
		return 0;
	}
	return SplineComponent->GetNumberOfSplinePoints();
}

#if WITH_EDITOR
void ASplinePathActor::SaveSplinePointsToArray()
{
	if (!SplineComponent)
	{
		return;
	}

	SavedSplinePoints.Empty();
	const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();

	for (int32 i = 0; i < NumPoints; ++i)
	{
		const FVector PointLocation = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		SavedSplinePoints.Add(PointLocation);
	}

	UE_LOG(LogTemp, Log, TEXT("SplinePathActor: Saved %d spline points"), SavedSplinePoints.Num());
}

void ASplinePathActor::LoadSplinePointsFromArray()
{
	if (!SplineComponent || SavedSplinePoints.Num() == 0)
	{
		return;
	}

	SplineComponent->ClearSplinePoints(false);

	for (int32 i = 0; i < SavedSplinePoints.Num(); ++i)
	{
		SplineComponent->AddSplinePoint(SavedSplinePoints[i], ESplineCoordinateSpace::World, false);
	}

	SplineComponent->UpdateSpline();

	UE_LOG(LogTemp, Log, TEXT("SplinePathActor: Loaded %d spline points"), SavedSplinePoints.Num());
}
#endif
