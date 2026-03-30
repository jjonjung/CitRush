#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInterface.h"
#include "Data/PingTypes.h"
#include "PingMarkerActor.generated.h"

UENUM(BlueprintType)
enum class EPingMarkerRotationAxis : uint8
{
	None UMETA(DisplayName = "None"),
	X    UMETA(DisplayName = "X (Pitch)"),
	Y    UMETA(DisplayName = "Y (Roll)"),
	Z    UMETA(DisplayName = "Z (Yaw)")
};

/**
 * 월드에 핑 마커를 표시하는 액터 (클라이언트 전용, 비복제)
 * 각 클라이언트가 로컬로 스폰하여 표시
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API APingMarkerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	APingMarkerActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public:
	/** 핑 데이터 설정 및 마커 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "Ping Marker")
	void SetPingData(const FPingData& InPingData);

	/** 마커 위치 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "Ping Marker")
	void UpdateLocation(const FVector& NewLocation);

	/** 마커 제거 */
	UFUNCTION(BlueprintCallable, Category = "Ping Marker")
	void RemoveMarker();

	/** 레이서가 기둥에 부딪혔을 때 처리 */
	UFUNCTION()
	void OnPillarOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                     bool bFromSweep, const FHitResult& SweepResult);

protected:
	/** 데칼 컴포넌트 (바닥에 표시) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDecalComponent> DecalComponent;

	/** 위젯 컴포넌트 (3D 위젯 표시, 선택적) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> WidgetComponent;

	/** 빛나는 기둥용 메쉬 컴포넌트 (머티리얼 Emissive 세게 설정) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PillarMeshComponent;

	/** 충돌 감지용 Box 컴포넌트 (레이서가 들어오면 감지) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBoxComponent;

	/** 핑 타입별로 사용할 메쉬 매핑 (에디터에서 설정, 선택 사항) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Marker|Visual")
	TMap<ECommanderPingType, TObjectPtr<UStaticMesh>> MeshByPingType;

	/** 핑 타입별로 사용할 머티리얼 매핑 (에디터에서 설정, 선택 사항) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Marker|Visual")
	TMap<ECommanderPingType, TObjectPtr<UMaterialInterface>> MaterialByPingType;

	/** 나이아가라 이펙트 컴포넌트 (선택적) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	/** 현재 핑 데이터 */
	UPROPERTY(BlueprintReadOnly, Category = "Ping Marker")
	FPingData CurrentPingData;

	/** 마커 높이 오프셋 (바닥에서 얼마나 위에 표시할지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Marker")
	float MarkerHeightOffset = 40.f;

	/** 어떤 축으로 회전할지 (기본: Z/Yaw) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Marker|Animation")
	EPingMarkerRotationAxis RotationAxis = EPingMarkerRotationAxis::Z;

	/** 초당 회전 횟수 (기본 1회전, 양수=시계 방향) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Marker|Animation", meta = (ClampMin = "0.0"))
	float RotationsPerSecond = 1.0f;
};
