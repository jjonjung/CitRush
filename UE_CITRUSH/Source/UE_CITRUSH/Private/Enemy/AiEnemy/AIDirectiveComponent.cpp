// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/AIEnemy/AIDirectiveComponent.h"
#include "Camera/CameraComponent.h"
#include "Enemy/AbstractEnemy.h"
#include "Enemy/PixelEnemy.h"
#include "AbilitySystemComponent.h"

#include "AIController.h"
#include "Network/AIDataManagerComponent.h"

#include "Private/Network/Schemas/HttpV1/HttpResponse2.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Stats/Coin/CoinActor.h"
#include "Enemy/Pellet/PelletActor.h"

#include "GameFramework/FloatingPawnMovement.h"

#include "Player/AbstractRacer.h"
#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"
#include "Player/Car/VehicleDemoCejCar.h"
#include "UObject/ConstructorHelpers.h"

#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "AI/NavigationSystemBase.h"
#include "Subsystems/EnemyAISubsystem.h"

UAIDirectiveComponent::UAIDirectiveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;

	// Multiplayer: Enable replication
	SetIsReplicatedByDefault(true);
}

void UAIDirectiveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate AI State to all clients
	DOREPLIFETIME(UAIDirectiveComponent, CurrentState);
	// CurrentParams와 TargetPosition은 Owner(서버의 Enemy)에선 이미 알고 있으므로 Skip
	DOREPLIFETIME_CONDITION(UAIDirectiveComponent, CurrentParams, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UAIDirectiveComponent, ReplicatedTargetPosition, COND_SkipOwner);
}

void UAIDirectiveComponent::OnRep_CurrentState()
{
	// Client-side: Broadcast state change event
	// This allows Blueprint/C++ listeners to react to state changes
	OnAIStateChanged.Broadcast(CurrentState, CurrentState);

	// Visual feedback on clients (optional logging)
	// UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] Client received state: %d"), static_cast<int32>(CurrentState));
}

void UAIDirectiveComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerEnemy = Cast<AAbstractEnemy>(GetOwner());
	if (!OwnerEnemy)
	{
		UE_LOG(LogTemp, Error, TEXT("[AIDirectiveComponent] Owner is not AAbstractEnemy!"));
		return;
	}

	ASC = OwnerEnemy->GetAbilitySystemComponent();

	// EnemyAISubsystem 캐싱 (GetAllActorsOfClass 대체)
	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		CachedAISubsystem = GI->GetSubsystem<UEnemyAISubsystem>();
	}

	if (OwnerEnemy->HasAuthority())
	{
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		LastDirectiveTime = GetWorld()->GetTimeSeconds();
	}
}

void UAIDirectiveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerEnemy || !OwnerEnemy->HasAuthority())
	{
		return;
	}
	
	UpdateCurrentState(DeltaTime);
}

// ========== Directive ==========
void UAIDirectiveComponent::OnAIDecisionReceived(bool bSuccess, FGetDecisionResponse2 Response)
{
	UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] OnAIDecisionReceived (레거시) 호출됨 - Subsystem 방식을 사용해주세요"));
}

void UAIDirectiveComponent::ProcessDirective(int32 DirectiveCode, const FDirectiveParams& Params)
{
	EAIDirectiveState NewState = static_cast<EAIDirectiveState>(DirectiveCode);

	UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] ProcessDirective - DirectiveCode: %d, TargetPlayerId: %s, TargetPos: %s, SpeedFactor: %.2f"),
		DirectiveCode, *Params.TargetPlayerId, *Params.TargetPosition.ToString(), Params.SpeedFactor);

	// 화면에 디버그 메시지 표시
	if (GEngine)
	{
		FString DirectiveName;
		switch (NewState)
		{
		case EAIDirectiveState::Idle: DirectiveName = TEXT("IDLE (0)"); break;
		case EAIDirectiveState::Ambush: DirectiveName = TEXT("AMBUSH (1)"); break;
		case EAIDirectiveState::MoveToLocation: DirectiveName = TEXT("MOVE_TO_LOCATION (2)"); break;
		case EAIDirectiveState::Intercept: DirectiveName = TEXT("INTERCEPT (3)"); break;
		case EAIDirectiveState::Chase: DirectiveName = TEXT("CHASE (4)"); break;
		case EAIDirectiveState::Retreat: DirectiveName = TEXT("RETREAT (5)"); break;
		case EAIDirectiveState::Patrol: DirectiveName = TEXT("PATROL (6)"); break;
		case EAIDirectiveState::ConsumePoint: DirectiveName = TEXT("CONSUME_POINT (7)"); break;
		case EAIDirectiveState::ConsumePellet: DirectiveName = TEXT("CONSUME_PELLET (8)"); break;
		case EAIDirectiveState::Guard: DirectiveName = TEXT("GUARD (9)"); break;
		case EAIDirectiveState::Flank: DirectiveName = TEXT("FLANK (10)"); break;
		case EAIDirectiveState::FakeRetreat: DirectiveName = TEXT("FAKE_RETREAT (11)"); break;
		case EAIDirectiveState::CoinChase: DirectiveName = TEXT("COIN_CHASE (12)"); break; // Legacy or Internal
		case EAIDirectiveState::NetworkFallback: DirectiveName = TEXT("NETWORK_FALLBACK"); break;
		default: DirectiveName = TEXT("UNKNOWN"); break;
		}

		FString DebugMessage = FString::Printf(
			TEXT("[AIDirective] Code: %d | Name: %s | Target: (%.0f, %.0f, %.0f) | Speed: %.2f"),
			DirectiveCode,
			*DirectiveName,
			Params.TargetPosition.X,
			Params.TargetPosition.Y,
			Params.TargetPosition.Z,
			Params.SpeedFactor
		);

		// 고유 키(this)를 사용하여 메시지가 누적되지 않고 업데이트되도록 수정
		GEngine->AddOnScreenDebugMessage((uint64)this, 5.0f, FColor::Yellow, DebugMessage);
	}

	UpdateLastDirectiveTime();
	TransitionToState(NewState, Params);
}

void UAIDirectiveComponent::UpdateLastDirectiveTime()
{
	if (GetWorld())
	{
		LastDirectiveTime = GetWorld()->GetTimeSeconds();
	}
}

bool UAIDirectiveComponent::IsGoalStale() const
{
	if (!GetWorld()) return true;
	return (GetWorld()->GetTimeSeconds() - LastDirectiveTime) > GoalMaxAge;
}

// ========== State 관리 ==========
void UAIDirectiveComponent::TransitionToState(EAIDirectiveState NewState, const FDirectiveParams& Params)
{
	/*UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] TransitionToState - From: %d to %d, TargetPlayerId: %s"),
		(int32)CurrentState, (int32)NewState, *Params.TargetPlayerId);*/

	// World 유효성 검사 - HTTP 비동기 응답 처리 중 Actor/World가 파괴될 수 있음
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogTemp, Error, TEXT("[AIDirectiveComponent] TransitionToState failed - World is invalid"));
		return;
	}

	// [개선] 같은 상태가 연속으로 들어올 때 움직임이 끊기지 않도록 처리
	if (CurrentState == NewState)
	{
		bool bShouldReset = true; 
		// 상태별로 리셋 방지 조건 체크
		if (NewState == EAIDirectiveState::Chase)
		{
			if (CurrentParams.TargetPlayerId == Params.TargetPlayerId)
			{
				//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] TransitionToState - Same Chase target, skipping reset"));
				bShouldReset = false;
			}
		}
		else if (NewState == EAIDirectiveState::MoveToLocation)
		{
			if (FVector::Dist(CurrentParams.TargetPosition, Params.TargetPosition) < 100.0f)
			{
				bShouldReset = false;
			}
		}
		else if (NewState == EAIDirectiveState::Patrol)
		{
			if (CurrentParams.PatrolZone == Params.PatrolZone)
			{
				bShouldReset = false;
			}
		}

		// 리셋이 필요 없다면 파라미터만 갱신하고 종료 (경로 유지)
		// Chase 상태라면 타임아웃 타이머 연장 (서버에서 계속 추격 명령이 오고 있으므로)
		if (!bShouldReset)
		{
			CurrentParams = Params;

			if (NewState == EAIDirectiveState::Chase)
			{
				ChaseStartTime = World->GetTimeSeconds();
			}

			return;
		}
	}

	// 이전 State GameplayTag 제거
	if (ASC)
	{
		FGameplayTag OldTag = GetStateTag(CurrentState);
		if (OldTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(OldTag);
		}
	}

	EAIDirectiveState OldState = CurrentState;
	CurrentState = NewState;
	CurrentParams = Params;
	StateStartTime = GetWorld()->GetTimeSeconds();

	// Multiplayer: Update replicated target position
	ReplicatedTargetPosition = Params.TargetPosition;

	// Broadcast state change event (Server-side, clients receive via OnRep)
	if (OldState != NewState)
	{
		OnAIStateChanged.Broadcast(NewState, OldState);
	}

	switch (NewState)
	{
	case EAIDirectiveState::MoveToLocation:
		// NavMesh 경로 초기화
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;
		break;
	case EAIDirectiveState::Ambush:
		AmbushPhase = 0;
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;
		break;
	case EAIDirectiveState::Chase:
		ChaseStartTime = GetWorld()->GetTimeSeconds();
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;
		break;
	case EAIDirectiveState::Intercept:
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;
		break;
	case EAIDirectiveState::Retreat:
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;
		break;
	case EAIDirectiveState::Patrol:
		CurrentWaypointIndex = 0;
		break;
	case EAIDirectiveState::Guard:
		GuardStartTime = GetWorld()->GetTimeSeconds();
		break;
	case EAIDirectiveState::Flank:
		FlankTargetPosition = FVector::ZeroVector;
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;
		break;
	case EAIDirectiveState::ConsumePellet:
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;
		break;
	default:
		break;
	}

	if (ASC)
	{
		FGameplayTag NewTag = GetStateTag(NewState);
		if (NewTag.IsValid())
		{
			ASC->AddLooseGameplayTag(NewTag);
		}
	}

	//  GEngine용: State가 실제로 바뀐 경우에만
	if (OldState != NewState && GEngine)
	{
		FString StateName;
		switch (NewState)
		{
		case EAIDirectiveState::Idle:
			StateName = TEXT("IDLE");
			break;
		case EAIDirectiveState::Ambush:
			StateName = TEXT("AMBUSH");
			break;
		case EAIDirectiveState::MoveToLocation:
			StateName = FString::Printf(TEXT("MOVE TO (%.0f, %.0f)"), Params.TargetPosition.X, Params.TargetPosition.Y);
			UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] MoveToLocation 목표 위치: %.0f, %.0f"), Params.TargetPosition.X, Params.TargetPosition.Y );
			break;
		case EAIDirectiveState::Intercept:
			StateName = TEXT("INTERCEPT");
			break;
		case EAIDirectiveState::Chase:
			StateName = TEXT("CHASE");
			break;
		case EAIDirectiveState::Retreat:
			StateName = TEXT("RETREAT");
			break;
		case EAIDirectiveState::Patrol:
			StateName = TEXT("PATROL");
			break;
		case EAIDirectiveState::ConsumePoint:
			StateName = TEXT("CONSUME P-POINT");
			break;
		case EAIDirectiveState::ConsumePellet:
			StateName = TEXT("CONSUME PELLET");
			break;
		case EAIDirectiveState::Guard:
			StateName = TEXT("GUARD");
			break;
		case EAIDirectiveState::Flank:
			StateName = TEXT("FLANK");
			break;
		case EAIDirectiveState::FakeRetreat:
			StateName = TEXT("FAKE RETREAT");
			break;
		case EAIDirectiveState::CoinChase:
			StateName = TEXT("COIN CHASE");
			break;
		default:
			StateName = TEXT("UNKNOWN");
			break;
		}

		GEngine->AddOnScreenDebugMessage(
			(uint64)this,
			5.0f,
			FColor::Magenta,
			FString::Printf(TEXT("[State] %s"), *StateName));
	}
}

