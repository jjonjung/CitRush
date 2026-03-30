// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/TestBossActor.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

ATestBossActor::ATestBossActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// NavSystemDataComponent 생성
	NavSystemDataComponent = CreateDefaultSubobject<UNavSystemDataComponent>(TEXT("NavSystemDataComponent"));

	// 기본값 설정
	UpdateInterval = 1.0f;
	bSaveToJSON = true;
	JSONSavePath = TEXT("Saved/NavSystemData/");
	TimeSinceLastUpdate = 0.0f;
}

void ATestBossActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("TestBossActor: BeginPlay - NavSystemDataComponent attached"));
}

void ATestBossActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastUpdate += DeltaTime;

	// 주기적으로 네비게이션 데이터 계산
	if (TimeSinceLastUpdate >= UpdateInterval)
	{
		TimeSinceLastUpdate = 0.0f;

		if (NavSystemDataComponent)
		{
			// 네비게이션 데이터 계산
			NavSystemDataComponent->CalculateNavigationData();

			// LLM 데이터 가져오기
			FNavSystemLLMData LLMData = NavSystemDataComponent->GetLLMData();

			// JSON 파일로 저장
			if (bSaveToJSON)
			{
				SaveToJSON(LLMData);
			}
		}
	}
}

void ATestBossActor::SaveToJSON(const FNavSystemLLMData& Data)
{
	// JSON Object 생성
	TSharedPtr<FJsonObject> JsonObject = LLMDataToJsonObject(Data);
	if (!JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("TestBossActor: Failed to create JSON object"));
		return;
	}

	// JSON 문자열로 변환 (Pretty Print)
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogTemp, Error, TEXT("TestBossActor: Failed to serialize JSON"));
		return;
	}

	// 파일 경로 생성
	FString FullPath = FPaths::ProjectDir() + JSONSavePath;
	FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	FString FileName = FString::Printf(TEXT("NavData_%s.json"), *Timestamp);
	FString FilePath = FPaths::Combine(FullPath, FileName);

	// 디렉토리 생성
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*FullPath))
	{
		PlatformFile.CreateDirectoryTree(*FullPath);
	}

	if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("TestBossActor: JSON saved to %s"), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TestBossActor: Failed to save JSON to %s"), *FilePath);
	}
}

TSharedPtr<FJsonObject> ATestBossActor::VectorToJsonObject(const FVector& Vec) const
{
	TSharedPtr<FJsonObject> VecObj = MakeShareable(new FJsonObject);
	VecObj->SetNumberField(TEXT("X"), Vec.X);
	VecObj->SetNumberField(TEXT("Y"), Vec.Y);
	VecObj->SetNumberField(TEXT("Z"), Vec.Z);
	return VecObj;
}

TSharedPtr<FJsonObject> ATestBossActor::RacerDataToJsonObject(const FRacerNavigationData& Data) const
{
	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);

	// 기본 정보
	JsonObj->SetNumberField(TEXT("PlayerIndex"), Data.PlayerIndex);
	JsonObj->SetBoolField(TEXT("IsValid"), Data.IsValid());

	// 경로 정보
	JsonObj->SetNumberField(TEXT("PathCost"), Data.PathCost);
	JsonObj->SetNumberField(TEXT("PathDistance"), Data.PathDistance);
	JsonObj->SetNumberField(TEXT("StraightDistance"), Data.StraightDistance);

	// 위치 정보
	JsonObj->SetObjectField(TEXT("PlayerWorldLocation"), VectorToJsonObject(Data.PlayerWorldLocation));

	// 각도 정보
	JsonObj->SetNumberField(TEXT("RelativeAngleToBoss"), Data.RelativeAngleToBoss);

	// Delta 정보
	JsonObj->SetObjectField(TEXT("DeltaPosition"), VectorToJsonObject(Data.DeltaPosition));
	JsonObj->SetNumberField(TEXT("DeltaStraightDistance"), Data.DeltaStraightDistance);
	JsonObj->SetNumberField(TEXT("DeltaPathDistance"), Data.DeltaPathDistance);
	JsonObj->SetNumberField(TEXT("DeltaPathCost"), Data.DeltaPathCost);
	JsonObj->SetNumberField(TEXT("MovementDirectionChange"), Data.MovementDirectionChange);
	JsonObj->SetNumberField(TEXT("AverageSpeed"), Data.AverageSpeed);
	JsonObj->SetNumberField(TEXT("RelativeBearingChange"), Data.RelativeBearingChange);

	return JsonObj;
}

TSharedPtr<FJsonObject> ATestBossActor::LLMDataToJsonObject(const FNavSystemLLMData& Data) const
{
	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);

	// 보스 정보
	TSharedPtr<FJsonObject> BossInfoObj = MakeShareable(new FJsonObject);
	BossInfoObj->SetObjectField(TEXT("Location"), VectorToJsonObject(Data.BossWorldLocation));
	JsonObj->SetObjectField(TEXT("BossInfo"), BossInfoObj);

	// 시간 정보
	TSharedPtr<FJsonObject> TimeInfoObj = MakeShareable(new FJsonObject);
	TimeInfoObj->SetNumberField(TEXT("Timestamp"), Data.Timestamp);
	TimeInfoObj->SetNumberField(TEXT("DeltaTime"), Data.DeltaTime);
	JsonObj->SetObjectField(TEXT("TimeInfo"), TimeInfoObj);

	// 레이서 데이터
	TArray<TSharedPtr<FJsonValue>> RacerArray;
	for (const FRacerNavigationData& RacerData : Data.RacerData)
	{
		TSharedPtr<FJsonObject> RacerObj = RacerDataToJsonObject(RacerData);
		RacerArray.Add(MakeShareable(new FJsonValueObject(RacerObj)));
	}
	JsonObj->SetArrayField(TEXT("RacerData"), RacerArray);

	// 전체 분산도 정보
	TSharedPtr<FJsonObject> DispersionObj = MakeShareable(new FJsonObject);	
	//DispersionObj->SetNumberField(TEXT("InterPlayerDistanceVariance"), Data.InterPlayerDistanceVariance);
	DispersionObj->SetNumberField(TEXT("DeltaInterPlayerDistance"), Data.DeltaInterPlayerDistance);
	JsonObj->SetObjectField(TEXT("DispersionInfo"), DispersionObj);

	return JsonObj;
}
