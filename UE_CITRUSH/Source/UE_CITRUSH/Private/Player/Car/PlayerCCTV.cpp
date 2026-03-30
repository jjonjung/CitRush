// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Car/PlayerCCTV.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Enemy/PixelEnemy.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_CLASS(APlayerCCTV, playerCCTV)

APlayerCCTV::APlayerCCTV()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	/*CCTVWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("CCTVWidgetComponent"));
	CCTVWidgetComponent->SetupAttachment(RootComponent);
	CCTVWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	CCTVWidgetComponent->SetDrawSize(FVector2D(1920.0f, 1080.0f));*/
}

void APlayerCCTV::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerCCTV] World is null!"));
		return;
	}

	TArray<AActor*> FoundPawns;
	UGameplayStatics::GetAllActorsOfClass(World, APawn::StaticClass(), FoundPawns);

	// Player 0, 1, 2 순서대로 저장하기 위한 임시 배열
	TArray<UTextureRenderTarget2D*> PlayerRenderTargets;
	PlayerRenderTargets.SetNum(3);

	for (AActor* Actor : FoundPawns)
	{
		APawn* Pawn = Cast<APawn>(Actor);
		if (!Pawn) continue;

		EAutoReceiveInput::Type AutoPossess = Pawn->AutoPossessPlayer;
		int32 PlayerIndex = (int32)AutoPossess - 1;

		if (PlayerIndex >= 0 && PlayerIndex < 3)
		{
			USceneCaptureComponent2D* SceneCapture = Pawn->FindComponentByClass<USceneCaptureComponent2D>();
			if (SceneCapture && SceneCapture->TextureTarget)
			{
				PlayerRenderTargets[PlayerIndex] = SceneCapture->TextureTarget;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[PlayerCCTV] Player%d에 SceneCaptureComponent2D 또는 RenderTarget이 없습니다"),
					PlayerIndex);
			}
		}
	}

	for (int32 i = 0; i < 3; ++i)
	{
		if (PlayerRenderTargets[i])
		{
			CCTVRenderTargets.Add(PlayerRenderTargets[i]);
		}
	}

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, APixelEnemy::StaticClass(), FoundEnemies);

	// 모든 PixelEnemy의 RenderTarget 추가 (RenderTarget만 직접 접근)
	for (AActor* Actor : FoundEnemies)
	{
		APixelEnemy* Enemy = Cast<APixelEnemy>(Actor);
		if (Enemy && Enemy->CCTVRenderTarget)
		{
			CCTVRenderTargets.Add(Enemy->CCTVRenderTarget);
			UE_LOG(LogTemp, Log, TEXT("[PlayerCCTV] PixelEnemy RenderTarget '%s' 감지 (총 %d개)"),
				*Enemy->CCTVRenderTarget->GetName(), CCTVRenderTargets.Num());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerCCTV] PixelEnemy '%s'에 RenderTarget이 없습니다"),
				*Enemy->GetName());
		}
	}

	if (CCTVRenderTargets.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerCCTV] CCTV RenderTarget을 하나도 찾을 수 없습니다!"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PlayerCCTV] 총 %d개의 CCTV RenderTarget 감지 완료"), CCTVRenderTargets.Num());

	CurrentCCTVIndex = 0;
	UpdateCCTVDisplay();
}

void APlayerCCTV::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCCTV::SwitchToNextCCTV()
{
	if (CCTVRenderTargets.Num() == 0) return;

	CurrentCCTVIndex = (CurrentCCTVIndex + 1) % CCTVRenderTargets.Num();
	UpdateCCTVDisplay();

	UE_LOG(LogTemp, Log, TEXT("[PlayerCCTV] CCTV 전환 -> 다음 (Index: %d)"), CurrentCCTVIndex);
}

