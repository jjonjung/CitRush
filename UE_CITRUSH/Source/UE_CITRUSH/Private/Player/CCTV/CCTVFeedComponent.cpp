// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CCTV/CCTVFeedComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AbstractRacer.h"
#include "Player/CCTV/CCTVCameraComponent.h"
#include "Enemy/PixelEnemy.h"
#include "GameFlow/CitRushGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"
#include "Player/Car/VehicleDemoCejCar.h"

DEFINE_LOG_CATEGORY(LogCCTVFeed);

// CCTV 로그 매크로 (모든 CCTV 관련 로그를 [CCTVLog] 태그로 통일)
#define CCTV_LOG(Verbosity, Format, ...) UE_LOG(LogCCTVFeed, Verbosity, TEXT("[CCTVLog] " Format), ##__VA_ARGS__)

UCCTVFeedComponent::UCCTVFeedComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	
	FeedSlots.SetNum(4);
	FocusIndex = 0;
	bExpanded = false;
	bUsePerformanceOptimization = false;
	LowFrequencyUpdateInterval = 0.1f;
}

void UCCTVFeedComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Match Start 타이밍 캐치
	if (ACitRushGameState* cGS = GetWorld()->GetGameState<ACitRushGameState>())
	{
		// 이미 Match가 시작되었는지 확인 (GetElapsedTime() > 0)
		// GetElapsedTime()이 0보다 크면 이미 Match가 시작된 것으로 간주
		float ElapsedTime = cGS->GetElapsedTime();
		if (ElapsedTime > 0.0f)
		{
			// 이미 Match가 시작되었으면 즉시 초기화
			CCTV_LOG(Log, "Match가 이미 시작되었습니다 (ElapsedTime: %.2f). 즉시 Feed 초기화", ElapsedTime);
			OnMatchStarted();
		}
		
		// Match가 아직 시작되지 않았거나, 나중에 Match가 시작될 경우를 대비해 항상 이벤트 바인딩
		// (이미 시작된 경우에도 바인딩해도 문제없음 - 중복 초기화는 bFeedsInitialized로 방지)
		cGS->OnMatchStarted.AddDynamic(this, &UCCTVFeedComponent::OnMatchStarted);
		CCTV_LOG(Log, "OnMatchStarted 델리게이트 바인딩 완료");
	}
	else
	{
		CCTV_LOG(Warning, "CitRushGameState를 찾을 수 없습니다. Feed 초기화는 외부에서 명시적으로 호출해야 합니다.");
	}
}

