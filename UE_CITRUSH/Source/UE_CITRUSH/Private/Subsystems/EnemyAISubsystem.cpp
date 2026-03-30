#include "Subsystems/EnemyAISubsystem.h"
#include "JsonUtilities.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NavigationSystem.h"
#include "NavigationSystemTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Stats/Coin/CoinActor.h"
#include "Enemy/Pellet/PelletSpawner.h"
#include "Enemy/Pellet/PelletActor.h"
#include "NavigationSystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Network/NavSystemDataComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "EngineUtils.h"
#include "Player/Car/VehicleDemoCejCar.h"
#include "GAS/AttributeSet/ASRacer.h"
#include "Player/Car/BoostComponent.h"
#include "AbilitySystemComponent.h"
#include "Enemy/PixelEnemy.h"
#include "Enemy/AiEnemy/AIDirectiveComponent.h"
#include "UI/Lobby/SessionCodeWidget.h"
#include "Components/TextBlock.h"
#include "GameFlow/CitRushGameState.h"
#include "Interfaces/IHttpResponse.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_CLASS(UEnemyAISubsystem, LogEnemyAI)

void UEnemyAISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LogDirectoryPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("../testlog"))
	);

	if (UWorld* World = GetWorld())
	{
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &UEnemyAISubsystem::OnActorSpawned)
		);

		// 이미 월드에 존재하는 액터들을 캐시에 등록 (Initialize는 1회만 호출)
		TArray<AActor*> FoundActors;

		UGameplayStatics::GetAllActorsOfClass(World, AVehicleDemoCejCar::StaticClass(), FoundActors);
		for (AActor* Actor : FoundActors)
		{
			if (AVehicleDemoCejCar* Vehicle = Cast<AVehicleDemoCejCar>(Actor))
			{
				CachedVehicleArray.AddUnique(Vehicle);
				Actor->OnDestroyed.AddDynamic(this, &UEnemyAISubsystem::OnActorDestroyed);
			}
		}

		FoundActors.Empty();
		UGameplayStatics::GetAllActorsOfClass(World, APelletActor::StaticClass(), FoundActors);
		for (AActor* Actor : FoundActors)
		{
			if (APelletActor* Pellet = Cast<APelletActor>(Actor))
			{
				CachedPelletArray.AddUnique(Pellet);
				Actor->OnDestroyed.AddDynamic(this, &UEnemyAISubsystem::OnActorDestroyed);
			}
		}

		FoundActors.Empty();
		UGameplayStatics::GetAllActorsOfClass(World, ACoinActor::StaticClass(), FoundActors);
		for (AActor* Actor : FoundActors)
		{
			if (ACoinActor* Coin = Cast<ACoinActor>(Actor))
			{
				CachedCoinArray.AddUnique(Coin);
				Actor->OnDestroyed.AddDynamic(this, &UEnemyAISubsystem::OnActorDestroyed);
			}
		}

		FoundActors.Empty();
		UGameplayStatics::GetAllActorsOfClass(World, APelletSpawner::StaticClass(), FoundActors);
		for (AActor* Actor : FoundActors)
		{
			if (APelletSpawner* Spawner = Cast<APelletSpawner>(Actor))
			{
				CachedPelletSpawnerArray.AddUnique(Spawner);
				Actor->OnDestroyed.AddDynamic(this, &UEnemyAISubsystem::OnActorDestroyed);
			}
		}
	}
}

void UEnemyAISubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}

	StopAutoDecisionRequests();
	RegisteredEnemies.Empty();

	// 액터 캐시 정리
	CachedVehicleArray.Empty();
	CachedPelletArray.Empty();
	CachedCoinArray.Empty();
	CachedPelletSpawnerArray.Empty();

	Super::Deinitialize();
}

void UEnemyAISubsystem::OnActorSpawned(AActor* SpawnedActor)
{
	if (!SpawnedActor)
	{
		return;
	}

	if (SpawnedActor->IsA(APixelEnemy::StaticClass()))
	{
		PixelEnemy = Cast<APixelEnemy>(SpawnedActor);
		GetPixelLocation();
		UE_LOG(LogEnemyAI, Log, TEXT("PixelEnemy spawned and location updated!"));
	}

	// 액터 캐시 자동 등록
	if (AVehicleDemoCejCar* Vehicle = Cast<AVehicleDemoCejCar>(SpawnedActor))
	{
		CachedVehicleArray.AddUnique(Vehicle);
		SpawnedActor->OnDestroyed.AddDynamic(this, &UEnemyAISubsystem::OnActorDestroyed);
	}
	else if (APelletActor* Pellet = Cast<APelletActor>(SpawnedActor))
	{
		CachedPelletArray.AddUnique(Pellet);
		SpawnedActor->OnDestroyed.AddDynamic(this, &UEnemyAISubsystem::OnActorDestroyed);
	}
	else if (ACoinActor* Coin = Cast<ACoinActor>(SpawnedActor))
	{
		CachedCoinArray.AddUnique(Coin);
		SpawnedActor->OnDestroyed.AddDynamic(this, &UEnemyAISubsystem::OnActorDestroyed);
	}
	else if (APelletSpawner* Spawner = Cast<APelletSpawner>(SpawnedActor))
	{
		CachedPelletSpawnerArray.AddUnique(Spawner);
		SpawnedActor->OnDestroyed.AddDynamic(this, &UEnemyAISubsystem::OnActorDestroyed);
	}
}

void UEnemyAISubsystem::OnActorDestroyed(AActor* DestroyedActor)
{
	if (!DestroyedActor)
	{
		return;
	}

	if (AVehicleDemoCejCar* Vehicle = Cast<AVehicleDemoCejCar>(DestroyedActor))
	{
		CachedVehicleArray.RemoveAll([Vehicle](const TWeakObjectPtr<AVehicleDemoCejCar>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Vehicle; });
	}
	else if (APelletActor* Pellet = Cast<APelletActor>(DestroyedActor))
	{
		CachedPelletArray.RemoveAll([Pellet](const TWeakObjectPtr<APelletActor>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Pellet; });
	}
	else if (ACoinActor* Coin = Cast<ACoinActor>(DestroyedActor))
	{
		CachedCoinArray.RemoveAll([Coin](const TWeakObjectPtr<ACoinActor>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Coin; });
	}
	else if (APelletSpawner* Spawner = Cast<APelletSpawner>(DestroyedActor))
	{
		CachedPelletSpawnerArray.RemoveAll([Spawner](const TWeakObjectPtr<APelletSpawner>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Spawner; });
	}
}

FVector UEnemyAISubsystem::GetPixelLocation()
{
	if (PixelEnemy.IsValid())
	{
		PixelLocation = PixelEnemy.Get()->GetActorLocation();
	}
	else
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APixelEnemy::StaticClass(), Found);
		if (Found.Num() > 0)
		{
			PixelEnemy = Cast<APixelEnemy>(Found[0]);
			if (PixelEnemy.IsValid())
			{
				PixelLocation = PixelEnemy.Get()->GetActorLocation();
			}
		}
		else
		{
			PixelLocation = FVector::ZeroVector;
		}
	}

	return PixelLocation;
}


UEnemyAISubsystem::UEnemyAISubsystem(): bIsRequestPending(false)
{
}

// Enemy 관리
void UEnemyAISubsystem::RegisterEnemy(TScriptInterface<IAIDecisionReceiver> Enemy)
{
	if (!Enemy.GetInterface())
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Attempted to register null Enemy"));
		return;
	}

	if (RegisteredEnemies.Contains(Enemy)) // 중복 등록 방지
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Enemy %s already registered"), *Enemy->GetEnemyID());
		return;
	}

	RegisteredEnemies.Add(Enemy);

	// PixelEnemy 멤버 변수 설정 (SendBatchDecisionRequest에서 사용)
	if (AActor* EnemyActor = Cast<AActor>(Enemy.GetObject()))
	{
		if (APixelEnemy* PixelEnemyActor = Cast<APixelEnemy>(EnemyActor))
		{
			PixelEnemy = PixelEnemyActor;
			//UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] PixelEnemy registered: %s"), *PixelEnemyActor->GetName());
		}
	}
}

void UEnemyAISubsystem::UnregisterEnemy(TScriptInterface<IAIDecisionReceiver> Enemy)
{
	if (!Enemy.GetInterface())
	{
		return;
	}

	const int32 RemovedCount = RegisteredEnemies.Remove(Enemy);
	if (RemovedCount > 0)
	{
		/*UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Enemy unregistered: %s (Remaining: %d)"),
		       *Enemy->GetEnemyID(), RegisteredEnemies.Num());*/
	}
}

TScriptInterface<IAIDecisionReceiver> UEnemyAISubsystem::FindEnemyByID(const FString& EnemyID) const
{
	for (const TScriptInterface<IAIDecisionReceiver>& Enemy : RegisteredEnemies)
	{
		if (Enemy.GetInterface() && Enemy->GetEnemyID() == EnemyID)
		{
			return Enemy;
		}
	}

	return nullptr;
}

