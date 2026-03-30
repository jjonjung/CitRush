// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFlow/CitRushGameState.h"

#include "OnlineSubsystemUtils.h"
#include "Net/UnrealNetwork.h"
#include "Player/CitRushPlayerState.h"
#include "TimerManager.h"
#include "Data/RatingELO.h"
#include "Engine/World.h"
#include "GameFlow/CitRushGameMode.h"
#include "Player/Stats//MapBoundsActor.h"
#include "Player/Stats/PingObjectiveSphere.h"
#include "Player/Stats/PingMarkerManager.h"
#include "EngineUtils.h"
#include "GameFlow/CitRushGameInstance.h"
#include "GameFlow/LocalDataFlowSubsystem.h"
#include "Utility/ContainerHelper.h"
#include "Utility/DebugHelper.h"
//#include <limits>

// 한 종류의 핑이 동시에 존재할 수 있는 최대 개수
#define MAX_PINGS_PER_TYPE 5

ACitRushGameState::ACitRushGameState()
{
	//bReplicates = true; // TODO: 없으면 안되는거면 다시 원복
	CitRushPSs.Reserve(8);
}

void ACitRushGameState::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACitRushGameState::AddPlayerState(APlayerState* PlayerState)
{
	PlayerState->bHasBeenWelcomed = false;
	Super::AddPlayerState(PlayerState);

	if (ACitRushPlayerState* cPS = Cast<ACitRushPlayerState>(PlayerState))
	{
		CITRUSH_TIME("Loading");
		if (HasAuthority()/* && bLoadWorld*/)
		{
			cPS->OnTravelEnd.AddUObject(this, &ACitRushGameState::AddCustomPlayerArray);
			cPS->HasAuthority() ?
				cPS->ClientRPC_ApplyPlayerNickName_Implementation()
				: cPS->ClientRPC_ApplyPlayerNickName();
		}
	}
}

void ACitRushGameState::RemovePlayerState(APlayerState* PlayerState)
{
	if (HasAuthority())
	{
		if (ACitRushPlayerState* cPS = Cast<ACitRushPlayerState>(PlayerState))
		{
			CitRushPSs.RemoveSingle(cPS);
			OnRep_CitRushPlayerArray();
		}
	}
	Super::RemovePlayerState(PlayerState);
}

void ACitRushGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACitRushGameState, CitRushPSs);
	DOREPLIFETIME(ACitRushGameState, AliveRacerArray);
	DOREPLIFETIME(ACitRushGameState, matchStartTime);
	DOREPLIFETIME(ACitRushGameState, matchDuration);
	DOREPLIFETIME(ACitRushGameState, gameEndedTime);
	DOREPLIFETIME(ACitRushGameState, bPlayerVictory);
	DOREPLIFETIME(ACitRushGameState, ActivePing);
	DOREPLIFETIME(ACitRushGameState, bHasActivePing);
	DOREPLIFETIME(ACitRushGameState, ActivePings);
}

#pragma region PlayerTracking
TArray<ACitRushPlayerState*> ACitRushGameState::GetPlayerStatesByRole(EPlayerRole TargetRole) const
{
	TArray<ACitRushPlayerState*> Result;

	for (APlayerState* PS : CitRushPSs)
	{
		if (ACitRushPlayerState* cPS = Cast<ACitRushPlayerState>(PS))
		{
			if (cPS->GetPlayerRole() == TargetRole)
			{
				Result.Add(cPS);
			}
		}
	}

	return Result;
}

TArray<ACitRushPlayerState*> ACitRushGameState::GetCitRushPlayers() const
{
	return CitRushPSs;
}

void ACitRushGameState::NetMulticastRPC_CollectStringData_Implementation(FName key, const FString& value)
{
	GetGameInstance()->GetSubsystem<ULocalDataFlowSubsystem>()->CollectStringData(key, value);
}

void ACitRushGameState::NetMulticastRPC_CollectFloatData_Implementation(FName key, float value)
{
	GetGameInstance()->GetSubsystem<ULocalDataFlowSubsystem>()->CollectFloatData(key, value);
}

