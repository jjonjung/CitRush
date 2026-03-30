#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PingTypes.generated.h"

/**
 * 핑 타입 (목표, 위험, 수집 등)
 */
UENUM(BlueprintType)
enum class ECommanderPingType : uint8
{
	Objective	UMETA(DisplayName = "Objective"),
	Danger		UMETA(DisplayName = "Danger"),
	Collect		UMETA(DisplayName = "Collect"),
	Custom		UMETA(DisplayName = "Custom"),
	Extra1		UMETA(DisplayName = "Extra1"),
	Extra2		UMETA(DisplayName = "Extra2")
};

/**
 * 핑 데이터 구조체
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FPingData
{
	GENERATED_BODY()

	/** 핑 고유 ID */
	UPROPERTY(BlueprintReadOnly)
	FGuid PingId;

	/** 월드 위치 */
	UPROPERTY(BlueprintReadOnly)
	FVector WorldLocation;

	/** 핑 타입 */
	UPROPERTY(BlueprintReadOnly)
	ECommanderPingType Type = ECommanderPingType::Objective;

	/** 서버에서 스폰된 시간 */
	UPROPERTY(BlueprintReadOnly)
	float SpawnServerTime = 0.f;

	/** 지속 시간 (초, 0이면 무한) */
	UPROPERTY(BlueprintReadOnly)
	float Duration = 0.f;

	/** 핑을 생성한 플레이어의 PlayerState (WeakPtr) */
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<APlayerState> OwnerPS;

	/** 기본 생성자 */
	FPingData()
		: PingId(FGuid::NewGuid())
		, WorldLocation(FVector::ZeroVector)
		, Type(ECommanderPingType::Objective)
		, SpawnServerTime(0.f)
		, Duration(0.f)
		, OwnerPS(nullptr)
	{}

	/** 파라미터 생성자 */
	FPingData(const FVector& InLocation, ECommanderPingType InType, float InDuration, APlayerState* InOwnerPS)
		: PingId(FGuid::NewGuid())
		, WorldLocation(InLocation)
		, Type(InType)
		, SpawnServerTime(0.f)
		, Duration(InDuration)
		, OwnerPS(InOwnerPS ? TWeakObjectPtr<APlayerState>(InOwnerPS) : TWeakObjectPtr<APlayerState>())
	{}

	/** 유효한 핑인지 확인 */
	bool IsValid() const
	{
		return !PingId.IsValid() == false && WorldLocation != FVector::ZeroVector;
	}

	/** 만료되었는지 확인 */
	bool IsExpired(float CurrentServerTime) const
	{
		if (Duration <= 0.f)
		{
			return false; // 무한 지속
		}
		return (CurrentServerTime - SpawnServerTime) >= Duration;
	}
};