void UAIDirectiveComponent::UpdateCurrentState(float DeltaTime)
{
	if (GEngine)
	{
		FString StateName;

		switch (CurrentState)
		{
		case EAIDirectiveState::Idle:             StateName = TEXT("Idle"); break;
		case EAIDirectiveState::Ambush:           StateName = TEXT("Ambush"); break;
		case EAIDirectiveState::MoveToLocation:   StateName = TEXT("MoveToLocation"); break;
		case EAIDirectiveState::Intercept:        StateName = TEXT("Intercept"); break;
		case EAIDirectiveState::Chase:            StateName = TEXT("Chase"); break;
		case EAIDirectiveState::Retreat:          StateName = TEXT("Retreat"); break;
		case EAIDirectiveState::Patrol:           StateName = TEXT("Patrol"); break;
		case EAIDirectiveState::ConsumePoint:     StateName = TEXT("ConsumePoint"); break;
		case EAIDirectiveState::ConsumePellet:    StateName = TEXT("ConsumePellet"); break;
		case EAIDirectiveState::Guard:            StateName = TEXT("Guard"); break;
		case EAIDirectiveState::Flank:            StateName = TEXT("Flank"); break;
		case EAIDirectiveState::FakeRetreat:      StateName = TEXT("FakeRetreat"); break;
		case EAIDirectiveState::CoinChase:        StateName = TEXT("CoinChase"); break;
		case EAIDirectiveState::NetworkFallback:  StateName = TEXT("NetworkFallback (Retreat)"); break;
		default:                                  StateName = TEXT("Unknown"); break;
		}

		// 화면 좌측 상단에 고정된 키(this)로 지속적으로 갱신
		GEngine->AddOnScreenDebugMessage(
			(uint64)this,        // Key (this = 인스턴스별로 업데이트)
			3.0f,                // TimeToDisplay
			FColor::Yellow,
			FString::Printf(TEXT("AI State: %s"), *StateName)
		);
	}

	// Goal Buffer: stale Idle 상태에서는 아무것도 하지 않음
	// (PixelEnemy가 연결 상태 변화 감지 시 StateTree로 전환)
	if (IsGoalStale() && CurrentState == EAIDirectiveState::Idle)
	{
		return;
	}

	switch (CurrentState)
	{
	case EAIDirectiveState::Idle:
		ExecuteIdle(DeltaTime);
		break;
	case EAIDirectiveState::Ambush:
		ExecuteAmbush(DeltaTime);
		break;
	case EAIDirectiveState::MoveToLocation:
		ExecuteMoveToLocation(DeltaTime);
		break;
	case EAIDirectiveState::Intercept:
		ExecuteIntercept(DeltaTime);
		break;
	case EAIDirectiveState::Chase:
		ExecuteChase(DeltaTime);
		break;
	case EAIDirectiveState::Retreat:
		ExecuteRetreat(DeltaTime);
		break;
	case EAIDirectiveState::Patrol:
		ExecutePatrol(DeltaTime);
		break;
	case EAIDirectiveState::ConsumePoint:
		ExecuteConsumePoint(DeltaTime);
		break;
	case EAIDirectiveState::ConsumePellet:
		ExecuteConsumePellet(DeltaTime);
		break;
	case EAIDirectiveState::Guard:
		ExecuteGuard(DeltaTime);
		break;
	case EAIDirectiveState::Flank:
		ExecuteFlank(DeltaTime);
		break;
	case EAIDirectiveState::FakeRetreat:
		ExecuteFakeRetreat(DeltaTime);
		break;
	case EAIDirectiveState::CoinChase:
		ExecuteCoinChase(DeltaTime);
		break;
	case EAIDirectiveState::NetworkFallback:
		ExecuteRetreat(DeltaTime); // Fallback은 Retreat과 동일
		break;
	default:
		break;
	}
}

// 기본 동작
void UAIDirectiveComponent::ExecuteIdle(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}

	// 1-VehicleDemoCejCar를 찾아 무조건 추격
	AVehicleDemoCejCar* NearestCar = FindNearestVehicleCar();
	if (NearestCar)
	{
		FDirectiveParams ChaseParams;
		FVector CarLocation = NearestCar->GetActorLocation();
		ChaseParams.TargetPosition = CarLocation;
		ChaseParams.SpeedFactor = 1.0f;

		if (APlayerState* PS = NearestCar->GetPlayerState())
		{
			ChaseParams.TargetPlayerId = FString::Printf(TEXT("steam_%d"), PS->GetPlayerId());
		}

		/*UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] Idle -> Chase VehicleDemoCejCar at (%.0f, %.0f, %.0f)"),
			CarLocation.X, CarLocation.Y, CarLocation.Z);*/

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				11,
				3.0f,
				FColor::Cyan,
				FString::Printf(TEXT("[Idle Fallback] Chasing VehicleDemoCejCar at (%.0f, %.0f)"), CarLocation.X, CarLocation.Y));
		}

		TransitionToState(EAIDirectiveState::Chase, ChaseParams);
		return;
	}

	// VehicleDemoCejCar가 없으면 가장 가까운 플레이어 추격
	APawn* NearestPlayer = FindNearestPlayer();
	if (!NearestPlayer)
	{
		return;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	FVector PlayerLocation = NearestPlayer->GetActorLocation();
	float Distance = FVector::Dist(CurrentLocation, PlayerLocation);
	if (Distance > 10.0f)
	{
		return;
	}

	if (Distance < 100.0f)
	{
		FVector LookDirection = PlayerLocation - CurrentLocation;
		LookDirection.Z = 0.0f;
		if (!LookDirection.IsNearlyZero())
		{
			LookDirection.Normalize();
			FRotator TargetRotation = LookDirection.Rotation();
			FRotator NewRotation = FMath::RInterpTo(
				OwnerEnemy->GetActorRotation(),
				TargetRotation,
				DeltaTime,
				3.0f
			);
			OwnerEnemy->SetActorRotation(NewRotation);
		}
		return;
	}

	bool bNeedRecalculate = false;
	if (CurrentPathPoints.Num() == 0)
	{
		bNeedRecalculate = true;
	}
	else if (CurrentPathPoints.Num() > 0)
	{
		FVector LastPathPoint = CurrentPathPoints[CurrentPathPoints.Num() - 1];
		float DistanceToTarget = FVector::Dist(LastPathPoint, PlayerLocation);
		if (DistanceToTarget > 600.0f)
		{
			bNeedRecalculate = true;
		}
	}

	if (bNeedRecalculate)
	{
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;

		if (FindNavMeshPath(CurrentLocation, PlayerLocation, CurrentPathPoints))
		{
			CurrentPathPointIndex = 0;
		}
	}

	if (CurrentPathPoints.Num() > 0)
	{
		bool bReachedDestination = MoveAlongPath(DeltaTime, 1.2f);
		if (bReachedDestination)
		{
			CurrentPathPoints.Empty();
			CurrentPathPointIndex = 0;
		}

	}

	FVector Direction = PlayerLocation - CurrentLocation;
	Direction.Z = 0.0f;

	if (!Direction.IsNearlyZero())
	{
		Direction.Normalize();
		MoveInDirection(Direction, 1.2f, DeltaTime);
		/*FRotator TargetRotation = Direction.Rotation();
		FRotator NewRotation = FMath::RInterpTo(
			OwnerEnemy->GetActorRotation(),
			TargetRotation,
			DeltaTime,
			4.0f
		);
		OwnerEnemy->SetActorRotation(NewRotation);*/
		
		// 1. Interp Speed를 낮춰서 (4.0f -> 1.5f ~ 2.5f) 더 천천히 회전
		// 2. DeltaTime을 Clamp 해서 프레임 드랍 시 과도한 스냅 방지
		// 3. Clamp로 180도 이상 회전 방지 (짧은 방향 선택)
		// 4. 필요 시 FInterpEaseInOut으로 Ease 적용 (더 자연스러움)
		Direction.Normalize();  // 중요: 정규화

		// 목표 회전 계산 (Yaw와 Pitch만 사용, Roll은 보통 0)
		FRotator TargetRotation = Direction.Rotation();

		// 현재 회전 가져오기
		FRotator CurrentRotation = OwnerEnemy->GetActorRotation();

		// DeltaTime 안정화 (프레임 드랍 방지)
		float DeltaTime = GetWorld()->GetDeltaSeconds();
		float ClampedDT = FMath::Min(DeltaTime, 0.033f);  // 최대 30FPS 기준

		// 가장 중요한 부분: 최단 경로 회전 보간 (휙 도는 현상 방지)
		FRotator InterpolatedRotation = FMath::RInterpTo(
			CurrentRotation,
			TargetRotation,
			ClampedDT,
			4.0f  // 회전 속도: 2.0f ~ 6.0f 추천 (값이 작을수록 느리고 부드러움)
		);

		OwnerEnemy->SetActorRotation(InterpolatedRotation);
	}
}