void ACitRushGameState::OnRep_CitRushPlayerArray()
{
	for (int32 index = 0; index < CitRushPSs.Num(); ++index)
	{
		ACitRushPlayerState* cPS = CitRushPSs[index];
		if (IsValid(cPS))
		{
			if (cPS->GetPlayerController() && cPS->GetPlayerController()->IsLocalController())
			{
				cPS->ServerRPC_SetLoadingState(ELoadingState::Registered);
			}
		}
		else
		{
			UE_LOG(LogGameMode, Error, TEXT("[ACitRushGameMode] Found InValid PlayerState"))
			return;
		}
	}
	OnPlayerArrayChanged.Broadcast(CitRushPSs);
}


#pragma endregion

#pragma region GameProgress

void ACitRushGameState::NetMulticastRPC_QuitMatch_Implementation(ACitRushPlayerState* requestor)
{
	if (requestor->HasAuthority())
	{
		for (ACitRushPlayerState* cPS : CitRushPSs)
		{
			if (cPS != requestor)
			{
				cPS->ClientRPC_ExitToMainMenu();
			}
		}
		requestor->ClientRPC_ExitToMainMenu_Implementation();
	}
	requestor->ClientRPC_ExitToMainMenu_Implementation();
}

float ACitRushGameState::GetElapsedTime() const
{
	if (matchStartTime <= 0.f)
	{
		return 0.f;
	}

	return GetServerWorldTimeSeconds() - matchStartTime;
}

float ACitRushGameState::GetRemainingTime() const
{
	return FMath::Max(0.f, matchDuration - GetElapsedTime());
}

void ACitRushGameState::OnMatchStart()
{
	if (HasAuthority())
	{
		if (matchDuration > 0.f && !MatchProgressTimer.IsValid())
		{
			GetWorld()->GetTimerManager().SetTimer(MatchProgressTimer
				,this, &ACitRushGameState::TimeUp
				,matchDuration, false
			);
		}
		
		InitializeAliveRacer();
		FindAndCacheMapBounds();
		
		for (ACitRushPlayerState* cPS : CitRushPSs)
		{
			cPS->ServerRPC_SetLoadingState_Implementation(ELoadingState::MatchStarted);
		}

		UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] Game started at %.2f"), matchStartTime);
		matchStartTime = GetServerWorldTimeSeconds() + 0.000001f;
		
		OnRep_MatchStartTime();
	}
}

void ACitRushGameState::AddCustomPlayerArray(ACitRushPlayerState* CitRushPlayerState)
{
	if (!HasAuthority()) {return;}
	if (!IsValid(CitRushPlayerState)) {return;}
	int32 index = ArrayHelper::SortedInsertUnique<ACitRushPlayerState*>(
		CitRushPSs, CitRushPlayerState,
		[](const ACitRushPlayerState* forward, const ACitRushPlayerState* backward)
		{
			check(forward && backward);
			return forward->GetPlayerInfo().targetIndex <= backward->GetPlayerInfo().targetIndex;
		}
	);
	OnRep_CitRushPlayerArray();
	if (GetWorld()->IsPlayInEditor())
	{
		CitRushPlayerState->ServerRPC_SetRacerIndex(index);
	}
	
	CITRUSH_TIME("Loading");
}

void ACitRushGameState::NotifyFirstPlayerInput()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameState] NotifyFirstPlayerInput: 서버가 아닙니다!"));
		return;
	}

	if (bFirstInputReceived)
	{
		UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] NotifyFirstPlayerInput: 이미 첫 입력이 처리되었습니다."));
		return;
	}

	bFirstInputReceived = true;
	UE_LOG(LogTemp, Warning, TEXT("[CitRushGameState] ===== 첫 번째 플레이어 입력 감지! PixelEnemy 활성화 ====="));

	// 델리게이트 브로드캐스트 (PixelEnemy 등이 구독)
	OnFirstPlayerInput.Broadcast();
}

