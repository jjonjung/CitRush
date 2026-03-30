// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavGridExtractorTool.generated.h"

/**
 * NavMesh → 그리드 → RLE 압축 JSON (다중 연결 컴포넌트 지원)
 */
UCLASS()
class UE_CITRUSH_API UNavGridExtractorTool : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "NavGrid")
	void GenerateWalkableGrid();

	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* GenerateGridButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* DirectiveTestButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* ResultText;

	FORCEINLINE int32 DistSquared(const FIntPoint& A, const FIntPoint& B)
	{
		const int32 DX = A.X - B.X;
		const int32 DY = A.Y - B.Y;
		return DX*DX + DY*DY;
	}

	FORCEINLINE float Dist(const FIntPoint& A, const FIntPoint& B)
	{
		return FMath::Sqrt(static_cast<float>(DistSquared(A, B)));
	}

private:
	UFUNCTION()
	void OnGenerateGridButtonClicked();

	UFUNCTION()
	void OnDirectiveTestButtonClicked();

	void SendDirective();

	FTimerHandle TimerHandle_DirectiveLoop;
	int32 CurrentDirectiveCode = 1;

	// 헬퍼 함수
	bool IsWalkableAndUnvisited(const FIntPoint& Pos, TSet<FIntPoint>& Visited, const TSet<FIntPoint>& AllWalkable);
	TArray<FIntPoint> GetNeighbors(const FIntPoint& Pos, const TSet<FIntPoint>& AllWalkable);
};
