// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/CitRushGameMode.h"

#include "GameFlow/CitRushGameState.h"
#include "GameFramework/PlayerState.h"
#include "Player/CitRushPlayerState.h"
#include "VehicleTemplate/UE_CITRUSHPlayerController.h"
#include "VehicleTemplate/UE_CITRUSHSportsCar.h"
#include "Data/MapDataAsset.h"
#include "GameFlow/LocalDataFlowSubsystem.h"
#include "Player/AbstractCommander.h"
#include "Utility/MapAssetLoader.h"
#include "Subsystems/EnemyAISubsystem.h"

#include "Interfaces/VoiceInterface.h"
#include "Player/Controller/CitRushPlayerController.h"

ACitRushGameMode::ACitRushGameMode()
{
	bUseSeamlessTravel = true;

	DefaultPawnClass = APawn::StaticClass();
	PlayerControllerClass = ACitRushPlayerController::StaticClass();
	GameStateClass = ACitRushGameState::StaticClass();
	PlayerStateClass = ACitRushPlayerState::StaticClass();
	const UMapAssetLoader* mapAssetLoader = UMapAssetLoader::Get();
	mapAsset = mapAssetLoader->PDA_Map.LoadSynchronous();
	
}

void ACitRushGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GIsPlayInEditorWorld)
	{
		StartGameCountdown();
	}
}

void ACitRushGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ACitRushPlayerController* cPC = Cast<ACitRushPlayerController>(NewPlayer))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] PostLogin called for %s (ptr: %p). Has to Auth Check.")
			, *NewPlayer->GetName(), NewPlayer);

		if (ACitRushPlayerState* PS = NewPlayer->GetPlayerState<ACitRushPlayerState>())
		{
			PS->ServerRPC_SetLoadingState(ELoadingState::PostLogin);
		}

		return;
	}
	UE_LOG(LogTemp, Warning
		, TEXT("[CitRushGameMode] PostLogin called but Not CitRush PC for %s (ptr: %p). Has to Auth Check.")
		, *NewPlayer->GetName(), NewPlayer);
}

void ACitRushGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);
}

void ACitRushGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	if (loadingCheckTimer.IsValid()) {return;}
	CITRUSH_TIME("Loading");
	retryCount = 100;
	GetWorldTimerManager().SetTimer(loadingCheckTimer
		, this, &ACitRushGameMode::CheckAllPlayersReady
		, 0.6f, true
	);
}

/*AActor* ACitRushGameMode::ChoosePlayerStart_Implementation(AController* player)
{
	ACitRushPlayerState* cPS = player->GetPlayerState<ACitRushPlayerState>();
	
	if (!cPS) {return Super::ChoosePlayerStart_Implementation(player);}
	EPlayerRole role = GetPlayerRoleHelper(cPS);
	const UEnum* enumPtr = StaticEnum<EPlayerRole>();
	UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] ChoosePlayerStart // Role : %s"), *enumPtr->GetNameStringByIndex(static_cast<int32>(role)));

	switch (role)
	{
	case EPlayerRole::Commander:
		return FindPlayerStart(player, "Commander");

	case EPlayerRole::Racer:
		if (ACitRushGameState* cGS = Cast<ACitRushGameState>(GameState))
		{
			TArray<ACitRushPlayerState*> racers = cGS->GetPlayerStatesByRole(EPlayerRole::Racer);
			for (int32 i = 0; i < racers.Num(); ++i)
			{
				if (racers[i] == cPS)
				{
					UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] ChoosePlayerStart Racer%d"), i + 1);
					return FindPlayerStart(player, FString::Printf(TEXT("Racer%d"), i + 1));
				}
			}
		}
		return FindPlayerStart(player, "Racer");

	default:
		return Super::ChoosePlayerStart_Implementation(player);
	}
	
}*/

void ACitRushGameMode::Logout(AController* Exiting)
{
	if (APlayerController* PC = Cast<APlayerController>(Exiting))
	{
		//UnregisterVoiceForPlayer(PC);
	}
	
	Super::Logout(Exiting);
}