// AI 결정 요청
void UEnemyAISubsystem::RequestDecisionForEnemy(const FString& EnemyID)
{
	TScriptInterface<IAIDecisionReceiver> Enemy = FindEnemyByID(EnemyID);

	if (!Enemy.GetInterface())
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Enemy not found: %s"), *EnemyID);
		return;
	}

	// 개별 요청 구성
	FGetDecisionRequest2 Request;
	Request.room_id = TEXT("room_ue"); 

	FEnemyGameState State = Enemy->GetCurrentGameState();
	
	// 요청 전송
	FString JsonBody;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Request, JsonBody))
	{
		//UE_LOG(LogEnemyAI, Error, TEXT("[EnemyAISubsystem] Failed to serialize request for %s"), *EnemyID);
		OnAIError.Broadcast(EnemyID, TEXT("Serialization failed"));
		return;
	}

	LastDecisionRequest = Request;
	bIsRequestPending = true;

	TWeakObjectPtr<UEnemyAISubsystem> WeakThis(this);

	//서버가 답장을 주면 나중에 실행되는 콜백 영역
	SendPostRequest(DecisionEndpoint, JsonBody, [WeakThis, EnemyID](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (!WeakThis.IsValid())
		{
			return;
		}

		UEnemyAISubsystem* StrongThis = WeakThis.Get();
		StrongThis->bIsRequestPending = false;

		StrongThis->HandleResponse(Response, bWasSuccessful, [WeakThis, EnemyID](const FString& Content, int32 StatusCode)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			UEnemyAISubsystem* StrongThisInner = WeakThis.Get();

			FGetDecisionResponse2 DecisionResponse;
			
			// 서버 값 이빨이한테 최종 명령 
			if (FJsonObjectConverter::JsonObjectStringToUStruct(Content, &DecisionResponse, 0, 0))
			{
				StrongThisInner->HandleDecisionResponse(DecisionResponse);
			}
			else
			{
				UE_LOG(LogEnemyAI, Error, TEXT("[EnemyAISubsystem] Failed to parse response for %s"), *EnemyID);
				StrongThisInner->OnAIError.Broadcast(EnemyID, TEXT("Parse failed"));
			}
		});
	});
}

