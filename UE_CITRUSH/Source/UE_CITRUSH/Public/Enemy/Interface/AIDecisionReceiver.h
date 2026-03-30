// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Private/Network/Schemas/HttpV1/HttpRequest2.h"
#include "Private/Network/Schemas/HttpV1/HttpResponse2.h"
#include "AIDecisionReceiver.generated.h"

/**
 * Enemy 게임 상태 구조체 (AI 서버 전송용)
 */
USTRUCT(BlueprintType)
struct FEnemyGameState
{
	GENERATED_BODY()

	/** Enemy 고유 ID */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FString unit_id;

	/** 현재 위치 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FVector position;

	/** 현재 체력 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float health = 100.0f;

	/** 최대 체력 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float max_health = 100.0f;

	/** 현재 타겟 ID (없으면 빈 문자열) */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FString target_id;

	/** 현재 상태 (Idle, Chase, Attack 등) */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FString current_state;

	/** 이동 속도 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float speed = 1000.0f;

	/** 포획 게이지 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float capture_gauge = 0.0f;

	/** 무적 상태 여부 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	bool is_invulnerable = false;

	/** 방향 정보 (Yaw, 도 단위) - v1.4.0 추가 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float rotation_yaw_deg = 0.0f;

	/** 전방 벡터 - v1.4.0 추가 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FVector forward_vector = FVector::ZeroVector;

	/** P-Pellet 쿨타임 - v1.4.0 추가 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	float p_pellet_cooldown = 0.0f;

	/** 마지막 P-Pellet 섭취 시각 - v1.4.0 추가 */
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FString last_pellet_consumed_at;
};

/**
 * AI 결정을 받을 수 있는 인터페이스
 * Enemy들이 이 인터페이스를 구현하여 AI Subsystem과 통신
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UAIDecisionReceiver : public UInterface
{
	GENERATED_BODY()
};

class UE_CITRUSH_API IAIDecisionReceiver
{
	GENERATED_BODY()

public:
	/**
	 * AI 서버로부터 받은 명령 처리
	 * @param Command - AI 서버에서 받은 유닛 명령
	 */
	virtual void OnAIDecisionReceived(const FUnitCommand& Command) = 0;

	/**
	 * Enemy 고유 ID 반환
	 * @return Enemy를 식별하는 고유 문자열
	 */
	virtual FString GetEnemyID() const = 0;

	/**
	 * 현재 게임 상태 반환 (AI 서버 전송용)
	 * @return 현재 Enemy의 상태 정보
	 */
	virtual FEnemyGameState GetCurrentGameState() const = 0;

	/**
	 * AI 연결 활성화 여부 확인
	 * @return AI 시스템 사용 여부
	 */
	virtual bool IsAIEnabled() const { return true; }
};
