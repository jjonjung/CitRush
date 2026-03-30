// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/MainMenuGameMode.h"


#include "GameFlow/MainMenuGameState.h"
#include "Network/SteamSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "Player/CitRushPlayerState.h"

#include "Player/UIPawn.h"
#include "Player/Controller/CitRushPlayerController.h"
#include "Utility/DebugHelper.h"
#include "Utility/MapAssetLoader.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	DefaultPawnClass = AUIPawn::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	GameStateClass = AMainMenuGameState::StaticClass();
	PlayerStateClass = ACitRushPlayerState::StaticClass();

	const UMapAssetLoader* loader = UMapAssetLoader::Get();
	if (loader->IsValidLowLevel())
	{
		mapDataAsset = loader->PDA_Map.LoadSynchronous();
	}

}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AMainMenuGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
}

void AMainMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CreateSession.Reset();
	FindSessions.Reset();
	FindSession.Reset();
	JoinSession.Reset();
	CreateMatchMaking.Reset();
	SearchMatchMaking.Reset();

	if (OnCompleteFindSessions.IsBound())
	{
		OnCompleteFindSessions.Unbind();
		OnCompleteFindSessions = nullptr;
	}
	if (OnCompleteFindSessionByCode.IsBound())
	{
		OnCompleteFindSessionByCode.Unbind();
		OnCompleteFindSessionByCode = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}

bool AMainMenuGameMode::StartMatchMaking() const
{
	ACitRushPlayerState* cps = GetWorld()->GetFirstPlayerController()->GetPlayerState<ACitRushPlayerState>();
	if (!IsValid(cps) || !cps->HasAuthority()) {return false;}

	if (!IsValid(mapDataAsset)) {return false;}
	FMapInfo mapInfo;
	if (!mapDataAsset->GetMapInfoByKey("Rank", mapInfo)) {return false;}
	
	if (cps->GetPlayerRole() == EPlayerRole::Commander)
	{
		return CreateMatchMaking ? CreateMatchMaking() : false;
	}
	if (cps->GetPlayerRole() == EPlayerRole::Racer)
	{
		return SearchMatchMaking ? SearchMatchMaking() : false;
	}
	return false;
}

bool AMainMenuGameMode::CreateLobby(const FCustomizeSessionSettings& settings) const
{
	ACitRushPlayerState* cps = GetWorld()->GetFirstPlayerController()->GetPlayerState<ACitRushPlayerState>();
	if (!IsValid(cps) || !cps->HasAuthority()) {return false;}
	
	CITRUSH_LOG("SetRole To Commander");
	cps->ServerRPC_RoleChange(EPlayerRole::Commander);
	return CreateSession ? CreateSession(settings) : false;
}

bool AMainMenuGameMode::SearchLobbies() const
{
	ACitRushPlayerState* cps = GetWorld()->GetFirstPlayerController()->GetPlayerState<ACitRushPlayerState>();
	if (!IsValid(cps) || !cps->HasAuthority()) {return false;}
	
	return FindSessions ? FindSessions() : false;
}

bool AMainMenuGameMode::SearchLobbyByCode(const FString& sessionCode) const
{
	return FindSession ? FindSession(sessionCode) : false;
}

bool AMainMenuGameMode::JoinLobby(const FOnlineSessionSearchResult& session) const
{
	ACitRushPlayerState* cps = GetWorld()->GetFirstPlayerController()->GetPlayerState<ACitRushPlayerState>();
	if (!IsValid(cps) || !cps->HasAuthority()) {return false;}
	
	cps->ServerRPC_RoleChange(EPlayerRole::Racer);
	return JoinSession ? JoinSession(session) : false;
}

bool AMainMenuGameMode::CancelSearchSession() const
{
	return CancelFindSession ? CancelFindSession() : false;
}

TDelegate<void(const TSharedPtr<FOnlineSessionSearch>&)>& AMainMenuGameMode::GetEventOnCompleteFindSessions()
{
	if (OnCompleteFindSessions.IsBound())
	{
		OnCompleteFindSessions.Unbind();
	}
	return OnCompleteFindSessions;
}

TDelegate<void(const FOnlineSessionSearchResult&)>& AMainMenuGameMode::GetEventOnCompleteFindSessionByCode()
{
	if (OnCompleteFindSessionByCode.IsBound())
	{
		OnCompleteFindSessionByCode.Unbind();
	}
	return OnCompleteFindSessionByCode;
}