void UCCTVFeedComponent::InitializeFeeds()
{
	// 중복 초기화 방지
	if (bFeedsInitialized)
	{
		CCTV_LOG(Warning, "InitializeFeeds가 이미 호출되었습니다. 중복 호출을 무시합니다.");
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogCCTVFeed, Error, TEXT("[CCTVFeed] World is null!"));
		return;
	}

	// Feed 슬롯 초기화
	FeedSlots.Empty();
	FeedSlots.SetNum(4);

	// 레이서 3명 찾기 (멀티플레이 지원: GameState를 통해 찾기)
	TArray<APawn*> SortedRacers;
	SortedRacers.SetNum(3);

	// GameState의 PlayerArray를 통해 레이서 찾기 (멀티플레이에서도 작동)
	// 이미 할당된 레이서를 추적하여 중복 할당 방지
	TSet<APawn*> AssignedRacers;
	
	if (AGameStateBase* GameStateBase = World->GetGameState())
	{
		// PlayerArray를 순회하며 Racer 역할의 PlayerState 찾기
		int32 RacerCount = 0;
		for (APlayerState* PS : GameStateBase->PlayerArray)
		{
			if (!PS) continue;
			
			ACitRushPlayerState* CitRushPS = Cast<ACitRushPlayerState>(PS);
			if (!CitRushPS) continue;
			
			// Racer 역할만 처리
			if (CitRushPS->GetPlayerRole() != EPlayerRole::Racer) continue;
			
			RacerCount++;
			
			APawn* RacerPawn = CitRushPS->GetPawn();
			if (!RacerPawn)
			{
				// GetPawn()이 null이면 Controller를 통해 찾기 시도
				if (APlayerController* PC = CitRushPS->GetPlayerController())
				{
					RacerPawn = PC->GetPawn();
				}
			}
			
			if (!RacerPawn)
			{
				CCTV_LOG(Warning, "레이서 PlayerState는 있지만 Pawn을 찾을 수 없습니다 - PlayerName=%s", 
					*CitRushPS->GetPlayerInfo().playerName);
				continue;
			}
			
			AAbstractRacer* Racer = Cast<AAbstractRacer>(RacerPawn);
			if (!Racer) continue;
			
			// 이미 다른 슬롯에 할당된 레이서는 건너뛰기
			if (AssignedRacers.Contains(RacerPawn))
			{
				CCTV_LOG(Warning, "레이서가 이미 할당되어 있습니다 - PlayerName=%s, Pawn=%s", 
					*CitRushPS->GetPlayerInfo().playerName, *RacerPawn->GetName());
				continue;
			}
			
			// PlayerInfo의 targetIndex를 사용하여 정렬 (ETargetRacer를 int32로 변환)
			ETargetRacer TargetRacer = CitRushPS->GetPlayerInfo().targetIndex;
			int32 TargetIndex = -1;
			if (TargetRacer == ETargetRacer::Racer1) TargetIndex = 0;
			else if (TargetRacer == ETargetRacer::Racer2) TargetIndex = 1;
			else if (TargetRacer == ETargetRacer::Racer3) TargetIndex = 2;
			
			if (TargetIndex >= 0 && TargetIndex < 3)
			{
				// 해당 슬롯이 이미 할당되어 있으면 건너뛰기
				if (SortedRacers[TargetIndex] && SortedRacers[TargetIndex] != RacerPawn)
				{
					CCTV_LOG(Warning, "FeedIndex %d가 이미 다른 레이서에 할당되어 있습니다 - 기존=%s, 새=%s", 
						TargetIndex, 
						*GetNameSafe(SortedRacers[TargetIndex]), 
						*RacerPawn->GetName());
					continue;
				}
				
				SortedRacers[TargetIndex] = RacerPawn;
				AssignedRacers.Add(RacerPawn);
				CCTV_LOG(Log, "레이서 할당 완료 - FeedIndex=%d, targetIndex=%d, PlayerName=%s", 
					TargetIndex, (int32)TargetRacer, *CitRushPS->GetPlayerInfo().playerName);
			}
			else
			{
				CCTV_LOG(Warning, "레이서의 targetIndex가 유효하지 않습니다 - targetIndex=%d, PlayerName=%s", 
					(int32)TargetRacer, *CitRushPS->GetPlayerInfo().playerName);
			}
		}
		
		CCTV_LOG(Log, "PlayerArray에서 Racer 역할의 PlayerState 발견: %d명, 할당된 레이서: %d명", 
			RacerCount, AssignedRacers.Num());
	}
	else
	{
		CCTV_LOG(Warning, "GameState를 찾을 수 없습니다. Fallback 방식으로 레이서를 찾습니다.");
	}
	
	// Fallback: 레이서가 할당되지 않았으면 기존 방식 사용 (AutoPossessPlayer 기반)
	bool bHasAnyRacer = false;
	for (int32 i = 0; i < 3; ++i)
	{
		if (SortedRacers[i])
		{
			bHasAnyRacer = true;
			break;
		}
	}
	
	if (!bHasAnyRacer)
	{
		CCTV_LOG(Log, "GameState에서 레이서를 찾지 못했습니다. AutoPossessPlayer 기반으로 레이서를 찾습니다.");
		TArray<AActor*> FoundRacers;
		UGameplayStatics::GetAllActorsOfClass(World, AAbstractRacer::StaticClass(), FoundRacers);

		CCTV_LOG(Log, "발견된 레이서 수: %d", FoundRacers.Num());

		for (AActor* Actor : FoundRacers)
		{
			APawn* Pawn = Cast<APawn>(Actor);
			if (!Pawn) continue;
			
			// 이미 할당된 레이서는 건너뛰기
			if (AssignedRacers.Contains(Pawn))
			{
				CCTV_LOG(Warning, "레이서가 이미 할당되어 있습니다 (Fallback) - Pawn=%s", *Pawn->GetName());
				continue;
			}

			EAutoReceiveInput::Type AutoPossess = Pawn->AutoPossessPlayer;
			int32 PlayerIndex = (int32)AutoPossess - 1;

			CCTV_LOG(Log, "레이서 발견 - Pawn=%s, AutoPossess=%d, PlayerIndex=%d", 
				*Pawn->GetName(), (int32)AutoPossess, PlayerIndex);

			if (PlayerIndex >= 0 && PlayerIndex < 3)
			{
				if (!SortedRacers[PlayerIndex])
				{
					SortedRacers[PlayerIndex] = Pawn;
					AssignedRacers.Add(Pawn);
					CCTV_LOG(Log, "레이서 Fallback 할당 완료 - FeedIndex=%d, Pawn=%s", PlayerIndex, *Pawn->GetName());
				}
				else
				{
					CCTV_LOG(Warning, "레이서 FeedIndex %d가 이미 할당되어 있습니다 - 기존=%s, 새=%s", 
						PlayerIndex, *SortedRacers[PlayerIndex]->GetName(), *Pawn->GetName());
				}
			}
		}
	}

	// 레이서 3명 Feed 생성 (레이서가 없어도 슬롯은 생성)
	// PixelEnemy와 동일한 방식: 기존 SceneCaptureComponent 사용 (Blueprint 설정 유지)
	for (int32 i = 0; i < 3; ++i)
	{
		FCCTVFeedSlot& Slot = FeedSlots[i];
		
		if (SortedRacers[i])
		{
			AAbstractRacer* Racer = Cast<AAbstractRacer>(SortedRacers[i]);
			if (!Racer)
			{
				CCTV_LOG(Warning, "레이서 %d가 AAbstractRacer가 아닙니다", i);
				Slot.RenderTarget = nullptr;
				Slot.SceneCapture = nullptr;
				Slot.TargetPawn = nullptr;
				Slot.CurrentCameraSlot = 0;
				continue;
			}
			
			// 레이서의 기존 SceneCaptureComponent 사용 (Blueprint 설정 완전 유지)
			USceneCaptureComponent2D* ExistingSceneCapture = Racer->GetCCTVSceneCaptureComponent();
			
			if (ExistingSceneCapture && ExistingSceneCapture->TextureTarget)
			{
				// 레이서의 기존 SceneCaptureComponent와 RenderTarget 사용
				Slot.SceneCapture = ExistingSceneCapture;
				Slot.RenderTarget = ExistingSceneCapture->TextureTarget;
				
				CCTV_LOG(Log, "레이서 %d의 기존 SceneCaptureComponent 사용 - Racer: %s, RT: %s", 
					i, *Racer->GetName(), *Slot.RenderTarget->GetName());
			}
			else
			{
				// 기존 SceneCaptureComponent가 없으면 새로 생성
				Slot.RenderTarget = CreateCCTVRenderTarget(RenderTargetWidth, RenderTargetHeight);
				
				Slot.SceneCapture = NewObject<USceneCaptureComponent2D>(GetOwner(), 
					USceneCaptureComponent2D::StaticClass(), 
					*FString::Printf(TEXT("CCTVSceneCapture_%d"), i));
				Slot.SceneCapture->AttachToComponent(GetOwner()->GetRootComponent(), 
					FAttachmentTransformRules::KeepWorldTransform);
				Slot.SceneCapture->RegisterComponent();
				Slot.SceneCapture->TextureTarget = Slot.RenderTarget;
				
				// 기본 설정만 적용 (Blueprint 설정은 덮어쓰지 않음)
				SetupSceneCaptureBasicSettings(Slot.SceneCapture);
				
				CCTV_LOG(Log, "레이서 %d의 새로운 SceneCaptureComponent 생성 - Racer: %s, RT: %s", 
					i, *Racer->GetName(), *Slot.RenderTarget->GetName());
			}
			
			Slot.TargetPawn = SortedRacers[i];
			Slot.CurrentCameraSlot = 0;

			UpdateFeedSlot(i);
		}
		else
		{
			// 레이서가 없으면 RenderTarget은 null로 유지 (UI에서 텍스트 표시)
			Slot.RenderTarget = nullptr;
			Slot.SceneCapture = nullptr;
			Slot.TargetPawn = nullptr;
			Slot.CurrentCameraSlot = 0;
		}
	}

	// 팩맨 찾기 (BP 클래스 필터 적용)
	TArray<AActor*> FoundEnemies;
	
	// PixelEnemyBPClass가 설정되어 있으면 해당 클래스만 검색, 없으면 모든 APixelEnemy 검색
	UClass* SearchClass = PixelEnemyBPClass ? PixelEnemyBPClass.Get() : APixelEnemy::StaticClass();
	UGameplayStatics::GetAllActorsOfClass(World, SearchClass, FoundEnemies);

	CCTV_LOG(Log, "Enemy 검색 결과: %d개 발견 (검색 클래스: %s)", 
		FoundEnemies.Num(), *GetNameSafe(SearchClass));

	if (FoundEnemies.Num() > 0)
	{
		APixelEnemy* PixelEnemy = Cast<APixelEnemy>(FoundEnemies[0]);
		if (PixelEnemy)
		{
			FCCTVFeedSlot& Slot = FeedSlots[3];
			
			// PixelEnemy의 기존 SceneCaptureComponent 사용 (BP_CCTV_Pixel과 동일한 view를 위해)
			USceneCaptureComponent2D* ExistingSceneCapture = PixelEnemy->GetCCTVSceneCaptureComponent();
			
			if (ExistingSceneCapture && ExistingSceneCapture->TextureTarget)
			{
				// PixelEnemy의 기존 SceneCaptureComponent와 RenderTarget 사용
				Slot.SceneCapture = ExistingSceneCapture;
				Slot.RenderTarget = ExistingSceneCapture->TextureTarget;
				
				CCTV_LOG(Log, "PixelEnemy의 기존 SceneCaptureComponent 사용 - Enemy: %s, RT: %s", 
					*PixelEnemy->GetName(), *Slot.RenderTarget->GetName());
			}
			else
			{
				// 기존 SceneCaptureComponent가 없으면 새로 생성
				if (PixelEnemy->CCTVRenderTarget)
				{
					// Blueprint에서 지정한 RenderTarget 사용
					Slot.RenderTarget = PixelEnemy->CCTVRenderTarget;
				}
				else
				{
					// RenderTarget이 없으면 새로 생성
					Slot.RenderTarget = CreateCCTVRenderTarget(RenderTargetWidth, RenderTargetHeight);
					CCTV_LOG(Warning, "PixelEnemy의 CCTVRenderTarget이 Blueprint에서 설정되지 않아 새로 생성했습니다 - Enemy: %s", 
						*PixelEnemy->GetName());
				}
				
				Slot.SceneCapture = NewObject<USceneCaptureComponent2D>(GetOwner(), 
					USceneCaptureComponent2D::StaticClass(), 
					*FString::Printf(TEXT("CCTVSceneCapture_Enemy")));
				Slot.SceneCapture->AttachToComponent(GetOwner()->GetRootComponent(), 
					FAttachmentTransformRules::KeepWorldTransform);
				Slot.SceneCapture->RegisterComponent();
				Slot.SceneCapture->TextureTarget = Slot.RenderTarget;
				
				SetupSceneCaptureBasicSettings(Slot.SceneCapture);
				
				CCTV_LOG(Log, "PixelEnemy의 새로운 SceneCaptureComponent 생성 - Enemy: %s, RT: %s", 
					*PixelEnemy->GetName(), *Slot.RenderTarget->GetName());
			}
			
			Slot.TargetPawn = PixelEnemy;
			Slot.CurrentCameraSlot = 0;
			
			UpdateFeedSlot(3);
			
			CCTV_LOG(Log, "PixelEnemy Feed 초기화 완료 - Enemy: %s, RT: %s",
				*PixelEnemy->GetName(), *Slot.RenderTarget->GetName());
		}
		else
		{
			CCTV_LOG(Error, "PixelEnemy를 찾지 못했습니다");
		}
	}
	else
	{
		FCCTVFeedSlot& Slot = FeedSlots[3];
		Slot.RenderTarget = nullptr;
		Slot.SceneCapture = nullptr;
		Slot.TargetPawn = nullptr;
		Slot.CurrentCameraSlot = 0;
		
		CCTV_LOG(Warning, "Enemy를 찾을 수 없습니다! (검색 클래스: %s)", 
			*GetNameSafe(SearchClass));
	}

	// 성능 최적화 설정
	UpdateCaptureSettings();

	bFeedsInitialized = true;
	CCTV_LOG(Log, "초기화 완료 - Feed 개수: %d", FeedSlots.Num());
}