void ACitRushGameState::OnRep_MatchStartTime()
{
	OnMatchStarted.Broadcast();
	GetWorld()->bMatchStarted = true;
	GetGameInstance<UCitRushGameInstance>()->StopLoadingMoviePlayer();
	int32 i = 0;
	GetGameInstance()->GetSubsystem<ULocalDataFlowSubsystem>()->CollectParticipantData(PlayerArray);
}

void ACitRushGameState::OnRep_MatchEnded()
{
	if (gameEndedTime > 0.f)
	{
		//URatingELO::CalculateFinalRatingChanges()

		GEngine->AddOnScreenDebugMessage(44444, 3.f, FColor::Cyan, TEXT("Game Ended!"), true);
		UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] Game ended - Victory: %s, Time: %.2f"),
			bPlayerVictory ? TEXT("true") : TEXT("false"), gameEndedTime);

		// Blueprint/UI에 알림
		OnGameEndedDelegate.Broadcast();
		TMap<FName, FString> data;
		GetGameInstance()->GetSubsystem<ULocalDataFlowSubsystem>()->CollectFloatData(TEXT("Result"), bPlayerVictory ? 1 : -1);
		GetGameInstance()->GetSubsystem<ULocalDataFlowSubsystem>()->CollectStringData(TEXT("Time"), FString::SanitizeFloat(gameEndedTime, 3));
	}
}

void ACitRushGameState::Victory()
{
	if (MatchProgressTimer.IsValid())
	{
		GetWorldTimerManager().ClearTimer(MatchProgressTimer);
		gameEndedTime = GetElapsedTime();
		bPlayerVictory = true;
		if (HasAuthority())
		{
			OnRep_MatchEnded();
		}
	}
}

void ACitRushGameState::Lose()
{
	if (MatchProgressTimer.IsValid())  // only in Auth Host
	{
		GetWorldTimerManager().ClearTimer(MatchProgressTimer);
		gameEndedTime = GetElapsedTime();
		bPlayerVictory = false;
		if (HasAuthority())
		{
			OnRep_MatchEnded();
		}
	}
}

void ACitRushGameState::TimeUp()
{
	if (MatchProgressTimer.IsValid())
	{
		GetWorldTimerManager().ClearTimer(MatchProgressTimer);
		gameEndedTime = matchDuration;
		bPlayerVictory = false;
		if (HasAuthority())
		{
			OnRep_MatchEnded();
		}
	}
}

#pragma endregion

#pragma region SurvivorTracking

void ACitRushGameState::InitializeAliveRacer()
{
	if (!HasAuthority())
	{
		return;
	}

	//racer 강제 할당 
	for (APlayerState* PS : CitRushPSs)
	{
		if (ACitRushPlayerState* CitRushPS = Cast<ACitRushPlayerState>(PS))
		{
			if (CitRushPS->GetPlayerRole() == EPlayerRole::None)
			{
				CitRushPS->ServerRPC_RoleChange_Implementation(EPlayerRole::Racer);
			}
		}
	}
	
	for (APlayerState* racer : GetPlayerStatesByRole(EPlayerRole::Racer))
	{
		AliveRacerArray.Add(true);
	}

	UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] Initialized AliveRacerCount: %d"), AliveRacerArray.Num());
}

void ACitRushGameState::OnRacerDied(ACitRushPlayerState* DiedRacer)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameState] OnRacerDied called on client, ignoring"));
		return;
	}

	if (!IsValid(DiedRacer))
	{
		UE_LOG(LogTemp, Error, TEXT("[CitRushGameState] OnRacerDied: DiedRacer is null"));
		return;
	}

	if (DiedRacer->GetPlayerRole() != EPlayerRole::Racer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameState] OnRacerDied called for non-Racer: %s"),
			*DiedRacer->GetPlayerName());
		return;
	}
	

	UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] Racer died: %s"),
		*DiedRacer->GetPlayerName());

	if (IsValid(DiedRacer))
	{
		int32 index = index = static_cast<int32>(DiedRacer->GetPlayerInfo().targetIndex) - 1;
		if (!AliveRacerArray.IsValidIndex(index)) {return;}
		AliveRacerArray[index] = false;
		
		int32 aliveCount = 0;
		for (const bool& bAlive : AliveRacerArray) {aliveCount += bAlive;}
		
		if (aliveCount == 0)
		{
			if (HasAuthority())
			{
				gameEndedTime = GetElapsedTime();
				bPlayerVictory = false;
				OnRep_MatchEnded();
			}
		}
		
	}
}

