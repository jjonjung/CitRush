// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/SteamSubsystem.h"

#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemSteam.h"
#include "OnlineSessionClient.h"
#include "OnlineSubsystemUtils.h"

#include "Data/MapDataAsset.h"
#include "GameFramework/GameModeBase.h"

#include "Utility/GameFlowUtility.h"
#include "Utility/Base64Converter.h"
#include "Utility/MapAssetLoader.h"

#include "SocketSubsystem.h"
#include "GameFlow/CitRushGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CitRushPlayerState.h"

DEFINE_LOG_CATEGORY_CLASS(USteamSubsystem, SteamSubsystemLog)

USteamSubsystem::USteamSubsystem()
{
	const UMapAssetLoader* mapAssetLoader = UMapAssetLoader::Get();
	mapAssets = mapAssetLoader->PDA_Map.LoadSynchronous();
}

void USteamSubsystem::InitializeProperties()
{
	hostNamePair.Value = GetSteamNickname();
	
	displayNamePair.Value = "";
	participantsCountPair.Value = 0;
	gameMapNamePair.Value = NAME_None;
	contentModePair.Value = EContentMode::None;
	bCancelFindSession = false;
}

void USteamSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	IOnlineSubsystem* onlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!onlineSubsystem) {return;}

	SetNetIdentity(onlineSubsystem->GetIdentityInterface()->GetUniquePlayerId(0));
	onlineSubsystemName = onlineSubsystem->GetSubsystemName();
	//if (onlineSubsystemName != STEAM_SUBSYSTEM) {return;}
	sessionInterface = onlineSubsystem->GetSessionInterface();
	InitializeProperties();
	
	FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &USteamSubsystem::OnWorldActorInitialized);
	
	if (!sessionInterface) {return;}
	
	sessionInterface->OnCreateSessionCompleteDelegates.AddUObject(
		this, &USteamSubsystem::OnCompleteCreateSession);
	sessionInterface->OnFindSessionsCompleteDelegates.AddUObject(
		this, &USteamSubsystem::OnCompleteFindSessions);
	sessionInterface->OnJoinSessionCompleteDelegates.AddUObject(
		this, &USteamSubsystem::OnCompleteJoinSession);
	sessionInterface->OnStartSessionCompleteDelegates.AddUObject(
		this, &USteamSubsystem::OnCompleteStartSession);
	sessionInterface->OnDestroySessionCompleteDelegates.AddUObject(
		this, &USteamSubsystem::OnCompleteDestroySession);
	sessionInterface->OnCancelFindSessionsCompleteDelegates.AddUObject(
		this, &USteamSubsystem::OnCompleteCancelFindSession);
	sessionInterface->OnSessionFailureDelegates.AddUObject(
		this, &USteamSubsystem::ErrorSession);
	
	FMapInfo mapInfo;
	if (mapAssets->GetMapInfoByKey("Lobby", mapInfo))
	{
		lobbyMapNamePair.Value = mapInfo.GetMapAsset().GetAssetName();
	}
	if (mapAssets->GetMapInfoByKey("MainMenu", mapInfo))
	{
		mainMenuMapNamePair.Value = mapInfo.GetMapAsset().GetAssetName();
	}
}

void USteamSubsystem::Deinitialize()
{
	IOnlineSubsystem* onlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!onlineSubsystem) {return;}
	DestroySession();
	
	if (sessionInterface)
	{
		sessionInterface->ClearOnCancelFindSessionsCompleteDelegates(this);
		sessionInterface->ClearOnFindSessionsCompleteDelegates(this);
		sessionInterface->ClearOnJoinSessionCompleteDelegates(this);
		sessionInterface->ClearOnStartSessionCompleteDelegates(this);
		sessionInterface->ClearOnDestroySessionCompleteDelegates(this);
		sessionInterface->ClearOnSessionFailureDelegates(this);
	}
	sessionInterface.Reset();

	FWorldDelegates::OnWorldInitializedActors.Clear();
	
	Super::Deinitialize();
}

void USteamSubsystem::SetNetIdentity(const FUniqueNetIdPtr& newNetId)
{
	if (netId.IsValid() || !newNetId.IsValid()) {return;}

	netId = newNetId;
}

