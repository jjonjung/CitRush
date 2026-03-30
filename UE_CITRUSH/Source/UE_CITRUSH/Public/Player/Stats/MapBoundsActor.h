#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "MapBoundsActor.generated.h"

/**
 * 맵 경계를 정의하는 액터
 * Commander UI에서 클릭 좌표를 월드 좌표로 변환하는 데 사용
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API AMapBoundsActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AMapBoundsActor();

protected:
	virtual void BeginPlay() override;

public:
	/** 경계 박스 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Bounds")
	TObjectPtr<UBoxComponent> BoundsBox;

	/** XY 평면 경계 반환 (월드 좌표) */
	UFUNCTION(BlueprintPure, Category = "Map Bounds")
	FBox2D GetXYBounds() const;

	/** Min/Max XY 반환 */
	UFUNCTION(BlueprintPure, Category = "Map Bounds")
	void GetMinMaxXY(FVector2D& OutMin, FVector2D& OutMax) const;

	/** UV 좌표(0~1)를 월드 XY 좌표로 변환 */
	UFUNCTION(BlueprintPure, Category = "Map Bounds")
	FVector2D UVToWorldXY(const FVector2D& UV) const;

	/** 월드 XY 좌표를 UV 좌표(0~1)로 변환 */
	UFUNCTION(BlueprintPure, Category = "Map Bounds")
	FVector2D WorldXYToUV(const FVector2D& WorldXY) const;
};
