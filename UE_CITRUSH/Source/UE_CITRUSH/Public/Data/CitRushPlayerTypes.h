#pragma once

#include "CoreMinimal.h"
#include "CitRushPlayerTypes.generated.h"

/**
 * 플레이어 역할 타입 (Commander, Racer, Spectator)
 */
UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
	None = 0		UMETA(DisplayName = "None"),
	Commander		UMETA(DisplayName = "Commander"),
	Racer			UMETA(DisplayName = "Racer"),
	Spectator		UMETA(DisplayName = "Spectator")
};

/**
 * 타겟 Racer 지정 타입 (Racer1~3, All)
 */
UENUM(BlueprintType)
enum class ETargetRacer : uint8
{
	None = 0,
	Racer1,
	Racer2,
	Racer3,
	All
};

/**
 * 레벨 전환 로딩 상태 (NotStarted, Traveling, Ready, GameStarted)
 */
UENUM(BlueprintType)
enum class ELoadingState : uint8
{
	NotStarted			UMETA(DisplayName = "NotStarted"),
	StartTravel			UMETA(DisplayName = "StartTravel"),
	StartServerTravel	UMETA(DisplayName = "StartServerTravel"),
	StartClientTravel	UMETA(DisplayName = "StartClientTravel"),
	Traveling			UMETA(DisplayName = "Traveling"),
	PostLogin			UMETA(DisplayName = "PostLogin"),
	Registered			UMETA(DisplayName = "Registered"),
	Ready				UMETA(DisplayName = "Ready"),
	TimerStarted		UMETA(DisplayName = "TimerStarted"),
	MatchStarted		UMETA(DisplayName = "GameStarted")
};

/**
 * 플레이어 정보 (이름, 역할, Ready 상태)
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FPlayerInfo
{
	GENERATED_BODY()
	
	/** 플레이어 이름 */
	UPROPERTY(BlueprintReadOnly)
	FString playerName = FString();

	/** 플레이어 역할 (Commander, Racer, Spectator) */
	UPROPERTY(BlueprintReadOnly)
	EPlayerRole playerRole = EPlayerRole::None;

	UPROPERTY(BlueprintReadOnly)
	ETargetRacer targetIndex = ETargetRacer::None;
	
	/** Ready 상태 */
	UPROPERTY(BlueprintReadOnly)
	bool bIsReady = false;
};

