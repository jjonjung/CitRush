// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/NavMeshData/NavGridExtractorTool.h"
#include "JsonUtilities.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"         // FJsonObject
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

/*
 * Run-Length Encoding (RLE) 기반 경로 압축
 * - start: 시작 그리드 좌표 [x, y]
 * - moves: [[방향, 거리], ...] 배열
 * - 방향: 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
 * - width, height: 전체 맵 크기
 */

// 8방향 벡터 (UE Y축: +Y = 남, -Y = 북)
static const FIntPoint Directions[8] = {
    FIntPoint(0, -1),   // 0: N
    FIntPoint(1, -1),   // 1: NE
    FIntPoint(1, 0),    // 2: E
    FIntPoint(1, 1),    // 3: SE
    FIntPoint(0, 1),    // 4: S
    FIntPoint(-1, 1),   // 5: SW
    FIntPoint(-1, 0),   // 6: W
    FIntPoint(-1, -1)   // 7: NW
};

static const TCHAR* DirectionNames[8] = {
    TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"),
    TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW")
};

// 8방향 중 하나 반환 (-1 = 비인접)
int32 GetDirection8(const FIntPoint& From, const FIntPoint& To)
{
    FIntPoint Delta = To - From;
    int32 DX = FMath::Clamp(Delta.X, -1, 1);
    int32 DY = FMath::Clamp(Delta.Y, -1, 1);

    for (int32 i = 0; i < 8; ++i)
    {
        if (Directions[i].X == DX && Directions[i].Y == DY)
        {
            return i;
        }
    }
    return -1;
}

void UNavGridExtractorTool::NativeConstruct()
{
    Super::NativeConstruct();
    if (GenerateGridButton)
    {
        GenerateGridButton->OnClicked.AddDynamic(this, &UNavGridExtractorTool::OnGenerateGridButtonClicked);
    }

	if (DirectiveTestButton)
	{
		DirectiveTestButton->OnClicked.AddDynamic(this, &UNavGridExtractorTool::OnDirectiveTestButtonClicked);
	}
}

void UNavGridExtractorTool::OnGenerateGridButtonClicked()
{
    GenerateWalkableGrid();
}

void UNavGridExtractorTool::OnDirectiveTestButtonClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (TimerHandle_DirectiveLoop.IsValid())
		{
			World->GetTimerManager().ClearTimer(TimerHandle_DirectiveLoop);
		}

		CurrentDirectiveCode = 1;
		
		// 3초마다 SendDirective 호출
		World->GetTimerManager().SetTimer(
			TimerHandle_DirectiveLoop,
			this,
			&UNavGridExtractorTool::SendDirective,
			3.0f,
			true,
			0.0f // 즉시 첫 실행
		);

		if (ResultText)
		{
			ResultText->SetText(FText::FromString(TEXT("Directive Test Started (1 ~ 11, Every 5s)")));
		}
	}
}