void UCCTVFeedComponent::OnMatchStarted()
{
	CCTV_LOG(Log, "OnMatchStarted 이벤트 수신 - CCTV Racer 불러오기 시작");
	
	// InitializeFeeds() 호출하여 레이서 불러오기
	// InitializeFeeds() 내부에서 GameState->PlayerArray를 통해 레이서를 찾음
	InitializeFeeds();
	
	// 레이서 연결 상태 확인 및 재시도
	RetryConnectRacersCount = 0;
	RetryConnectRacers();
}

void UCCTVFeedComponent::RetryConnectRacers()
{
	// 최대 재시도 횟수 확인 (최대 5회)
	if (RetryConnectRacersCount >= 5)
	{
		CCTV_LOG(Warning, "레이서 연결 재시도 횟수 초과 (5회). 재시도를 중단합니다.");
		return;
	}
	
	// 연결되지 않은 레이서 확인
	TArray<int32> UnconnectedRacerIndices;
	for (int32 i = 0; i < 3; ++i)
	{
		if (!FeedSlots.IsValidIndex(i))
		{
			continue;
		}
		
		FCCTVFeedSlot& Slot = FeedSlots[i];
		if (!Slot.TargetPawn || !IsValid(Slot.TargetPawn))
		{
			UnconnectedRacerIndices.Add(i);
		}
	}
	
	// 모든 레이서가 연결되었으면 재시도 중단
	if (UnconnectedRacerIndices.Num() == 0)
	{
		CCTV_LOG(Log, "모든 레이서가 연결되었습니다. 재시도를 중단합니다.");
		return;
	}
	
	CCTV_LOG(Log, "연결되지 않은 레이서 발견: %d명 (재시도 횟수: %d/5)", 
		UnconnectedRacerIndices.Num(), RetryConnectRacersCount + 1);
	
	UWorld* World = GetWorld();
	if (!World)
	{
		CCTV_LOG(Error, "World가 null입니다. 재시도를 중단합니다.");
		return;
	}
	
	// GameState에서 레이서 다시 찾기
	TArray<APawn*> SortedRacers;
	SortedRacers.SetNum(3);
	
	// 이미 할당된 레이서 추적 (중복 할당 방지)
	TSet<APawn*> AssignedRacers;
	for (int32 i = 0; i < 3; ++i)
	{
		if (FeedSlots.IsValidIndex(i) && FeedSlots[i].TargetPawn && IsValid(FeedSlots[i].TargetPawn))
		{
			AssignedRacers.Add(FeedSlots[i].TargetPawn);
		}
	}
	
	if (AGameStateBase* GameStateBase = World->GetGameState())
	{
		for (APlayerState* PS : GameStateBase->PlayerArray)
		{
			if (!PS) continue;
			
			ACitRushPlayerState* CitRushPS = Cast<ACitRushPlayerState>(PS);
			if (!CitRushPS) continue;
			
			if (CitRushPS->GetPlayerRole() != EPlayerRole::Racer) continue;
			
			APawn* RacerPawn = CitRushPS->GetPawn();
			if (!RacerPawn)
			{
				if (APlayerController* PC = CitRushPS->GetPlayerController())
				{
					RacerPawn = PC->GetPawn();
				}
			}
			
			if (!RacerPawn) continue;
			
			// 이미 할당된 레이서는 건너뛰기
			if (AssignedRacers.Contains(RacerPawn))
			{
				continue;
			}
			
			AAbstractRacer* Racer = Cast<AAbstractRacer>(RacerPawn);
			if (!Racer) continue;
			
			ETargetRacer TargetRacer = CitRushPS->GetPlayerInfo().targetIndex;
			int32 TargetIndex = -1;
			if (TargetRacer == ETargetRacer::Racer1) TargetIndex = 0;
			else if (TargetRacer == ETargetRacer::Racer2) TargetIndex = 1;
			else if (TargetRacer == ETargetRacer::Racer3) TargetIndex = 2;
			
			if (TargetIndex >= 0 && TargetIndex < 3)
			{
				// 연결되지 않은 슬롯에만 할당
				if (!FeedSlots.IsValidIndex(TargetIndex) || 
					!FeedSlots[TargetIndex].TargetPawn || 
					!IsValid(FeedSlots[TargetIndex].TargetPawn))
				{
					// 해당 슬롯이 이미 다른 레이서에 할당되어 있지 않은지 확인
					if (!SortedRacers[TargetIndex] || SortedRacers[TargetIndex] == RacerPawn)
					{
						SortedRacers[TargetIndex] = RacerPawn;
						AssignedRacers.Add(RacerPawn);
					}
				}
			}
		}
	}
	
	// 연결되지 않은 레이서 슬롯에만 연결 시도
	// PixelEnemy와 동일한 방식: 기존 SceneCaptureComponent 사용 (Blueprint 설정 유지)
	int32 ConnectedCount = 0;
	for (int32 i : UnconnectedRacerIndices)
	{
		if (!SortedRacers[i]) continue;
		
		FCCTVFeedSlot& Slot = FeedSlots[i];
		AAbstractRacer* Racer = Cast<AAbstractRacer>(SortedRacers[i]);
		if (!Racer) continue;
		
		// 레이서의 기존 SceneCaptureComponent 사용 (Blueprint 설정 완전 유지)
		USceneCaptureComponent2D* ExistingSceneCapture = Racer->GetCCTVSceneCaptureComponent();
		
		if (ExistingSceneCapture && ExistingSceneCapture->TextureTarget)
		{
			// 레이서의 기존 SceneCaptureComponent와 RenderTarget 사용
			Slot.SceneCapture = ExistingSceneCapture;
			Slot.RenderTarget = ExistingSceneCapture->TextureTarget;
			
			CCTV_LOG(Log, "레이서 %d 재연결: 기존 SceneCaptureComponent 사용 - Racer: %s, RT: %s", 
				i, *Racer->GetName(), *Slot.RenderTarget->GetName());
		}
		else
		{
			// 기존 SceneCaptureComponent가 없으면 새로 생성
			Slot.RenderTarget = CreateCCTVRenderTarget(RenderTargetWidth, RenderTargetHeight);
			
			Slot.SceneCapture = NewObject<USceneCaptureComponent2D>(GetOwner(), 
				USceneCaptureComponent2D::StaticClass(), 
				*FString::Printf(TEXT("CCTVSceneCapture_%d"), i));
			Slot.SceneCapture->AttachToComponent(GetOwner()->GetRootComponent(), 
				FAttachmentTransformRules::KeepWorldTransform);
			Slot.SceneCapture->RegisterComponent();
			Slot.SceneCapture->TextureTarget = Slot.RenderTarget;
			
			// 기본 설정만 적용 (Blueprint 설정은 덮어쓰지 않음)
			SetupSceneCaptureBasicSettings(Slot.SceneCapture);
			
			CCTV_LOG(Log, "레이서 %d 재연결: 새로운 SceneCaptureComponent 생성 - Racer: %s, RT: %s", 
				i, *Racer->GetName(), *Slot.RenderTarget->GetName());
		}
		
		Slot.TargetPawn = SortedRacers[i];
		Slot.CurrentCameraSlot = 0;
		
		UpdateFeedSlot(i);
		ConnectedCount++;
		
		// PlayerState에서 이름 가져오기
		FString PlayerName = TEXT("Unknown");
		if (APlayerState* PS = SortedRacers[i]->GetPlayerState())
		{
			if (ACitRushPlayerState* CitRushPS = Cast<ACitRushPlayerState>(PS))
			{
				PlayerName = CitRushPS->GetPlayerInfo().playerName;
			}
			else
			{
				PlayerName = PS->GetPlayerName();
			}
		}
		
		CCTV_LOG(Log, "레이서 %d 재연결 완료 - PlayerName: %s", i, *PlayerName);
	}
	
	if (ConnectedCount > 0)
	{
		CCTV_LOG(Log, "레이서 재연결 성공: %d명", ConnectedCount);
		UpdateCaptureSettings();
	}
	
	// 아직 연결되지 않은 레이서가 있으면 재시도 예약
	TArray<int32> StillUnconnected;
	for (int32 i = 0; i < 3; ++i)
	{
		if (!FeedSlots.IsValidIndex(i)) continue;
		FCCTVFeedSlot& Slot = FeedSlots[i];
		if (!Slot.TargetPawn || !IsValid(Slot.TargetPawn))
		{
			StillUnconnected.Add(i);
		}
	}
	
	if (StillUnconnected.Num() > 0)
	{
		RetryConnectRacersCount++;
		
		// 1초 후 재시도
		World->GetTimerManager().SetTimer(RetryConnectRacersTimerHandle, 
			this, &UCCTVFeedComponent::RetryConnectRacers, 1.0f, false);
		
		CCTV_LOG(Log, "레이서 재연결 재시도 예약 (1초 후, 재시도 횟수: %d/5)", RetryConnectRacersCount);
	}
	else
	{
		CCTV_LOG(Log, "모든 레이서 연결 완료");
	}
}

