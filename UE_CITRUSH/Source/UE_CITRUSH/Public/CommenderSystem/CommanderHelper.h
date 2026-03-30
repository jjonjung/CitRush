// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CommanderHelper.generated.h"

class ACommenderCharacter;
class UWorld;

/**
 * Commander 찾기 헬퍼 클래스 (최적화된 공통 로직)
 */
UCLASS()
class UE_CITRUSH_API UCommanderHelper : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 월드에서 Commander를 효율적으로 찾기
	 * @param World 월드 참조
	 * @param bRequireHUDWidget HUD 위젯이 필수인지 여부
	 * @return 찾은 Commander, 없으면 nullptr
	 */
	static ACommenderCharacter* FindCommander(UWorld* World, bool bRequireHUDWidget = false);
};

