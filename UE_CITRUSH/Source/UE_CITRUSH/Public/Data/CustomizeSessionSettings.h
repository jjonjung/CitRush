#pragma once

#include "CoreMinimal.h"
#include "MapDataAsset.h"
#include "CustomizeSessionSettings.generated.h"

UENUM()
enum EContentMode : int32
{
	None = 0,
	PVE = 1 << 1,
	PVP = 1 << 2,
	Normal = 1 << 5,
	Event = 1 << 6,
	Rank = 1 << 7,
	Bronze = Rank << 1,
	Silver = Rank << 2,
	Gold = Rank << 3
};

/**
 * 세션 생성 커스터마이즈 설정 (맵, 플레이어 수, 광고 여부 등)
 */
USTRUCT()
struct UE_CITRUSH_API FCustomizeSessionSettings
{
	GENERATED_BODY()

	/** 맵 정보 */
	FMapInfo mapInfo;

	/** 세션 표시 이름 */
	FString displayName = "";

	/** 최대 플레이어 수 */
	int32 maxPlayerNum = 0;

	/** 세션 광고 여부 (검색 가능 여부) */
	bool bShouldAdvertise = true;

	
};