void UCCTVFeedComponent::MoveFocus(bool bNext)
{
	if (FeedSlots.Num() == 0) return;

	int32 OldIndex = FocusIndex;
	if (bNext)
	{
		FocusIndex = (FocusIndex + 1) % FeedSlots.Num();
	}
	else
	{
		FocusIndex = (FocusIndex - 1 + FeedSlots.Num()) % FeedSlots.Num();
	}

	// 포커스 변경 시 해당 슬롯의 카메라 업데이트
	if (FeedSlots.IsValidIndex(FocusIndex))
	{
		UpdateFeedSlot(FocusIndex);
	}

	UpdateCaptureSettings();
	OnFocusIndexChanged.Broadcast(OldIndex, FocusIndex);

	CCTV_LOG(Log, "포커스 이동 -> Index: %d", FocusIndex);
}

void UCCTVFeedComponent::SetFocusIndex(int32 NewFocusIndex)
{
	if (FeedSlots.IsValidIndex(NewFocusIndex) && FocusIndex != NewFocusIndex)
	{
		int32 OldIndex = FocusIndex;
		FocusIndex = NewFocusIndex;
		
		// 포커스 변경 시 해당 슬롯의 카메라 업데이트
		UpdateFeedSlot(FocusIndex);
		
		UpdateCaptureSettings();
		OnFocusIndexChanged.Broadcast(OldIndex, FocusIndex);
		CCTV_LOG(Log, "포커스 설정 -> Index: %d", FocusIndex);
	}
}