// 4 | CHASE | 추격 – 플레이어 압박 
void UAIDirectiveComponent::ExecuteChase(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}

	APawn* TargetPlayer = nullptr;

	float MinDistance = TNumericLimits<float>::Max();
	FVector CurrentLocation = OwnerEnemy->GetActorLocation();

	if (CachedAISubsystem)
	{
		for (const TWeakObjectPtr<AVehicleDemoCejCar>& CarPtr : CachedAISubsystem->GetCachedVehicles())
		{
			AVehicleDemoCejCar* Car = CarPtr.Get();
			if (!Car) continue;

			if (ACitRushPlayerState* PS = Car->GetPlayerState<ACitRushPlayerState>())
			{
				if (PS->GetPlayerRole() != EPlayerRole::Racer)
				{
					continue;
				}
			}

			float Dist = FVector::Dist(CurrentLocation, Car->GetActorLocation());
			if (Dist < MinDistance)
			{
				MinDistance = Dist;
				TargetPlayer = Car;
			}
		}
	}

	//Racer 차량을 못 찾았으면 기존 로직 (ID로 찾기 -> 가장 가까운 플레이어 찾기)
	if (!TargetPlayer)
	{
		TargetPlayer = FindPlayerByID(CurrentParams.TargetPlayerId);

		if (!TargetPlayer)
		{
			TargetPlayer = FindNearestPlayer();
			
			if (!TargetPlayer)
			{
				if (!CurrentParams.TargetPosition.IsZero())
				{
					//UE_LOG(LogTemp, Warning, TEXT("[ExecuteChase] Target lost, moving to last known position: %s"), *CurrentParams.TargetPosition.ToString());
					
					FVector TargetPos = CurrentParams.TargetPosition;
					FVector Dir = TargetPos - CurrentLocation;
					Dir.Z = 0.0f;
					
					// 도착 판정
					if (Dir.Size() < 10.0f)
					{
						TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
						return;
					}
					
					Dir.Normalize();
					MoveInDirection(Dir, CurrentParams.SpeedFactor, DeltaTime);
					
					/*FRotator TargetRot = Dir.Rotation();
					FRotator NewRot = FMath::RInterpTo(OwnerEnemy->GetActorRotation(), TargetRot, DeltaTime, 8.0f);
					OwnerEnemy->SetActorRotation(NewRot);*/
					return;
				}

				TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
				return;
			}
		}
	}

	// 타겟을 찾았으면 TargetPosition 갱신 
	CurrentParams.TargetPosition = TargetPlayer->GetActorLocation();

	float ElapsedTime = GetWorld()->GetTimeSeconds() - ChaseStartTime;
	if (ElapsedTime > CurrentParams.MaxChaseDuration)
	{
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		return;
	}

	FVector PlayerLocation = TargetPlayer->GetActorLocation();

	float Distance = FVector::Dist(CurrentLocation, PlayerLocation);

	if (Distance < 100.0f)
	{
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		return;
	}

	// NavMesh 경로 재계산 조건:
	// 1. 경로가 없거나
	// 2. 경로의 최종 목표와 현재 플레이어 위치가 500cm 이상 차이날 때
	bool bNeedRecalculate = false;
	if (CurrentPathPoints.Num() == 0)
	{
		bNeedRecalculate = true;
	}
	else if (CurrentPathPoints.Num() > 0)
	{
		FVector LastPathPoint = CurrentPathPoints[CurrentPathPoints.Num() - 1];
		float DistanceToTarget = FVector::Dist(LastPathPoint, PlayerLocation);
		if (DistanceToTarget > 500.0f)
		{
			bNeedRecalculate = true;
		}
	}

	// 경로 재계산
	if (bNeedRecalculate)
	{
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;

		if (FindNavMeshPath(CurrentLocation, PlayerLocation, CurrentPathPoints))
		{
			CurrentPathPointIndex = 0;
		}
	}

	// NavMesh 경로가 있으면 경로를 따라 이동
	if (CurrentPathPoints.Num() > 0)
	{
		bool bReachedDestination = MoveAlongPath(DeltaTime, CurrentParams.SpeedFactor);

		// 경로 완료 시 Idle로 전환하지 않고 경로 재계산 (플레이어는 계속 이동 중)
		if (bReachedDestination)
		{
			CurrentPathPoints.Empty();
			CurrentPathPointIndex = 0;
		}

		return;
	}

	// Fallback: 직선 이동
	FVector Direction = PlayerLocation - CurrentLocation;
	Direction.Z = 0.0f;
	Direction.Normalize();

	// MoveInDirection 사용 (AddMovementInput 기반)
	MoveInDirection(Direction, CurrentParams.SpeedFactor, DeltaTime);

	/*FRotator TargetRotation = Direction.Rotation();
	FRotator NewRotation = FMath::RInterpTo(
		OwnerEnemy->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		8.0f
	);
	OwnerEnemy->SetActorRotation(NewRotation);*/

	// 1. Interp Speed를 낮춰서 (4.0f -> 1.5f ~ 2.5f) 더 천천히 회전
	// 2. DeltaTime을 Clamp 해서 프레임 드랍 시 과도한 스냅 방지
	// 3. Clamp로 180도 이상 회전 방지 (짧은 방향 선택)
	// 4. 필요 시 FInterpEaseInOut으로 Ease 적용 (더 자연스러움)
	Direction.Normalize();  // 중요: 정규화

	// 목표 회전 계산 (Yaw와 Pitch만 사용, Roll은 보통 0)
	FRotator TargetRotation = Direction.Rotation();

	// 현재 회전 가져오기
	FRotator CurrentRotation = OwnerEnemy->GetActorRotation();

	// DeltaTime 안정화 (프레임 드랍 방지)
	float ClampedDT = FMath::Min(DeltaTime, 0.033f);  // 최대 30FPS 기준

	// 가장 중요한 부분: 최단 경로 회전 보간 (휙 도는 현상 방지)
	FRotator InterpolatedRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		ClampedDT,
		4.0f  // 회전 속도: 2.0f ~ 6.0f 추천 (값이 작을수록 느리고 부드러움)
	);

	OwnerEnemy->SetActorRotation(InterpolatedRotation);
}

//2-MOVE_TO_LOCATION
void UAIDirectiveComponent::ExecuteMoveToLocation(float DeltaTime)
{
	
	if (!OwnerEnemy)
	{
		return;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	FVector TargetLocation = CurrentParams.TargetPosition;

	// NavMesh 경로 찾기
	if (CurrentPathPoints.Num() == 0)
	{
		if (FindNavMeshPath(CurrentLocation, TargetLocation, CurrentPathPoints))
		{
			CurrentPathPointIndex = 0;
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] MoveToLocation: NavMesh path failed - using direct movement"));
			// NavMesh 경로 찾기 실패 시 직선 이동 (fallback)
		}
	}

	// NavMesh 경로가 있으면 경로를 따라 이동
	if (CurrentPathPoints.Num() > 0)
	{
		bool bReachedDestination = MoveAlongPath(DeltaTime, CurrentParams.SpeedFactor);

		if (bReachedDestination)
		{
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] 목표 위치 도달 (NavMesh) - Idle로 전환"));
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		}
		return;
	}

	// Fallback: 직선 이동 (NavMesh 경로가 없을 때)
	FVector Direction = TargetLocation - CurrentLocation;
	Direction.Z = 0.0f;
	float Distance = Direction.Size();

	if (Distance < 200.0f)
	{
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		return;
	}

	Direction.Normalize();
	MoveInDirection(Direction, CurrentParams.SpeedFactor, DeltaTime);

	/*FRotator TargetRotation = Direction.Rotation();
	FRotator NewRotation = FMath::RInterpTo(
		OwnerEnemy->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		5.0f
	);
	OwnerEnemy->SetActorRotation(NewRotation);*/
	// 1. Interp Speed를 낮춰서 (4.0f -> 1.5f ~ 2.5f) 더 천천히 회전
	// 2. DeltaTime을 Clamp 해서 프레임 드랍 시 과도한 스냅 방지
	// 3. Clamp로 180도 이상 회전 방지 (짧은 방향 선택)
	// 4. 필요 시 FInterpEaseInOut으로 Ease 적용 (더 자연스러움)
	Direction.Normalize();  // 중요: 정규화

	// 목표 회전 계산 (Yaw와 Pitch만 사용, Roll은 보통 0)
	FRotator TargetRotation = Direction.Rotation();

	// 현재 회전 가져오기
	FRotator CurrentRotation = OwnerEnemy->GetActorRotation();

	// DeltaTime 안정화 (프레임 드랍 방지)
	float ClampedDT = FMath::Min(DeltaTime, 0.033f);  // 최대 30FPS 기준

	// 가장 중요한 부분: 최단 경로 회전 보간 (휙 도는 현상 방지)
	FRotator InterpolatedRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		ClampedDT,
		4.0f  // 회전 속도: 2.0f ~ 6.0f 추천 (값이 작을수록 느리고 부드러움)
	);

	OwnerEnemy->SetActorRotation(InterpolatedRotation);
}