void ACitRushGameState::OnRep_AliveRacerArrayChanged() const
{
	OnRacerAliveChangedDelegate.Broadcast(AliveRacerArray);
}

#pragma endregion

#pragma region PingSystem

void ACitRushGameState::SetActivePing(const FVector& WorldLocation, ECommanderPingType Type, float Duration, APlayerState* OwnerPS)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameState] SetActivePing called on client, ignoring"));
		return;
	}

	// 기존 핑이 있으면 ObjectiveSphere 이동, 없으면 생성
	if (bHasActivePing && ObjectiveSphere)
	{
		ObjectiveSphere->SetActorLocation(WorldLocation);
		ObjectiveSphere->ResetScoredPlayers();
	}
	else
	{
		// ObjectiveSphere 생성
		if (!ObjectiveSphere)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			ObjectiveSphere = GetWorld()->SpawnActor<APingObjectiveSphere>(APingObjectiveSphere::StaticClass(), WorldLocation, FRotator::ZeroRotator, SpawnParams);
		}
		else
		{
			ObjectiveSphere->SetActorLocation(WorldLocation);
		}
		
		if (ObjectiveSphere)
		{
			ObjectiveSphere->ResetScoredPlayers();
		}
	}

	// 핑 데이터 설정
	FPingData NewPing(WorldLocation, Type, Duration, OwnerPS);
	NewPing.SpawnServerTime = GetServerWorldTimeSeconds();

	// 타입별 최대 개수 관리
	int32 SameTypeCount = 0;
	for (const FPingData& Ping : ActivePings)
	{
		if (Ping.Type == Type)
		{
			SameTypeCount++;
		}
	}
	if (SameTypeCount >= MAX_PINGS_PER_TYPE)
	{
		// 가장 오래된 핑 제거
		int32 OldestIndex = INDEX_NONE;
		float OldestTime = TNumericLimits<float>::Max();
		for (int32 i = 0; i < ActivePings.Num(); ++i)
		{
			if (ActivePings[i].Type == Type && ActivePings[i].SpawnServerTime < OldestTime)
			{
				OldestTime = ActivePings[i].SpawnServerTime;
				OldestIndex = i;
			}
		}
		if (OldestIndex != INDEX_NONE)
		{
			ActivePings.RemoveAt(OldestIndex);
		}
	}

	ActivePings.Add(NewPing);

	// 기존 단일 ActivePing도 최신 값으로 유지 (기존 HUD 호환)
	ActivePing = NewPing;
	bHasActivePing = true;

	// 만료 타이머: 항상 짧은 간격으로 반복 체크
	// (개별 핑의 Duration/SpawnServerTime 을 이용해 만료 처리)
	if (!PingExpirationTimer.IsValid())
	{
		// 0.1초마다 만료 여부 확인 (부하가 크지 않은 선에서 부드럽게 처리)
		GetWorld()->GetTimerManager().SetTimer(
			PingExpirationTimer,
			this,
			&ACitRushGameState::CheckPingExpiration,
			0.1f,
			true);
	}

	UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] Ping set at %s, Type: %d, Duration: %.2f"), 
		*WorldLocation.ToString(), (int32)Type, Duration);

	// 리스트 변경 알림 (OnRep_ActivePings에서 PingMarkerManager 업데이트 처리)
	OnRep_ActivePings();
}

void ACitRushGameState::ClearActivePing()
{
	if (!HasAuthority())
	{
		return;
	}

	bHasActivePing = false;
	ActivePing = FPingData();
	ActivePings.Empty();
	GetWorld()->GetTimerManager().ClearTimer(PingExpirationTimer);

	if (ObjectiveSphere)
	{
		ObjectiveSphere->Destroy();
		ObjectiveSphere = nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] Ping cleared"));

	OnRep_ActivePing();
	OnRep_ActivePings();
}