void UCCTVFeedComponent::SwitchCameraSlot(bool bNext)
{
	CCTV_LOG(Log, "SwitchCameraSlot 호출 - FocusIndex: %d, bNext: %s", FocusIndex, bNext ? TEXT("true") : TEXT("false"));
	
	if (!FeedSlots.IsValidIndex(FocusIndex))
	{
		CCTV_LOG(Warning, "FeedSlots 인덱스 범위 초과 - FocusIndex: %d", FocusIndex);
		return;
	}

	FCCTVFeedSlot& Slot = FeedSlots[FocusIndex];
	if (!Slot.TargetPawn)
	{
		CCTV_LOG(Warning, "TargetPawn이 null입니다 - FocusIndex: %d", FocusIndex);
		return;
	}
	
	if (!IsValid(Slot.TargetPawn))
	{
		CCTV_LOG(Warning, "TargetPawn이 유효하지 않습니다 - FocusIndex: %d, Pawn: %s", FocusIndex, *GetNameSafe(Slot.TargetPawn));
		return;
	}

	int32 OldSlot = Slot.CurrentCameraSlot;
	CCTV_LOG(Log, "현재 카메라 슬롯: %d, TargetPawn: %s", OldSlot, *Slot.TargetPawn->GetName());

	// 사용 가능한 카메라 개수 동적으로 계산
	int32 MaxCameraSlots = 3; // 기본값
	if (AAbstractRacer* Racer = Cast<AAbstractRacer>(Slot.TargetPawn))
	{
		if (!Racer->CCTVCameraComponent)
		{
			CCTV_LOG(Warning, "CCTVCameraComponent가 null입니다 - Racer: %s", *Racer->GetName());
			return;
		}
		
		// UE_CITRUSHPawn의 경우 5개 (0=Back, 1=Front, 2=Left, 3=Right, 4=BackSide)
		FString RacerClassName = Racer->GetClass()->GetName();
		if (RacerClassName.Contains(TEXT("UE_CITRUSHPawn")))
		{
			MaxCameraSlots = 5;
		}
		else
		{
			// GetCCTVCamera가 null을 반환할 때까지 시도하여 카메라 개수 확인 (최대 10개)
			for (int32 i = 0; i < 10; ++i)
			{
				UCameraComponent* TestCamera = Racer->CCTVCameraComponent->GetCCTVCamera(i);
				if (!TestCamera)
				{
					MaxCameraSlots = i;
					break;
				}
				MaxCameraSlots = i + 1;
			}
		}
		
		CCTV_LOG(Log, "카메라 슬롯 개수 확인 - Racer: %s, MaxCameraSlots: %d", *Racer->GetName(), MaxCameraSlots);
	}
	else
	{
		CCTV_LOG(Warning, "TargetPawn이 AAbstractRacer가 아닙니다 - Type: %s", Slot.TargetPawn ? *Slot.TargetPawn->GetClass()->GetName() : TEXT("null"));
		return;
	}

	// 카메라 슬롯 순환
	if (MaxCameraSlots > 0)
	{
		if (bNext)
		{
			Slot.CurrentCameraSlot = (OldSlot + 1) % MaxCameraSlots;
		}
		else
		{
			Slot.CurrentCameraSlot = (OldSlot - 1 + MaxCameraSlots) % MaxCameraSlots;
		}
	}
	else
	{
		CCTV_LOG(Warning, "MaxCameraSlots가 0입니다 - 슬롯 변경 불가");
		return;
	}
	
	UpdateFeedSlot(FocusIndex);
	OnCameraSlotChanged.Broadcast(FocusIndex, Slot.CurrentCameraSlot);
	
	CCTV_LOG(Log, "카메라 슬롯 전환 -> Feed: %d, Slot: %d -> %d", FocusIndex, OldSlot, Slot.CurrentCameraSlot);
}

void UCCTVFeedComponent::ToggleExpand()
{
	bool bWasExpanded = bExpanded;
	bExpanded = !bExpanded;
	
	// 확대 해제 시 (4분할로 돌아올 때) 모든 FeedSlot을 강제로 캡처하여 마지막 이미지 유지
	if (bWasExpanded && !bExpanded)
	{
		for (int32 i = 0; i < FeedSlots.Num(); ++i)
		{
			if (FeedSlots[i].SceneCapture && FeedSlots[i].TargetPawn)
			{
				// 각 FeedSlot의 카메라에 다시 attach하고 강제 캡처 (마지막 이미지 확보)
				UpdateFeedSlot(i);
			}
		}
		CCTV_LOG(Log, "확대 해제: 모든 FeedSlot 업데이트 완료");
	}
	
	UpdateCaptureSettings();
	
	// 확대 해제 시 포커스되지 않은 FeedSlot은 실시간 업데이트 중지 (마지막 이미지 고정)
	if (bWasExpanded && !bExpanded)
	{
		for (int32 i = 0; i < FeedSlots.Num(); ++i)
		{
			if (i != FocusIndex && FeedSlots[i].SceneCapture)
			{
				FeedSlots[i].SceneCapture->bCaptureEveryFrame = false;
			}
		}
		CCTV_LOG(Log, "확대 해제: 포커스되지 않은 Feed는 마지막 이미지 고정");
	}

	CCTV_LOG(Log, "확대 상태 변경 -> %s", bExpanded ? TEXT("확대") : TEXT("4분할"));
}

void UCCTVFeedComponent::ResetToDefaultState()
{
	bool bWasExpanded = bExpanded;
	int32 OldFocusIndex = FocusIndex;
	
	// 확대 상태 해제
	bExpanded = false;
	
	// 포커스 인덱스를 0으로 리셋
	FocusIndex = 0;
	
	// 확대 해제 시 모든 FeedSlot 업데이트
	if (bWasExpanded)
	{
		for (int32 i = 0; i < FeedSlots.Num(); ++i)
		{
			if (FeedSlots[i].SceneCapture && FeedSlots[i].TargetPawn)
			{
				UpdateFeedSlot(i);
			}
		}
	}
	
	UpdateCaptureSettings();
	
	// 포커스되지 않은 FeedSlot은 실시간 업데이트 중지 (마지막 이미지 고정)
	for (int32 i = 0; i < FeedSlots.Num(); ++i)
	{
		if (i != FocusIndex && FeedSlots[i].SceneCapture)
		{
			FeedSlots[i].SceneCapture->bCaptureEveryFrame = false;
		}
	}
	
	// 이벤트 브로드캐스트 (항상 브로드캐스트하여 UI가 확실히 업데이트되도록)
	OnExpandedChanged.Broadcast(false);
	if (OldFocusIndex != FocusIndex)
	{
		OnFocusIndexChanged.Broadcast(OldFocusIndex, FocusIndex);
	}
}

FCCTVFeedSlot UCCTVFeedComponent::GetFeedSlot(int32 Index) const
{
	if (FeedSlots.IsValidIndex(Index))
	{
		return FeedSlots[Index];
	}
	return FCCTVFeedSlot();
}

UTextureRenderTarget2D* UCCTVFeedComponent::GetRenderTarget(int32 FeedIndex) const
{
	if (FeedSlots.IsValidIndex(FeedIndex))
	{
		return FeedSlots[FeedIndex].RenderTarget;
	}
	return nullptr;
}

void UCCTVFeedComponent::UpdateFeedSlot(int32 Index)
{
	if (!FeedSlots.IsValidIndex(Index))
	{
		CCTV_LOG(Warning, "FeedSlots 인덱스 범위 초과 - Index: %d", Index);
		return;
	}

	FCCTVFeedSlot& Slot = FeedSlots[Index];
	if (!Slot.SceneCapture || !Slot.TargetPawn)
	{
		return;
	}

	// Pawn의 CCTV 카메라에 Attach
	AttachSceneCaptureToCamera(Slot.SceneCapture, Slot.TargetPawn, Slot.CurrentCameraSlot);
	
	// Transform 동기화 후 즉시 캡처 (첫 오픈 시 흰 화면 방지)
	ForceCaptureSlot(Index);
}

