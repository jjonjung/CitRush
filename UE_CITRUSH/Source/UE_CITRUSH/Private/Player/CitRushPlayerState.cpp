// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CitRushPlayerState.h"

#include "GameFlow/CitRushGameInstance.h"
#include "GameFlow/CitRushGameState.h"
#include "GameFlow/LobbyGameState.h"
#include "GameFlow/LocalDataFlowSubsystem.h"
#include "GAS/AbilitySystemComponent/BaseASC.h"
#include "GAS/AttributeSet/ASCommander.h"
#include "GAS/AttributeSet/ASRacer.h"
#include "Net/UnrealNetwork.h"
#include "Network/SteamSubsystem.h"
#include "Player/Controller/CitRushPlayerController.h"
#include "UI/HUD/LobbyHUD.h"
#include "WorldPartition/WorldPartition.h"
#include "Player/Car/VehicleDemoUITest.h"
#include "VehicleTemplate/UE_CITRUSHPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

ACitRushPlayerState::ACitRushPlayerState()
{
	PrimaryActorTick.bCanEverTick = false;
	//SetNetUpdateFrequency(100.f);

	if (abilitySystemComponent == nullptr)
	{
		abilitySystemComponent = CreateDefaultSubobject<UBaseASC>("PlayerASC");
	}
	abilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);  // 소유자에게 Effect 복제
	racerAttributeSet = CreateDefaultSubobject<UASRacer>("RacerAttribute");
	commanderAttributeSet = CreateDefaultSubobject<UASCommander>("CommanderAttribute");
	bUseCustomPlayerNames = true;
	bHasBeenWelcomed = false;
}

void ACitRushPlayerState::ClientRPC_ApplyPlayerNickName_Implementation()
{
	if (APlayerController* pc = GetPlayerController();
		IsValid(pc) && pc->IsLocalController())
	{
		CITRUSH_TIME("Travel")
		if (USteamSubsystem* steamSubsystem = GetGameInstance()->GetSubsystem<USteamSubsystem>())
		{
			FString nickname = steamSubsystem->GetSteamNickname();
			SetPlayerName(nickname);
		}
	}
}

void ACitRushPlayerState::OnRep_PlayerName()
{
	if (APlayerController* pc = GetPlayerController();
		IsValid(pc) && pc->IsLocalController() && !bHasBeenWelcomed)
	{
		int64 now = FDateTime::UtcNow().ToUnixTimestamp();
		ServerRPC_SetTravelEndTime(now);
	}
	Super::OnRep_PlayerName();
}

void ACitRushPlayerState::ServerRPC_SetTravelEndTime_Implementation(int64 InEndTime)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushPlayerState] SetTravelEndTime called on client - ignoring"));
		return;
	}

	travelEndTime = InEndTime;
	if (OnTravelEnd.IsBound())
	{
		OnTravelEnd.Broadcast(this);
	}
	OnRep_TravelEndTime();
}

void ACitRushPlayerState::OnRep_TravelEndTime()
{
	UE_LOG(LogTemp, Log
		, TEXT("[CitRushPlayerState] Player %s travel took %lli seconds")
		, *GetPlayerName(), travelEndTime - travelStartTime
	);
	
}

void ACitRushPlayerState::BeginPlay()
{
	Super::BeginPlay();
	CITRUSH_TIME("[Loading]");
	
	UE_LOG(LogTemp, Warning, TEXT("[PlayerState] BeginPlay called for %s (ptr: %p), Role: %d"),*GetPlayerName(), this, (int32)playerRole);
#if IF_WITH_EDITOR
	if (playerRole == 0)
	{
		if (IsNetMode(NM_ListenServer))
		{
			playerRole = EPlayerRole::Commander;
		}
		else if (IsNetMode(NM_Client))
		{
			playerRole = EPlayerRole::Racer;
		}
	}
#endif
	/*if (USteamSubsystem* sss = GetGameInstance()->GetSubsystem<USteamSubsystem>())
	{
		sss->GetSteamNickname();
		sss->SetNetIdentity(GetPlayerController()->GetLocalPlayer()->GetUniqueNetIdForPlatformUser().GetUniqueNetId());
	}*/
}

void ACitRushPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (ACitRushPlayerState* newCPS = Cast<ACitRushPlayerState>(PlayerState))
	{
		newCPS->playerRole = this->playerRole;
		newCPS->targetIndex = this->targetIndex;
		newCPS->loadingState = this->loadingState;
		UE_LOG(LogTemp, Warning, TEXT("[CitRushPlayerState] Copy Properties to new PlayerState"));
	}
}

void ACitRushPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACitRushPlayerState, playerRole);
	DOREPLIFETIME(ACitRushPlayerState, targetIndex);
	DOREPLIFETIME(ACitRushPlayerState, bIsReady);
	DOREPLIFETIME(ACitRushPlayerState, bIsDied);
	DOREPLIFETIME(ACitRushPlayerState, selectedAbilities);
	DOREPLIFETIME(ACitRushPlayerState, loadingState);
	DOREPLIFETIME(ACitRushPlayerState, travelStartTime);
	DOREPLIFETIME(ACitRushPlayerState, travelEndTime);
	DOREPLIFETIME(ACitRushPlayerState, loadingProgress);
}

