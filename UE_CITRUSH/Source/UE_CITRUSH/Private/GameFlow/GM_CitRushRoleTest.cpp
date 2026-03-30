// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFlow/GM_CitRushRoleTest.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"
#include "Player/AbstractCommander.h"
#include "Player/AbstractRacer.h"
#include "Engine/World.h"

AGM_CitRushRoleTest::AGM_CitRushRoleTest()
{
	// 기본 Pawn 클래스 설정 (나중에 역할에 따라 교체됨)
	DefaultPawnClass = APawn::StaticClass();
}

TArray<ACitRushPlayerState*> AGM_CitRushRoleTest::GetCitRushPlayers() const
{
	TArray<ACitRushPlayerState*> Out;
	if (AGameStateBase* GS = GetGameState<AGameStateBase>())
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (ACitRushPlayerState* CRPS = Cast<ACitRushPlayerState>(PS))
			{
				Out.Add(CRPS);
			}
		}
	}
	return Out;
}

void AGM_CitRushRoleTest::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	// 서버에서만 실행
	if (!HasAuthority() || !NewPlayer)
	{
		return;
	}

	ACitRushPlayerState* NewPS = NewPlayer->GetPlayerState<ACitRushPlayerState>();
	if (!NewPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] PostLogin: NewPS is null"));
		return;
	}

	// 이미 역할이 설정되어 있으면 스킵
	if (NewPS->GetPlayerRole() != EPlayerRole::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] PostLogin: %s already has role %d"), 
			*NewPS->GetPlayerName(), (int32)NewPS->GetPlayerRole());
		return;
	}

	// 현재 등록된 모든 플레이어 가져오기
	const TArray<ACitRushPlayerState*> Players = GetCitRushPlayers();
	
	// 이미 Commander가 있는지 확인
	bool bHasCommander = false;
	for (const ACitRushPlayerState* PS : Players)
	{
		if (PS && PS != NewPS && PS->GetPlayerRole() == EPlayerRole::Commander)
		{
			bHasCommander = true;
			UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] Found existing Commander: %s"), *PS->GetPlayerName());
			break;
		}
	}

	// 역할 할당: Commander가 없으면 Commander, 있으면 Racer
	EPlayerRole AssignedRole = bHasCommander ? EPlayerRole::Racer : EPlayerRole::Commander;
	
	// 역할 설정 (ServerRPC 사용 - 서버에서도 작동하며 OnRep_PlayerRole 자동 호출)
	NewPS->ServerRPC_RoleChange(AssignedRole);

	UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] %s -> %s | PlayerArray=%d"),
		*NewPS->GetPlayerName(),
		AssignedRole == EPlayerRole::Commander ? TEXT("Commander") : TEXT("Racer"),
		Players.Num());
}

void AGM_CitRushRoleTest::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (bStartPlayersAsSpectators || MustSpectate(NewPlayer) || !PlayerCanRestart(NewPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] HandleStartingNewPlayer: Player cannot restart"));
		return;
	}
	
	if (!NewPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("[RoleTestGM] HandleStartingNewPlayer: NewPlayer is null"));
		return;
	}
	
	// PlayerState 가져오기
	ACitRushPlayerState* cPS = NewPlayer->GetPlayerState<ACitRushPlayerState>();
	if (!cPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] HandleStartingNewPlayer: PlayerState is null"));
		return;
	}
	
	// PlayerRole 확인
	EPlayerRole role = cPS->GetPlayerRole();
	const UEnum* enumPtr = StaticEnum<EPlayerRole>();
	UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] HandleStartingNewPlayer: Player %s has role %s (%d)"), 
		*cPS->GetPlayerName(), 
		*enumPtr->GetNameStringByIndex(static_cast<int32>(role)),
		static_cast<int32>(role));
	
	// 역할이 설정되지 않았으면 기본 Pawn 유지
	if (role == EPlayerRole::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] HandleStartingNewPlayer: Role is None, using default pawn"));
		return;
	}
	
	// 현재 Pawn 저장
	APawn* CurrentPawn = NewPlayer->GetPawn();
	
	// PlayerRole에 따라 Pawn Class 선택
	TSubclassOf<APawn> PawnClassToSpawn = nullptr;
	
	switch (role)
	{
	case EPlayerRole::Commander:
		PawnClassToSpawn = CommanderPawnClass;
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] Spawning Commander for %s"), *cPS->GetPlayerName());
		break;
	
	case EPlayerRole::Racer:
		PawnClassToSpawn = RacerPawnClass;
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] Spawning Racer for %s"), *cPS->GetPlayerName());
		break;
	
	case EPlayerRole::Spectator:
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] Player %s is Spectator, keeping default pawn"), *cPS->GetPlayerName());
		return;
	
	case EPlayerRole::None:
	default:
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] Invalid role for %s"), *cPS->GetPlayerName());
		return;
	}
	
	// Pawn Class가 없으면 종료
	if (!PawnClassToSpawn)
	{
		UE_LOG(LogTemp, Error, TEXT("[RoleTestGM] PawnClass not set for role %d (CommanderPawnClass=%p, RacerPawnClass=%p)"), 
			static_cast<int32>(role), CommanderPawnClass.Get(), RacerPawnClass.Get());
		return;
	}
	
	// 현재 Pawn과 같은 클래스면 교체 불필요
	if (CurrentPawn && CurrentPawn->IsA(PawnClassToSpawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] Player %s already has correct pawn class"), *cPS->GetPlayerName());
		return;
	}
	
	// PlayerStart 위치 가져오기
	AActor* StartSpot = FindPlayerStart(NewPlayer);
	FVector SpawnLocation = StartSpot ? StartSpot->GetActorLocation() : FVector::ZeroVector;
	FRotator SpawnRotation = StartSpot ? StartSpot->GetActorRotation() : FRotator::ZeroRotator;
	
	// 기존 Pawn 위치 사용 (StartSpot이 없는 경우)
	if (CurrentPawn && !StartSpot)
	{
		SpawnLocation = CurrentPawn->GetActorLocation();
		SpawnRotation = CurrentPawn->GetActorRotation();
	}
	
	// 새 Pawn 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(PawnClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (!NewPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("[RoleTestGM] Failed to spawn new pawn for %s"), *cPS->GetPlayerName());
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] Successfully spawned %s for %s"), *PawnClassToSpawn->GetName(), *cPS->GetPlayerName());
	
	// Controller Unpossess & Possess
	if (CurrentPawn)
	{
		NewPlayer->UnPossess();
		CurrentPawn->Destroy();
	}
	
	NewPlayer->Possess(NewPawn);
	
	UE_LOG(LogTemp, Warning, TEXT("[RoleTestGM] Player %s possessed %s"), *cPS->GetPlayerName(), *NewPawn->GetName());
}