void UCCTVFeedComponent::AttachSceneCaptureToCamera(USceneCaptureComponent2D* SceneCapture, APawn* TargetPawn, int32 CameraSlot)
{
	if (!SceneCapture)
	{
		CCTV_LOG(Error, "SceneCapture가 null입니다!");
		return;
	}
	
	if (!TargetPawn)
	{
		CCTV_LOG(Error, "TargetPawn이 null입니다!");
		return;
	}

	// PixelEnemy인 경우 PostProcessSettings를 덮어쓰지 않음 (PixelEnemy에서 직접 관리)
	APixelEnemy* PixelEnemy = Cast<APixelEnemy>(TargetPawn);
	bool bIsPixelEnemy = PixelEnemy != nullptr;
	
	// 레이서의 경우
	AAbstractRacer* Racer = Cast<AAbstractRacer>(TargetPawn);
	bool bIsRacer = Racer != nullptr;
	
	// 기존 SceneCaptureComponent인지 확인 (PixelEnemy와 레이서 모두 동일한 방식)
	bool bIsExistingSceneCapture = false;
	if (bIsPixelEnemy)
	{
		bIsExistingSceneCapture = (PixelEnemy->GetCCTVSceneCaptureComponent() == SceneCapture);
	}
	else if (bIsRacer)
	{
		bIsExistingSceneCapture = (Racer->GetCCTVSceneCaptureComponent() == SceneCapture);
	}
	
	// 기존 SceneCaptureComponent인 경우 Camera에 Attach만 수행 (Blueprint 설정 완전 유지)
	if (bIsExistingSceneCapture)
	{
		UCameraComponent* CCTVCamera = GetCCTVCamera(TargetPawn, CameraSlot);
		if (CCTVCamera)
		{
			// Camera에 Attach (고정되어 있지 않도록)
			SceneCapture->AttachToComponent(CCTVCamera, FAttachmentTransformRules::SnapToTargetIncludingScale);
			SceneCapture->FOVAngle = CCTVCamera->FieldOfView;
			SceneCapture->ProjectionType = CCTVCamera->ProjectionMode;
			
			// PostProcessSettings는 절대 변경하지 않음 (Blueprint 설정 완전 유지)
			// SceneCapture->PostProcessSettings의 모든 bOverride 플래그를 확인하여 덮어쓰지 않도록 보장
			
			CCTV_LOG(Log, "기존 SceneCaptureComponent Camera에 Attach 완료 - Pawn: %s, Slot: %d, FOV: %.1f, AutoExposureMethod: %d, AutoExposureBias: %.2f", 
				*TargetPawn->GetName(), CameraSlot, SceneCapture->FOVAngle,
				(int32)SceneCapture->PostProcessSettings.AutoExposureMethod,
				SceneCapture->PostProcessSettings.AutoExposureBias);
		}
		else
		{
			CCTV_LOG(Warning, "기존 SceneCapture의 CCTV 카메라를 찾을 수 없습니다 - Pawn: %s, Slot: %d", *TargetPawn->GetName(), CameraSlot);
		}
		return;
	}
	
	// 새로 생성한 SceneCapture는 초기화 수행 (레이서와 PixelEnemy 모두 동일한 방식)
	// ShowFlags와 기본 설정만 적용 (PostProcessSettings는 Blueprint 설정 유지)
	SetupSceneCaptureBasicSettings(SceneCapture);
	
	if (bIsRacer)
	{
		CCTV_LOG(Log, "레이서 새 SceneCapture 설정 (PixelEnemy와 동일한 방식) - Pawn: %s", *TargetPawn->GetName());
	}
	else if (bIsPixelEnemy)
	{
		CCTV_LOG(Log, "PixelEnemy 새 SceneCapture 설정 - Pawn: %s", *TargetPawn->GetName());
	}

	UCameraComponent* CCTVCamera = GetCCTVCamera(TargetPawn, CameraSlot);
	if (!CCTVCamera)
	{
		CCTV_LOG(Warning, "CCTV 카메라를 찾을 수 없습니다 - Pawn: %s (타입: %s), Slot: %d, NetMode: %d", 
			*TargetPawn->GetName(), *TargetPawn->GetClass()->GetName(), CameraSlot, (int32)GetWorld()->GetNetMode());
		
		// PixelEnemy인 경우 Camera 컴포넌트 직접 확인
		if (PixelEnemy)
		{
			CCTV_LOG(Error, "PixelEnemy Camera 직접 확인 - Camera 포인터: %p", PixelEnemy->Camera.Get());
			if (PixelEnemy->Camera)
			{
				CCTV_LOG(Error, "PixelEnemy Camera는 존재하지만 GetCCTVCamera가 null을 반환했습니다!");
			}
			else
			{
				CCTV_LOG(Error, "PixelEnemy Camera가 null입니다! Enemy에 Camera 컴포넌트가 없습니다.");
			}
		}
		// 레이서인 경우 CCTVCameraComponent 확인 및 재시도 로직
		else if (bIsRacer && Racer)
		{
			CCTV_LOG(Warning, "레이서 CCTVCameraComponent 확인 - Component 포인터: %p, Racer: %s, NetMode: %d", 
				Racer->CCTVCameraComponent.Get(), *Racer->GetName(), (int32)GetWorld()->GetNetMode());
			if (Racer->CCTVCameraComponent)
			{
				UCameraComponent* DirectCamera = Racer->CCTVCameraComponent->GetCCTVCamera(CameraSlot);
				CCTV_LOG(Warning, "레이서 CCTVCameraComponent->GetCCTVCamera(%d) 결과: %p", CameraSlot, DirectCamera);
				if (!DirectCamera)
				{
					CCTV_LOG(Warning, "레이서의 CCTVCameraComponent는 존재하지만 GetCCTVCamera가 null을 반환했습니다! 카메라가 아직 준비되지 않았을 수 있습니다. 0.5초 후 재시도합니다.");
					
					// 재시도 로직: FeedSlot 인덱스를 찾아서 나중에 다시 시도
					UWorld* World = GetWorld();
					if (World)
					{
						// FeedSlot 인덱스 찾기
						int32 FeedSlotIndex = -1;
						for (int32 i = 0; i < FeedSlots.Num(); ++i)
						{
							if (FeedSlots[i].TargetPawn == TargetPawn && FeedSlots[i].SceneCapture == SceneCapture)
							{
								FeedSlotIndex = i;
								break;
							}
						}
						
						if (FeedSlotIndex >= 0)
						{
							// 재시도 횟수 확인 (최대 10회)
							int32* RetryCount = RetryAttachCounts.Find(FeedSlotIndex);
							int32 CurrentRetryCount = RetryCount ? *RetryCount : 0;
							
							if (CurrentRetryCount >= 10)
							{
								CCTV_LOG(Error, "레이서 %d의 카메라 attach 재시도 횟수 초과 (10회). 재시도를 중단합니다.", FeedSlotIndex);
								RetryAttachCounts.Remove(FeedSlotIndex);
								if (FTimerHandle* ExistingHandle = RetryAttachTimerHandles.Find(FeedSlotIndex))
								{
									World->GetTimerManager().ClearTimer(*ExistingHandle);
									RetryAttachTimerHandles.Remove(FeedSlotIndex);
								}
								return;
							}
							
							// 기존 타이머가 있으면 클리어
							if (FTimerHandle* ExistingHandle = RetryAttachTimerHandles.Find(FeedSlotIndex))
							{
								World->GetTimerManager().ClearTimer(*ExistingHandle);
							}
							
							// 재시도 횟수 증가
							RetryAttachCounts.Add(FeedSlotIndex, CurrentRetryCount + 1);
							
							// 0.5초 후 재시도
							FTimerHandle RetryHandle;
							FTimerDelegate RetryDelegate;
							RetryDelegate.BindUObject(this, &UCCTVFeedComponent::UpdateFeedSlot, FeedSlotIndex);
							World->GetTimerManager().SetTimer(RetryHandle, RetryDelegate, 0.5f, false);
							RetryAttachTimerHandles.Add(FeedSlotIndex, RetryHandle);
							
							CCTV_LOG(Log, "레이서 %d의 카메라 attach 재시도 예약됨 (0.5초 후, 재시도 횟수: %d/10)", FeedSlotIndex, CurrentRetryCount + 1);
						}
					}
				}
			}
			else
			{
				CCTV_LOG(Error, "레이서의 CCTVCameraComponent가 null입니다!");
			}
		}
		return;
	}
	
	CCTV_LOG(Log, "CCTV 카메라 찾기 성공 - Pawn: %s, Slot: %d, Camera: %s", 
		*TargetPawn->GetName(), CameraSlot, *CCTVCamera->GetName());

	// SceneCapture를 CCTV 카메라에 Attach
	SceneCapture->AttachToComponent(CCTVCamera, FAttachmentTransformRules::SnapToTargetIncludingScale);
	
	// Camera의 FOV와 Projection Mode 동기화
	SceneCapture->FOVAngle = CCTVCamera->FieldOfView;
	SceneCapture->ProjectionType = CCTVCamera->ProjectionMode;
	
	// 재시도 타이머 정리 (성공적으로 attach되었으므로)
	UWorld* World = GetWorld();
	if (World)
	{
		// FeedSlot 인덱스 찾기
		int32 FeedSlotIndex = -1;
		for (int32 i = 0; i < FeedSlots.Num(); ++i)
		{
			if (FeedSlots[i].TargetPawn == TargetPawn && FeedSlots[i].SceneCapture == SceneCapture)
			{
				FeedSlotIndex = i;
				break;
			}
		}
		
		if (FeedSlotIndex >= 0)
		{
			if (FTimerHandle* RetryHandle = RetryAttachTimerHandles.Find(FeedSlotIndex))
			{
				World->GetTimerManager().ClearTimer(*RetryHandle);
				RetryAttachTimerHandles.Remove(FeedSlotIndex);
			}
			RetryAttachCounts.Remove(FeedSlotIndex);
		}
	}
	
	CCTV_LOG(Log, "SceneCapture Attach 완료 - Pawn: %s, Slot: %d, FOV: %.1f", 
		*TargetPawn->GetName(), CameraSlot, SceneCapture->FOVAngle);
}