void ACitRushPlayerState::ClientRPC_ExitToMainMenu_Implementation()
{
	GetGameInstance()->ReturnToMainMenu();
}


void ACitRushPlayerState::ServerRPC_SetDied_Implementation(bool bDied)
{
	if (bIsDied != bDied)
	{
		bIsDied = bDied;
		
		if (bIsDied && playerRole == EPlayerRole::Racer)
		{
			if (UWorld* world = GetWorld())
			{
				if (ACitRushGameState* gs = world->GetGameState<ACitRushGameState>())
				{
					gs->OnRacerDied(this);
				}
			}
		}
		OnRep_IsDied();
	}
}

void ACitRushPlayerState::ServerRPC_RoleChange_Implementation(EPlayerRole newRole)
{
	if (playerRole != newRole)
	{
		playerRole = newRole;
		ServerRPC_SetReady(false);
		OnRep_PlayerRole();
	}
}

void ACitRushPlayerState::ServerRPC_SetReady_Implementation(bool bReady)
{
	if (playerRole == EPlayerRole::None)
	{
		// 불가능한 이유 띄워주는 UI
		return;
	}
	
	if (bIsReady != bReady)
	{
		bIsReady = bReady;
		OnRep_IsReady();
	}
}

void ACitRushPlayerState::ServerRPC_SetRacerIndex_Implementation(int32 newIndex)
{
	UEnum* targetRacerEnumPtr = StaticEnum<ETargetRacer>();
	if (!HasAuthority() || !targetRacerEnumPtr) {return;}
	if (targetRacerEnumPtr->IsValidEnumValue(newIndex))
	{
		ETargetRacer newTargetIndex = static_cast<ETargetRacer>(newIndex);
		if (targetIndex != newTargetIndex)
		{
			targetIndex = newTargetIndex;
			OnRep_TargetIndex();
		}
		if (playerRole == EPlayerRole::Commander && newIndex > 0)
		{
			ServerRPC_RoleChange_Implementation(EPlayerRole::Racer);
		}
		ServerRPC_SetLoadingState_Implementation(ELoadingState::Registered);
	}
}

EPlayerRole ACitRushPlayerState::GetPlayerRole() const
{
	return playerRole;
}

bool ACitRushPlayerState::CanReady()
{
	return playerRole != EPlayerRole::None;
}

FPlayerInfo ACitRushPlayerState::GetPlayerInfo() const
{
	return {GetPlayerName(), playerRole, targetIndex, bIsReady};
}


void ACitRushPlayerState::ServerRPC_SelectAbilityByName_Implementation(FName abilityName, int32 slotIndex)
{
	if (!HasAuthority()) {return;}
	if (slotIndex < 0) {return;}

	if (selectedAbilities.Num() <= slotIndex)
	{
		// 추후 limit 걸기
		selectedAbilities.SetNum(slotIndex + 1);
	}
	selectedAbilities[slotIndex] = abilityName;
}


void ACitRushPlayerState::OnRep_PlayerRole()
{
	UE_LOG(LogTemp, Warning, TEXT("=========Role======= : %d"), static_cast<int32>(playerRole));
	OnPlayerRoleChangedDelegate.Broadcast(playerRole);
}

void ACitRushPlayerState::OnRep_TargetIndex()
{
	UE_LOG(LogTemp, Warning, TEXT("=========Index======= : %d"), static_cast<int32>(targetIndex));
	OnTargetIndexChanged.Broadcast(static_cast<int32>(targetIndex));
}

void ACitRushPlayerState::OnRep_IsReady()
{
	OnPlayerReadyChangedDelegate.Broadcast(bIsReady);
}