/*
UClass* ACitRushGameMode::GetDefaultPawnClassForController_Implementation(APlayerController* InController)
{
	if (!InController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] GetDefaultPawnClassForController: InController is null"));
		return Super::GetDefaultPawnClassForController_Implementation(InController);
	}

	// PlayerState 가져오기
	ACitRushPlayerState* PS = InController->GetPlayerState<ACitRushPlayerState>();
	if (!PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] GetDefaultPawnClassForController: PlayerState not found, using default"));
		return Super::GetDefaultPawnClassForController_Implementation(InController);
	}

	// PlayerRole에 따라 Pawn Class 반환
	switch (PS->GetPlayerRole())
	{
	case EPlayerRole::Commander:
		if (CommanderPawnClass)
		{
			UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Selected Commander class for %s"), *PS->GetPlayerName());
			return CommanderPawnClass;
		}
		break;

	case EPlayerRole::Racer:
		if (RacerPawnClass)
		{
			UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Selected Racer class for %s"), *PS->GetPlayerName());
			return RacerPawnClass;
		}
		break;

	case EPlayerRole::Spectator:
	case EPlayerRole::None:
	default:
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] PlayerRole is Spectator/None for %s, using default"), *PS->GetPlayerName());
		break;
	}

	// 기본 클래스 반환
	return Super::GetDefaultPawnClassForController_Implementation(InController);

}
*/

void ACitRushGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (bStartPlayersAsSpectators || MustSpectate(NewPlayer) || !PlayerCanRestart(NewPlayer))
	{
		return;
	}
	
	if (!NewPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("[CitRushGameMode] HandleStartingNewPlayer: NewPlayer is null"));
		return;
	}
	
	ACitRushPlayerState* cPS = NewPlayer->GetPlayerState<ACitRushPlayerState>();
	if (!cPS) {return;}

	EPlayerRole role = GetPlayerRoleHelper(cPS);
	const UEnum* enumPtr = StaticEnum<EPlayerRole>();
	UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] HandleStartingNewPlayer: Player %s has role %s (%d)"), 
		*cPS->GetPlayerName(), 
		*enumPtr->GetNameStringByIndex(static_cast<int32>(role)),
		static_cast<int32>(role));

	APawn* CurrentPawn = NewPlayer->GetPawn();

	TSubclassOf<APawn> PawnClassToSpawn = nullptr;
	AActor* StartSpot = nullptr;
	FVector SpawnLocation = FVector::ZeroVector;
	int32 targetIndex = 0;
		
	if (role == EPlayerRole::None)
	{
		if (GetWorld()->IsPlayInEditor())
		{
			if (NewPlayer->IsLocalController())
			{
				PawnClassToSpawn = CommanderPawnClass;
				StartSpot = FindPlayerStart(NewPlayer, TEXT("Commander"));
				SpawnLocation = StartSpot->GetActorLocation();
			}
			else
			{
				PawnClassToSpawn = RacerPawnClass;
				StartSpot = FindPlayerStart(NewPlayer, TEXT("Racer"));
				SpawnLocation = StartSpot->GetActorLocation();
				targetIndex = static_cast<int32>(cPS->GetPlayerInfo().targetIndex);
				if (targetIndex != INDEX_NONE)
				{
					SpawnLocation += StartSpot->GetActorRightVector() * 300.f * (targetIndex - 1);
				}
			}
		}
	}
	else
	{
		switch (role)
		{
		case EPlayerRole::Commander:
			PawnClassToSpawn = CommanderPawnClass;
			StartSpot = FindPlayerStart(NewPlayer, TEXT("Commander"));
			SpawnLocation = StartSpot->GetActorLocation();
			UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Spawning Commander for %s"), *cPS->GetPlayerName());
			break;

		case EPlayerRole::Racer:
			PawnClassToSpawn = RacerPawnClass;
			StartSpot = FindPlayerStart(NewPlayer, TEXT("Racer"));
			SpawnLocation = StartSpot->GetActorLocation();
			targetIndex = static_cast<int32>(cPS->GetPlayerInfo().targetIndex);
			if (targetIndex != INDEX_NONE)
			{
				SpawnLocation += StartSpot->GetActorRightVector() * 500.f * (targetIndex - 1);
			}
			UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Spawning Racer for %s"), *cPS->GetPlayerName());
			break;

		case EPlayerRole::Spectator:
			UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Player %s is Spectator, keeping default pawn"), *cPS->GetPlayerName());
			break;

		case EPlayerRole::None:
		default:
			UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] Invalid role for %s"), *cPS->GetPlayerName());
			break;
		}
	}
	
    // Pawn Class가 없으면 종료
    if (!PawnClassToSpawn)
    {
        UE_LOG(LogTemp, Error, TEXT("[CitRushGameMode] PawnClass not set for role %d"), static_cast<int32>(cPS->GetPlayerRole()));
        return;
    }

    // 현재 Pawn과 같은 클래스면 교체 불필요
    if (CurrentPawn && CurrentPawn->IsA(PawnClassToSpawn))
    {
        UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Player %s already has correct pawn class"), *cPS->GetPlayerName());

        // Ability만 부여
        GrantAbilitiesToPlayer(CurrentPawn, cPS);
        return;
    }

    // PlayerStart 위치 가져오기
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
        UE_LOG(LogTemp, Error, TEXT("[CitRushGameMode] Failed to spawn new pawn for %s"), *cPS->GetPlayerName());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Successfully spawned %s for %s"), *PawnClassToSpawn->GetName(), *cPS->GetPlayerName());
	if (ACitRushPlayerController* cPC = Cast<ACitRushPlayerController>(NewPlayer))
	{
		if (cPC->HasAuthority())
		{
			cPC->ClientRPC_StartCheckWorldPartition_Implementation(SpawnLocation);
		}
		else
		{
			cPC->ClientRPC_StartCheckWorldPartition(SpawnLocation);
		}
	}

    // Controller Unpossess & Possess
    if (CurrentPawn)
    {
        NewPlayer->UnPossess();
        CurrentPawn->Destroy();
    }

    NewPlayer->Possess(NewPawn);
	spawnedPlayersNum++;

    // Ability 부여
    GrantAbilitiesToPlayer(NewPawn, cPS);

}