//3- Intercept 진로 차단
void UAIDirectiveComponent::ExecuteIntercept(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}

	APawn* TargetPlayer = FindPlayerByID(CurrentParams.TargetPlayerId);
	if (!TargetPlayer)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] INTERCEPT: Target player not found - Idle"));
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		return;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	FVector PlayerLocation = TargetPlayer->GetActorLocation();
	FVector PlayerVelocity = TargetPlayer->GetVelocity();

	// 플레이어 진로 예측 
	float PredictionTime = 1.0f;
	FVector PredictedLocation = PlayerLocation + (PlayerVelocity * PredictionTime);

	// 차단 위치 계산 (예측 위치에서 ApproachAngle 각도로 접근)
	FVector ToPlayer = PlayerLocation - CurrentLocation;
	ToPlayer.Z = 0.0f;

	// ApproachAngle을 적용하여 차단 각도 계산
	float AngleRad = FMath::DegreesToRadians(CurrentParams.ApproachAngle);
	FVector InterceptDirection = ToPlayer.RotateAngleAxis(AngleRad, FVector::UpVector);
	InterceptDirection.Normalize();

	// InterceptDistance만큼 떨어진 위치로 이동
	FVector InterceptPosition = PredictedLocation + (InterceptDirection * CurrentParams.InterceptDistance);

	// 차단 위치 도달 또는 플레이어와 충분히 가까워지면 Chase로 전환
	float DistanceToPlayer = FVector::Dist(CurrentLocation, PlayerLocation);
	if (DistanceToPlayer < 300.0f)
	{
		//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] INTERCEPT 완료 - Chase로 전환"));
		FDirectiveParams ChaseParams;
		ChaseParams.TargetPlayerId = CurrentParams.TargetPlayerId;
		ChaseParams.SpeedFactor = CurrentParams.Aggressiveness;
		TransitionToState(EAIDirectiveState::Chase, ChaseParams);
		return;
	}

	// NavMesh 경로 재계산 조건:
	// 1. 경로가 없거나
	// 2. 경로의 최종 목표와 현재 차단 위치가 400cm 이상 차이날 때 (플레이어 이동 반영)
	bool bNeedRecalculate = false;
	if (CurrentPathPoints.Num() == 0)
	{
		bNeedRecalculate = true;
	}
	else if (CurrentPathPoints.Num() > 0)
	{
		FVector LastPathPoint = CurrentPathPoints[CurrentPathPoints.Num() - 1];
		float DistanceToTarget = FVector::Dist(LastPathPoint, InterceptPosition);
		if (DistanceToTarget > 400.0f)
		{
			bNeedRecalculate = true;
			//UE_LOG(LogTemp, Verbose, TEXT("[AIDirectiveComponent] INTERCEPT: Target moved %.1f cm, recalculating path"), DistanceToTarget);
		}
	}

	// 경로 재계산
	if (bNeedRecalculate)
	{
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;

		if (FindNavMeshPath(CurrentLocation, InterceptPosition, CurrentPathPoints))
		{
			CurrentPathPointIndex = 0;
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] Intercept: NavMesh path found with %d points"),CurrentPathPoints.Num());
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] Intercept: NavMesh path failed - using direct movement"));
		}
	}

	// NavMesh 경로가 있으면 경로를 따라 이동
	float MoveSpeed = FMath::Lerp(0.8f, 1.5f, CurrentParams.Aggressiveness);
	if (CurrentPathPoints.Num() > 0)
	{
		bool bReachedDestination = MoveAlongPath(DeltaTime, MoveSpeed);

		// 경로 완료 시 경로 재계산 (차단 위치는 계속 변함)
		if (bReachedDestination)
		{
			CurrentPathPoints.Empty();
			CurrentPathPointIndex = 0;
		}

		//UE_LOG(LogTemp, VeryVerbose, TEXT("[AIDirectiveComponent] INTERCEPT (NavMesh): Distance to player %.1f"), DistanceToPlayer);
		return;
	}

	// Fallback: 직선 이동
	FVector Direction = InterceptPosition - CurrentLocation;
	Direction.Z = 0.0f;

	float Distance = Direction.Size();

	if (Distance < 150.0f)
	{
		//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] INTERCEPT 차단 위치 도달 (직선) - Chase로 전환"));
		FDirectiveParams ChaseParams;
		ChaseParams.TargetPlayerId = CurrentParams.TargetPlayerId;
		ChaseParams.SpeedFactor = CurrentParams.Aggressiveness;
		TransitionToState(EAIDirectiveState::Chase, ChaseParams);
		return;
	}

	Direction.Normalize();
	MoveInDirection(Direction, MoveSpeed, DeltaTime);

	FRotator TargetRotation = Direction.Rotation();
	FRotator NewRotation = FMath::RInterpTo(
		OwnerEnemy->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		8.0f
	);
	OwnerEnemy->SetActorRotation(NewRotation);
	//UE_LOG(LogTemp, VeryVerbose, TEXT("[AIDirectiveComponent] INTERCEPT (직선): Distance %.1f"), Distance);
}

//5-후퇴 | 안전 지대로 이탈
void UAIDirectiveComponent::ExecuteRetreat(float DeltaTime)
{
	if (!OwnerEnemy) return;

	// 안전 지대로 후퇴 (v1.4.0 API: 순수 후퇴만 수행)
	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	FVector SafeLocation = CurrentParams.TargetPosition;

	// safe_zone_position이 없으면 플레이어 반대 방향으로 기본 후퇴 위치 설정
	if (SafeLocation.IsZero())
	{
		APawn* NearestPlayer = FindNearestPlayer();
		if (NearestPlayer)
		{
			FVector DirFromPlayer = CurrentLocation - NearestPlayer->GetActorLocation();
			DirFromPlayer.Z = 0.0f;
			DirFromPlayer.Normalize();
			SafeLocation = CurrentLocation + (DirFromPlayer * 2000.0f);
		}
		else
		{
			// 플레이어도 없으면 뒤쪽으로 후퇴
			SafeLocation = CurrentLocation - (OwnerEnemy->GetActorForwardVector() * 2000.0f);
		}
		CurrentParams.TargetPosition = SafeLocation;
	}

	// SpeedFactor 적용 (기본 1.0)
	float RetreatSpeed = CurrentParams.SpeedFactor > 0.0f ? CurrentParams.SpeedFactor : 1.0f;

	// 플레이어 주시 (뒷걸음질 연출)
	APawn* TargetPlayer = FindNearestPlayer();
	if (TargetPlayer)
	{
		FVector LookDir = TargetPlayer->GetActorLocation() - CurrentLocation;
		LookDir.Z = 0.0f;
		if (!LookDir.IsNearlyZero())
		{
			FRotator TargetRot = LookDir.Rotation();
			FRotator CurrentRot = OwnerEnemy->GetActorRotation();
			OwnerEnemy->SetActorRotation(FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 5.0f));
		}
	}

	// 경로 탐색
	if (CurrentPathPoints.Num() == 0)
	{
		if (FindNavMeshPath(CurrentLocation, SafeLocation, CurrentPathPoints))
		{
			CurrentPathPointIndex = 0;
		}
	}

	// 2-3. 이동 처리 (회전 없이 위치만 이동)
	FVector MoveDirection = FVector::ZeroVector;
	bool bReachedFinal = false;

	if (CurrentPathPoints.Num() > 0)
	{
		if (CurrentPathPointIndex < CurrentPathPoints.Num())
		{
			FVector TargetPoint = CurrentPathPoints[CurrentPathPointIndex];
			MoveDirection = TargetPoint - CurrentLocation;
			MoveDirection.Z = 0.0f;
			float Distance = MoveDirection.Size();

			if (Distance < PathPointTolerance)
			{
				CurrentPathPointIndex++;
				if (CurrentPathPointIndex >= CurrentPathPoints.Num())
				{
					bReachedFinal = true;
				}
			}
			MoveDirection.Normalize();
		}
		else
		{
			bReachedFinal = true;
		}
	}
	else
	{
		// Fallback: 직선 이동
		MoveDirection = SafeLocation - CurrentLocation;
		MoveDirection.Z = 0.0f;
		if (MoveDirection.Size() < 150.0f) bReachedFinal = true;
		MoveDirection.Normalize();
	}

	if (bReachedFinal)
	{
		CurrentPathPoints.Empty();
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		return;
	}

	// 위치 업데이트 (회전 없이 이동만)
	UCharacterMovementComponent* CharMovement = OwnerEnemy->GetCharacterMovement();
	if (CharMovement && !MoveDirection.IsNearlyZero())
	{
		float MoveSpeed = CharMovement->MaxWalkSpeed * RetreatSpeed;
		FVector NewLocation = CurrentLocation + (MoveDirection * MoveSpeed * DeltaTime);
		OwnerEnemy->SetActorLocation(NewLocation, true);
	}
}

//Patrol 순찰 
void UAIDirectiveComponent::ExecutePatrol(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}

	if (CurrentParams.Waypoints.Num() == 0)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] PATROL: No waypoints defined - Idle"));
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		return;
	}

	// 현재 웨이포인트 인덱스 유효성 체크
	if (CurrentWaypointIndex < 0 || CurrentWaypointIndex >= CurrentParams.Waypoints.Num())
	{
		CurrentWaypointIndex = 0; // 순환
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	FVector TargetWaypoint = CurrentParams.Waypoints[CurrentWaypointIndex];
	FVector Direction = TargetWaypoint - CurrentLocation;
	Direction.Z = 0.0f;

	float Distance = Direction.Size();

	// 현재 웨이포인트 도달 (150cm 이내) -> 다음 웨이포인트로
	if (Distance < 150.0f)
	{
		CurrentWaypointIndex++;
		if (CurrentWaypointIndex >= CurrentParams.Waypoints.Num())
		{
			CurrentWaypointIndex = 0; // 순환
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] PATROL: Completed one patrol loop, restarting"));
		}
		else
		{
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] PATROL: Moving to waypoint %d/%d"),CurrentWaypointIndex + 1, CurrentParams.Waypoints.Num());
		}
		return;
	}

	// 순찰 이동 (PatrolSpeed 적용, 기본 0.8)
	Direction.Normalize();
	MoveInDirection(Direction, CurrentParams.PatrolSpeed, DeltaTime);

	/*// 회전
	FRotator TargetRotation = Direction.Rotation();
	FRotator NewRotation = FMath::RInterpTo(
		OwnerEnemy->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		4.0f 
	);
	OwnerEnemy->SetActorRotation(NewRotation);*/

	// 1. Interp Speed를 낮춰서 (4.0f -> 1.5f ~ 2.5f) 더 천천히 회전
	// 2. DeltaTime을 Clamp 해서 프레임 드랍 시 과도한 스냅 방지
	// 3. Clamp로 180도 이상 회전 방지 (짧은 방향 선택)
	// 4. 필요 시 FInterpEaseInOut으로 Ease 적용 (더 자연스러움)
	Direction.Normalize();  // 중요: 정규화

	// 목표 회전 계산 (Yaw와 Pitch만 사용, Roll은 보통 0)
	FRotator TargetRotation = Direction.Rotation();

	// 현재 회전 가져오기
	FRotator CurrentRotation = OwnerEnemy->GetActorRotation();

	// DeltaTime 안정화 (프레임 드랍 방지)
	float ClampedDT = FMath::Min(DeltaTime, 0.033f);  // 최대 30FPS 기준

	// 가장 중요한 부분: 최단 경로 회전 보간 (휙 도는 현상 방지)
	FRotator InterpolatedRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		ClampedDT,
		4.0f  // 회전 속도: 2.0f ~ 6.0f 추천 (값이 작을수록 느리고 부드러움)
	);

	OwnerEnemy->SetActorRotation(InterpolatedRotation);

	//UE_LOG(LogTemp, VeryVerbose, TEXT("[AIDirectiveComponent] PATROL: Waypoint %d/%d, Distance %.1f"),CurrentWaypointIndex + 1, CurrentParams.Waypoints.Num(), Distance);
}