void ACitRushPlayerState::OnRep_IsDied()
{
	OnPlayerDied.Broadcast(this);
	
	// 레이서가 죽었을 때 전체 레이서들에게 알림 전송 (서버에서만)
	if (bIsDied && playerRole == EPlayerRole::Racer && HasAuthority())
	{
		UWorld* World = GetWorld();
		if (World)
		{
			if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
			{
				// 모든 레이서 PlayerState 가져오기
				TArray<ACitRushPlayerState*> RacerPlayerStates = GameState->GetPlayerStatesByRole(EPlayerRole::Racer);
				
				// 사망한 레이서의 이름 가져오기
				FString DeadRacerName = GetPlayerName();
				FString NotificationText = FString::Printf(TEXT("%s가 사망했습니다!"), *DeadRacerName);
				
				// 각 레이서에게 알림 전송 (Client RPC 사용)
				for (ACitRushPlayerState* RacerPS : RacerPlayerStates)
				{
					if (!RacerPS)
					{
						continue;
					}
					
					// PlayerController 찾기 및 메시지 전송
					if (APlayerController* PC = RacerPS->GetPlayerController())
					{
						// AUE_CITRUSHPlayerController로 캐스팅 시도
						if (AUE_CITRUSHPlayerController* VehiclePC = Cast<AUE_CITRUSHPlayerController>(PC))
						{
							VehiclePC->ClientShowStateMessage(NotificationText);
						}
						// CitRushPlayerController로 캐스팅 시도 (멀티플레이 지원)
						else if (ACitRushPlayerController* CitRushPC = Cast<ACitRushPlayerController>(PC))
						{
							CitRushPC->ClientShowStateMessage(NotificationText);
						}
					}
				}
				
				// 커맨더에게도 알림 전송 (Client RPC 사용)
				TArray<ACitRushPlayerState*> CommanderPlayerStates = GameState->GetPlayerStatesByRole(EPlayerRole::Commander);
				for (ACitRushPlayerState* CommanderPS : CommanderPlayerStates)
				{
					if (!CommanderPS)
					{
						continue;
					}
					
					// PlayerController 찾기 및 메시지 전송
					if (APlayerController* PC = CommanderPS->GetPlayerController())
					{
						// CitRushPlayerController로 캐스팅 시도
						if (ACitRushPlayerController* CitRushPC = Cast<ACitRushPlayerController>(PC))
						{
							CitRushPC->ClientShowStateMessage(NotificationText);
							UE_LOG(LogTemp, Log, TEXT("[CitRushPlayerState] 커맨더에게 레이서 사망 알림 전송: %s"), *NotificationText);
						}
					}
				}
				
				UE_LOG(LogTemp, Log, TEXT("[CitRushPlayerState] 레이서 사망 알림 전송: %s"), *NotificationText);
			}
		}
	}
}

void ACitRushPlayerState::ServerRPC_SetLoadingState_Implementation(ELoadingState NewState)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushPlayerState] SetLoadingState called on client - ignoring"));
		return;
	}

	if (NewState == ELoadingState::Registered && loadingState > NewState)
	{
		return;
	}

	if (loadingState != NewState)
	{
		loadingState = NewState;
		if (HasAuthority())
		{
			OnRep_LoadingState();
		}
	}
}

void ACitRushPlayerState::ServerRPC_SetTravelStartTime_Implementation(int64 InStartTime)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushPlayerState] SetTravelStartTime called on client - ignoring"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CitRushPlayerState] Player %s StartTravel : %lld"), *GetPlayerName(), InStartTime);
	travelStartTime = InStartTime;
}

void ACitRushPlayerState::UpdateLoadingProgress(float Progress)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CitRushPlayerState] UpdateLoadingProgress called on client - ignoring"));
		return;
	}

	loadingProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
}

void ACitRushPlayerState::OnRep_LoadingState()
{
	APlayerController* pc = GetPlayerController();
	ACitRushPlayerController* cPC = Cast<ACitRushPlayerController>(pc);
	if (!cPC || !cPC->IsLocalController()) {return;}

	// DEBUG
	{
		const UEnum* enumPtr = StaticEnum<ELoadingState>();
		FString enumNameString = enumPtr->GetNameStringByIndex(static_cast<int32>(loadingState));
		FString netRole = UEnum::GetValueAsString<ENetRole>(GetLocalRole());
	
		UE_LOG(LogTemp, Log, TEXT("[CitRushPlayerState] NET-%s { %s } LoadingState changed to: %s"), *netRole, *GetPlayerName(), *enumNameString);
	}
	int64 now = -1;
	switch (loadingState)
	{
		case ELoadingState::NotStarted:
			break;
		case ELoadingState::StartTravel:
			now  = FDateTime::UtcNow().ToUnixTimestamp();
			ServerRPC_SetTravelStartTime(now);
			GetGameInstance<UCitRushGameInstance>()->PlayLoadingMoviePlayer();
			if (ALobbyHUD* lHUD = cPC->GetHUD<ALobbyHUD>())
			{
				lHUD->OnTransition();
			}
			break;
		case ELoadingState::StartServerTravel:
		case ELoadingState::StartClientTravel:
			break;
		case ELoadingState::Traveling:
			break;
		case ELoadingState::PostLogin:
			break;
		case ELoadingState::Registered:
			break;
		case ELoadingState::Ready:
			break;
		case ELoadingState::TimerStarted:
			GetGameInstance<UCitRushGameInstance>()->OnStartInGame(GetWorld());
			break;
		case ELoadingState::MatchStarted:
			break;
	}
	
	OnLoadingStateChanged.Broadcast(loadingState);
}

UAbilitySystemComponent* ACitRushPlayerState::GetAbilitySystemComponent() const
{
	return abilitySystemComponent;
}

UAttributeSet* ACitRushPlayerState::GetAttributeSet(const EPlayerRole& role) const
{
	switch (role)
	{
	case EPlayerRole::Commander:
		return commanderAttributeSet;

	case EPlayerRole::Racer:
		return racerAttributeSet;
		
	case EPlayerRole::Spectator:
	case EPlayerRole::None:
		break;
	}
	return nullptr;
}