void ACitRushGameMode::GrantAbilitiesToPlayer(APawn* PlayerPawn, ACitRushPlayerState* CitRushPlayerState)
{
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("[CitRushGameMode] GrantAbilitiesToPlayer: PlayerPawn is null"));
		return;
	}

	if (!CitRushPlayerState)
	{
		UE_LOG(LogTemp, Error, TEXT("[CitRushGameMode] GrantAbilitiesToPlayer: CitRushPlayerState is null"));
		return;
	}

	// ICitRushPlayerInterface 구현 확인
	ICitRushPlayerInterface* PlayerInterface = Cast<ICitRushPlayerInterface>(PlayerPawn);
	if (!PlayerInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] GrantAbilitiesToPlayer: %s does not implement ICitRushPlayerInterface"), *PlayerPawn->GetName());
		return;
	}

	// PlayerState에서 선택한 Ability 목록 가져오기
	const TArray<FName>& SelectedAbilities = CitRushPlayerState->GetSelectedAbilities();

	if (SelectedAbilities.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] No abilities selected for %s"), *CitRushPlayerState->GetPlayerName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Granting %d abilities to %s for player %s"),
		SelectedAbilities.Num(), *PlayerPawn->GetName(), *CitRushPlayerState->GetPlayerName());

	// 각 Ability를 Pawn에 부여
	int32 SuccessCount = 0;
	for (const FName& AbilityName : SelectedAbilities)
	{
		if (AbilityName.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] Empty ability name, skipping"));
			continue;
		}

		if (PlayerInterface->ReceiveAbility(AbilityName))
		{
			SuccessCount++;
			UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Successfully granted ability '%s' to %s"),
				*AbilityName.ToString(), *CitRushPlayerState->GetPlayerName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[CitRushGameMode] Failed to grant ability '%s' to %s"),
				*AbilityName.ToString(), *CitRushPlayerState->GetPlayerName());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Granted %d/%d abilities to %s"),
		SuccessCount, SelectedAbilities.Num(), *CitRushPlayerState->GetPlayerName());
}