UCameraComponent* UCCTVFeedComponent::GetCCTVCamera(APawn* Pawn, int32 SlotIndex) const
{
	if (!Pawn) return nullptr;

	// 레이서인 경우
	if (AAbstractRacer* Racer = Cast<AAbstractRacer>(Pawn))
	{
		return Racer->GetCCTVCamera(SlotIndex);
	}

	// 팩맨인 경우
	if (APixelEnemy* PixelEnemy = Cast<APixelEnemy>(Pawn))
	{
		return PixelEnemy->GetCCTVCamera(SlotIndex);
	}

	return nullptr;
}

void UCCTVFeedComponent::UpdateCaptureSettings()
{
	// CCTV UI가 비활성화되어 있으면 모든 SceneCapture 비활성화 (VRAM 절약)
	if (!bCCTVUIActive)
	{
		for (FCCTVFeedSlot& Slot : FeedSlots)
		{
			if (Slot.SceneCapture)
			{
				Slot.SceneCapture->bCaptureEveryFrame = false;
			}
		}
		// PixelEnemy의 SceneCaptureComponent는 별도로 관리 (SetCCTVUIActive에서 처리)
		// 저주기 업데이트 타이머 정리
		if (LowFrequencyUpdateTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(LowFrequencyUpdateTimerHandle);
		}
		return;
	}

	// 확대 상태일 때는 포커스된 Feed만 업데이트하고 나머지는 중단
	if (bExpanded)
	{
		for (int32 i = 0; i < FeedSlots.Num(); ++i)
		{
			FCCTVFeedSlot& Slot = FeedSlots[i];
			
			// 모든 Feed (레이서 + Enemy) 동일하게 처리
			if (Slot.SceneCapture)
			{
				// 포커스된 Feed만 실시간 캡처
				Slot.SceneCapture->bCaptureEveryFrame = (i == FocusIndex);
			}
		}
		
		// 저주기 업데이트 타이머 정리 (확대 상태에서는 다른 Feed 업데이트 불필요)
		if (LowFrequencyUpdateTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(LowFrequencyUpdateTimerHandle);
		}
		
		return;
	}

	// 4분할 상태
	if (!bUsePerformanceOptimization)
	{
		// 모든 Feed 실시간 업데이트 (UI가 활성화된 경우에만)
		for (FCCTVFeedSlot& Slot : FeedSlots)
		{
			if (Slot.SceneCapture)
			{
				Slot.SceneCapture->bCaptureEveryFrame = true;
			}
		}
		
		// 저주기 업데이트 타이머 정리 (모든 Feed가 실시간 업데이트되므로 불필요)
		if (LowFrequencyUpdateTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(LowFrequencyUpdateTimerHandle);
		}
		
		return;
	}

	// 성능 최적화 모드: 포커스된 Feed만 실시간
	for (int32 i = 0; i < FeedSlots.Num(); ++i)
	{
		FCCTVFeedSlot& Slot = FeedSlots[i];
		
		// 모든 Feed (레이서 + Enemy) 동일하게 처리
		if (Slot.SceneCapture)
		{
			// 포커스된 Feed만 실시간 캡처
			Slot.SceneCapture->bCaptureEveryFrame = (i == FocusIndex);
		}
	}

	// 저주기 업데이트 타이머 설정
	if (LowFrequencyUpdateTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(LowFrequencyUpdateTimerHandle);
	}

	// 포커스되지 않은 Feed들을 저주기로 업데이트
	GetWorld()->GetTimerManager().SetTimer(LowFrequencyUpdateTimerHandle, [this]()
	{
		for (int32 i = 0; i < FeedSlots.Num(); ++i)
		{
			if (i == FocusIndex) continue; // 포커스된 Feed는 제외

			FCCTVFeedSlot& Slot = FeedSlots[i];
			if (Slot.SceneCapture && !Slot.SceneCapture->bCaptureEveryFrame)
			{
				Slot.SceneCapture->CaptureScene();
			}
			// PixelEnemy의 경우 자신의 SceneCaptureComponent가 자동으로 렌더링하므로 별도 처리 불필요
		}
	}, LowFrequencyUpdateInterval, true);
}