void UNavGridExtractorTool::SendDirective()
{
	// 11번까지만 전송
	if (CurrentDirectiveCode > 12)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TimerHandle_DirectiveLoop);
		}
		
		if (ResultText)
		{
			ResultText->SetText(FText::FromString(TEXT("Directive Test Completed")));
		}
		
		CurrentDirectiveCode = 1; // 리셋
		return;
	}

	// JSON 생성
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetNumberField(TEXT("directive_code"), CurrentDirectiveCode);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// HTTP 요청 생성
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("http://34.22.65.238:8000/api/v1/directive_test"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	int32 CapturedCode = CurrentDirectiveCode; // 캡처용 복사

	Request->OnProcessRequestComplete().BindLambda(
		[this, CapturedCode](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (bWasSuccessful && Response.IsValid())
			{
				FString ResponseStr = Response->GetContentAsString();
				UE_LOG(LogTemp, Log, TEXT("[DirectiveTest] Sent: %d, Response: %s"), CapturedCode, *ResponseStr);
				
				if (ResultText)
				{
					FString StatusMsg = FString::Printf(TEXT("Sent Code: %d\nResponse: %s"), CapturedCode, *ResponseStr);
					ResultText->SetText(FText::FromString(StatusMsg));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[DirectiveTest] Failed to send: %d"), CapturedCode);
				if (ResultText)
				{
					ResultText->SetText(FText::FromString(FString::Printf(TEXT("Failed to send code: %d"), CapturedCode)));
				}
			}
		});

	Request->ProcessRequest();
	UE_LOG(LogTemp, Log, TEXT("[DirectiveTest] Sending code: %d"), CurrentDirectiveCode);

	// 다음 코드로 증가
	CurrentDirectiveCode++;
}

void UNavGridExtractorTool::GenerateWalkableGrid()
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

    TSet<FIntPoint> WalkableSet;
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
                WalkableSet.Add(FIntPoint(X, Y));
            }
        }
    }

    if (WalkableSet.Num() == 0)
    {
        if (ResultText) ResultText->SetText(FText::FromString("No walkable cells found"));
        return;
    }

    // DFS로 연결된 컴포넌트별 경로 생성 + RLE 압축
    TArray<TSharedPtr<FJsonObject>> PathObjects;
    TSet<FIntPoint> Visited;

    for (const FIntPoint& Seed : WalkableSet)
    {
        if (Visited.Contains(Seed)) continue;

        // DFS: 연결된 모든 셀 순회하며 경로 리스트 생성
        TArray<FIntPoint> ComponentPath;
        TArray<FIntPoint> Stack;
        Stack.Push(Seed);
        Visited.Add(Seed);
        ComponentPath.Add(Seed);

        while (!Stack.IsEmpty())
        {
            FIntPoint Curr = Stack.Pop();
            TArray<FIntPoint> Neighbors = GetNeighbors(Curr, WalkableSet);
            for (FIntPoint& Neigh : Neighbors)
            {
                if (!Visited.Contains(Neigh))
                {
                    Visited.Add(Neigh);
                    Stack.Push(Neigh);
                    ComponentPath.Add(Neigh);
                }
            }
        }

        if (ComponentPath.Num() < 2) continue; // 1셀 무시

        // RLE 압축
        TArray<TPair<int32, int32>> CurrentMoves; // [Dir, Count]
        FIntPoint StartPos = ComponentPath[0];
        FIntPoint CurrPos = StartPos;
        int32 CurrDir = -1;
        int32 CurrCount = 0;

        for (int32 i = 1; i < ComponentPath.Num(); ++i)
        {
            FIntPoint Next = ComponentPath[i];
            float DistVal = Dist(CurrPos, Next);
            int32 Dir = GetDirection8(CurrPos, Next);

            if (Dir == -1 || DistVal > 1.5f)
            {
                // DFS라 거의 안 나오지만 안전장치
                CurrPos = Next;
                continue;
            }

            if (Dir == CurrDir && CurrCount < 255)
            {
                CurrCount++;
            }
            else
            {
                if (CurrDir != -1 && CurrCount > 0)
                {
                    CurrentMoves.Add(TPair<int32, int32>(CurrDir, CurrCount));
                }
                CurrDir = Dir;
                CurrCount = 1;
            }
            CurrPos = Next;
        }

        if (CurrDir != -1 && CurrCount > 0)
        {
            CurrentMoves.Add(TPair<int32, int32>(CurrDir, CurrCount));
        }

        if (CurrentMoves.Num() > 0)
        {
            TSharedPtr<FJsonObject> PathObj = MakeShareable(new FJsonObject);

            TArray<TSharedPtr<FJsonValue>> StartArr;
            StartArr.Add(MakeShareable(new FJsonValueNumber(StartPos.X)));
            StartArr.Add(MakeShareable(new FJsonValueNumber(StartPos.Y)));

            PathObj->SetArrayField(TEXT("start"), StartArr);

            TArray<TSharedPtr<FJsonValue>> MovesArr;
            for (const auto& Move : CurrentMoves)
            {
                TArray<TSharedPtr<FJsonValue>> Entry;
                Entry.Add(MakeShareable(new FJsonValueString(DirectionNames[Move.Key])));
                Entry.Add(MakeShareable(new FJsonValueNumber(Move.Value)));
                MovesArr.Add(MakeShareable(new FJsonValueArray(Entry)));
            }
            PathObj->SetArrayField(TEXT("moves"), MovesArr);
            PathObj->SetNumberField(TEXT("length"), ComponentPath.Num());

            PathObjects.Add(PathObj);
        }
    }

    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> PathsArr;
    for (const TSharedPtr<FJsonObject>& PathObj : PathObjects)
    {
        PathsArr.Add(MakeShareable(new FJsonValueObject(PathObj)));
    }
    RootObject->SetArrayField(TEXT("paths"), PathsArr);
    RootObject->SetNumberField(TEXT("width"), GridWidth);
    RootObject->SetNumberField(TEXT("height"), GridHeight);
    RootObject->SetNumberField(TEXT("cell_size"), CellSize);
    RootObject->SetNumberField(TEXT("total_walkable"), WalkableSet.Num());
    RootObject->SetNumberField(TEXT("compressed_paths"), PathObjects.Num());

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    int32 TotalPathLength = 0;
    for (const TSharedPtr<FJsonObject>& PathObj : PathObjects)
    {
        TotalPathLength += PathObj->GetNumberField(TEXT("length"));
    }
    float CompressionRatio = (WalkableSet.Num() > 0) ? 
        (static_cast<float>(TotalPathLength - PathObjects.Num()) / WalkableSet.Num() * 100.0f) : 0.0f;

    FString ResultMessage = FString::Printf(
        TEXT("NavMesh RLE Compressed\nGrid: %dx%d\nWalkable: %d cells\nPaths: %d\nCompression: %.1f%%\nSaved to: Saved/NavGrid_RLE.json"),
        GridWidth, GridHeight, WalkableSet.Num(), PathObjects.Num(), CompressionRatio
    );

    if (ResultText)
    {
        ResultText->SetText(FText::FromString(ResultMessage));
    }

    UE_LOG(LogTemp, Display, TEXT("%s"), *ResultMessage);

    FString SavePath = FPaths::ProjectSavedDir() / TEXT("NavGrid_RLE.json");
    if (FFileHelper::SaveStringToFile(JsonString, *SavePath))
    {
        UE_LOG(LogTemp, Log, TEXT("Saved: %s"), *SavePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Save failed: %s"), *SavePath);
    }
}

bool UNavGridExtractorTool::IsWalkableAndUnvisited(const FIntPoint& Pos, TSet<FIntPoint>& Visited, const TSet<FIntPoint>& AllWalkable)
{
    return AllWalkable.Contains(Pos) && !Visited.Contains(Pos);
}

TArray<FIntPoint> UNavGridExtractorTool::GetNeighbors(const FIntPoint& Pos, const TSet<FIntPoint>& AllWalkable)
{
    TArray<FIntPoint> Neighbors;
    for (int32 d = 0; d < 8; ++d)
    {
        FIntPoint Neigh = Pos + Directions[d];
        if (AllWalkable.Contains(Neigh))
        {
            Neighbors.Add(Neigh);
        }
    }
    return Neighbors;
}