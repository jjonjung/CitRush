// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavGridExtractorToolUncompressed.generated.h"

/**
 * NavMesh → 비압축 그리드 JSON (개별 셀 저장)
 */
UCLASS()
class UE_CITRUSH_API UNavGridExtractorToolUncompressed : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "NavGrid")
	void GenerateWalkableGrid();

	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* GenerateGridButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* ResultText;

private:
	UFUNCTION()
	void OnGenerateGridButtonClicked();
};