// 11 | FAKE_RETREAT | 후퇴 위장 - 후퇴인 척 역습 (v1.4.0 API)
void UAIDirectiveComponent::ExecuteFakeRetreat(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}

	float ElapsedTime = GetWorld()->GetTimeSeconds() - StateStartTime;
	FVector CurrentLocation = OwnerEnemy->GetActorLocation();

	// fake_retreat_duration 필수 파라미터 (기본값 2.0초)
	float RetreatDuration = CurrentParams.FakeRetreatDuration > 0.0f
		? CurrentParams.FakeRetreatDuration
		: 2.0f;

	// counter_attack_position 선택 파라미터 (없으면 가장 가까운 플레이어 위치)
	FVector CounterAttackPosition = CurrentParams.TargetPosition;
	if (CounterAttackPosition.IsZero())
	{
		APawn* NearestPlayer = FindNearestPlayer();
		if (NearestPlayer)
		{
			CounterAttackPosition = NearestPlayer->GetActorLocation();
		}
		else
		{
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
			return;
		}
	}

	// Phase 1: 후퇴 위장 (fake_retreat_duration 동안)
	if (ElapsedTime < RetreatDuration)
	{
		FVector Direction = CurrentLocation - CounterAttackPosition;
		Direction.Z = 0.0f;
		Direction.Normalize();

		MoveInDirection(Direction, 0.7f, DeltaTime);

		FRotator TargetRotation = Direction.Rotation();
		FRotator NewRotation = FMath::RInterpTo(
			OwnerEnemy->GetActorRotation(),
			TargetRotation,
			DeltaTime,
			5.0f
		);
		OwnerEnemy->SetActorRotation(NewRotation);
	}
	// Phase 2: 역습 (counter_attack_position으로 급습)
	else
	{
		FVector Direction = CounterAttackPosition - CurrentLocation;
		Direction.Z = 0.0f;

		float Distance = Direction.Size();
		if (Distance < 100.0f)
		{
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
			return;
		}

		Direction.Normalize();
		MoveInDirection(Direction, 1.5f, DeltaTime);

		FRotator TargetRotation = Direction.Rotation();
		FRotator NewRotation = FMath::RInterpTo(
			OwnerEnemy->GetActorRotation(),
			TargetRotation,
			DeltaTime,
			10.0f
		);
		OwnerEnemy->SetActorRotation(NewRotation);
	}
}

// CoinChase
void UAIDirectiveComponent::ExecuteCoinChase(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}
	if (!IsValid(TargetCoin))
	{
		TargetCoin = FindNearestCoin();
	}
	if (!IsValid(TargetCoin))
	{
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		return;
	}

	// [개선] 코인 획득 거리 체크 (실제로 먹었을 때만 실행)
	float Distance = FVector::Dist(OwnerEnemy->GetActorLocation(), TargetCoin->GetActorLocation());
	if (Distance < 150.0f)
	{
		CoinEatCount++;

		if (CoinEatitemUse > 0 && CoinEatCount % CoinEatitemUse == 0)
		{
			HidePlayerCamera();
		}
		
		// MoveToCoin 내부에서 TargetCoin을 nullptr로 설정하여 중복 카운트 방지
	}

	MoveToCoin(DeltaTime);
}

void UAIDirectiveComponent::HidePlayerCamera()
{
	if (!GetWorld()) return;

	// 모든 플레이어 컨트롤러를 순회하여 '지휘관' 역할만 찾음
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC)
		{
			ACitRushPlayerState* PS = PC->GetPlayerState<ACitRushPlayerState>();
			if (PS && PS->GetPlayerRole() == EPlayerRole::Commander)
			{
				// [수정] 50% Gray 페이드 적용 (Alpha = 0.5)
				// ClientSetCameraFade(bEnable, Color, Alpha(Start, End), Time, bAudio)
				PC->ClientSetCameraFade(true, FColor(128, 128, 128), FVector2D(0.0f, 0.5f), 0.5f, false);

				// 2초 후 다시 원래대로 복구하는 타이머 설정
				FTimerDelegate RestoreDelegate;
				RestoreDelegate.BindWeakLambda(this, [PC]()
				{
					if (IsValid(PC))
					{
						// 다시 투명하게 (0.5 -> 0.0)
						PC->ClientSetCameraFade(true, FColor(128, 128, 128), FVector2D(0.5f, 0.0f), 0.5f, false);
					}
				});

				GetWorld()->GetTimerManager().SetTimer(CameraHideTimerHandle, RestoreDelegate, 2.0f, false);
			}
		}
	}
}

void UAIDirectiveComponent::ShowPlayerCamera()
{
	// HidePlayerCamera의 타이머 람다에서 직접 복구하므로 비워둡니다.
}

//1-AMBUSH: 매복(특정위치 대기 후 급습)
void UAIDirectiveComponent::ExecuteAmbush(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	float ElapsedTime = GetWorld()->GetTimeSeconds() - StateStartTime;

	// Phase 0: 대기 중 - TriggerDistance 내 플레이어 감지
	if (AmbushPhase == 0)
	{
		APawn* NearestPlayer = FindNearestPlayer();
		if (NearestPlayer)
		{
			float DistanceToPlayer = FVector::Dist(CurrentLocation, NearestPlayer->GetActorLocation());

			if (DistanceToPlayer < CurrentParams.TriggerDistance)
			{
				// 플레이어 감지 시 가장 가까운 아이템(Coin vs Pellet) 탐색
				AmbushPhase = 1;

				ACoinActor* NearestCoin = FindNearestCoin();
				APelletActor* NearestPellet = FindNearestPellet();

				float DistCoin = FLT_MAX;
				float DistPellet = FLT_MAX;

				if (IsValid(NearestCoin))
				{
					DistCoin = FVector::Dist(CurrentLocation, NearestCoin->GetActorLocation());
				}

				if (IsValid(NearestPellet) && NearestPellet->IsAvailable())
				{
					DistPellet = FVector::Dist(CurrentLocation, NearestPellet->GetActorLocation());
				}
				else
				{
					NearestPellet = nullptr;
				}

				// 타겟 초기화
				TargetCoin = nullptr;
				TargetPellet = nullptr;

				// 더 가까운 아이템 선택
				if (NearestCoin && DistCoin <= DistPellet)
				{
					TargetCoin = NearestCoin;
				}
				else if (NearestPellet)
				{
					TargetPellet = NearestPellet;
				}
			}
		}

		if (ElapsedTime > CurrentParams.WaitDuration)
		{
			AmbushPhase = 0; 
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
			return;
		}
	}
	// Phase 1: 주변 아이템(Coin or Pellet) 획득
	else if (AmbushPhase == 1)
	{
		bool bIsPelletTarget = IsValid(TargetPellet) && TargetPellet->IsAvailable();
		bool bIsCoinTarget = IsValid(TargetCoin);

		if (bIsPelletTarget)
		{
			FVector PelletLocation = TargetPellet->GetActorLocation();
			float Distance = FVector::Dist(CurrentLocation, PelletLocation);

			if (Distance < 120.0f)
			{
				// 펠릿 섭취 성공
				if (APixelEnemy* PixelEnemy = Cast<APixelEnemy>(OwnerEnemy))
				{
					PixelEnemy->OnPelletCollected(10.0f); // 10초 파워업
				}
				AmbushPhase = 2; // 공격 단계로
				TargetPellet = nullptr;
				CurrentPathPoints.Empty();
			}
			else
			{
				// 펠릿으로 이동
				FVector Direction = PelletLocation - CurrentLocation;
				Direction.Z = 0.0f;
				Direction.Normalize();
				MoveInDirection(Direction, 1.2f, DeltaTime);
				
				FRotator TargetRotation = Direction.Rotation();
				FRotator CurrentRotation = OwnerEnemy->GetActorRotation();
				float ClampedDT = FMath::Min(DeltaTime, 0.033f);
				FRotator InterpolatedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, ClampedDT, 6.0f);
				OwnerEnemy->SetActorRotation(InterpolatedRotation);
			}
		}
		else if (bIsCoinTarget)
		{
			FVector CoinLocation = TargetCoin->GetActorLocation();
			float Distance = FVector::Dist(CurrentLocation, CoinLocation);

			if (Distance < 120.0f)
			{
				// 코인 섭취(도달) 성공 -> 공격 단계로
				AmbushPhase = 2;
				TargetCoin = nullptr;
				CurrentPathPoints.Empty();
			}
			else
			{
				// 코인으로 이동
				FVector Direction = CoinLocation - CurrentLocation;
				Direction.Z = 0.0f;
				Direction.Normalize();
				MoveInDirection(Direction, 1.2f, DeltaTime);

				FRotator TargetRotation = Direction.Rotation();
				FRotator CurrentRotation = OwnerEnemy->GetActorRotation();
				float ClampedDT = FMath::Min(DeltaTime, 0.033f);
				FRotator InterpolatedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, ClampedDT, 6.0f);
				OwnerEnemy->SetActorRotation(InterpolatedRotation);
			}
		}
		else
		{
			// 타겟이 없거나 유효하지 않게 됨(누가 먹음) -> 즉시 공격 단계로
			AmbushPhase = 2;
		}
	}
	// Phase 2: 기습 공격 - 1.5배 가속하여 레이서 추격
	else if (AmbushPhase == 2)
	{
		APawn* TargetPlayer = FindNearestPlayer();
		if (!TargetPlayer)
		{
			AmbushPhase = 0;
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
			return;
		}

		FVector PlayerLocation = TargetPlayer->GetActorLocation();
		FVector Direction = PlayerLocation - CurrentLocation;
		Direction.Z = 0.0f;

		float Distance = Direction.Size();

		if (Distance < 100.0f)
		{
			AmbushPhase = 0;
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
			return;
		}

		Direction.Normalize();
		// 1.5배 가속하여 공격
		MoveInDirection(Direction, 1.5f, DeltaTime);

		Direction.Normalize();
		FRotator TargetRotation = Direction.Rotation();
		FRotator CurrentRotation = OwnerEnemy->GetActorRotation();
		float ClampedDT = FMath::Min(DeltaTime, 0.033f);
		FRotator InterpolatedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, ClampedDT, 8.0f);
		OwnerEnemy->SetActorRotation(InterpolatedRotation);
	}
}