void ACitRushGameState::RemovePingByLocation(const FVector& WorldLocation, float Tolerance)
{
	if (!HasAuthority())
	{
		return;
	}

	// 위치 기반으로 가장 가까운 핑 찾기
	int32 ClosestIndex = INDEX_NONE;
	float ClosestDistance = TNumericLimits<float>::Max();

	for (int32 i = 0; i < ActivePings.Num(); ++i)
	{
		const float Distance = FVector::Dist(ActivePings[i].WorldLocation, WorldLocation);
		if (Distance <= Tolerance && Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestIndex = i;
		}
	}

		// 가장 가까운 핑 제거
	if (ClosestIndex != INDEX_NONE)
	{
		UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] RemovePingByLocation: 핑 제거 (위치: %s, 거리: %.2f)"), 
			*ActivePings[ClosestIndex].WorldLocation.ToString(), ClosestDistance);
		
		ActivePings.RemoveAt(ClosestIndex);

		// ActivePings가 비어있으면 전체 핑 제거
		if (ActivePings.Num() == 0)
		{
			bHasActivePing = false;
			ActivePing = FPingData();
			GetWorld()->GetTimerManager().ClearTimer(PingExpirationTimer);

			if (ObjectiveSphere)
			{
				ObjectiveSphere->Destroy();
				ObjectiveSphere = nullptr;
			}
		}
		else
		{
			// 가장 최신 핑을 ActivePing으로 설정
			ActivePing = ActivePings.Last();
		}

		// 서버에서도 UpdatePingMarkerManager를 호출하여 서버의 PingActor도 삭제
		UpdatePingMarkerManager();
		
		// OnRep_ActivePing도 호출하여 Map UI 업데이트 이벤트 브로드캐스트
		OnRep_ActivePing();
		
		// ActivePings 배열 변경 시 자동으로 리플리케이션이 발생하여
		// 클라이언트에서 OnRep_ActivePings()가 호출되어 클라이언트의 PingActor도 삭제됨
		// 서버에서도 OnRep_ActivePings()를 호출하여 서버의 로직도 실행
		OnRep_ActivePings();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameState] RemovePingByLocation: 해당 위치에 핑을 찾을 수 없습니다 (위치: %s, 허용 오차: %.2f)"), 
			*WorldLocation.ToString(), Tolerance);
	}
}

void ACitRushGameState::FindAndCacheMapBounds()
{
	if (!HasAuthority())
	{
		return;
	}

	// TActorIterator로 MapBoundsActor 찾기
	for (TActorIterator<AMapBoundsActor> It(GetWorld()); It; ++It)
	{
		if (AMapBoundsActor* Bounds = *It)
		{
			MapBoundsActor = Bounds;
			UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] MapBoundsActor found: %s"), *Bounds->GetName());
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[CitRushGameState] MapBoundsActor not found in level"));
}

void ACitRushGameState::OnRep_ActivePing()
{
	// 클라이언트에서 핑 업데이트 이벤트 브로드캐스트
	if (bHasActivePing && ActivePing.IsValid())
	{
		OnPingUpdated.Broadcast(ActivePing);
		UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] OnRep_ActivePing: Ping updated at %s"), 
			*ActivePing.WorldLocation.ToString());
	}
	else
	{
		// 핑이 삭제되었을 때도 이벤트 브로드캐스트 (빈 핑 데이터로, MapUI 업데이트를 위해)
		FPingData EmptyPing;
		OnPingUpdated.Broadcast(EmptyPing);
		UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] OnRep_ActivePing: Ping cleared"));
	}
}

void ACitRushGameState::OnRep_HasActivePing()
{
	if (!bHasActivePing)
	{
		// 핑이 제거되었을 때도 이벤트 브로드캐스트 (빈 핑 데이터로)
		FPingData EmptyPing;
		OnPingUpdated.Broadcast(EmptyPing);
		
		// 클라이언트에서 마커 제거
		if (UPingMarkerManager* MarkerManager = UPingMarkerManager::Get(GetWorld(), PingMarkerManagerClass))
		{
			MarkerManager->ClearPingMarker();
		}
		
		UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] OnRep_HasActivePing: Ping cleared"));
	}
}