void APlayerCCTV::SwitchToPreviousCCTV()
{
	if (CCTVRenderTargets.Num() == 0) return;

	CurrentCCTVIndex = (CurrentCCTVIndex - 1 + CCTVRenderTargets.Num()) % CCTVRenderTargets.Num();
	UpdateCCTVDisplay();

	UE_LOG(LogTemp, Log, TEXT("[PlayerCCTV] CCTV 전환 -> 이전 (Index: %d)"), CurrentCCTVIndex);
}

UTextureRenderTarget2D* APlayerCCTV::GetCurrentCCTVRenderTarget() const
{
	if (CCTVRenderTargets.IsValidIndex(CurrentCCTVIndex))
	{
		return CCTVRenderTargets[CurrentCCTVIndex];
	}
	return nullptr;
}

void APlayerCCTV::UpdateCCTVDisplay()
{
	UTextureRenderTarget2D* CurrentRenderTarget = GetCurrentCCTVRenderTarget();
	if (!CurrentRenderTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerCCTV] UpdateCCTVDisplay: 유효한 RenderTarget이 없습니다"));
		return;
	}

	// 현재 RenderTarget을 사용하는 SceneCaptureComponent 찾아서 강제 캡처
	// 다음 프레임에 실행하여 초기화가 완전히 완료된 후 캡처
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimerForNextTick([this, CurrentRenderTarget, World]()
		{
			// 모든 Pawn에서 SceneCaptureComponent 찾기
			TArray<AActor*> FoundPawns;
			UGameplayStatics::GetAllActorsOfClass(World, APawn::StaticClass(), FoundPawns);
			
			for (AActor* Actor : FoundPawns)
			{
				APawn* Pawn = Cast<APawn>(Actor);
				if (!Pawn) continue;
				
				USceneCaptureComponent2D* SceneCapture = Pawn->FindComponentByClass<USceneCaptureComponent2D>();
				if (SceneCapture && SceneCapture->TextureTarget == CurrentRenderTarget)
				{
					// 첫 프레임 강제 캡처
					SceneCapture->CaptureScene();
					UE_LOG(LogTemp, Log, TEXT("[PlayerCCTV] SceneCapture 강제 캡처 실행: %s"), *Pawn->GetName());
					return;
				}
			}
			
			// PixelEnemy에서도 찾기 (RenderTarget만 사용)
			TArray<AActor*> FoundEnemies;
			UGameplayStatics::GetAllActorsOfClass(World, APixelEnemy::StaticClass(), FoundEnemies);
			
			for (AActor* Actor : FoundEnemies)
			{
				APixelEnemy* Enemy = Cast<APixelEnemy>(Actor);
				if (Enemy && Enemy->CCTVRenderTarget == CurrentRenderTarget)
				{
					// 첫 프레임 강제 캡처 (RenderTarget만 사용)
					Enemy->CaptureCCTVScene();
					UE_LOG(LogTemp, Log, TEXT("[PlayerCCTV] PixelEnemy SceneCapture 강제 캡처 실행: %s"), *Enemy->GetName());
					return;
				}
			}
		});
	}

	if (CCTVDisplayActor)
	{
		TArray<UStaticMeshComponent*> MeshComponents;
		CCTVDisplayActor->GetComponents<UStaticMeshComponent>(MeshComponents);

		if (MeshComponents.Num() > 0)
		{
			UStaticMeshComponent* PlaneMesh = MeshComponents[0];
			if (PlaneMesh)
			{
				UMaterialInstanceDynamic* DynamicMaterial = PlaneMesh->CreateDynamicMaterialInstance(0);
				if (DynamicMaterial)
				{
					DynamicMaterial->SetTextureParameterValue(TextureParameterName, CurrentRenderTarget);
					UE_LOG(LogTemp, Log, TEXT("[PlayerCCTV] CCTVDisplayActor Material 업데이트: %s"),
						*CurrentRenderTarget->GetName());
				}
			}
		}
	}

	// Widget에서도 CurrentRenderTarget을 사용할 수 있도록 업데이트
	// Widget Blueprint에서 GetCurrentCCTVRenderTarget() 함수를 호출하여 사용
}

