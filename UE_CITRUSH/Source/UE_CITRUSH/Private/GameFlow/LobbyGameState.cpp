// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/LobbyGameState.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Player/CitRushPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "UI/HUD/LobbyHUD.h"

#include "Utility/ContainerHelper.h"


DEFINE_ACLASS_LOG(LobbyGameState);

ALobbyGameState::ALobbyGameState()
{
	requiredCommanderCount = 0;
	requiredRacerCount = 0;
	CitRushPSs.Reserve(8);
}

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyGameState, CitRushPSs);
}

void ALobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	ACitRushPlayerState* cPS = Cast<ACitRushPlayerState>(PlayerState);
	if (!IsValid(cPS)) {return;}
	
	CITRUSH_TIME("Lobby")
	if (HasAuthority())
	{
		cPS->OnTravelEnd.AddUObject(this, &ALobbyGameState::AddCustomPlayerArray);
		GetWorldTimerManager().SetTimerForNextTick([cPS]()
		{
			if (!IsValid(cPS)) {CITRUSH_LOG("Fail ApplyPlayerNickName"); return;}
			cPS->HasAuthority() ?
			   cPS->ClientRPC_ApplyPlayerNickName_Implementation()
			   : cPS->ClientRPC_ApplyPlayerNickName();
		});
	}
	CITRUSH_TIME("Lobby")

	if (APlayerController* pc = GetWorld()->GetFirstPlayerController();
		pc->IsLocalController())
	{
		IOnlineSubsystem* oss = Online::GetSubsystem(GetWorld());
		IOnlineVoicePtr voiceInterface = oss->GetVoiceInterface();
		if (voiceInterface.IsValid())
		{
			FUniqueNetIdRepl uniqueId = PlayerState->GetUniqueId();
			if (uniqueId.IsValid())
			{
				voiceInterface->UnmuteRemoteTalker(0, *uniqueId, true);
			}
		}
		else {CITRUSH_LOG("Voice Interface Is Not Valid");}
	}
	
#if IF_WITH_EDITOR
	cPS->RoleChange(PlayerArray.Num() == 1 ? EPlayerRole::Commander : EPlayerRole::Racer);
#endif
}

void ALobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	//OnRemovedPlayer.Broadcast(PlayerState);
	if (HasAuthority())
	{
		if (ACitRushPlayerState* cPS = Cast<ACitRushPlayerState>(PlayerState))
		{
			if (CitRushPSs.Contains(cPS))
			{
				CitRushPSs.RemoveSingle(cPS);
				OnRep_CitRushPlayerArray();
			}
		}
	}
	Super::RemovePlayerState(PlayerState);
}

void ALobbyGameState::StartGame()
{
	if (!HasAuthority()) {return;}
	for (ACitRushPlayerState* cPS : CitRushPSs)
	{
		cPS->ServerRPC_SetLoadingState_Implementation(ELoadingState::StartTravel);
		cPS->OnTravelEnd.Clear();
	}
}

void ALobbyGameState::NetMulticastRPC_QuitLobby_Implementation(ACitRushPlayerState* requestor)
{
	if (requestor->HasAuthority())
	{
		for (ACitRushPlayerState* cPS : CitRushPSs)
		{
			if (cPS != requestor)
			{
				cPS->OnTravelEnd.Clear();
				cPS->ClientRPC_ExitToMainMenu();
			}
		}
	}
	if (CitRushPSs.Contains(requestor))
	{
		if (APlayerController* pc = requestor->GetPlayerController())
		{
			pc->GetHUD<ALobbyHUD>()->OnRemovedPlayer(requestor);
		}
		CitRushPSs.RemoveSingle(requestor);
	}
	GetWorldTimerManager().SetTimerForNextTick(
		[requestor]()-> void
		{
			requestor->ClientRPC_ExitToMainMenu_Implementation();
		}
	);
}

TArray<ACitRushPlayerState*> ALobbyGameState::GetCitRushPlayers() const
{
	return CitRushPSs;
}

TArray<FPlayerInfo> ALobbyGameState::GetAllPlayerInfos() const
{
	TArray<FPlayerInfo> result;
	
	for (ACitRushPlayerState* cPS : CitRushPSs)
	{
		FPlayerInfo playerInfo = cPS->GetPlayerInfo();
		result.Add(playerInfo);
	}
	
	return result;
}

bool ALobbyGameState::CanStartGame()
{
	int32 commanderCount = 0;
	int32 racerCount = 0;
	int32 requiredTotalCount = 0; 
	
	for (ACitRushPlayerState* cPS : CitRushPSs)
	{
		//ACitRushPlayerState* cPS = Cast<ACitRushPlayerState>(PS);
		if (!IsValid(cPS)) { return false; }

		commanderCount += cPS->GetPlayerRole() == EPlayerRole::Commander;
		racerCount += cPS->GetPlayerRole() == EPlayerRole::Racer;
		requiredTotalCount += cPS->IsReady();
	}
	Log(TEXT("\n	Commander Count : %d\n	Racer Count : %d\n	RequiredCount : %d/%d"), commanderCount, racerCount, requiredTotalCount, CitRushPSs.Num());
	
	return racerCount >= requiredRacerCount
		&& commanderCount >= requiredCommanderCount
		&& requiredTotalCount >= CitRushPSs.Num()
	;
}

void ALobbyGameState::OnRep_CitRushPlayerArray()
{
	OnUpdatedPlayerArray.Broadcast(CitRushPSs);
	CITRUSH_TIME("Lobby")
}

void ALobbyGameState::AddCustomPlayerArray(ACitRushPlayerState* cPS)
{
	CITRUSH_TIME("Lobby")
	if (!HasAuthority()) {return;}
	if (!IsValid(cPS)) {return;}
	int32 index = ArrayHelper::SortedInsertUnique<ACitRushPlayerState*>(
		CitRushPSs, cPS,
		[](const ACitRushPlayerState* forward, const ACitRushPlayerState* backward)
		{
			check(forward && backward);
			return forward->GetEndTravelTime() <= backward->GetEndTravelTime();
		}
	);
	if (index < 0) {return;}
	cPS->ServerRPC_SetRacerIndex_Implementation(index);
	OnRep_CitRushPlayerArray();
}

