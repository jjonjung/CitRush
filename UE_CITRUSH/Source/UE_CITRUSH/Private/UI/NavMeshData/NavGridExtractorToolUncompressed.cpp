// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NavMeshData/NavGridExtractorToolUncompressed.h"
#include "JsonUtilities.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

/*
- grid_x, grid_y: 그리드 상의 좌표 (0부터 시작하는 인덱스)
- world_x, world_y: 언리얼 엔진의 실제 월드 좌표 (cm 단위)
- 계산: world_x = grid_x * 100 + 50 (셀 중심점)
	 예: 210 * 100 + 50 = 21,050 cm
- world_z: NavMesh에서 ProjectPointToNavigation()으로 찾은 실제 높이값 (cm)
- walkable: 해당 셀이 걸어갈 수 있는지 여부 (항상 true, walkable 셀만 저장)
*/

void UNavGridExtractorToolUncompressed::NativeConstruct()
{
	Super::NativeConstruct();
	if (GenerateGridButton)
	{
		GenerateGridButton->OnClicked.AddDynamic(this, &UNavGridExtractorToolUncompressed::OnGenerateGridButtonClicked);
	}
}

void UNavGridExtractorToolUncompressed::OnGenerateGridButtonClicked()
{
	GenerateWalkableGrid();
}

void UNavGridExtractorToolUncompressed::GenerateWalkableGrid()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys) {
		if (ResultText) ResultText->SetText(FText::FromString("NavigationSystem not found"));
		return;
	}

	const ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavSys->GetMainNavData());
	if (!NavMesh) {
		if (ResultText) ResultText->SetText(FText::FromString("No RecastNavMesh"));
		return;
	}

	FBox Bounds = NavMesh->GetBounds();
	const float CellSize = 100.0f; // 그리드 셀 크기 (cm)

	int32 GridWidth = FMath::CeilToInt(Bounds.GetSize().X / CellSize);
	int32 GridHeight = FMath::CeilToInt(Bounds.GetSize().Y / CellSize);
	int32 TotalCells = GridWidth * GridHeight;

	TSharedPtr<FJsonObject> BoundsObject = MakeShareable(new FJsonObject);
	BoundsObject->SetNumberField(TEXT("min_x"), Bounds.Min.X);
	BoundsObject->SetNumberField(TEXT("min_y"), Bounds.Min.Y);
	BoundsObject->SetNumberField(TEXT("min_z"), Bounds.Min.Z);
	BoundsObject->SetNumberField(TEXT("max_x"), Bounds.Max.X);
	BoundsObject->SetNumberField(TEXT("max_y"), Bounds.Max.Y);
	BoundsObject->SetNumberField(TEXT("max_z"), Bounds.Max.Z);

	TArray<TSharedPtr<FJsonValue>> WalkableCellsArray;
	int32 WalkableCount = 0;

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			FVector TestPoint = Bounds.Min + FVector(
				X * CellSize + CellSize / 2.0f,
				Y * CellSize + CellSize / 2.0f,
				50.0f
			);

			FNavLocation NavLoc;
			bool bWalkable = NavSys->ProjectPointToNavigation(
				TestPoint, NavLoc, FVector(0, 0, 100.0f)
			);

			if (bWalkable)
			{
				TSharedPtr<FJsonObject> CellObject = MakeShareable(new FJsonObject);
				CellObject->SetNumberField(TEXT("grid_x"), X);
				CellObject->SetNumberField(TEXT("grid_y"), Y);
				CellObject->SetNumberField(TEXT("world_x"), X * CellSize + CellSize / 2.0f);
				CellObject->SetNumberField(TEXT("world_y"), Y * CellSize + CellSize / 2.0f);
				CellObject->SetNumberField(TEXT("world_z"), NavLoc.Location.Z);
				CellObject->SetBoolField(TEXT("walkable"), true);

				WalkableCellsArray.Add(MakeShareable(new FJsonValueObject(CellObject)));
				WalkableCount++;
			}
		}
	}

	if (WalkableCount == 0)
	{
		if (ResultText) ResultText->SetText(FText::FromString("No walkable cells found"));
		return;
	}

	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetNumberField(TEXT("cell_size"), CellSize);
	RootObject->SetNumberField(TEXT("grid_width"), GridWidth);
	RootObject->SetNumberField(TEXT("grid_height"), GridHeight);
	RootObject->SetNumberField(TEXT("total_cells"), TotalCells);
	RootObject->SetNumberField(TEXT("walkable_cells"), WalkableCount);
	RootObject->SetObjectField(TEXT("bounds"), BoundsObject);
	RootObject->SetArrayField(TEXT("walkable_cells_data"), WalkableCellsArray);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	FString ResultMessage = FString::Printf(
		TEXT("NavMesh Uncompressed Export\\nGrid: %dx%d\\nTotal Cells: %d\\nWalkable: %d cells\\nSaved to: Saved/NavGrid.json"),
		GridWidth, GridHeight, TotalCells, WalkableCount
	);

	if (ResultText)
	{
		ResultText->SetText(FText::FromString(ResultMessage));
	}

	UE_LOG(LogTemp, Display, TEXT("%s"), *ResultMessage);

	FString SavePath = FPaths::ProjectSavedDir() / TEXT("NavGrid.json");
	if (FFileHelper::SaveStringToFile(JsonString, *SavePath))
	{
		UE_LOG(LogTemp, Log, TEXT("Saved: %s"), *SavePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Save failed: %s"), *SavePath);
	}
}