// 7 | CONSUME_P_POINT | P-Point 섭취/획득
void UAIDirectiveComponent::ExecuteConsumePoint(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}

	ACoinActor* NearestCoin = FindNearestCoin();
	if (NearestCoin)
	{
		FVector CoinLocation = NearestCoin->GetActorLocation();
		FVector CurrentLocation = OwnerEnemy->GetActorLocation();
		float Distance = FVector::Dist(CurrentLocation, CoinLocation);

		if (Distance < 100.0f)
		{
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] CONSUME_POINT: Reached Coin - %s"), *NearestCoin->GetCoinID());
			TargetCoin = nullptr;
			return;
		}

		TargetCoin = NearestCoin;
		CurrentParams.TargetPosition = CoinLocation;
		ExecuteMoveToLocation(DeltaTime);

		//UE_LOG(LogTemp, VeryVerbose, TEXT("[AIDirectiveComponent] CONSUME_POINT: Moving to Coin - Distance: %.1f"), Distance);
		return;
	}

	// 모든 타겟이 없으면 Idle
	TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
}

//8-파워펠릿(P-Pellet) 섭취
void UAIDirectiveComponent::ExecuteConsumePellet(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}
	
	APelletActor* NearestPellet = FindNearestPellet();
	if (NearestPellet && NearestPellet->IsAvailable())
	{
		FVector PelletLocation = NearestPellet->GetActorLocation();
		FVector CurrentLocation = OwnerEnemy->GetActorLocation();
		float Distance = FVector::Dist(CurrentLocation, PelletLocation);

		if (Distance < 100.0f)
		{
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] ExecuteConsumePellet: Reached Pellet"));
			TargetPellet = nullptr;
			return;
		}

		TargetPellet = NearestPellet;
		CurrentParams.TargetPosition = PelletLocation;
		ExecuteMoveToLocation(DeltaTime);

		//UE_LOG(LogTemp, VeryVerbose, TEXT("[AIDirectiveComponent] CONSUME_POINT: Moving to Pellet - Distance: %.1f"), Distance);
		return;
	}

	// 1. 타겟 펠릿 선정 (없거나 유효하지 않은 경우)
	if (!IsValid(TargetPellet) || !TargetPellet->IsAvailable())
	{
		TargetPellet = nullptr;
		CurrentPathPoints.Empty(); // 타겟 변경 시 경로 초기화

		TArray<TWeakObjectPtr<APelletActor>> FoundPelletPtrs;
		if (CachedAISubsystem)
		{
			FoundPelletPtrs = CachedAISubsystem->GetCachedPellets();
		}

		if (FoundPelletPtrs.Num() == 0)
		{
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
			return;
		}

		float MinCost = TNumericLimits<float>::Max();
		APelletActor* BestPellet = nullptr;
		FVector CurrentLocation = OwnerEnemy->GetActorLocation();

		// 최적(NavMesh Cost 최소) 펠릿 탐색
		for (const TWeakObjectPtr<APelletActor>& PelletPtr : FoundPelletPtrs)
		{
			APelletActor* Pellet = PelletPtr.Get();
			if (!Pellet || !Pellet->IsAvailable())
			{
				continue;
			}

			float Cost = GetNavPathCost(CurrentLocation, Pellet->GetActorLocation());

			// NavMesh 경로가 없는 경우, 직선 거리로 대체하되 페널티 부여
			if (Cost < 0.0f)
			{
				float Dist = FVector::Dist(CurrentLocation, Pellet->GetActorLocation());
				Cost = Dist * 10.0f; // 페널티 (경로 없음)
			}

			if (Cost < MinCost)
			{
				MinCost = Cost;
				BestPellet = Pellet;
			}
		}

		if (BestPellet)
		{
			TargetPellet = BestPellet;
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] ConsumePellet: 최적 펠릿 선정 %s (Cost: %.1f)"), *TargetPellet->GetName(), MinCost);
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] ConsumePellet: 획득 가능한 펠릿이 없습니다. -> Idle"));
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
			return;
		}
	}

	if (IsValid(TargetPellet))
	{
		FVector TargetLocation = TargetPellet->GetActorLocation();
		FVector CurrentLocation = OwnerEnemy->GetActorLocation();

		// 섭취 거리 체크 (OverlapRadius + 여유분)
		float Distance = FVector::Dist(CurrentLocation, TargetLocation);
		if (Distance < 120.0f) 
		{
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] ConsumePellet: 펠릿 섭취 및 데이터 전송 %s"), *TargetPellet->GetName());
			// PixelEnemy에게 섭취 알림 (무적 및 쉴드 효과 적용)
			if (APixelEnemy* PixelEnemy = Cast<APixelEnemy>(OwnerEnemy))
			{
				PixelEnemy->OnPelletCollected(10.0f); // 10초 무적
			}

			FString ConsumedTime = FDateTime::Now().ToIso8601();
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] Server Data Sent: p_pellet consumed at %s"), *ConsumedTime);

			// TODO: AIDataManager에 섭취 정보 전달 (인터페이스 필요)
			// if (AIDataManager) AIDataManager->ReportPelletConsumption(TargetPellet->GetName(), ConsumedTime);

			// [수정] 펠릿 섭취 후 후퇴(Retreat) - 안전 지대로 이탈
			// "pellet을 먹고 rotaion하지 않고 뒤로 후진"을 위해 Retreat 상태로 전환
			FDirectiveParams RetreatParams;
			APawn* NearestPlayer = FindNearestPlayer();
			if (NearestPlayer)
			{
				FVector DirToPlayer = NearestPlayer->GetActorLocation() - CurrentLocation;
				DirToPlayer.Z = 0.0f;
				DirToPlayer.Normalize();
				// 플레이어 반대 방향으로 2000cm(20m) 후퇴
				RetreatParams.TargetPosition = CurrentLocation - (DirToPlayer * 2000.0f);
			}
			else
			{
				// 플레이어가 없으면 현재 바라보는 방향 반대로 후퇴
				RetreatParams.TargetPosition = CurrentLocation - (OwnerEnemy->GetActorForwardVector() * 2000.0f);
			}

			TargetPellet = nullptr;
			TransitionToState(EAIDirectiveState::Retreat, RetreatParams);
			return;
		}

		// NavMesh 경로 찾기 (경로가 없거나 타겟이 변경되었을 때)
		if (CurrentPathPoints.Num() == 0)
		{
			if (FindNavMeshPath(CurrentLocation, TargetLocation, CurrentPathPoints))
			{
				CurrentPathPointIndex = 0;
				//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] ConsumePellet: NavMesh path found with %d points"), CurrentPathPoints.Num());
			}
			else
			{
				//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] ConsumePellet: NavMesh path failed - using direct movement"));
			}
		}

		float MoveSpeed = CurrentParams.EmergencyPriority > 0 ? 1.5f : 1.2f; // 긴급 시 속도 증가
		bool bReached = false;
		if (CurrentPathPoints.Num() > 0)
		{
			bReached = MoveAlongPath(DeltaTime, MoveSpeed);
			if (bReached)
			{
				CurrentPathPoints.Empty(); // 경로 끝 도달 시 재계산 유도
			}
		}
		else
		{
			// NavMesh 경로 실패 시 직선 이동 (Fallback)
			FVector Direction = TargetLocation - CurrentLocation;
			Direction.Z = 0.0f;
			Direction.Normalize();
			MoveInDirection(Direction, MoveSpeed, DeltaTime);

			/*FRotator TargetRotation = Direction.Rotation();
			FRotator NewRotation = FMath::RInterpTo(OwnerEnemy->GetActorRotation(), TargetRotation, DeltaTime, 10.0f);
			OwnerEnemy->SetActorRotation(NewRotation);*/
		}

		//UE_LOG(LogTemp, VeryVerbose, TEXT("[AIDirectiveComponent] ConsumePellet: Moving to Pellet - Distance: %.1f"), Distance);
		return;
	}

	// 타겟 펠릿이 없으면 Idle로 전환
	//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] ConsumePellet: No target pellet - transitioning to Idle"));
	TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
}