// 즉시 첫 요청
void UEnemyAISubsystem::RequestBatchDecision()
{
	if (RegisteredEnemies.Num() == 0)
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] `No enemies registered for batch request"));
		return;
	}

	if (bIsRequestPending)
	{
		UE_LOG(LogEnemyAI, Verbose, TEXT("[EnemyAISubsystem] Request already pending, skipping batch"));
		return;
	}

	SendBatchDecisionRequest();
}

//최종 서버 전송 JSON 매핑 
void UEnemyAISubsystem::SendBatchDecisionRequest()
{
	// 계산 시작 전 팩맨(PixelEnemy)의 현재 위치를 최신화
	GetPixelLocation();

	FGetDecisionRequest2 Request; //언리얼 구조체(USTRUCT)에 게임의 모든 상태(체력, 맵 정보 등)를 대입하는 과정

	// Room ID 가져오기
	FString RoomID = GetCurrentRoomId();
	Request.room_id = RoomID;

	// v1.4.0: request_num 생성 및 저장
	Request.request_num = GenerateRequestNum(RoomID);
	LastSentRequestNum = Request.request_num; // Request-Response 매칭용 저장

	// v1.4.0: current_directive_code 설정
	// PixelEnemy의 AIDirectiveComponent에서 현재 상태 읽기
	Request.current_directive_code = 0; // 기본값: 명령 없음
	if (PixelEnemy.IsValid())
	{
		if (UAIDirectiveComponent* DirectiveComp = PixelEnemy->FindComponentByClass<UAIDirectiveComponent>())
		{
			// EAIDirectiveState를 int32로 캐스팅
			int32 RawCode = static_cast<int32>(DirectiveComp->GetCurrentState());
			
			// [수정] 50(NetworkFallback) 또는 99(내부 상태)일 경우 서버 전송 시 0(Idle)으로 매핑
			if (RawCode == 50 || RawCode == 99)
			{
				Request.current_directive_code = 0;
			}
			else
			{
				Request.current_directive_code = RawCode;
			}

			/*UE_LOG(LogEnemyAI, Verbose, TEXT("[EnemyAISubsystem] current_directive_code: %d (Raw: %d)"),
				Request.current_directive_code, RawCode);*/
		}
	}

	// Global Context 채우기 (GameState에서 가져오기)
	ACitRushGameState* GS = GetWorld() ? GetWorld()->GetGameState<ACitRushGameState>() : nullptr;
	if (GS)
	{
		float Elapsed = GS->GetElapsedTime();
		float Remaining = GS->GetRemainingTime();
		float TotalDuration = Elapsed + Remaining;

		Request.global_context.remaining_time = Remaining;
		
		// 매치 시작 시간이 설정되어 있으면 사용, 아니면 역산
		if (!MatchStartTime.IsEmpty())
		{
			Request.global_context.match_start_time = MatchStartTime;
		}
		else
		{
			Request.global_context.match_start_time = (FDateTime::UtcNow() - FTimespan::FromSeconds(Elapsed)).ToIso8601();
		}

		// Game Phase 계산 (30%, 70% 구간)
		float Progress = (TotalDuration > 0.f) ? (Elapsed / TotalDuration) : 0.f;
		if (Progress < 0.3f)
		{
			Request.global_context.game_phase = TEXT("EARLY_GAME");
		}
		else if (Progress < 0.7f)
		{
			Request.global_context.game_phase = TEXT("MID_GAME");
		}
		else
		{
			Request.global_context.game_phase = TEXT("LATE_GAME");
		}
	}
	else
	{
		Request.global_context.game_phase = TEXT("EARLY_GAME"); // 기본값
		Request.global_context.remaining_time = 300.0f; // 기본값
		Request.global_context.match_start_time = FDateTime::UtcNow().ToIso8601(); // 현재 시간
	}

	// nav_frame 채우기 (v1.4.0) - PixelEnemy의 NavSystemDataComponent 사용
	UNavSystemDataComponent* NavDataComponent = nullptr;
	if (UWorld* World = GetWorld())
	{
		// PixelEnemy 액터를 찾아 NavSystemDataComponent 접근
		for (TActorIterator<APixelEnemy> It(World); It; ++It)
		{
			APixelEnemy* BossEnemy = *It;
			if (BossEnemy && BossEnemy->NavSystemDataComponent)
			{
				NavDataComponent = BossEnemy->NavSystemDataComponent;

				// 최신 데이터 계산
				NavDataComponent->CalculateNavigationData();
				FNavSystemLLMData NavData = NavDataComponent->GetLLMData();

				// nav_frame에 매핑
				Request.global_context.nav_frame.timestamp_sec = NavData.Timestamp;
				Request.global_context.nav_frame.delta_time_sec = NavData.DeltaTime;
				Request.global_context.nav_frame.inter_driver_avg_distance = NavData.InterPlayerAvgDistance;
				Request.global_context.nav_frame.delta_inter_driver_avg_distance = NavData.DeltaInterPlayerDistance;

				UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] nav_frame updated - Time: %.2f, DeltaTime: %.2f, AvgDistance: %.1f, DeltaAvgDist: %.1f"),
					NavData.Timestamp, NavData.DeltaTime, NavData.InterPlayerAvgDistance, NavData.DeltaInterPlayerDistance);

				break; // 찾았으면 종료
			}
		}

		if (!NavDataComponent)
		{
			UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] No PixelEnemy with NavSystemDataComponent found in World"));
		}
	}

	// AI Squad Context 채우기 (첫 번째 Enemy 기준)
	if (RegisteredEnemies.Num() > 0)
	{
		TScriptInterface<IAIDecisionReceiver> FirstEnemy = RegisteredEnemies[0];
		if (FirstEnemy.GetInterface())
		{
			FEnemyGameState State = FirstEnemy->GetCurrentGameState();

			// Pacman Main 설정
			Request.ai_squad_context.pacman_main.position.x = State.position.X;
			Request.ai_squad_context.pacman_main.position.y = State.position.Y;
			Request.ai_squad_context.pacman_main.position.z = State.position.Z;
			Request.ai_squad_context.pacman_main.hp = State.health;
			Request.ai_squad_context.pacman_main.speed = State.speed;
			Request.ai_squad_context.pacman_main.capture_gauge = State.capture_gauge; // CaptureGaugeComponent 계산값 사용
			Request.ai_squad_context.pacman_main.is_invulnerable = State.is_invulnerable;

			UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Sending capture_gauge to server: %.1f (HP: %.1f, Speed: %.1f, Invulnerable: %d)"),
				State.capture_gauge, State.health, State.speed, State.is_invulnerable ? 1 : 0);

			// v1.4.0: 방향 정보 추가
			Request.ai_squad_context.pacman_main.rotation_yaw_deg = State.rotation_yaw_deg;
			Request.ai_squad_context.pacman_main.forward_vector.x = State.forward_vector.X;
			Request.ai_squad_context.pacman_main.forward_vector.y = State.forward_vector.Y;
			Request.ai_squad_context.pacman_main.forward_vector.z = State.forward_vector.Z;

			// Clones: 등록된 clone_ 접두사 Enemy에서 데이터 수집 (v1.5.0)
			Request.ai_squad_context.clones.Empty();
			for (const auto& Enemy : RegisteredEnemies)
			{
				if (!Enemy.GetInterface()) continue;
				FString ID = Enemy->GetEnemyID();
				if (!ID.StartsWith(TEXT("clone_"))) continue;

				FEnemyGameState CS = Enemy->GetCurrentGameState();
				FCloneUnit2 CU;
				CU.unit_id = CS.unit_id;
				CU.position = { static_cast<float>(CS.position.X), static_cast<float>(CS.position.Y), static_cast<float>(CS.position.Z) };
				CU.hp = CS.health;
				CU.is_alive = CS.health > 0.0f;
				CU.speed = CS.speed;
				CU.rotation_yaw_deg = CS.rotation_yaw_deg;
				CU.forward_vector = { static_cast<float>(CS.forward_vector.X), static_cast<float>(CS.forward_vector.Y), static_cast<float>(CS.forward_vector.Z) };
				Request.ai_squad_context.clones.Add(CU);
			}

			// P-Pellet 정보 (FEnemyGameState에서 가져오기)
			Request.ai_squad_context.p_pellet_cooldown = State.p_pellet_cooldown;

			// last_pellet_consumed_at: 빈 문자열이면 유효한 ISO 8601 datetime으로 변환
			if (State.last_pellet_consumed_at.IsEmpty())
			{
				// 사용 이력이 없으면 현재 시각으로 설정
				Request.ai_squad_context.last_pellet_consumed_at = FDateTime::UtcNow().ToIso8601();
			}
			else
			{
				Request.ai_squad_context.last_pellet_consumed_at = State.last_pellet_consumed_at;
			}
		}
	}

	Request.player_team_context.Empty();

	if (UWorld* World = GetWorld())
	{
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PC = Iterator->Get();
			if (PC && PC->GetPawn() && PC->PlayerState) // PlayerState도 확인
			{
				APawn* Pawn = PC->GetPawn();
				FVector PlayerLocation = Pawn->GetActorLocation();
				APlayerState* PS = PC->PlayerState;

				FPlayerTeamContext2 PlayerInfo;

				// 기본 정보 - player_id: 접속 순서 누적 카운터 기반 (1부터 ++1)
				int32 UEPlayerId = PS->GetPlayerId();
				if (!PlayerIdAssignmentMap.Contains(UEPlayerId))
				{
					++PlayerIdCounter;
					PlayerIdAssignmentMap.Add(UEPlayerId, PlayerIdCounter);
					FString InitTime = MatchStartTime.IsEmpty() ? FDateTime::UtcNow().ToIso8601() : MatchStartTime;
					PlayerLastCommandTimeMap.Add(UEPlayerId, InitTime);
				}
				int32 AssignedId = PlayerIdAssignmentMap[UEPlayerId];
				bool bIsCommander = (AssignedId == 1);

				PlayerInfo.player_id = FString::FromInt(AssignedId);
				PlayerInfo.player_name = PS->GetPlayerName();
				PlayerInfo.player_type = bIsCommander ? TEXT("COMMANDER") : TEXT("DRIVER");
				PlayerInfo.role = AssignedId; // 1=COMMANDER, 2=DRIVER1, 3=DRIVER2, 4=DRIVER3
				PlayerInfo.position.x = PlayerLocation.X;
				PlayerInfo.position.y = PlayerLocation.Y;
				PlayerInfo.position.z = PlayerLocation.Z;
				
				FVector Velocity = Pawn->GetVelocity();
				PlayerInfo.velocity.x = Velocity.X;
				PlayerInfo.velocity.y = Velocity.Y;

				PlayerInfo.hp = 100.0f;
				PlayerInfo.is_boosting = false;

				float CurrentFuel = 100.0f;
				float MaxFuel = 100.0f;
				float CurrentHealth = 100.0f;
				float MaxHealth = 100.0f;

				// VehicleDemoCejCar로 캐스팅하여 AttributeSet 접근
				if (AVehicleDemoCejCar* VehicleCar = Cast<AVehicleDemoCejCar>(Pawn))
				{
					// AttributeSet에서 연료 및 체력 가져오기
					if (UAbilitySystemComponent* ASC = VehicleCar->GetAbilitySystemComponent())
					{
						if (UASRacer* RacerAS = Cast<UASRacer>(VehicleCar->GetAttributeSet()))
						{
							CurrentFuel = RacerAS->GetFuel();
							MaxFuel = RacerAS->GetMaxFuel();
							CurrentHealth = RacerAS->GetHealth();
							MaxHealth = RacerAS->GetMaxHealth();

							PlayerInfo.hp = CurrentHealth;
						}
					}

					// 부스터 사용 여부 확인
					if (UBoostComponent* BoostComp = VehicleCar->GetBoost())
					{
						EBoostState CurrentBoostState = BoostComp->GetBoostState();
						PlayerInfo.is_boosting = (CurrentBoostState == EBoostState::Active);

						// 디버깅: 부스터 상태 로깅
						static int32 LogCounter = 0;
						if (LogCounter++ % 30 == 0) // 30프레임마다 로깅 (약 0.5초)
						{
							FString StateStr;
							switch (CurrentBoostState)
							{
								case EBoostState::Ready:    StateStr = TEXT("Ready"); break;
								case EBoostState::Active:   StateStr = TEXT("Active"); break;
								case EBoostState::Cooldown: StateStr = TEXT("Cooldown"); break;
								case EBoostState::Depleted: StateStr = TEXT("Depleted"); break;
								default: StateStr = TEXT("Unknown"); break;
							}

							float BoostFuel = BoostComp->GetBoostFuel();
							float Speed = VehicleCar->GetChaosWheeledVehicleMovement() ?
								VehicleCar->GetChaosWheeledVehicleMovement()->GetForwardSpeed() * 0.036f : 0.0f;

							UE_LOG(LogEnemyAI, Log, TEXT("[BoostDebug] Player:%s | State:%s | Fuel:%.1f%% | Speed:%.1f km/h | is_boosting:%s"),
								*PS->GetPlayerName(), *StateStr, BoostFuel, Speed, PlayerInfo.is_boosting ? TEXT("TRUE") : TEXT("FALSE"));
						}
					}
				}

				PlayerInfo.recent_action = TEXT("normal_drive"); // 기본값
				PlayerInfo.events.Empty();

				// 1. 연료 부족 이벤트 (FUEL_LOW)
				float FuelPercent = (MaxFuel > 0.0f) ? (CurrentFuel / MaxFuel) : 1.0f;
				if (FuelPercent <= 0.3f) // 30% 이하
				{
					FPlayerEvent2 FuelEvent;
					FuelEvent.event_code = 100; // FUEL_LOW
					FuelEvent.event_type = TEXT("FUEL_LOW");

					if (FuelPercent <= 0.1f)
					{
						FuelEvent.severity = TEXT("CRITICAL");
					}
					else if (FuelPercent <= 0.2f)
					{
						FuelEvent.severity = TEXT("HIGH");
					}
					else
					{
						FuelEvent.severity = TEXT("MEDIUM");
					}

					FuelEvent.value = CurrentFuel;
					FuelEvent.threshold = MaxFuel * 0.3f; // 30%

					PlayerInfo.events.Add(FuelEvent);
				}

				// 2. 체력 부족 이벤트 (HP_LOW)
				float HealthPercent = (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 1.0f;
				if (HealthPercent <= 0.5f) // 50% 이하
				{
					FPlayerEvent2 HealthEvent;
					HealthEvent.event_code = 101; // HP_LOW
					HealthEvent.event_type = TEXT("HP_LOW");

					if (HealthPercent <= 0.2f)
					{
						HealthEvent.severity = TEXT("CRITICAL");
					}
					else if (HealthPercent <= 0.35f)
					{
						HealthEvent.severity = TEXT("HIGH");
					}
					else
					{
						HealthEvent.severity = TEXT("MEDIUM");
					}

					HealthEvent.value = CurrentHealth;
					HealthEvent.threshold = MaxHealth * 0.5f; // 50%

					PlayerInfo.events.Add(HealthEvent);
				}

				// 3. 위험도 높음 이벤트 (RISK_HIGH)
				// Capture Gauge를 기반으로 판단 (계산된 값 사용)
				// TODO: Capture Gauge 값을 PlayerInfo에 추가하거나 별도로 계산

				// ========================================================================
				// 장착 룬 목록 생성 (v1.2.0+)
				// ========================================================================
				PlayerInfo.equipped_runes.Empty();

				// 부스터 사용 중이면 "boost" 룬 추가
				if (PlayerInfo.is_boosting)
				{
					FEquippedRune BoostRune;
					BoostRune.rune_id = TEXT("boost_active");
					BoostRune.name = TEXT("Boost");
					BoostRune.duration_remaining = 5.0f; // TODO: 실제 남은 시간 계산

					PlayerInfo.equipped_runes.Add(BoostRune);
				}

				// 지휘관 상호작용 정보 - 실제 마지막 지시 시각 사용 (항상 현재시각 전송 시 AI 방치 감지 불가)
				{
					FString LastCmdTime = MatchStartTime.IsEmpty() ? FDateTime::UtcNow().ToIso8601() : MatchStartTime;
					if (FString* Mapped = PlayerLastCommandTimeMap.Find(UEPlayerId))
					{
						LastCmdTime = *Mapped;
					}
					PlayerInfo.commander_interaction.last_voice_command_at = LastCmdTime;
					PlayerInfo.commander_interaction.last_button_command_at = LastCmdTime;
				}

				// NavMesh 비용 계산
				if (NavSys && !PixelLocation.IsZero())
				{
					/*FPathFindingQuery Query;
					Query.StartLocation = PixelLocation;
					Query.EndLocation = PlayerLocation;
					Query.NavData = NavSys->GetDefaultNavDataInstance();
					Query.Owner = this;

					FPathFindingResult Result = NavSys->FindPathSync(Query);

					if (Result.IsSuccessful())
					{
						PlayerInfo.navmesh_cost_to_pacman = Result.Path->GetCost();
						PlayerInfo.navmesh_distance_to_pacman = Result.Path->GetLength();
						PlayerInfo.distance_to_pacman = FVector::Dist(PixelLocation, PlayerLocation); 
						PlayerInfo.navmesh_path_valid = true;
					}
					else
					{
						// 경로 없음
						PlayerInfo.navmesh_cost_to_pacman = -1.0f;
						PlayerInfo.navmesh_distance_to_pacman = -1.0f;
						PlayerInfo.distance_to_pacman = FVector::Dist(PixelLocation, PlayerLocation);
						PlayerInfo.navmesh_path_valid = false;
					}*/

					
					/*실패 조건
					* NavSys가 null이거나 PixelLocation이 (0,0,0)인 경우
					* PixelLocation 또는 PlayerLocation이 NavMesh 영역 밖에 있는 경우 (FindPathSync는 위치를 자동으로 투영하지 않음)
					* NavMesh 데이터가 없거나 경로가 존재하지 않는 경우*/

					// 시작 위치와 끝 위치를 NavMesh 위로 투영 (NavSystemDataComponent처럼)
		            FNavLocation StartNavLoc, EndNavLoc;
		            bool bStartOnNavMesh = NavSys->ProjectPointToNavigation(PixelLocation, StartNavLoc,
		            	FVector(1000.0f, 1000.0f, 5000.0f));
		            bool bEndOnNavMesh = NavSys->ProjectPointToNavigation(PlayerLocation, EndNavLoc,
		            	FVector(1000.0f, 1000.0f, 5000.0f));

		            if (bStartOnNavMesh && bEndOnNavMesh)
		            {
		                FPathFindingQuery Query;
		                Query.StartLocation = StartNavLoc.Location;  // 투영된 위치 사용
		                Query.EndLocation = EndNavLoc.Location;      // 투영된 위치 사용
		                Query.NavData = NavSys->GetDefaultNavDataInstance();
		                Query.Owner = this;

		                FPathFindingResult Result = NavSys->FindPathSync(Query);

		                if (Result.IsSuccessful() && Result.Path.IsValid())
		                {
		                    PlayerInfo.navmesh_cost_to_pacman = Result.Path->GetCost();
		                    PlayerInfo.navmesh_distance_to_pacman = Result.Path->GetLength() * 0.01f;  // cm to m
		                    PlayerInfo.distance_to_pacman = FVector::Dist(PixelLocation, PlayerLocation) * 0.01f;  // cm to m
		                    PlayerInfo.navmesh_path_valid = true;
		                }
		                else
		                {
		                    // 경로 없음
		                    PlayerInfo.navmesh_cost_to_pacman = -1.0f;
		                    PlayerInfo.navmesh_distance_to_pacman = -1.0f;
		                    PlayerInfo.distance_to_pacman = FVector::Dist(PixelLocation, PlayerLocation) * 0.01f;
		                    PlayerInfo.navmesh_path_valid = false;
		                }
		            }
		            else
		            {
		                // 투영 실패
		                PlayerInfo.distance_to_pacman = FVector::Dist(PixelLocation, PlayerLocation) * 0.01f;
		                PlayerInfo.navmesh_cost_to_pacman = PlayerInfo.distance_to_pacman;  // 직선 거리로 대체//-1.0f;
		                PlayerInfo.navmesh_distance_to_pacman = PlayerInfo.distance_to_pacman; //-1.0f;
		                PlayerInfo.navmesh_path_valid = false;
		            	//*NavMesh 빌드**: 에디터에서 NavMesh Bounds Volume이 레벨을 커버하는지, RecastNavMesh 액터가 제대로 설정되었는지 확인
		            	//UE_LOG를 추가하여 bStartOnNavMesh, bEndOnNavMesh 값을 로깅하고, PixelLocation과 PlayerLocation이 NavMesh 영역 내에 있는지 검증
						//단위 변환**: navmesh_distance_to_pacman은 m 단위로 변환해야 하므로 * 0.01f를 적용
		            }

				}
				else
				{
					// NavSystem 없거나 Pacman 위치 모름
					PlayerInfo.distance_to_pacman = FVector::Dist(PixelLocation, PlayerLocation);
					PlayerInfo.navmesh_cost_to_pacman = PlayerInfo.distance_to_pacman; // 직선 거리로 대체
					PlayerInfo.navmesh_distance_to_pacman = PlayerInfo.distance_to_pacman;
					PlayerInfo.navmesh_path_valid = false;
				}

				if (PlayerInfo.player_type == TEXT("DRIVER") && NavDataComponent)
				{
					if (AGameStateBase* GameState = World->GetGameState())
					{
						const TArray<TObjectPtr<APlayerState>>& PlayerArray = GameState->PlayerArray;
						int32 PlayerArrayIndex = PlayerArray.IndexOfByKey(PS);

						UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] DRIVER PlayerArrayIndex: %d"), PlayerArrayIndex);

						// PlayerArrayIndex가 유효한 범위인지 확인 (0-based)
						if (PlayerArrayIndex >= 0 && PlayerArrayIndex < PlayerArray.Num())
						{
							FNavSystemLLMData NavData = NavDataComponent->GetLLMData();

							UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] NavData.RacerData.Num(): %d"), NavData.RacerData.Num());

							bool bFoundMatch = false;
							for (const FRacerNavigationData& RacerData : NavData.RacerData)
							{
								UE_LOG(LogEnemyAI, Verbose, TEXT("[EnemyAISubsystem] Comparing RacerData.PlayerIndex=%d with PlayerArrayIndex=%d"), RacerData.PlayerIndex, PlayerArrayIndex);

								if (RacerData.PlayerIndex == PlayerArrayIndex)
								{
									bFoundMatch = true;
									// PlayerArrayIndex가 이미 0-based이므로 그대로 사용
									PlayerInfo.racer_nav_delta.player_index = RacerData.PlayerIndex;
									PlayerInfo.racer_nav_delta.relative_angle_to_pacman_deg = RacerData.RelativeAngleToBoss;

									PlayerInfo.racer_nav_delta.delta_position.x = RacerData.DeltaPosition.X;
									PlayerInfo.racer_nav_delta.delta_position.y = RacerData.DeltaPosition.Y;
									PlayerInfo.racer_nav_delta.delta_position.z = RacerData.DeltaPosition.Z;

									PlayerInfo.racer_nav_delta.delta_straight_distance = RacerData.DeltaStraightDistance;
									PlayerInfo.racer_nav_delta.delta_path_distance = RacerData.DeltaPathDistance;
									PlayerInfo.racer_nav_delta.delta_path_cost = RacerData.DeltaPathCost;
									PlayerInfo.racer_nav_delta.movement_direction_change_deg = RacerData.MovementDirectionChange;
									PlayerInfo.racer_nav_delta.average_speed = RacerData.AverageSpeed;
									PlayerInfo.racer_nav_delta.relative_bearing_change_deg = RacerData.RelativeBearingChange;

									UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Matched! Delta Position: (%.1f, %.1f, %.1f), Delta Straight: %.1f"),
										RacerData.DeltaPosition.X, RacerData.DeltaPosition.Y, RacerData.DeltaPosition.Z, RacerData.DeltaStraightDistance);

									break;
								}
							}

							if (!bFoundMatch)
							{
								UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] No matching RacerData found for PlayerArrayIndex=%d"), PlayerArrayIndex);
							}
						}
						else
						{
							UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] PlayerArrayIndex %d is out of range (PlayerArray.Num: %d)"),
								PlayerArrayIndex, PlayerArray.Num());
						}
					}
				}

				Request.player_team_context.Add(PlayerInfo);
			}
		}
	}

	// NavMesh Cost Min/Avg 계산 (Player Team Context 준비 완료 후)
	float NavCostMin = -1.0f;
	float NavCostAvg = -1.0f;

	TArray<float> ValidNavCosts;
	for (const FPlayerTeamContext2& Driver : Request.player_team_context)
	{
		// 유효한 NavMesh 경로만 수집
		if (Driver.navmesh_path_valid && Driver.navmesh_cost_to_pacman > 0.0f)
		{
			ValidNavCosts.Add(Driver.navmesh_cost_to_pacman);
		}
	}

	if (ValidNavCosts.Num() > 0)
	{
		// Min 값 찾기
		NavCostMin = FMath::Min(ValidNavCosts);

		// Avg 값 계산
		float TotalCost = 0.0f;
		for (float Cost : ValidNavCosts)
		{
			TotalCost += Cost;
		}
		NavCostAvg = TotalCost / ValidNavCosts.Num();
	}
	else
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] No valid NavMesh paths found for NavCost calculation"));
	}

	// Request에 설정 //v1.4삭제됨
	//Request.ai_squad_context.pacman_main.nav_cost_min = NavCostMin;
	//Request.ai_squad_context.pacman_main.nav_cost_avg = NavCostAvg;

	// Capture Gauge는 이미 Line 370에서 State.capture_gauge로 설정됨 (CaptureGaugeComponent 사용)

	// Commander Context (더미 데이터)
	Request.commander_context.commander_id = TEXT("commander_dummy");
	Request.commander_context.recent_actions.Empty();
	// cash_status, management_stats는 기본값 사용

	// Map Context
	// Pacman Spawn (PixelEnemy 시작 위치 - 런타임 위치 사용)
	// AI 서버 프로토콜에 따라 pacman_spawn 필드는 초기 스폰 위치 또는 현재 위치를 의미할 수 있으나,
	// 여기서는 요청 시점의 PixelEnemy 위치를 전송하여 서버가 맵 상의 기준점으로 삼도록 함.
	if (PixelEnemy.IsValid())
	{
		PixelLocation = PixelEnemy->GetActorLocation();
	}
	else
	{
		PixelLocation = FVector::ZeroVector;
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] PixelEnemy 없음 is invalid during SendBatchDecisionRequest!"));
	}

	if (!PixelLocation.IsZero())
	{
		Request.map_context.pacman_spawn.x = PixelLocation.X;
		Request.map_context.pacman_spawn.y = PixelLocation.Y;
		Request.map_context.pacman_spawn.z = PixelLocation.Z;
	}
	else
	{
		// PixelEnemy를 찾지 못한 경우 (0,0,0)으로 초기화되지 않도록 명시적 NULL 설정은 불가하므로 0으로 설정하되 로그 경고
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] PixelLocation is Zero! Sending (0,0,0) as pacman_spawn"));
		Request.map_context.pacman_spawn.x = 0.0f;
		Request.map_context.pacman_spawn.y = 0.0f;
		Request.map_context.pacman_spawn.z = 0.0f;
	}

	Request.map_context.p_pellet_locations.Empty();

	{
		const auto& CachedSpawners = GetCachedPelletSpawners();
		for (const TWeakObjectPtr<APelletSpawner>& SpawnerPtr : CachedSpawners)
		{
			if (APelletSpawner* PelletSpawner = SpawnerPtr.Get())
			{
				TArray<FPPelletLocation> PPellets = PelletSpawner->CollectPPelletData();
				Request.map_context.p_pellet_locations.Append(PPellets);
			}
		}
	}

	// P-Point (코인) 위치 수집
	Request.map_context.p_point_locations.Empty();
	CoinActorMap.Empty(); // 매 요청마다 초기화

	{
		const auto& CachedCoinList = GetCachedCoins();

		int32 CoinIndex = 1;

		// 1) 아직 획득되지 않은 코인들 (available = true)
		for (const TWeakObjectPtr<ACoinActor>& CoinPtr : CachedCoinList)
		{
			ACoinActor* Coin = CoinPtr.Get();
			if (IsValid(Coin))
			{
				FVector CoinLocation = Coin->GetActorLocation();

				// Coin ID 할당 (아직 없으면)
				FString CoinID = Coin->GetCoinID();
				if (CoinID.IsEmpty())
				{
					CoinID = FString::Printf(TEXT("p_point_%d"), CoinIndex);
					Coin->SetCoinID(CoinID);
				}

				// Coin ID -> Actor 매핑 저장
				CoinActorMap.Add(CoinID, Coin);

				FPPointLocation PPoint;
				PPoint.id = CoinID;
				PPoint.x = CoinLocation.X;
				PPoint.y = CoinLocation.Y;
				PPoint.z = CoinLocation.Z;
				PPoint.available = true; // 아직 획득 가능
				PPoint.cooldown = 0.f;
				Request.map_context.p_point_locations.Add(PPoint);
				CoinIndex++;
			}
		}

		// 2) 이미 획득된 코인들 (available = false)
		for (const TPair<FString, FVector>& CollectedPair : CollectedCoinLocations)
		{
			const FString& CoinID = CollectedPair.Key;
			const FVector& CoinLocation = CollectedPair.Value;

			FPPointLocation PPoint;
			PPoint.id = CoinID;
			PPoint.x = CoinLocation.X;
			PPoint.y = CoinLocation.Y;
			PPoint.z = CoinLocation.Z;
			PPoint.available = false; // 이미 획득됨
			PPoint.cooldown = 0.f;
			Request.map_context.p_point_locations.Add(PPoint);
		}

		UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Collected %d P-Points for AI server (%d available, %d collected)"),
		       Request.map_context.p_point_locations.Num(), CachedCoinList.Num(), CollectedCoinLocations.Num());
	}

	// JSON 직렬화
	FString JsonBody;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Request, JsonBody))
	{
		UE_LOG(LogEnemyAI, Error, TEXT("[EnemyAISubsystem] Failed to serialize batch request"));
		return;
	}

	// [수정] Request Num을 로그 파일명에 포함
	FString RequestNum = Request.request_num;
	SaveJsonLog(FString::Printf(TEXT("request_%s"), *RequestNum), JsonBody);

	LastDecisionRequest = Request;
	bIsRequestPending = true;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 3.0f, FColor::Red,
			FString::Printf(TEXT("[AI Server|EnemyAISubsystem] (%d enemies)"), RegisteredEnemies.Num()));
	}

	TWeakObjectPtr<UEnemyAISubsystem> WeakThis(this);

	//최종 서버 전송 데이터 
	SendPostRequest(DecisionEndpoint, JsonBody, [WeakThis, RequestNum](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (!WeakThis.IsValid())
		{
			return;
		}

		UEnemyAISubsystem* StrongThis = WeakThis.Get();

		StrongThis->bIsRequestPending = false;

		if (!bWasSuccessful || !Response.IsValid())
		{
			UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Batch request failed (timeout or network error)"));

			// 연결 상태 추적: 실패 카운터 증가
			StrongThis->ConsecutiveFailureCount++;
			if (StrongThis->ConsecutiveFailureCount >= DisconnectThreshold && StrongThis->IsConnected())
			{
				StrongThis->SetConnected(false);
			}

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(1, 3.0f, FColor::Red,
					TEXT("[AI Server] Request FAILED"));
			}

			// 네트워크 실패 시 폴백 시도
			if (StrongThis->bHasSuccessfulResponse)
			{
				// 1순위: 마지막 성공 응답 재사용
				UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Network failure - Using last successful response as fallback"));
				StrongThis->HandleDecisionResponse(StrongThis->LastSuccessfulResponse);

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(1, 3.0f, FColor::Yellow,
						TEXT("[AI Server] FALLBACK: Using last action pattern"));
				}
			}
			else
			{
				// 2순위: 기본 폴백 행동 (Racer 추격/공격)
				UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Network failure - No previous response, creating fallback CHASE command"));
				FGetDecisionResponse2 FallbackResponse = StrongThis->CreateFallbackAttackResponse();
				StrongThis->HandleDecisionResponse(FallbackResponse);

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(1, 3.0f, FColor::Orange,
						TEXT("[AI Server] FALLBACK: Chasing nearest Racer"));
				}
			}

			return;
		}

		const int32 StatusCode = Response->GetResponseCode();
		const FString Content = Response->GetContentAsString();

		// HTTP 422 에러: 서버가 요청을 거부한 경우 (Unprocessable Entity)
		if (StatusCode == 422)
		{
			UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] HTTP 422 Error - Server cannot process request"));
			UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Response: %s"), *Content);

			// 연결 상태 추적: 실패 카운터 증가
			StrongThis->ConsecutiveFailureCount++;
			if (StrongThis->ConsecutiveFailureCount >= DisconnectThreshold && StrongThis->IsConnected())
			{
				StrongThis->SetConnected(false);
			}

			// 저장된 성공 응답이 있으면 재사용
			if (StrongThis->bHasSuccessfulResponse)
			{
				// 1순위: 마지막 성공 응답 재사용
				UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Using last successful response as fallback (commands: %d)"),
					StrongThis->LastSuccessfulResponse.decision.unit_commands.Num());

				StrongThis->HandleDecisionResponse(StrongThis->LastSuccessfulResponse);

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(1, 5.0f, FColor::Yellow,
						FString::Printf(TEXT("[AI Server] HTTP 422 FALLBACK (%d commands)"),
							StrongThis->LastSuccessfulResponse.decision.unit_commands.Num()));
				}
			}
			else
			{
				// 2순위: 기본 폴백 행동 (Racer 추격/공격)
				UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] HTTP 422 - No previous response, creating fallback CHASE command"));
				FGetDecisionResponse2 FallbackResponse = StrongThis->CreateFallbackAttackResponse();
				StrongThis->HandleDecisionResponse(FallbackResponse);

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(1, 5.0f, FColor::Orange,
						TEXT("[AI Server] HTTP 422 FALLBACK: Chasing nearest Racer"));
				}
			}

			return;
		}

		// HTTP 200 성공
		if (StatusCode == 200)
		{
			// 연결 상태 추적: 성공 시 리셋
			StrongThis->ConsecutiveFailureCount = 0;
			if (!StrongThis->IsConnected()) { StrongThis->SetConnected(true); }

			// [Debug] 응답 로그 저장 (request_num 사용)
			StrongThis->SaveJsonLog(FString::Printf(TEXT("response_%s"), *RequestNum), Content);

			// 응답 JSON 로깅 (디버깅용)
			//UE_LOG(LogEnemyAI, Verbose, TEXT("[EnemyAISubsystem] Response JSON: %s"), *Content);

			FGetDecisionResponse2 DecisionResponse;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(Content, &DecisionResponse, 0, 0))
			{
				/*UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Batch decision received - Commands: %d"),
					DecisionResponse.decision.unit_commands.Num());*/

				// 화면에 응답 성공 표시
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(1, 3.0f, FColor::Cyan,
						FString::Printf(TEXT("[AI Server] Response OK (%d commands)"), DecisionResponse.decision.unit_commands.Num()));
				}

				// 각 명령의 내용 로깅
				/*for (const FUnitCommand& Cmd : DecisionResponse.decision.unit_commands)
				{
					UE_LOG(LogEnemyAI, Verbose, TEXT("  - unit_id: %s, directive: %d (%s), target: (%.1f, %.1f, %.1f)"),
						*Cmd.unit_id, Cmd.directive_code, *Cmd.directive_name,
						Cmd.params.target_position.x, Cmd.params.target_position.y, Cmd.params.target_position.z);
				}*/

				StrongThis->HandleDecisionResponse(DecisionResponse);
			}
			else
			{
				UE_LOG(LogEnemyAI, Error, TEXT("[EnemyAISubsystem] Failed to parse batch response"));
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(1, 3.0f, FColor::Red,
						TEXT("[AI Server] 화면에 파싱 실패 표시 Response PARSE FAILED"));
				}
			}
		}
		else
		{
			UE_LOG(LogEnemyAI, Error, TEXT("[EnemyAISubsystem] 기타 HTTP 에러 %d: %s"), StatusCode, *Content);

			// 연결 상태 추적: 실패 카운터 증가
			StrongThis->ConsecutiveFailureCount++;
			if (StrongThis->ConsecutiveFailureCount >= DisconnectThreshold && StrongThis->IsConnected())
			{
				StrongThis->SetConnected(false);
			}

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(1, 3.0f, FColor::Red,
					FString::Printf(TEXT("[AI Server] HTTP Error %d"), StatusCode));
			}
		}
	});
}

void UEnemyAISubsystem::HandleDecisionResponse(const FGetDecisionResponse2& Response)
{
	/*UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] HandleDecisionResponse - request_num: %s, commands: %d"),
		*Response.request_num, Response.decision.unit_commands.Num());*/

	if (!ValidateResponseOrder(Response.request_num))
	{
		//UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Response validation FAILED - discarding"));
		return;
	}

	//UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Response validation PASSED - processing commands"));

	int32 CloneMissingCount = 0;
	TArray<FVector> CloneTargetPositions;

	for (const FUnitCommand& Command : Response.decision.unit_commands)
	{
		/*UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Processing command - unit_id: %s, directive: %d (%s)"),
			*Command.unit_id, Command.directive_code, *Command.directive_name);*/

		TScriptInterface<IAIDecisionReceiver> Enemy = FindEnemyByID(Command.unit_id);

		if (Enemy.GetInterface())
		{
			/*UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Enemy found - calling OnAIDecisionReceived for %s"),
				*Command.unit_id);*/
			Enemy->OnAIDecisionReceived(Command);
		}
		else
		{
			if (Command.unit_id.StartsWith(TEXT("clone_")))
			{
				CloneMissingCount++;
				FVector TargetPos(
					Command.params.target_position.x,
					Command.params.target_position.y,
					Command.params.target_position.z
				);

				// (0,0,0)이 아닌 유효한 위치만 수집
				if (!TargetPos.IsNearlyZero(1.0f))
				{
					CloneTargetPositions.Add(TargetPos);
					/*UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Clone missing: %s - Target: %s"),
					       *Command.unit_id, *TargetPos.ToString());*/
				}
				else
				{
					UE_LOG(LogEnemyAI, Verbose, TEXT("[EnemyAISubsystem] Clone missing: %s - Invalid target (0,0,0)"),
					       *Command.unit_id);
				}
			}
			else
			{
				UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Enemy not found for command: %s"),
				       *Command.unit_id);
			}
		}
	}

	// clone이 2개 이상 없으면 pacman_main을 coin spawn 위치로 이동
	if (CloneMissingCount >= 2)
	{
		TScriptInterface<IAIDecisionReceiver> PacmanMain = FindEnemyByID(TEXT("pacman_main"));

		if (PacmanMain.GetInterface())
		{
			FVector TargetCoinSpawn;

			// clone의 유효한 목표 위치가 있으면 가장 가까운 곳 선택
			if (CloneTargetPositions.Num() > 0)
			{
				FEnemyGameState PacmanState = PacmanMain->GetCurrentGameState();
				FVector PacmanPos = PacmanState.position;

				TargetCoinSpawn = CloneTargetPositions[0];
				float MinDistance = FVector::Dist(PacmanPos, TargetCoinSpawn);

				for (int32 i = 1; i < CloneTargetPositions.Num(); i++)
				{
					float Distance = FVector::Dist(PacmanPos, CloneTargetPositions[i]);
					if (Distance < MinDistance)
					{
						MinDistance = Distance;
						TargetCoinSpawn = CloneTargetPositions[i];
					}
				}
			}
			else // clone 목표가 없으면 고정된 coin spawn 위치 사용
			{
				TargetCoinSpawn = FVector(5190.0f, 45450.0f, 500.0f);
				//UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] No valid clone targets, using default coin spawn"));
			}

			// coin spawn 위치로 이동 명령 생성
			FUnitCommand FallbackCommand;
			FallbackCommand.unit_id = TEXT("pacman_main");
			FallbackCommand.directive_code = 2; // MOVE_TO_LOCATION
			FallbackCommand.directive_name = TEXT("MOVE_TO_COIN_SPAWN");
			FallbackCommand.params.target_position.x = TargetCoinSpawn.X;
			FallbackCommand.params.target_position.y = TargetCoinSpawn.Y;
			FallbackCommand.params.target_position.z = TargetCoinSpawn.Z;
			FallbackCommand.params.speed_factor = 1.0f;

			PacmanMain->OnAIDecisionReceived(FallbackCommand);

			/*UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Fallback: %d clones missing, moving to coin spawn at %s"),
				CloneMissingCount, *TargetCoinSpawn.ToString());*/

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(2, 5.0f, FColor::Orange,
					FString::Printf(TEXT("[Fallback] %d clones missing -> Coin spawn (%.0f, %.0f)"),
						CloneMissingCount, TargetCoinSpawn.X, TargetCoinSpawn.Y));
			}
		}
	}

	// Brain Cam 데이터 브로드캐스트
	if (!Response.brain_cam_data.perception.summary.IsEmpty() || !Response.brain_cam_data.decision.final_choice.IsEmpty())
	{
		// FRetrievedDoc -> FString 변환
		TArray<FString> ReasoningDocs;
		for (const FRetrievedDoc& Doc : Response.brain_cam_data.reasoning.retrieved_docs)
		{
			// "Title (ID, Similarity)" 형식으로 변환
			FString DocString = FString::Printf(TEXT("%s (%.2f)"), *Doc.title, Doc.similarity);
			ReasoningDocs.Add(DocString);
		}

		OnBrainCamDataReceived.Broadcast(Response.brain_cam_data.perception.summary, Response.brain_cam_data.decision.final_choice, ReasoningDocs);
	}
	OnDecisionReceived.Broadcast(TEXT(""), Response);

	// HTTP 422 폴백을 위해 성공한 응답 저장
	LastSuccessfulResponse = Response;
	bHasSuccessfulResponse = true;
}

void UEnemyAISubsystem::StartMatch(const FString& InMatchStartTime)
{
	MatchStartTime = InMatchStartTime;

	// 매치마다 player ID 카운터 리셋
	PlayerIdCounter = 0;
	PlayerIdAssignmentMap.Empty();
	PlayerLastCommandTimeMap.Empty();

	UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] StartMatch called - MatchStartTime: %s"), *MatchStartTime);

	if (RegisteredEnemies.Num() == 0)
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] StartMatch called but NO ENEMIES REGISTERED! AI will not function correctly."));
	}
	else
	{
		UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] StartMatch - Starting requests for %d enemies."), RegisteredEnemies.Num());
	}

	// POST /api/v1/match/start
	SendMatchStartToServer();

	StartAutoDecisionRequests();
}

void UEnemyAISubsystem::StartAutoDecisionRequests(float IntervalSeconds)
{
	if (!GetWorld())
	{
		//UE_LOG(LogEnemyAI, Error, TEXT("[EnemyAISubsystem] Cannot start auto requests - Invalid World"));
		return;
	}

	StopAutoDecisionRequests();

	//UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Starting auto decision requests - Interval: %.2f seconds"),IntervalSeconds);

	GetWorld()->GetTimerManager().SetTimer(
		DecisionTimer,
		[this]()
		{
			RequestBatchDecision();
		},
		IntervalSeconds,
		true // 반복
	);
	
	RequestBatchDecision();
}

// 기존 타이머 정리
void UEnemyAISubsystem::StopAutoDecisionRequests()
{
	if (!GetWorld())
	{
		return;
	}

	if (DecisionTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(DecisionTimer);
		//UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Stopped auto decision requests"));
	}
}

// 현재 Room ID 반환 (USessionCodeWidget에서 읽거나 기본값)
FString UEnemyAISubsystem::GetCurrentRoomId() const
{
	FString RoomId = TEXT("room_ue");

	if (UWorld* World = GetWorld())
	{
		for (TObjectIterator<USessionCodeWidget> It; It; ++It)
		{
			if (It->GetWorld() == World)
			{
				if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(USessionCodeWidget::StaticClass(), TEXT("sessionCodeText")))
				{
					if (UTextBlock* TextBlock = Cast<UTextBlock>(Prop->GetObjectPropertyValue_InContainer(*It)))
					{
						if (!TextBlock->GetText().IsEmpty())
						{
							RoomId = TextBlock->GetText().ToString();
							break;
						}
					}
				}
			}
		}
	}

	return RoomId;
}

// POST /api/v1/match/start 전송
void UEnemyAISubsystem::SendMatchStartToServer()
{
	if (!GetWorld()) return;

	FMatchStartRequest2 StartRequest;
	StartRequest.room_id = GetCurrentRoomId();
	StartRequest.match_start_time = MatchStartTime.IsEmpty() ? FDateTime::UtcNow().ToIso8601() : MatchStartTime;
	StartRequest.mode = TEXT("CASUAL");

	// 플레이어 순번 배정 (순번 1 = COMMANDER, 2~4 = DRIVER)
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->PlayerState) continue;

		int32 UEId = PC->PlayerState->GetPlayerId();
		if (!PlayerIdAssignmentMap.Contains(UEId))
		{
			++PlayerIdCounter;
			PlayerIdAssignmentMap.Add(UEId, PlayerIdCounter);
			FString InitTime = MatchStartTime.IsEmpty() ? FDateTime::UtcNow().ToIso8601() : MatchStartTime;
			PlayerLastCommandTimeMap.Add(UEId, InitTime);
		}
	}

	// 순번 기준 정렬 후 배열 구성 (0=COMMANDER, 1~3=DRIVER)
	TArray<TPair<int32, APlayerState*>> SortedPlayers;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->PlayerState) continue;

		int32 UEId = PC->PlayerState->GetPlayerId();
		if (int32* AssignedId = PlayerIdAssignmentMap.Find(UEId))
		{
			SortedPlayers.Add(TPair<int32, APlayerState*>(*AssignedId, PC->PlayerState));
		}
	}
	SortedPlayers.Sort([](const TPair<int32, APlayerState*>& A, const TPair<int32, APlayerState*>& B)
	{
		return A.Key < B.Key;
	});

	for (const TPair<int32, APlayerState*>& Pair : SortedPlayers)
	{
		int32 AssignedId = Pair.Key;
		APlayerState* PS = Pair.Value;
		bool bIsCommander = (AssignedId == 1);

		StartRequest.player_ids.Add(FString::FromInt(AssignedId));
		StartRequest.player_names.Add(PS->GetPlayerName());
		StartRequest.player_types.Add(bIsCommander ? TEXT("COMMANDER") : TEXT("DRIVER"));
	}

	FString JsonBody;
	if (!FJsonObjectConverter::UStructToJsonObjectString(StartRequest, JsonBody))
	{
		UE_LOG(LogEnemyAI, Error, TEXT("[EnemyAISubsystem] match/start 직렬화 실패"));
		return;
	}

	UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] POST /api/v1/match/start (room:%s, players:%d)"), *StartRequest.room_id, StartRequest.player_ids.Num());

	TWeakObjectPtr<UEnemyAISubsystem> WeakThis(this);
	SendPostRequest(TEXT("/api/v1/match/start"), JsonBody, [WeakThis](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (!WeakThis.IsValid()) return;

		int32 Code = (Response.IsValid()) ? Response->GetResponseCode() : 0;
		if (bWasSuccessful && Code == 200)
		{
			UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] match/start 성공 (HTTP 200)"));
		}
		else
		{
			UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] match/start 실패 (HTTP %d)"), Code);
		}
	});
}

// POST /api/v1/match/end 전송
void UEnemyAISubsystem::SendMatchEndToServer(const FString& Result, const FString& EndReason)
{
	FMatchEndRequest2 EndRequest;
	EndRequest.room_id = GetCurrentRoomId();
	EndRequest.result = Result;
	EndRequest.winner_team = TEXT("PLAYERS");
	EndRequest.end_reason = EndReason;
	EndRequest.match_end_time = FDateTime::UtcNow().ToIso8601();

	if (!MatchStartTime.IsEmpty())
	{
		FDateTime StartTime;
		if (FDateTime::ParseIso8601(*MatchStartTime, StartTime))
		{
			FTimespan Elapsed = FDateTime::UtcNow() - StartTime;
			EndRequest.match_duration_seconds = FMath::FloorToInt(Elapsed.GetTotalSeconds());
		}
	}

	FString JsonBody;
	if (!FJsonObjectConverter::UStructToJsonObjectString(EndRequest, JsonBody))
	{
		UE_LOG(LogEnemyAI, Error, TEXT("[EnemyAISubsystem] match/end 직렬화 실패"));
		return;
	}

	UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] POST /api/v1/match/end (room:%s, result:%s, reason:%s)"), *EndRequest.room_id, *Result, *EndReason);

	TWeakObjectPtr<UEnemyAISubsystem> WeakThis(this);
	SendPostRequest(TEXT("/api/v1/match/end"), JsonBody, [WeakThis](FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (!WeakThis.IsValid()) return;

		int32 Code = (Response.IsValid()) ? Response->GetResponseCode() : 0;
		if (bWasSuccessful && Code == 200)
		{
			UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] match/end 성공 (HTTP 200)"));
		}
		else
		{
			UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] match/end 실패 (HTTP %d)"), Code);
		}
	});
}

// Capture Gauge 계산 (포획 위험도 게이지)
float UEnemyAISubsystem::CalculateCaptureGauge(
	const FVector& PacmanLocation,
	const FVector& PacmanForward,
	bool bIsInvulnerable,
	const TArray<FPlayerTeamContext2>& PlayerTeam,
	UNavigationSystemV1* NavSys) const
{
	// 무적 상태면 상한 20 (거의 안전)
	const float InvulnerableMaxGauge = 20.0f;

	// 추격 거리 임계값 (cm 단위, 60m = 6000cm)
	const float ChaseDist = 6000.0f;

	// 백어택 각도 범위 (뒤쪽 120도 = 팩맨 뒤 좌우 60도씩)
	const float BackAttackAngle = 60.0f;

	// 위협도 누적 변수
	float ThreatTotal = 0.0f;

	// 뒤쪽 120도 안쪽 추격자 수
	int32 BackAttackCount = 0;

	// 가까운 추격자 수 (60m 이내)
	int32 CloseDriverCount = 0;

	for (const FPlayerTeamContext2& Driver : PlayerTeam)
	{
		FVector DriverLocation(Driver.position.x, Driver.position.y, Driver.position.z);
		float DistanceToPacman = Driver.distance_to_pacman;

		// 60m 이내에 있지 않으면 위협도 낮음
		if (DistanceToPacman > ChaseDist)
		{
			continue;
		}

		CloseDriverCount++;

		// 1. 거리 기반 위협도 (가까울수록 높음)
		// 거리가 0일 때 최대, 6000일 때 최소
		float DistanceThreat = FMath::Clamp(1.0f - (DistanceToPacman / ChaseDist), 0.0f, 1.0f) * 20.0f;

		// 2. NavMesh 비용 기반 위협도 (경로가 짧을수록 높음)
		float NavCostThreat = 0.0f;
		if (Driver.navmesh_path_valid && Driver.navmesh_cost_to_pacman > 0.0f)
		{
			// NavMesh 비용이 낮을수록 위협도 증가
			// 직선 거리 대비 경로 비용 비율
			float CostRatio = Driver.navmesh_cost_to_pacman / FMath::Max(DistanceToPacman, 1.0f);

			// 비율이 1에 가까우면 직선 경로 (위협도 높음)
			// 비율이 높으면 우회 경로 (위협도 낮음)
			NavCostThreat = FMath::Clamp(2.0f - CostRatio, 0.0f, 1.0f) * 15.0f;
		}
		else
		{
			// NavMesh 경로가 없으면 중간 위협도
			NavCostThreat = 5.0f;
		}

		// 3. 백어택 각도 체크 (팩맨 뒤쪽 120도 안쪽)
		FVector ToPacmanDir = (PacmanLocation - DriverLocation).GetSafeNormal();
		float DotProduct = FVector::DotProduct(PacmanForward, ToPacmanDir);

		// DotProduct가 -1에 가까우면 뒤쪽, 1에 가까우면 앞쪽
		// 뒤쪽 120도: DotProduct < -0.5 (120도/2 = 60도, cos(120) = -0.5)
		float BackAttackThreat = 0.0f;
		if (DotProduct < -0.5f)
		{
			BackAttackCount++;
			BackAttackThreat = 20.0f; // 백어택 추가 위협
		}

		// 4. 부스터 사용 중이면 추가 위협
		float BoostThreat = 0.0f;
		if (Driver.is_boosting)
		{
			BoostThreat = 15.0f;
		}

		// 5. 상대 속도 (접근 속도)
		float ApproachThreat = 0.0f;
		FVector DriverVelocity(Driver.velocity.x, Driver.velocity.y, 0.0f);
		FVector ToDriver = (DriverLocation - PacmanLocation).GetSafeNormal();

		// 드라이버가 팩맨을 향해 이동 중인지 확인
		float ApproachDot = FVector::DotProduct(DriverVelocity.GetSafeNormal(), -ToDriver);
		if (ApproachDot > 0.0f) // 팩맨을 향해 이동 중
		{
			ApproachThreat = ApproachDot * 10.0f;
		}

		// 개별 드라이버 위협도 합산
		float DriverThreat = DistanceThreat + NavCostThreat + BackAttackThreat + BoostThreat + ApproachThreat;
		ThreatTotal += DriverThreat;
	}

	// 포위 보정 (여러 방향에서 추격 중)
	float EncirclementBonus = 0.0f;
	if (CloseDriverCount >= 2)
	{
		EncirclementBonus = CloseDriverCount * 10.0f;
	}

	// 백어택 추가 보정
	float BackAttackBonus = 0.0f;
	if (BackAttackCount >= 2)
	{
		BackAttackBonus = BackAttackCount * 15.0f;
	}

	// 최종 위협도
	ThreatTotal += EncirclementBonus + BackAttackBonus;

	// 0~100으로 정규화
	float CaptureGauge = FMath::Clamp(ThreatTotal, 0.0f, 100.0f);

	// 무적 상태면 상한 20
	if (bIsInvulnerable)
	{
		CaptureGauge = FMath::Min(CaptureGauge, InvulnerableMaxGauge);
	}

	/*UE_LOG(LogEnemyAI, Verbose, TEXT("[CaptureGauge] Total: %.1f (Close: %d, BackAttack: %d, Invulnerable: %s)"),
		CaptureGauge, CloseDriverCount, BackAttackCount, bIsInvulnerable ? TEXT("Yes") : TEXT("No"));*/

	return CaptureGauge;
}

// P-Point (Coin) 관리
void UEnemyAISubsystem::MarkPPointAsCollected(const FString& CoinID)
{
	if (CoinID.IsEmpty())
	{
		//UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] MarkPPointAsCollected - Empty CoinID"));
		return;
	}

	// CoinActorMap에서 해당 코인 찾기
	/*if (ACoinActor* CoinPtr = CoinActorMap.Find(CoinID))
	{
		ACoinActor* Coin = *CoinPtr;
		if (IsValid(Coin))
		{
			// 획득 전 위치 저장 (Destroy 후에도 유지)
			FVector CoinLocation = Coin->GetActorLocation();
			CollectedCoinLocations.Add(CoinID, CoinLocation);

			UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] P-Point collected: %s at (%.0f, %.0f, %.0f) - will send available=false"),
				*CoinID, CoinLocation.X, CoinLocation.Y, CoinLocation.Z);
		}
	}
	else
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] MarkPPointAsCollected - CoinID not found in map: %s"), *CoinID);
	}*/

	if (TObjectPtr<ACoinActor> CoinPtr = CoinActorMap.FindRef(CoinID))
	{
		if (IsValid(CoinPtr.Get()))
		{
			FVector CoinLocation = CoinPtr->GetActorLocation();
			CollectedCoinLocations.Add(CoinID, CoinLocation);
		}
	}
	else
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] MarkPPointAsCollected - CoinID not found in map: %s"), *CoinID);
	}
}

// ========== 액터 캐시 Getter ==========
const TArray<TWeakObjectPtr<AVehicleDemoCejCar>>& UEnemyAISubsystem::GetCachedVehicles()
{
	CleanupWeakArray(CachedVehicleArray);
	return CachedVehicleArray;
}

const TArray<TWeakObjectPtr<APelletActor>>& UEnemyAISubsystem::GetCachedPellets()
{
	CleanupWeakArray(CachedPelletArray);
	return CachedPelletArray;
}

const TArray<TWeakObjectPtr<ACoinActor>>& UEnemyAISubsystem::GetCachedCoins()
{
	CleanupWeakArray(CachedCoinArray);
	return CachedCoinArray;
}

const TArray<TWeakObjectPtr<APelletSpawner>>& UEnemyAISubsystem::GetCachedPelletSpawners()
{
	CleanupWeakArray(CachedPelletSpawnerArray);
	return CachedPelletSpawnerArray;
}

//...\CITRUSH_TestMap\testlog
void UEnemyAISubsystem::SaveJsonLog(const FString& FileNameWithoutExt, const FString& JsonContent)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*LogDirectoryPath))
	{
		PlatformFile.CreateDirectory(*LogDirectoryPath);
	}

	FString FileName = FString::Printf(TEXT("%s.json"), *FileNameWithoutExt);
	FString FullPath = FPaths::Combine(LogDirectoryPath, FileName);

	// 파일 저장
	if (FFileHelper::SaveStringToFile(JsonContent, *FullPath))
	{
		//UE_LOG(LogEnemyAI, Verbose, TEXT("[EnemyAISubsystem] Log saved: %s"), *FileName);
	}
	else
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] testlog Failed to save log to %s"), *FullPath);
	}

	// === 로그 파일 정리 ===
	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> LogFiles;
	FileManager.FindFiles(LogFiles, *FPaths::Combine(LogDirectoryPath, TEXT("*.json")), true, false);

	if (LogFiles.Num() > 10)
	{
		LogFiles.Sort([&](const FString& A, const FString& B) {
			return FileManager.GetTimeStamp(*FPaths::Combine(LogDirectoryPath, A)) <
				   FileManager.GetTimeStamp(*FPaths::Combine(LogDirectoryPath, B));
		});

		int32 FilesToDelete = LogFiles.Num() - 10;
		for (int32 i = 0; i < FilesToDelete; ++i)
		{
			FString FileToDelete = FPaths::Combine(LogDirectoryPath, LogFiles[i]);
			FileManager.Delete(*FileToDelete);
		}
	}
	
}

// ========== v1.4.0: Request 순서 보장 ==========
FString UEnemyAISubsystem::GenerateRequestNum(const FString& RoomID)
{
	// Unix Timestamp (초 단위)
	const int64 Timestamp = FDateTime::Now().ToUnixTimestamp();

	RequestSequenceNumber++;

	// 형식: {room_id}_{timestamp}_{sequence}
	const FString RequestNum = FString::Printf(TEXT("%s_%lld_%d"),
		*RoomID, Timestamp, RequestSequenceNumber);

	//UE_LOG(LogEnemyAI, Verbose, TEXT("[EnemyAISubsystem] Generated request_num: %s (Seq: %d)"), *RequestNum, RequestSequenceNumber);

	return RequestNum;
}

bool UEnemyAISubsystem::ValidateResponseOrder(const FString& IncomingRequestNum)
{
	// 폴백 응답은 항상 허용 (검증 우회)
	if (IncomingRequestNum.StartsWith(TEXT("fallback_")))
	{
		//UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Fallback response - Validation bypassed"));
		return true;
	}

	// 첫 응답인 경우
	if (LastSentRequestNum.IsEmpty())
	{
		//UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] No request sent yet, discarding response: %s"), *IncomingRequestNum);
		return false;
	}

	// request_num 형식: {room_id}_{timestamp}_{sequence}
	// 마지막 '_' 뒤의 숫자(sequence)를 추출해서 비교
	FString IncomingSeqStr;
	if (!IncomingRequestNum.Split(TEXT("_"), nullptr, &IncomingSeqStr, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		//UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Invalid request_num format: %s"), *IncomingRequestNum);
		return false;
	}

	FString LastSentSeqStr;
	if (!LastSentRequestNum.Split(TEXT("_"), nullptr, &LastSentSeqStr, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		//UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] Invalid LastSentRequestNum format: %s"), *LastSentRequestNum);
		return false;
	}

	const int32 IncomingSeq = FCString::Atoi(*IncomingSeqStr);
	const int32 LastSentSeq = FCString::Atoi(*LastSentSeqStr);

	// Request-Response 매칭 검증
	if (IncomingSeq < LastSentSeq)
	{
		// 오래된 응답 무시
		UE_LOG(LogEnemyAI, Warning,
			TEXT("[EnemyAISubsystem] Response DISCARDED - Old response (incoming seq=%d < sent seq=%d)"),
			IncomingSeq, LastSentSeq);
		UE_LOG(LogEnemyAI, Warning,
			TEXT("  Incoming: %s"), *IncomingRequestNum);
		UE_LOG(LogEnemyAI, Warning,
			TEXT("  LastSent: %s"), *LastSentRequestNum);
		return false;
	}
	else if (IncomingSeq == LastSentSeq)
	{
		// 현재 요청에 대한 응답 - 적용
		LastProcessedRequestNum = IncomingRequestNum;
		UE_LOG(LogEnemyAI, Log,
			TEXT("[EnemyAISubsystem] Response ACCEPTED - Matching request (seq=%d)"),
			IncomingSeq);
		return true;
	}
	else // IncomingSeq > LastSentSeq
	{
		// 미래 응답 무시 (아직 보내지 않은 요청에 대한 응답)
		UE_LOG(LogEnemyAI, Warning,
			TEXT("[EnemyAISubsystem] Response DISCARDED - Future response (incoming seq=%d > sent seq=%d)"),
			IncomingSeq, LastSentSeq);
		UE_LOG(LogEnemyAI, Warning,
			TEXT("  Incoming: %s"), *IncomingRequestNum);
		UE_LOG(LogEnemyAI, Warning,
			TEXT("  LastSent: %s"), *LastSentRequestNum);
		return false;
	}
}

// ========== Fallback Response 생성 ==========
FGetDecisionResponse2 UEnemyAISubsystem::CreateFallbackAttackResponse()
{
	FGetDecisionResponse2 FallbackResponse;

	// request_num은 마지막으로 보낸 request_num과 동일하게 설정 (검증 통과를 위해)
	if (LastSentRequestNum.IsEmpty())
	{
		// 아직 요청을 보내지 않은 경우 기본값 생성
		FallbackResponse.request_num = FString::Printf(TEXT("fallback_0"));
	}
	else
	{
		FallbackResponse.request_num = LastSentRequestNum;
	}

	if (!PixelEnemy.IsValid())
	{
		//UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] CreateFallbackAttackResponse - PixelEnemy가 없으면 빈 응답 반환"));
		return FallbackResponse;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return FallbackResponse;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return FallbackResponse;
	}

	FVector PacmanLocation = PixelEnemy->GetActorLocation();
	AAbstractRacer* NearestRacer = nullptr;
	float MinDistance = TNumericLimits<float>::Max();

	// 모든 PlayerState를 순회하며 Racer 찾기
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (!PS)
		{
			continue;
		}

		APawn* Pawn = PS->GetPawn();
		if (!Pawn)
		{
			continue;
		}

		// AbstractRacer 타입 체크
		AAbstractRacer* Racer = Cast<AAbstractRacer>(Pawn);
		if (!Racer)
		{
			continue;
		}

		// 거리 계산
		float Distance = FVector::Dist(PacmanLocation, Racer->GetActorLocation());
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			NearestRacer = Racer;
		}
	}

	if (!NearestRacer)
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EnemyAISubsystem] CreateFallbackAttackResponse - Racer를 찾지 못함"));

		FUnitCommand IdleCommand;
		IdleCommand.unit_id = TEXT("pacman_main");
		IdleCommand.directive_code = 0; // Idle
		IdleCommand.directive_name = TEXT("IDLE");
		FallbackResponse.decision.unit_commands.Add(IdleCommand);

		return FallbackResponse;
	}

	// Chase (추격) 명령 생성
	FUnitCommand ChaseCommand;
	ChaseCommand.unit_id = TEXT("pacman_main");
	ChaseCommand.directive_code = 4; // Chase
	ChaseCommand.directive_name = TEXT("CHASE");

	FVector RacerLocation = NearestRacer->GetActorLocation();
	ChaseCommand.params.target_position.x = RacerLocation.X;
	ChaseCommand.params.target_position.y = RacerLocation.Y;
	ChaseCommand.params.target_position.z = RacerLocation.Z;

	// 타겟 steam ID 설정 (PlayerState에서 가져오기)
	if (APlayerState* RacerPS = NearestRacer->GetPlayerState())
	{
		ChaseCommand.params.target_player_id = FString::Printf(TEXT("steam_%d"), RacerPS->GetPlayerId());
	}
	else
	{
		ChaseCommand.params.target_player_id = TEXT("steam_unknown");
	}

	ChaseCommand.params.speed_factor = 10.0f;
	FallbackResponse.decision.unit_commands.Add(ChaseCommand);

	/*UE_LOG(LogEnemyAI, Log, TEXT("[EnemyAISubsystem] Fallback CHASE command created - Target: %s at (%.1f, %.1f, %.1f), Distance: %.1f"),
		*ChaseCommand.params.target_player_id, RacerLocation.X, RacerLocation.Y, RacerLocation.Z, MinDistance);*/

	return FallbackResponse;
}
