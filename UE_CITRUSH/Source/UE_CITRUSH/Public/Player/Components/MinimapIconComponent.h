#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MinimapIconComponent.generated.h"

/**
 * 미니맵 아이콘 타입
 */
/**
 * 실시간 맵 아이콘 ID (아이콘 외형 선택 전용)
 * 팀/적군/성격 판단 로직과 분리된 아이콘 모양 결정 Enum
 */
UENUM(BlueprintType)
enum class ERealtimeMapIconId : uint8
{
	Racer1		UMETA(DisplayName = "Racer1"),
	Racer2		UMETA(DisplayName = "Racer2"),
	Racer3		UMETA(DisplayName = "Racer3"),
	Enemy		UMETA(DisplayName = "Enemy")
};

/**
 * 미니맵에 표시될 Actor를 선언하는 컴포넌트
 * 이 컴포넌트가 붙은 Actor는 전체지도(Map UI)에 실시간으로 표시됩니다.
 */
UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UMinimapIconComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMinimapIconComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 맵에 표시할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap Icon")
	bool bShowOnMap = true;

	/** 아이콘 ID (아이콘 외형 선택 전용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Realtime Map Icon")
	ERealtimeMapIconId IconId = ERealtimeMapIconId::Racer1;

	/** 팀 ID (색상 구분용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap Icon")
	int32 TeamId = 0;

	/** Actor 회전에 따라 아이콘도 회전할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap Icon")
	bool bRotateWithActor = false;

	/** 현재 Actor의 Yaw (회전) 값 반환 */
	UFUNCTION(BlueprintPure, Category = "Minimap Icon")
	float GetActorYaw() const;

	/** PlayerState의 PlayerRole 기반으로 아이콘 ID 설정 */
	UFUNCTION(BlueprintCallable, Category = "Minimap Icon")
	void ApplyIconIdFromPlayerState();

	/** Actor의 태그를 기반으로 아이콘 ID 설정 */
	UFUNCTION(BlueprintCallable, Category = "Minimap Icon")
	void ApplyIconIdFromActorTag();

protected:
	/** PlayerState 확인 및 아이콘 업데이트 (재시도용) */
	void TryUpdateIconFromPlayerState();

	/** PlayerState 역할 변경 델리게이트 구독 */
	void SubscribeToPlayerStateRoleChange();

	/** PlayerState 역할 변경 델리게이트 구독 해제 */
	void UnsubscribeFromPlayerStateRoleChange();

	/** PlayerState 역할 변경 시 호출되는 콜백 */
	UFUNCTION()
	void OnPlayerRoleChanged(const EPlayerRole& NewRole);

	/** PlayerState 재확인 타이머 핸들 */
	FTimerHandle RetryTimerHandle;
};