FString USteamSubsystem::GetSteamNickname()
{
	IOnlineSubsystem* onlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!onlineSubsystem) return TEXT("Unknown");

	IOnlineIdentityPtr Identity = onlineSubsystem->GetIdentityInterface();
	if (!Identity.IsValid()) return TEXT("Unknown");
	
	FString Nickname = Identity->GetPlayerNickname(0);
	if (!onlineSubsystem->GetSubsystemName().IsEqual("STEAM"))
	{
		Nickname = TEXT("Local_") + Nickname.Right(8);
	}
	return Nickname.IsEmpty() ? TEXT("Unknown") : Nickname;
}

TSharedPtr<FOnlineSessionInfo> USteamSubsystem::GetCurrentSessionInfo() const
{
	FNamedOnlineSession* session = sessionInterface->GetNamedSession(currentSessionName);
	
	return session ? session->SessionInfo : nullptr;
}

bool USteamSubsystem::CreateMatchMaking()
{
	if (!sessionInterface.IsValid()) {return false;}
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Start Create Session"));
	
	FMapInfo mapInfo;
	if (!mapAssets || !mapAssets->GetMapInfoByKey(TEXT("Game_Rank"), mapInfo)) {return false;}
	contentModePair.Value = EContentMode::Bronze;  // TODO: 하드코딩 된 ELO Rank
	gameMapNamePair.Value = mapInfo.GetMapAsset().GetLongPackageFName();
	
	// TODO : 추가로 customize 가능한 부분 생각해보기
	sessionSettings = FOnlineSessionSettings();
	sessionSettings.bIsDedicated = false;
	sessionSettings.bIsLANMatch = false;
	sessionSettings.NumPublicConnections = 4;
	sessionSettings.bShouldAdvertise = true;
	sessionSettings.bAllowJoinInProgress = true;  // allow participate in progress server
	/*sessionSettings.bUseLobbiesIfAvailable = true;
	sessionSettings.bUseLobbiesVoiceChatIfAvailable = true;*/
	//sessionSettings.BuildUniqueId = 22022;
	
	sessionSettings.Set<bool>(FName("CITRUSH"), true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	sessionSettings.Set<FString>(hostNamePair.Key, UBase64Converter::StringToBase64(hostNamePair.Value), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	sessionSettings.Set<int32>(contentModePair.Key, contentModePair.Value, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	sessionSettings.Set<int32>(participantsCountPair.Key, ++participantsCountPair.Value, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Complete SessionSettings"));
	
	if (!netId.IsValid()) {return false;}
	const bool bCall = sessionInterface->CreateSession(*netId, FName(hostNamePair.Value), sessionSettings);
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Create Session Result : %s"), *FString(bCall ? "Success" : "Failure"));
	return bCall;
}

bool USteamSubsystem::CreateSession(const FCustomizeSessionSettings& requestedSessionInfo)
{
	if (!sessionInterface.IsValid()) {return false;}
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Start Create Session"));
	FNamedOnlineSession* session = sessionInterface->GetNamedSession(FName(hostNamePair.Value));
	
	displayNamePair.Value = requestedSessionInfo.displayName;
	gameMapNamePair.Value = requestedSessionInfo.mapInfo.GetMapAsset().GetLongPackageFName();
	//TODO: Rank용으로 바꿈. 되돌리기
	contentModePair.Value = EContentMode::Rank;

	FMapInfo mapInfo;
	if (!mapAssets) {return false;}
	
	// TODO : 추가로 customize 가능한 부분 생각해보기
	sessionSettings = FOnlineSessionSettings();
	sessionSettings.bIsLANMatch = onlineSubsystemName != STEAM_SUBSYSTEM;
	sessionSettings.NumPublicConnections = requestedSessionInfo.maxPlayerNum;
	sessionSettings.bShouldAdvertise = true;
	sessionSettings.bAllowJoinInProgress = true;  // allow participate in progress server
	sessionSettings.bUsesPresence = true;  // for finding friends
	sessionSettings.bAllowJoinViaPresence = true;  // allow friends able to  participate directly
	sessionSettings.bUseLobbiesIfAvailable = true;
	sessionSettings.bUseLobbiesVoiceChatIfAvailable = true;
	sessionSettings.bIsDedicated = false;

	// 개발 중에는 BuildUniqueId 체크 비활성화 (0으로 설정하면 체크하지 않음)
	//sessionSettings.BuildUniqueId = 22022;
	
	//sessionSettings.BuildUniqueId = GetBuildUniqueId();

	sessionSettings.Set<bool>(FName("CITRUSH"), true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	sessionSettings.Set<FString>(hostNamePair.Key, UBase64Converter::StringToBase64(hostNamePair.Value), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	sessionSettings.Set<FString>(displayNamePair.Key, UBase64Converter::StringToBase64(displayNamePair.Value), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	sessionSettings.Set<int32>(contentModePair.Key, contentModePair.Value, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	sessionSettings.Set<int32>(participantsCountPair.Key, ++participantsCountPair.Value, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);	

	if (!netId.IsValid()) {return false;}
	const bool bCall = sessionInterface->CreateSession(*netId, FName(hostNamePair.Value), sessionSettings);
	return bCall;
}

void USteamSubsystem::OnCompleteCreateSession(FName sessionName, bool bWasSuccess)
{
	if (bWasSuccess)
	{
		if (lobbyMapNamePair.Value.IsEmpty()) {return;}
		FNamedOnlineSession* session = sessionInterface->GetNamedSession(sessionName);
		if (!session) return;
		currentSessionName = sessionName;
		UE_LOG(SteamSubsystemLog, Warning, TEXT("Complete Create Session : %s"), *sessionName.ToString());
		if (session->SessionSettings.bIsLANMatch)
		{
			bool bCanBindAll;
			TSharedPtr<FInternetAddr> LocalAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
			sessionSettings.Set(SETTING_GAME_SESSION_URI, LocalAddr->ToString(false));
		}
		else if (
			session->SessionSettings.Get(contentModePair.Key, contentModePair.Value)
			&& contentModePair.Value < EContentMode::Rank
		)
		{
			sessionSettings.Set(SETTING_GAME_SESSION_URI, session->SessionInfo->GetSessionId().ToString());
		}
		sessionInterface->UpdateSession(sessionName, sessionSettings);
		/*TMap<FName, FMapInfo> maps = mapAssets->GetMapDictionary();

		/*
		if (FMapInfo* mapInfo = maps.Find(FName(gameMapNamePair.Value)))
		{
			gameWorld = mapInfo->GetMapAsset().LoadSynchronous();
		}
		#1#*/
		UE_LOG(SteamSubsystemLog, Warning, TEXT("Before Change Level"));
		FString url = lobbyMapNamePair.Value + TEXT("?listen");
		UGameplayStatics::OpenLevel(GetWorld(), FName(url), false, "?listen");
	}
	
	
}

bool USteamSubsystem::SearchMatchMaking()
{
	if (!sessionInterface.IsValid()) {return false;}
	if (onlineSubsystemName.IsNone() || !netId.IsValid()) {return false;}

	if (sessionSearch == nullptr && !bCancelFindSession)
	{
		sessionSearch = MakeShared<FOnlineSessionSearch>();
		sessionSearch->bIsLanQuery = false;
		sessionSearch->MaxSearchResults = 10;
		sessionSearch->QuerySettings.Set<bool>(FName("CITRUSH"), true, EOnlineComparisonOp::Equals);
		sessionSearch->QuerySettings.Set<bool>(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
		// TODO: 하드코딩 된 ELO Rank
		sessionSearch->QuerySettings.Set<int32>(contentModePair.Key, static_cast<int32>(EContentMode::Rank), EOnlineComparisonOp::Equals);
		sessionSearch->QuerySettings.Set<bool>(SEARCH_DEDICATED_ONLY, false, EOnlineComparisonOp::Equals);
		//sessionSearch->QuerySettings.Set<bool>(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	}
	
	if (!sessionSearch.IsValid()) {return false;}

	sessionInterface->FindSessions(*netId, sessionSearch.ToSharedRef());
	
	return true;
}

/*void USteamSubsystem::InviteFriend()
{
	IOnlineExternalUIPtr ExternalUI = Online::GetSubsystem(GetWorld())->GetExternalUIInterface();
}*/

bool USteamSubsystem::FindSessions()
{
	if (!sessionInterface.IsValid()) {return false;}
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Start Find Sessions"));

	sessionSearch = MakeShared<FOnlineSessionSearch>();
	if (!sessionSearch || onlineSubsystemName.IsNone())
	{
		UE_LOG(SteamSubsystemLog, Error, TEXT("FindSessions Failed: Invalid sessionSearch or subsystem"));
		return false;
	}
	sessionSearch->bIsLanQuery = onlineSubsystemName.IsEqual(FName(TEXT("NULL")));
	sessionSearch->QuerySettings.Set<bool>(FName("CITRUSH"), true, EOnlineComparisonOp::Equals);
	sessionSearch->QuerySettings.Set<bool>(SEARCH_DEDICATED_ONLY, false, EOnlineComparisonOp::Equals);
	sessionSearch->QuerySettings.Set<bool>(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	//sessionSearch->QuerySettings.Set<bool>(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	sessionSearch->QuerySettings.Set(contentModePair.Key, static_cast<int32>(EContentMode::Rank), EOnlineComparisonOp::Equals);

	sessionSearch->MaxSearchResults = 12;

	if (!netId.IsValid())
	{
		UE_LOG(SteamSubsystemLog, Error, TEXT("FindSessions Failed: Invalid NetId"));
		return false;
	}
	sessionInterface->FindSessions(*netId, sessionSearch.ToSharedRef());
	
	//UE_LOG(SteamSubsystemLog, Warning, TEXT("FindSessions Result: %s"), bCall ? TEXT("SUCCESS") : TEXT("FAILED"));
	return true;
}

bool USteamSubsystem::FindSession(const FString& sessionCode)
{
	if (!sessionInterface.IsValid()) {return false;}

	sessionSearch = MakeShared<FOnlineSessionSearch>();
	if (!sessionSearch.IsValid() || onlineSubsystemName.IsNone()) {return false;}
	sessionSearch->bIsLanQuery = onlineSubsystemName != STEAM_SUBSYSTEM;;
	sessionSearch->QuerySettings.Set<FString>(FName("CITRUSH"), TEXT("CITRUSH"), EOnlineComparisonOp::Equals);
	sessionSearch->QuerySettings.Set(SETTING_GAME_SESSION_URI, sessionCode, EOnlineComparisonOp::Equals);
	//sessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	sessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	sessionSearch->QuerySettings.Set(contentModePair.Key, static_cast<int32>(EContentMode::Rank), EOnlineComparisonOp::LessThan);
	sessionSearch->MaxSearchResults = 1;

	if (!netId.IsValid()) {return false;}
	sessionInterface->FindSessions(*netId, sessionSearch.ToSharedRef());
	bCancelFindSession = false;
	
	return true;
}

void USteamSubsystem::OnCompleteFindSessions(bool bWasSuccess)
{
	UE_LOG(SteamSubsystemLog, Warning, TEXT("OnCompleteFindSessions Called! bWasSuccess: %s"), bWasSuccess ? TEXT("TRUE") : TEXT("FALSE"));

	if (bCancelFindSession) {return;}
	
	if (bWasSuccess)
	{
		if (sessionSearch->bIsLanQuery)
		{
			if (FString sessionCodeRef;
				sessionSearch->QuerySettings.Get(SETTING_GAME_SESSION_URI, sessionCodeRef)
			)
			{
				UGameplayStatics::OpenLevel(this, *sessionCodeRef, true);
			}
			return;
		}
		UE_LOG(SteamSubsystemLog, Warning, TEXT("Found Sessions Count: %d"), sessionSearch->SearchResults.Num());
		if (sessionSearch->SearchResults.Num() <= 0)
		{
			return;
		}
		/*
		for (const FOnlineSessionSearchResult& result : sessionSearch->SearchResults)
		{
			for (TPair pair : result.Session.SessionSettings.Settings)
			{
				UE_LOG(SteamSubsystemLog, Warning, TEXT(": %s"), *pair.Key.ToString());
			}
		}
		*/
		
		for (const FOnlineSessionSearchResult& session : sessionSearch->SearchResults)
		{
			int32 participantsCount = 0;
			if (session.Session.SessionSettings.Get(participantsCountPair.Key, participantsCount);
				participantsCount < 4)
			{
				JoinSession(session);
				return;
			}
		}
		if (sessionSearch.IsValid())
		{
			sessionInterface->FindSessions(*netId, sessionSearch.ToSharedRef());
			return;
		}
		
		if (FString sessionCodeRef;
			sessionSearch->QuerySettings.Get(SETTING_GAME_SESSION_URI, sessionCodeRef)
		)
		{
			UE_LOG(SteamSubsystemLog, Display, TEXT("Find By Code"));
			FString foundSessionCode;
			for (const FOnlineSessionSearchResult& result : sessionSearch->SearchResults)
			{
				result.Session.SessionSettings.Get(SETTING_GAME_SESSION_URI, foundSessionCode);
				if (sessionCodeRef == foundSessionCode)
				{
					OnSuccessSearchSessionByCode.ExecuteIfBound(result);
					JoinSession(result);
				}
			}
			return;
		}
		
		OnSuccessSearchSessions.ExecuteIfBound(sessionSearch);
		// TODO : 일단은 강제 입장
		JoinSession(sessionSearch->SearchResults[0]);
		return;
	}
	UE_LOG(SteamSubsystemLog, Display, TEXT("Find Session Failed"));
	
}

bool USteamSubsystem::CancelFindSession()
{
	if (!sessionInterface.IsValid()) {return false;}
	if (!sessionSearch.IsValid()) {return false;}

	sessionInterface->CancelFindSessions();
	bCancelFindSession = true;
	return true;
}

void USteamSubsystem::OnCompleteCancelFindSession(bool bWasSuccess)
{
	if (bWasSuccess)
	{
		sessionSearch.Reset();
		sessionSearch = nullptr;
		bCancelFindSession = false;
		UE_LOG(SteamSubsystemLog, Display, TEXT("Cancel Session Searching"));
	}
}

bool USteamSubsystem::JoinSession(const FOnlineSessionSearchResult& session)
{
	if (!(sessionSearch.IsValid() && session.IsSessionInfoValid()))
	{
		return false;
	}

	if (!netId.IsValid()) {return false;}
	return sessionInterface->JoinSession(*netId, FName(hostNamePair.Value), session);
}

bool USteamSubsystem::JoinSession(const int32& sessionIndex)
{
	TArray<FOnlineSessionSearchResult> results = sessionSearch->SearchResults;
	if (!sessionSearch.IsValid() || !results.IsValidIndex(sessionIndex))
	{
		return false;
	}

	if (!netId.IsValid()) {return false;}
	return sessionInterface->JoinSession(*netId, FName(hostNamePair.Value), results[sessionIndex]);
}

void USteamSubsystem::OnCompleteJoinSession(FName sessionName, EOnJoinSessionCompleteResult::Type result)
{
	switch (result)
	{
		case EOnJoinSessionCompleteResult::Success:
		{
			FString url;
			sessionInterface->GetResolvedConnectString(sessionName, url);
			UE_LOG(SteamSubsystemLog, Warning, TEXT("URL : %s"), *url);
			APlayerController* pc = GetWorld()->GetFirstPlayerController();
			FNamedOnlineSession* session = sessionInterface->GetNamedSession(sessionName);
			session->SessionSettings.Get(participantsCountPair.Key, participantsCountPair.Value);
			session->SessionSettings.Set(participantsCountPair.Key, ++participantsCountPair.Value);
			sessionInterface->UpdateSession(sessionName, session->SessionSettings);
			UE_LOG(SteamSubsystemLog, Warning, TEXT("Participants Count : %d"), participantsCountPair.Value);
			pc->ClientTravel(url, TRAVEL_Partial);
			currentSessionName = sessionName;
			return;
		}
		case EOnJoinSessionCompleteResult::SessionIsFull:
		{
			return;
		}
		case EOnJoinSessionCompleteResult::AlreadyInSession:
		{
			return;
		}

		default:
			break;
	}

	displayNamePair.Value = "";
	participantsCountPair.Value = 0;
}

bool USteamSubsystem::StartSession(FName& outMapName, const int32& participatedPlayersNum)
{
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Init Start Session"));
	if (hostNamePair.Value.IsEmpty() || gameMapNamePair.Value.IsNone()) {return false;}
	if (!sessionInterface.IsValid()) {return false;}
	if (currentSessionName.IsNone()) {return false;}

	UE_LOG(SteamSubsystemLog, Warning, TEXT("Start Session Called"));
	outMapName = gameMapNamePair.Value;
	participantsCountPair.Value = participatedPlayersNum;
	bool bStart = sessionInterface->StartSession(currentSessionName);
	return bStart;
}

void USteamSubsystem::OnCompleteStartSession(FName sessionName, bool bWasSuccess)
{
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Complete Start Session Called : %s_%s"), *sessionName.ToString(), *FString(bWasSuccess ? "Success" : "Failure"));
	currentSessionName = NAME_None;
	//if (!gameWorld) {return;}
	if (bWasSuccess)
	{
		currentSessionName = sessionName;
		if (FNamedOnlineSession* session = sessionInterface->GetNamedSession(sessionName))
		{
			session->SessionSettings.bAllowJoinInProgress = false;
			session->SessionSettings.Set(participantsCountPair.Key, participantsCountPair.Value);
			sessionInterface->UpdateSession(sessionName, session->SessionSettings, true);
		}
		ServerTravel();
		/*GetWorld()->GetTimerManager().SetTimer(serverTravelTimer
			,this, &USteamSubsystem::ServerTravel
			, 1.f, true, 0.5f
		);
		UE_LOG(SteamSubsystemLog, Warning, TEXT("Set Timer : %s"), *sessionName.ToString());
		*/
	}
}

void USteamSubsystem::ServerTravel()
{
	FString Url = gameMapNamePair.Value.ToString() + FString(TEXT("?listen"));
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Start Seamless Travel : %s"), *Url);
	GetWorld()->ServerTravel(Url);
	/*if (serverTravelTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(serverTravelTimer);
	}*/
}

bool USteamSubsystem::DestroySession()
{
	if (!sessionInterface.IsValid()) { return false; }
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Destroyed Session Name : %s"), *currentSessionName.ToString());
	InitializeProperties();
	return sessionInterface->DestroySession(currentSessionName);
}

void USteamSubsystem::OnCompleteDestroySession(FName sessionName, bool bWasSuccess)
{
	UGameplayStatics::OpenLevel(GetWorld(), FName(mainMenuMapNamePair.Value));
	
}

void USteamSubsystem::ErrorSession(const FUniqueNetId& id, ESessionFailure::Type type)
{
	switch (type)
	{
	case ESessionFailure::ServiceConnectionLost:
		DestroySession();
		break;
	default:
		break;
	}
}

void USteamSubsystem::OnWorldActorInitialized(const FActorsInitializedParams& inInitializationValues)
{
	UWorld* newWorld = GetWorld();
	if (!IsValid(newWorld)) {return;}
	UE_LOG(SteamSubsystemLog, Warning, TEXT(" World %s has been created and initialized."), *newWorld->GetName());

	/*if (netId == nullptr)
	{
		if (APlayerController* pc = GetWorld()->GetFirstPlayerController();
		 ULocalPlayer* player = pc->GetLocalPlayer())
		{
			SetNetIdentity(player->GetUniqueNetIdForPlatformUser().GetUniqueNetId());
		}
	}*/	
	UE_LOG(SteamSubsystemLog, Warning, TEXT("Before GM Initialize"));
	if (AGameModeBase* newGM = newWorld->GetAuthGameMode())
	{
		UE_LOG(SteamSubsystemLog, Warning, TEXT("GameMode %s in World %s has been created and initialized."), *newGM->GetName(), *newWorld->GetName());
		// Lambda Capture for Assure `this` Instance.
		if (FGameFlowUtility::Get()->SetSafeMainMenuGM(
				newGM,
				[this](const FCustomizeSessionSettings& Settings)->bool {return this->CreateSession(Settings);},
				[this](const FOnlineSessionSearchResult& Result)->bool {return this->JoinSession(Result);},
				[this]()->bool {return this->FindSessions();},
				[this](const FString& code)->bool {return this->FindSession(code);},
				[this]()->bool {return this->CreateMatchMaking();},
				[this]()->bool {return this->SearchMatchMaking();},
				[this]()->bool {return this->CancelFindSession();},
				OnSuccessSearchSessions,
				OnSuccessSearchSessionByCode
			)
		) {return;}

		FString sessionCode;
		if (!sessionSettings.Get(SETTING_GAME_SESSION_URI, sessionCode))
		{
			sessionCode = "";
		}
		
		if (FGameFlowUtility::Get()->SetSafeLobbyGM(
				newGM,
				sessionCode,
				[this](FName&, const int32& participatedPlayersNum)->bool {return this->StartSession(gameMapNamePair.Value, participatedPlayersNum);})
		) {return;}
		
		if (FGameFlowUtility::Get()->SetSafeInGameGM(
			newGM,
			participantsCountPair.Value	
			)
		) {return;}
		
	}
}
