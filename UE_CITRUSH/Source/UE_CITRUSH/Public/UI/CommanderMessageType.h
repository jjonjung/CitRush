// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CommanderMessageType.generated.h"

/**
 * Commander 메시지 타입
 */
UENUM(BlueprintType)
enum class ECommanderMessageType : uint8
{
	Info		UMETA(DisplayName = "정보"),
	Warning		UMETA(DisplayName = "경고"),
	Error		UMETA(DisplayName = "오류"),
	Success		UMETA(DisplayName = "성공")
};

/**
 * Commander 메시지 ID (메시지 일괄 관리용)
 */
UENUM(BlueprintType)
enum class ECommanderMessageID : uint8
{
	None = 0,
	
	// 자판기 관련
	VendingMachine_Busy					UMETA(DisplayName = "자판기 작동 중"),
	VendingMachine_InsufficientCoin		UMETA(DisplayName = "코인 부족"),
	
	// 아이템 관련
	Item_NotHolding						UMETA(DisplayName = "아이템 없음"),
	Item_SupplySuccess					UMETA(DisplayName = "아이템 부여 성공"),
	Item_SupplyFailed					UMETA(DisplayName = "아이템 부여 실패"),
	
	// 레이서 선택 관련
	RacerSelection_NeedSelectFirst		UMETA(DisplayName = "레이서 먼저 선택 필요"),
	RacerSelection_CannotSelect			UMETA(DisplayName = "레이서 선택 불가"),
	RacerSelection_HoldingItem			UMETA(DisplayName = "아이템 들고 있어서 선택 불가"),
	RacerSelection_NotConnected			UMETA(DisplayName = "레이서 연결되지 않음"),
};

/**
 * 메시지 정보 구조체
 */
USTRUCT(BlueprintType)
struct FCommanderMessageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	FText MessageText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	ECommanderMessageType MessageType = ECommanderMessageType::Info;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	float Duration = 2.0f;

	FCommanderMessageInfo()
		: MessageText(FText::GetEmpty())
		, MessageType(ECommanderMessageType::Info)
		, Duration(2.0f)
	{}

	FCommanderMessageInfo(const FText& InText, ECommanderMessageType InType, float InDuration)
		: MessageText(InText)
		, MessageType(InType)
		, Duration(InDuration)
	{}
};