void ACitRushGameState::OnRep_ActivePings()
{
	UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] OnRep_ActivePings 호출 - ActivePings 수: %d, HasAuthority: %d"), 
		ActivePings.Num(), HasAuthority() ? 1 : 0);
	
	// 최신 핑을 ActivePing으로 반영하여 기존 구독자와 호환
	if (ActivePings.Num() > 0)
	{
		ActivePing = ActivePings.Last();
		bHasActivePing = true;
		OnRep_ActivePing();
	}
	else
	{
		bHasActivePing = false;
		ActivePing = FPingData();
		// 핑이 모두 삭제되었을 때도 OnPingUpdated 호출 (MapUI 업데이트를 위해)
		OnRep_ActivePing();
	}

	// 레벨의 PingActor 동기화 (ActivePings에 없는 PingActor는 자동 삭제됨)
	// 이 함수는 서버와 클라이언트 모두에서 호출되어 각 클라이언트의 로컬 PingActor를 관리
	UpdatePingMarkerManager();
}

void ACitRushGameState::UpdatePingMarkerManager()
{
	if (UPingMarkerManager* MarkerManager = UPingMarkerManager::Get(GetWorld(), PingMarkerManagerClass))
	{
		UE_LOG(LogTemp, Log, TEXT("[CitRushGameState] UpdatePingMarkerManager: PingMarkerManager 찾기 성공 (ActivePings: %d, HasAuthority: %d)"), 
			ActivePings.Num(), HasAuthority() ? 1 : 0);
		if (ActivePings.Num() > 0)
		{
			MarkerManager->RefreshPingMarkers(ActivePings);
		}
		else
		{
			MarkerManager->ClearPingMarker();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameState] UpdatePingMarkerManager: PingMarkerManager를 찾을 수 없습니다! PingMarkerManagerClass가 설정되었는지 확인하세요."));
	}
}

void ACitRushGameState::CheckPingExpiration()
{
	if (!HasAuthority())
	{
		return;
	}

	if (ActivePings.Num() == 0)
	{
		// 더 이상 관리할 핑이 없으면 타이머 정지
		GetWorld()->GetTimerManager().ClearTimer(PingExpirationTimer);
		return;
	}

	const float CurrentTime = GetServerWorldTimeSeconds();

	// 앞에서부터 순회하며 만료된 핑을 하나만 제거 (찍힌 순서대로 차근차근 삭제)
	for (int32 Index = 0; Index < ActivePings.Num(); ++Index)
	{
		const FPingData& Ping = ActivePings[Index];
		if (Ping.Duration > 0.f && Ping.IsExpired(CurrentTime))
		{
			// 만료된 핑을 하나만 제거하고 즉시 반환 (차근차근 삭제)
			ActivePings.RemoveAt(Index);
			
			// 리스트가 변경되었다면 단일 ActivePing/bHasActivePing 을 최신 상태로 유지
			if (ActivePings.Num() > 0)
			{
				ActivePing = ActivePings.Last();
				bHasActivePing = true;
			}
			else
			{
				ActivePing = FPingData();
				bHasActivePing = false;
			}

			// ActivePings 배열 변경 시 자동으로 리플리케이션이 발생하여
			// 클라이언트에서 OnRep_ActivePings()가 호출됨
			// 서버에서도 UpdatePingMarkerManager를 호출하여 서버의 PingActor도 삭제
			UpdatePingMarkerManager();
			
			// OnRep_ActivePing도 호출하여 Map UI 업데이트 이벤트 브로드캐스트
			OnRep_ActivePing();
			
			// 하나만 제거하고 반환 (차근차근 삭제)
			return;
		}
	}

	// 더 이상 남은 핑이 없다면 타이머 중단
	if (ActivePings.Num() == 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(PingExpirationTimer);
	}
}

#pragma endregion
