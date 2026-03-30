// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnemyTypes.generated.h"

/**
 * 서버로부터 받는 AI 명령 타입
 */
UENUM(BlueprintType)
enum class EAICommand : uint8
{
	/** 대기 상태 */
	Idle UMETA(DisplayName = "Idle"),

	/** 특정 위치로 이동 */
	MOVE_TO_LOCATION UMETA(DisplayName = "Move To Location"),

	/** 타겟 가로채기 */
	INTERCEPT UMETA(DisplayName = "Intercept"),

	/** 안전 위치로 후퇴 */
	RETREAT UMETA(DisplayName = "Retreat")
};