void ACitRushGameMode::CheckAllPlayersReady()
{
	ACitRushGameState* cGS = GetGameState<ACitRushGameState>();
	if (!cGS) return;
	if (!loadingCheckTimer.IsValid()) {return;}

	int32 ReadyPlayers = 0;
	TArray<ACitRushPlayerState*> cPSs = cGS->GetCitRushPlayers();
	/*if (cPSs.Num() < playersNum)
	{
		--retryCount;
		if (retryCount <= 0)
		{
			GetWorldTimerManager().ClearTimer(loadingCheckTimer);
			for (ACitRushPlayerState* cPS : cPSs)
			{
				cPS->ExitMainMenu();
			}
		}
		return;
	}*/
	
	for (ACitRushPlayerState* cPS : cPSs)
	{
		if (cPS->GetLoadingState() == ELoadingState::Ready)
		{
			ReadyPlayers++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Players ready: %d/%d, Registered Player : %d"), ReadyPlayers, playersNum, cPSs.Num());

	// 모두 준비되면 게임 시작
	if (playersNum > 0 && ReadyPlayers == playersNum)
	{
		GetWorldTimerManager().ClearTimer(loadingCheckTimer);
		StartGameCountdown();
	}
	else if (playersNum == 0)
	{
		--retryCount;
		UE_LOG(LogGameMode, Error
			, TEXT("[ACitRushGameMode] Player Num Is Not Replicated, but Ready Player Num Is : %d"), ReadyPlayers);
	}
	/*else if (spawnedPlayersNum >= playersNum)
	{
		GetWorldTimerManager().ClearTimer(loadingCheckTimer);
		StartGameCountdown();
	}*/
}

void ACitRushGameMode::StartGameCountdown()
{
	UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] All players ready! Starting countdown..."));

	// 기존 타이머가 있으면 무시 (중복 실행 방지)
	if (matchStartTimer.IsValid())
	{
		return;
	}
	TArray<ACitRushPlayerState*> cPSs = GetGameState<ACitRushGameState>()->GetCitRushPlayers();
	for (ACitRushPlayerState* cPS : cPSs)
	{
		cPS->ServerRPC_SetLoadingState(ELoadingState::TimerStarted);
	}

	GetWorldTimerManager().SetTimer(
		matchStartTimer,
		this,
		&ACitRushGameMode::StartGame,
		3.0f,
		false
	);
}

void ACitRushGameMode::StartGame()
{
	UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Game Started!"));

	// 1. GameState 상태 변경 -> PixelEnemy::EquipBrain 호출 -> Subsystem에 Enemy 등록
	if (ACitRushGameState* cGS = GetGameState<ACitRushGameState>())
	{
		cGS->OnMatchStart();
	}

	// 2. AI Subsystem에 매치 시작 알림 (시간 전송) -> 등록된 Enemy들에 대해 통신 시작
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UEnemyAISubsystem* AISubsystem = GI->GetSubsystem<UEnemyAISubsystem>())
		{
			FString MatchStartTime = FDateTime::UtcNow().ToIso8601();
			AISubsystem->StartMatch(MatchStartTime);
			UE_LOG(LogTemp, Log, TEXT("[CitRushGameMode] Sent MatchStartTime to AI Subsystem: %s"), *MatchStartTime);
		}
	}
}

EPlayerRole ACitRushGameMode::GetPlayerRoleHelper(const ACitRushPlayerState* cPS) const
{
	if (!cPS) {return EPlayerRole::None;}
	if (ACitRushGameState* cGS = Cast<ACitRushGameState>(GameState);
		GetWorld()->IsPlayInEditor() && cPS->GetPlayerRole() == EPlayerRole::None)
	{
		TArray<ACitRushPlayerState*> players = cGS->GetCitRushPlayers();
		{
			for (int32 i = 0; i < players.Num(); ++i)
			{
				if (cPS == players[i])
				{
					return i == 0 ?
						EPlayerRole::Commander
						: EPlayerRole::Racer;
				}
			}
		}
	}

	return cPS->GetPlayerRole();
}


/*void ACitRushGameMode::RegisterVoiceForPlayer(APlayerController* NewPlayer)
{
	if (!NewPlayer) {return;}

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!OnlineSubsystem) {return;}

	IOnlineVoicePtr VoiceInterface = OnlineSubsystem->GetVoiceInterface();
	if (!VoiceInterface.IsValid()) {return;}

	if (APlayerState* PlayerState = NewPlayer->PlayerState)
	{
		TSharedPtr<const FUniqueNetId> UniqueNetId = PlayerState->GetUniqueId().GetUniqueNetId();
		if (UniqueNetId.IsValid())
		{
			VoiceInterface->RegisterRemoteTalker(*UniqueNetId);
			UE_LOG(LogTemp, Log, TEXT("Registered voice for player: %s"), *PlayerState->GetPlayerName());
		}
	}
}

void ACitRushGameMode::UnregisterVoiceForPlayer(APlayerController* ExitingPlayerController)
{
	if (!ExitingPlayerController) {return;}

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!OnlineSubsystem) {return;}

	IOnlineVoicePtr VoiceInterface = OnlineSubsystem->GetVoiceInterface();
	if (!VoiceInterface.IsValid()) {return;}

	if (APlayerState* PlayerState = ExitingPlayerController->PlayerState)
	{
		TSharedPtr<const FUniqueNetId> UniqueNetId = PlayerState->GetUniqueId().GetUniqueNetId();
		if (UniqueNetId.IsValid())
		{
			VoiceInterface->UnregisterRemoteTalker(*UniqueNetId);
			UE_LOG(LogTemp, Log, TEXT("Unregistered voice for player: %s"), *PlayerState->GetPlayerName());
		}
	}
}*/