void UCCTVFeedComponent::InitializeSceneCapture(USceneCaptureComponent2D* SceneCapture)
{
	if (!SceneCapture)
	{
		CCTV_LOG(Error, "InitializeSceneCapture: SceneCapture가 null입니다!");
		return;
	}

	// CaptureSource 설정
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	
	// CompositeMode 설정 (프레임 누적 방지)
	SceneCapture->CompositeMode = SCCM_Overwrite;
	
	// 캡처 설정
	SceneCapture->bCaptureEveryFrame = true;
	SceneCapture->bCaptureOnMovement = false;
	
	// ShowFlags 설정 (필수: Materials, Lighting, PostProcessing 활성화)
	FEngineShowFlags& ShowFlags = SceneCapture->ShowFlags;
	
	// 기본 게임 플래그 활성화
	ShowFlags.SetGame(true);
	
	// 렌더링 필수 플래그 활성화
	ShowFlags.SetMaterials(true);
	ShowFlags.SetLighting(true);
	ShowFlags.SetPostProcessing(true);
	
	// 디버그 플래그 비활성화 (바운딩 박스 등 제거)
	ShowFlags.SetBounds(false);
	ShowFlags.SetCollision(false);
	ShowFlags.SetVisualizeBuffer(false);
	
	// 기타 불필요한 플래그 비활성화
	ShowFlags.SetFog(false);
	ShowFlags.SetVolumetricFog(false);
	
	// PostProcessSettings는 Blueprint에서 설정한 값이 그대로 적용되도록 함 (코드에서 덮어쓰지 않음)
	// SceneCaptureComponent의 Blueprint 설정 (AutoExposureBias 등)이 완전히 유지됨
	
	CCTV_LOG(Log, "SceneCapture 초기화 완료 - Materials: %d, Lighting: %d, Bounds: %d, PostProcessSettings는 Blueprint 설정 유지", 
		ShowFlags.Materials ? 1 : 0, 
		ShowFlags.Lighting ? 1 : 0, 
		ShowFlags.Bounds ? 1 : 0);
}

UTextureRenderTarget2D* UCCTVFeedComponent::CreateCCTVRenderTarget(int32 Width, int32 Height)
{
	if (Width <= 0 || Height <= 0)
	{
		CCTV_LOG(Error, "CreateCCTVRenderTarget: 잘못된 크기 (%dx%d)", Width, Height);
		return nullptr;
	}

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(GetOwner());
	if (!RenderTarget)
	{
		CCTV_LOG(Error, "CreateCCTVRenderTarget: RenderTarget 생성 실패");
		return nullptr;
	}

	// 포맷 명시적 설정 (RGBA8, 알파 포함, LDR)
	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	
	// 크기 설정
	RenderTarget->InitAutoFormat(Width, Height);
	
	// ClearColor 설정 (알파 1 = 완전 불투명)
	RenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	// 리소스 즉시 업데이트
	RenderTarget->UpdateResourceImmediate(true);
	
	CCTV_LOG(Log, "RenderTarget 생성 완료 - 크기: %dx%d, 포맷: RTF_RGBA8, ClearColor.A: %.2f", 
		Width, Height, RenderTarget->ClearColor.A);
	
	return RenderTarget;
}

void UCCTVFeedComponent::SetCCTVUIActive(bool bActive)
{
	bCCTVUIActive = bActive;
	
	CCTV_LOG(Log, "CCTV UI 활성화 상태 변경: %s", bActive ? TEXT("활성화") : TEXT("비활성화"));
	
	if (bActive)
	{
		// CCTV UI 활성화 시: 모든 슬롯에 대해 즉시 강제 캡처 (첫 오픈 시 흰 화면 방지)
		for (int32 i = 0; i < FeedSlots.Num(); ++i)
		{
			ForceCaptureSlot(i);
		}
	}
	
	// SceneCapture 설정 업데이트
	UpdateCaptureSettings();
}

void UCCTVFeedComponent::ForceCaptureSlot(int32 SlotIndex)
{
	if (!FeedSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	FCCTVFeedSlot& Slot = FeedSlots[SlotIndex];
	
	if (!Slot.TargetPawn)
	{
		CCTV_LOG(Verbose, "ForceCaptureSlot: 슬롯 %d는 TargetPawn이 없습니다", SlotIndex);
		return;
	}

	// 모든 슬롯 (레이서 + Enemy) 동일하게 처리
	if (!Slot.SceneCapture || !Slot.RenderTarget)
	{
		CCTV_LOG(Verbose, "ForceCaptureSlot: 슬롯 %d는 SceneCapture나 RenderTarget이 없습니다", SlotIndex);
		return;
	}

	// Target 카메라 가져오기
	UCameraComponent* CCTVCamera = GetCCTVCamera(Slot.TargetPawn, Slot.CurrentCameraSlot);
	if (!CCTVCamera)
	{
		CCTV_LOG(Warning, "ForceCaptureSlot: 슬롯 %d의 CCTV 카메라를 찾을 수 없습니다", SlotIndex);
		return;
	}

	// Transform 강제 동기화 (원점 캡처 방지)
	Slot.SceneCapture->SetWorldTransform(CCTVCamera->GetComponentTransform());
	Slot.SceneCapture->FOVAngle = CCTVCamera->FieldOfView;
	Slot.SceneCapture->ProjectionType = CCTVCamera->ProjectionMode;
	
	// 즉시 캡처
	Slot.SceneCapture->CaptureScene();
	
	CCTV_LOG(Log, "ForceCaptureSlot 완료 - Slot: %d, Pawn: %s, WorldLoc: %s", 
		SlotIndex, *Slot.TargetPawn->GetName(), *Slot.SceneCapture->GetComponentLocation().ToString());
}

void UCCTVFeedComponent::SetupSceneCaptureBasicSettings(USceneCaptureComponent2D* SceneCapture)
{
	if (!SceneCapture)
	{
		return;
	}

	FEngineShowFlags& ShowFlags = SceneCapture->ShowFlags;
	ShowFlags.SetGame(true);
	ShowFlags.SetMaterials(true);
	ShowFlags.SetLighting(true);
	ShowFlags.SetPostProcessing(true);
	ShowFlags.SetBounds(false);
	ShowFlags.SetCollision(false);
	ShowFlags.SetVisualizeBuffer(false);
	ShowFlags.SetFog(false);
	ShowFlags.SetVolumetricFog(false);
	
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->CompositeMode = SCCM_Overwrite;
	SceneCapture->bCaptureEveryFrame = true;
	SceneCapture->bCaptureOnMovement = false;
	
	// PostProcessSettings는 Blueprint에서 설정한 값이 그대로 적용되도록 함 (코드에서 덮어쓰지 않음)
	// SceneCaptureComponent의 Blueprint 설정 (AutoExposureBias 등)이 완전히 유지됨
	
	CCTV_LOG(Log, "SetupSceneCaptureBasicSettings 완료 - PostProcessSettings는 Blueprint 설정 유지");
}