// 9 | GUARD | 방어 - 특정 위치/대상 방어 (v1.4.0 API)
void UAIDirectiveComponent::ExecuteGuard(float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}

	// guard_target 필수 파라미터 체크
	if (CurrentParams.GuardTarget.IsEmpty() && CurrentParams.TargetPosition.IsZero())
	{
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		return;
	}

	// duration 체크 (선택 파라미터)
	if (CurrentParams.GuardDuration > 0.0f)
	{
		float ElapsedTime = GetWorld()->GetTimeSeconds() - GuardStartTime;
		if (ElapsedTime >= CurrentParams.GuardDuration)
		{
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
			return;
		}
	}

	// 방어 위치 결정
	FVector GuardPosition = FVector::ZeroVector;

	if (!CurrentParams.TargetPosition.IsZero())
	{
		// guard_target이 위치인 경우
		GuardPosition = CurrentParams.TargetPosition;
	}
	else if (CurrentParams.GuardTarget.StartsWith(TEXT("steam_")))
	{
		// 특정 플레이어 방어
		APawn* TargetPlayer = FindPlayerByID(CurrentParams.GuardTarget);
		if (TargetPlayer)
		{
			GuardPosition = TargetPlayer->GetActorLocation();
		}
		else
		{
			TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
			return;
		}
	}
	else
	{
		// 기본: 가장 가까운 플레이어 근처 방어
		APawn* NearestPlayer = FindNearestPlayer();
		if (NearestPlayer)
		{
			GuardPosition = NearestPlayer->GetActorLocation();
		}
		else
		{
			GuardPosition = OwnerEnemy->GetActorLocation();
		}
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	FVector Direction = GuardPosition - CurrentLocation;
	Direction.Z = 0.0f;
	float Distance = Direction.Size();

	// guard_radius 기본값 설정 (선택 파라미터)
	float GuardRadius = CurrentParams.GuardRadius > 0.0f ? CurrentParams.GuardRadius : 500.0f;

	// GuardRadius 밖이면 방어 위치로 복귀
	if (Distance > GuardRadius)
	{
		Direction.Normalize();
		MoveInDirection(Direction, 0.8f, DeltaTime);

		FRotator TargetRotation = Direction.Rotation();
		FRotator NewRotation = FMath::RInterpTo(
			OwnerEnemy->GetActorRotation(),
			TargetRotation,
			DeltaTime,
			3.0f
		);
		OwnerEnemy->SetActorRotation(NewRotation);
	}
	else
	{
		// GuardRadius 내에서 경계 (플레이어 주시)
		APawn* NearestPlayer = FindNearestPlayer();
		if (NearestPlayer)
		{
			FVector LookDirection = NearestPlayer->GetActorLocation() - CurrentLocation;
			LookDirection.Z = 0.0f;
			if (!LookDirection.IsNearlyZero())
			{
				LookDirection.Normalize();
				FRotator TargetRotation = LookDirection.Rotation();
				FRotator NewRotation = FMath::RInterpTo(
					OwnerEnemy->GetActorRotation(),
					TargetRotation,
					DeltaTime,
					2.0f
				);
				OwnerEnemy->SetActorRotation(NewRotation);
			}
		}
	}
}

// 10-측면 우회 – 플레이어 측면 공략
void UAIDirectiveComponent::ExecuteFlank(float DeltaTime)
{
	
	if (!OwnerEnemy)
	{
		return;
	}

	APawn* TargetPlayer = FindNearestPlayer();
	if (!TargetPlayer)
	{
		TransitionToState(EAIDirectiveState::Idle, FDirectiveParams());
		return;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	FVector PlayerLocation = TargetPlayer->GetActorLocation();
	FVector PlayerForward = TargetPlayer->GetActorForwardVector();

	// FlankTargetPosition 계산 (처음 또는 목표 위치가 멀어졌을 때)
	if (FlankTargetPosition.IsZero() || FVector::Dist(CurrentLocation, FlankTargetPosition) > 1000.0f)
	{
		FVector FlankOffset;
		if (CurrentParams.FlankDirection.Equals(TEXT("LEFT"), ESearchCase::IgnoreCase))
		{
			FlankOffset = FVector::CrossProduct(PlayerForward, FVector::UpVector) * -1.0f;
		}
		else
		{
			FlankOffset = FVector::CrossProduct(PlayerForward, FVector::UpVector);
		}
		FlankOffset.Normalize();

		FlankTargetPosition = PlayerLocation + FlankOffset * CurrentParams.FlankDistance;
		FlankTargetPosition.Z = PlayerLocation.Z;

		// 새로운 목표 위치가 계산되면 경로 재계산
		CurrentPathPoints.Empty();
		CurrentPathPointIndex = 0;

		//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] FLANK: Calculated flank position (Direction=%s, Distance=%.1f)"),*CurrentParams.FlankDirection, CurrentParams.FlankDistance);
	}

	// NavMesh 경로가 없으면 경로 찾기
	if (CurrentPathPoints.Num() == 0)
	{
		//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] Flank: Finding NavMesh path to flank position"));

		if (FindNavMeshPath(CurrentLocation, FlankTargetPosition, CurrentPathPoints))
		{
			CurrentPathPointIndex = 0;
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] Flank: NavMesh path found with %d points"),CurrentPathPoints.Num());
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] Flank: NavMesh path failed - using direct movement"));
		}
	}

	// NavMesh 경로가 있으면 경로를 따라 이동
	if (CurrentPathPoints.Num() > 0)
	{
		bool bReachedDestination = MoveAlongPath(DeltaTime, 2.0f); // 빠른 속도

		if (bReachedDestination)
		{
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] FLANK: Reached flank position (NavMesh) - switching to Chase"));

			FDirectiveParams ChaseParams;
			ChaseParams.TargetPlayerId = CurrentParams.TargetPlayerId;
			ChaseParams.SpeedFactor = 1.2f;
			ChaseParams.MaxChaseDuration = 8.0f;
			FlankTargetPosition = FVector::ZeroVector;
			TransitionToState(EAIDirectiveState::Chase, ChaseParams);
		}
		return;
	}

	// Fallback: 직선 이동
	FVector Direction = FlankTargetPosition - CurrentLocation;
	Direction.Z = 0.0f;
	float Distance = Direction.Size();

	if (Distance < 100.0f)
	{
		//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] FLANK: Reached flank position (직선) - switching to Chase"));

		FDirectiveParams ChaseParams;
		ChaseParams.TargetPlayerId = CurrentParams.TargetPlayerId;
		ChaseParams.SpeedFactor = 1.2f;
		ChaseParams.MaxChaseDuration = 8.0f;
		FlankTargetPosition = FVector::ZeroVector;
		TransitionToState(EAIDirectiveState::Chase, ChaseParams);
		return;
	}

	Direction.Normalize();
	MoveInDirection(Direction, 2.f, DeltaTime);
	FRotator TargetRotation = Direction.Rotation();
	FRotator NewRotation = FMath::RInterpTo(
		OwnerEnemy->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		6.0f
	);
	OwnerEnemy->SetActorRotation(NewRotation);

	//UE_LOG(LogTemp, VeryVerbose, TEXT("[AIDirectiveComponent] ExecuteFlank: Distance to flank position %.1f"), Distance);
}

//player 찾기
APawn* UAIDirectiveComponent::FindPlayerByID(const FString& PlayerID)
{
	if (!OwnerEnemy || PlayerID.IsEmpty())
	{
		return nullptr;
	}

	UWorld* World = OwnerEnemy->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return nullptr;
	}

	// PlayerID로 찾는 경우, 가장 가까운 AbstractRacer를 반환
	if (PlayerID.StartsWith(TEXT("steam_")))
	{
		// AbstractRacer를 상속받은 가장 가까운 Pawn 찾기
		FVector CurrentLocation = OwnerEnemy->GetActorLocation();
		float MinDistance = TNumericLimits<float>::Max();
		APawn* NearestRacer = nullptr;

		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (!PS || !PS->GetPawn())
			{
				continue;
			}

			AAbstractRacer* Racer = Cast<AAbstractRacer>(PS->GetPawn());
			if (!Racer)
			{
				continue;
			}

			ACitRushPlayerState* CitRushPS = Cast<ACitRushPlayerState>(PS);
			if (CitRushPS && CitRushPS->GetPlayerRole() != EPlayerRole::Racer)
			{
				continue;
			}

			float Distance = FVector::Dist(CurrentLocation, Racer->GetActorLocation());
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				NearestRacer = Racer;
			}
		}

		if (NearestRacer)
		{
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] FindPlayerByID: Found nearest Racer (Distance: %.1f)"), MinDistance);
			return NearestRacer;
		}
	}
	else
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (PS && PS->GetPawn())
			{
				FString PSPlayerID = FString::FromInt(PS->GetPlayerId());
				if (PSPlayerID == PlayerID)
				{
					AAbstractRacer* Racer = Cast<AAbstractRacer>(PS->GetPawn());
					if (Racer)
					{
						return Racer;
					}
				}
			}
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] FindPlayerByID: Player not found for ID: %s"), *PlayerID);
	return nullptr;
}

APawn* UAIDirectiveComponent::FindNearestPlayer()
{
	if (!OwnerEnemy)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	float MinDistance = TNumericLimits<float>::Max();
	APawn* NearestPlayer = nullptr;

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (!PC || !PC->GetPawn())
		{
			continue;
		}

		APawn* PlayerPawn = PC->GetPawn();

		AAbstractRacer* Racer = Cast<AAbstractRacer>(PlayerPawn);
		if (!Racer)
		{
			continue;
		}

		APlayerState* PS = PC->GetPlayerState<APlayerState>();
		if (PS)
		{
			ACitRushPlayerState* CitRushPS = Cast<ACitRushPlayerState>(PS);
			if (CitRushPS && CitRushPS->GetPlayerRole() != EPlayerRole::Racer)
			{
				continue;
			}
		}

		float Distance = FVector::Dist(CurrentLocation, PlayerPawn->GetActorLocation());
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			NearestPlayer = PlayerPawn;
		}
	}

	return NearestPlayer;
}

