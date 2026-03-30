// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Network/NavSystemDataComponent.h"
#include "TestBossActor.generated.h"

/**
 * NavSystemDataComponent 테스트용 보스 액터
 *
 * 기능:
 * - NavSystemDataComponent를 포함
 * - 주기적으로 네비게이션 데이터 계산
 * - 화면에 모든 구조체 정보 표시
 */
UCLASS()
class UE_CITRUSH_API ATestBossActor : public AActor
{
	GENERATED_BODY()

public:
	ATestBossActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ============================================================
	// 컴포넌트
	// ============================================================

	/**
	 * 네비게이션 시스템 데이터 컴포넌트
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNavSystemDataComponent* NavSystemDataComponent;

	// ============================================================
	// 설정
	// ============================================================

	/**
	 * 네비게이션 데이터 계산 주기 (초)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavSystem|Debug", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float UpdateInterval;

	/**
	 * JSON 파일로 저장 활성화
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavSystem|Output")
	bool bSaveToJSON;

	/**
	 * JSON 파일 저장 경로 (프로젝트 기준 상대 경로)
	 * 기본값: Saved/NavSystemData/
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NavSystem|Output")
	FString JSONSavePath;

private:
	/**
	 * 마지막 업데이트 이후 경과 시간
	 */
	float TimeSinceLastUpdate;

	/**
	 * JSON 파일로 네비게이션 데이터 저장
	 */
	void SaveToJSON(const FNavSystemLLMData& Data);

	/**
	 * FRacerNavigationData를 JSON Object로 변환
	 */
	TSharedPtr<FJsonObject> RacerDataToJsonObject(const FRacerNavigationData& Data) const;

	/**
	 * FNavSystemLLMData를 JSON Object로 변환
	 */
	TSharedPtr<FJsonObject> LLMDataToJsonObject(const FNavSystemLLMData& Data) const;

	/**
	 * FVector를 JSON Object로 변환
	 */
	TSharedPtr<FJsonObject> VectorToJsonObject(const FVector& Vec) const;
};
