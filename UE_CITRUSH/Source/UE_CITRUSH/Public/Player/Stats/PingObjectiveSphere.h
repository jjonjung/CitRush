#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "PingObjectiveSphere.generated.h"

class APlayerState;

/**
 * 핑 목표 지점의 점수 처리용 Sphere
 * 서버에서만 작동하며, Racer가 오버랩하면 점수 처리
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API APingObjectiveSphere : public AActor
{
	GENERATED_BODY()
	
public:	
	APingObjectiveSphere();

protected:
	virtual void BeginPlay() override;

	/** 오버랩 시작 이벤트 */
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	/** 점수를 받은 플레이어 목록 초기화 */
	UFUNCTION(BlueprintCallable, Category = "Ping Objective")
	void ResetScoredPlayers();

	/** 점수 처리 (서버 전용) */
	UFUNCTION(BlueprintCallable, Category = "Ping Objective")
	void AwardScore(APlayerState* PlayerState);

protected:
	/** Sphere 충돌 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereComponent;

	/** 점수를 받은 플레이어 목록 (중복 방지) */
	UPROPERTY()
	TSet<APlayerState*> ScoredPlayers;

	/** Sphere 반경 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Objective")
	float SphereRadius = 500.f;

	/** 점수 값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Objective")
	int32 ScoreValue = 100;
};