AVehicleDemoCejCar* UAIDirectiveComponent::FindNearestVehicleCar()
{
	if (!OwnerEnemy)
	{
		return nullptr;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	float MinDistance = TNumericLimits<float>::Max();
	AVehicleDemoCejCar* NearestCar = nullptr;

	if (CachedAISubsystem)
	{
		for (const TWeakObjectPtr<AVehicleDemoCejCar>& CarPtr : CachedAISubsystem->GetCachedVehicles())
		{
			AVehicleDemoCejCar* VehicleCar = CarPtr.Get();
			if (VehicleCar)
			{
				float Distance = FVector::Dist(CurrentLocation, VehicleCar->GetActorLocation());
				if (Distance < MinDistance)
				{
					MinDistance = Distance;
					NearestCar = VehicleCar;
				}
			}
		}
	}

	return NearestCar;
}

void UAIDirectiveComponent::MoveInDirection(const FVector& Direction, float SpeedFactor, float DeltaTime)
{
	if (!OwnerEnemy)
	{
		return;
	}

	UCharacterMovementComponent* CharMovement = OwnerEnemy->GetCharacterMovement();
	if (!CharMovement)
	{
		return;
	}

	// Direction은 이미 정규화되어 있어야 함
	FVector NormalizedDirection = Direction;
	if (!NormalizedDirection.IsNormalized())
	{
		NormalizedDirection.Normalize();
	}

	//  SpeedFactor를 MaxFlySpeed에 적용 
	// APixelEnemy로 캐스팅하여 기본 속도 가져오기
	if (APixelEnemy* PixelEnemy = Cast<APixelEnemy>(OwnerEnemy))
	{
		CharMovement->MaxFlySpeed = PixelEnemy->MoveSpeed * SpeedFactor;
	}
	else
	{
		CharMovement->MaxFlySpeed = 4000.0f * SpeedFactor;
	}

	// AddMovementInput은 항상 1.0f로 입력 (방향만 전달)
	OwnerEnemy->AddMovementInput(NormalizedDirection, 1.0f);
}

ACoinActor* UAIDirectiveComponent::FindNearestCoin()
{
	if (!OwnerEnemy)
	{
		return nullptr;
	}

	if (!CachedAISubsystem)
	{
		return nullptr;
	}

	const auto& CachedCoinList = CachedAISubsystem->GetCachedCoins();

	if (CachedCoinList.Num() == 0)
	{
		return nullptr;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	float MinDistance = TNumericLimits<float>::Max();
	ACoinActor* NearestCoin = nullptr;

	for (const TWeakObjectPtr<ACoinActor>& CoinPtr : CachedCoinList)
	{
		ACoinActor* Coin = CoinPtr.Get();
		if (!IsValid(Coin))
		{
			continue;
		}

		float Distance = FVector::Dist(CurrentLocation, Coin->GetActorLocation());
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			NearestCoin = Coin;
		}
	}

	if (NearestCoin)
	{
		/*UE_LOG(LogTemp, VeryVerbose, TEXT("[AIDirectiveComponent] 가장 가까운 Coin 발견: %s (거리: %.1f)"),
			*NearestCoin->GetName(), MinDistance);*/
	}

	return NearestCoin;
}

APelletActor* UAIDirectiveComponent::FindNearestPellet()
{
	if (!OwnerEnemy)
	{
		return nullptr;
	}

	if (!CachedAISubsystem)
	{
		return nullptr;
	}

	const auto& CachedPelletList = CachedAISubsystem->GetCachedPellets();

	if (CachedPelletList.Num() == 0)
	{
		return nullptr;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	float MinDistance = TNumericLimits<float>::Max();
	APelletActor* NearestPellet = nullptr;

	for (const TWeakObjectPtr<APelletActor>& PelletPtr : CachedPelletList)
	{
		APelletActor* Pellet = PelletPtr.Get();
		if (!IsValid(Pellet) || !Pellet->IsAvailable())
		{
			continue;
		}

		float Distance = FVector::Dist(CurrentLocation, Pellet->GetActorLocation());
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			NearestPellet = Pellet;
		}
	}

	if (NearestPellet)
	{
		//UE_LOG(LogTemp, VeryVerbose, TEXT("[AIDirectiveComponent] 가장 가까운 Pellet 발견 (거리: %.1f)"), MinDistance);
	}

	return NearestPellet;
}

void UAIDirectiveComponent::MoveToCoin(float DeltaTime)
{
	if (!OwnerEnemy || !IsValid(TargetCoin))
	{
		return;
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	FVector CoinLocation = TargetCoin->GetActorLocation();

	FVector Direction = CoinLocation - CurrentLocation;
	Direction.Z = 0.0f;

	float Distance = Direction.Size();

	if (Distance < 100.0f)
	{
		//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] Coin 도달 - 새로운 Coin 탐색"));
		TargetCoin = nullptr;
		FindNearestCoin();
		return;
	}

	Direction.Normalize();
	MoveInDirection(Direction, 1.0f, DeltaTime);

	FRotator TargetRotation = Direction.Rotation();
	FRotator NewRotation = FMath::RInterpTo(
		OwnerEnemy->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		5.0f
	);
	OwnerEnemy->SetActorRotation(NewRotation);
}

// ========== 헬퍼 함수 ==========
FGameplayTag UAIDirectiveComponent::GetStateTag(EAIDirectiveState State) const
{
	const FGameplayTag* FoundTag = StateTags.Find(State);
	return FoundTag ? *FoundTag : FGameplayTag::EmptyTag;
}

// ========== NavMesh 경로 찾기 ==========
bool UAIDirectiveComponent::FindNavMeshPath(const FVector& StartLocation, const FVector& EndLocation, TArray<FVector>& OutPathPoints)
{
	OutPathPoints.Empty();

	if (!OwnerEnemy)
	{
		//UE_LOG(LogTemp, Error, TEXT("[AIDirectiveComponent] FindNavMeshPath: OwnerEnemy is null"));
		return false;
	}

	UWorld* World = OwnerEnemy->GetWorld();
	if (!World)
	{
		//UE_LOG(LogTemp, Error, TEXT("[AIDirectiveComponent] FindNavMeshPath: World is null"));
		return false;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] FindNavMeshPath: NavigationSystem not found"));
		return false;
	}

	const ANavigationData* NavData = NavSys->GetDefaultNavDataInstance();
	if (!NavData)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] FindNavMeshPath: NavData not found"));
		return false;
	}

	// NavMesh에 시작/끝 위치 투영
	FNavLocation StartNavLoc, EndNavLoc;
	const FVector ProjectExtent(500.0f, 500.0f, 500.0f);

	if (!NavSys->ProjectPointToNavigation(StartLocation, StartNavLoc, ProjectExtent))
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] FindNavMeshPath: Failed to project start location to NavMesh"));
		return false;
	}

	if (!NavSys->ProjectPointToNavigation(EndLocation, EndNavLoc, ProjectExtent))
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] FindNavMeshPath: Failed to project end location to NavMesh"));
		return false;
	}

	// 경로 찾기 쿼리 생성
	FPathFindingQuery Query(OwnerEnemy, *NavData, StartNavLoc.Location, EndNavLoc.Location);
	FPathFindingResult PathResult = NavSys->FindPathSync(Query);

	if (!PathResult.IsSuccessful() || !PathResult.Path.IsValid())
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] FindNavMeshPath: Path finding failed"));
		return false;
	}

	// 경로 포인트 추출
	const TArray<FNavPathPoint>& PathPoints = PathResult.Path->GetPathPoints();
	if (PathPoints.Num() == 0)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] FindNavMeshPath: Path has no points"));
		return false;
	}

	// FNavPathPoint를 FVector로 변환
	for (const FNavPathPoint& PathPoint : PathPoints)
	{
		OutPathPoints.Add(PathPoint.Location);
	}

	/*UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] FindNavMeshPath: Path found with %d points, cost=%.1f, length=%.1f"),
		OutPathPoints.Num(), PathResult.Path->GetCost(), PathResult.Path->GetLength());*/

	return true;
}

float UAIDirectiveComponent::GetNavPathCost(const FVector& StartLocation, const FVector& EndLocation)
{
	if (!OwnerEnemy)
	{
		return -1.0f;
	}

	UWorld* World = OwnerEnemy->GetWorld();
	if (!World)
	{
		return -1.0f;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return -1.0f;
	}

	const ANavigationData* NavData = NavSys->GetDefaultNavDataInstance();
	if (!NavData)
	{
		return -1.0f;
	}

	// NavMesh에 시작/끝 위치 투영
	FNavLocation StartNavLoc, EndNavLoc;
	const FVector ProjectExtent(500.0f, 500.0f, 1000.0f);

	bool bStartOnNavMesh = NavSys->ProjectPointToNavigation(StartLocation, StartNavLoc, ProjectExtent);
	bool bEndOnNavMesh = NavSys->ProjectPointToNavigation(EndLocation, EndNavLoc, ProjectExtent);

	if (!bStartOnNavMesh || !bEndOnNavMesh)
	{
		return -1.0f; // NavMesh 위에 없음
	}

	// 경로 찾기 쿼리 생성
	FPathFindingQuery Query(OwnerEnemy, *NavData, StartNavLoc.Location, EndNavLoc.Location);
	FPathFindingResult PathResult = NavSys->FindPathSync(Query);

	if (PathResult.IsSuccessful() && PathResult.Path.IsValid())
	{
		return PathResult.Path->GetCost();
	}

	return -1.0f;
}

bool UAIDirectiveComponent::MoveAlongPath(float DeltaTime, float SpeedFactor)
{
	if (!OwnerEnemy)
	{
		return false;
	}

	// 경로가 없으면 실패
	if (CurrentPathPoints.Num() == 0)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] MoveAlongPath: No path points"));
		return false;
	}

	// 현재 경로 포인트 인덱스 유효성 체크
	if (CurrentPathPointIndex < 0 || CurrentPathPointIndex >= CurrentPathPoints.Num())
	{
		//UE_LOG(LogTemp, Warning, TEXT("[AIDirectiveComponent] MoveAlongPath: Invalid path point index %d/%d"),CurrentPathPointIndex, CurrentPathPoints.Num());
		return true; // 경로 완료
	}

	FVector CurrentLocation = OwnerEnemy->GetActorLocation();
	FVector TargetPoint = CurrentPathPoints[CurrentPathPointIndex];

	// 2D 거리 계산 (Z축 무시)
	FVector Direction = TargetPoint - CurrentLocation;
	Direction.Z = 0.0f;
	float Distance = Direction.Size();

	// 현재 웨이포인트 도달
	if (Distance < PathPointTolerance)
	{
		CurrentPathPointIndex++;

		// 마지막 웨이포인트 도달
		if (CurrentPathPointIndex >= CurrentPathPoints.Num())
		{
			//UE_LOG(LogTemp, Log, TEXT("[AIDirectiveComponent] MoveAlongPath: Reached final destination"));
			CurrentPathPoints.Empty();
			CurrentPathPointIndex = 0;
			return true; // 경로 완료
		}

		//UE_LOG(LogTemp, Verbose, TEXT("[AIDirectiveComponent] MoveAlongPath: Reached waypoint %d/%d"),CurrentPathPointIndex, CurrentPathPoints.Num());
	}

	// 다음 웨이포인트로 이동
	Direction.Normalize();

	// AddMovementInput 대신 직접 위치 설정 (AIController 의존성 제거)
	UCharacterMovementComponent* CharMovement = OwnerEnemy->GetCharacterMovement();
	if (CharMovement)
	{
		float MoveSpeed = CharMovement->MaxWalkSpeed * SpeedFactor;
		FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;
		OwnerEnemy->SetActorLocation(NewLocation, true);
	}

	// 회전
	FRotator TargetRotation = Direction.Rotation();
	FRotator NewRotation = FMath::RInterpTo(
		OwnerEnemy->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		5.0f
	);
	OwnerEnemy->SetActorRotation(NewRotation);

	return false; // 아직 이동 중
}
