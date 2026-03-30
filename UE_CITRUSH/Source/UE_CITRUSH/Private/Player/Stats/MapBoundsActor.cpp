#include "Player/Stats/MapBoundsActor.h"

AMapBoundsActor::AMapBoundsActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BoundsBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsBox"));
	RootComponent = BoundsBox;
	BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsBox->SetBoxExtent(FVector(10000.f, 10000.f, 1000.f)); // 기본 크기
}

void AMapBoundsActor::BeginPlay()
{
	Super::BeginPlay();
}

FBox2D AMapBoundsActor::GetXYBounds() const
{
	if (!BoundsBox)
	{
		return FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
	}

	FVector Origin = BoundsBox->GetComponentLocation();
	FVector Extent = BoundsBox->GetScaledBoxExtent();

	FVector2D Min(Origin.X - Extent.X, Origin.Y - Extent.Y);
	FVector2D Max(Origin.X + Extent.X, Origin.Y + Extent.Y);

	return FBox2D(Min, Max);
}

void AMapBoundsActor::GetMinMaxXY(FVector2D& OutMin, FVector2D& OutMax) const
{
	FBox2D Bounds = GetXYBounds();
	OutMin = Bounds.Min;
	OutMax = Bounds.Max;
}

FVector2D AMapBoundsActor::UVToWorldXY(const FVector2D& UV) const
{
	FBox2D Bounds = GetXYBounds();
	
	// UV를 0~1로 클램프
	FVector2D ClampedUV = FVector2D(
		FMath::Clamp(UV.X, 0.f, 1.f),
		FMath::Clamp(UV.Y, 0.f, 1.f)
	);

	// UV (0~1)를 Bounds.Min ~ Bounds.Max 로 직선 보간
	FVector2D WorldXY = FVector2D(
		FMath::Lerp(Bounds.Min.X, Bounds.Max.X, ClampedUV.X),
		FMath::Lerp(Bounds.Min.Y, Bounds.Max.Y, ClampedUV.Y)
	);

	return WorldXY;
}

FVector2D AMapBoundsActor::WorldXYToUV(const FVector2D& WorldXY) const
{
	FBox2D Bounds = GetXYBounds();
	
	FVector2D UV = FVector2D(
		FMath::GetMappedRangeValueClamped(FVector2D(Bounds.Min.X, Bounds.Max.X), FVector2D(0.f, 1.f), WorldXY.X),
		FMath::GetMappedRangeValueClamped(FVector2D(Bounds.Min.Y, Bounds.Max.Y), FVector2D(0.f, 1.f), WorldXY.Y)
	);

	return UV;
}
