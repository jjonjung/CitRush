#include "Utility/GameFlowUtility.h"

#include "DebugHelper.h"
#include "GameFlow/MainMenuGameMode.h"
#include "GameFlow/MainMenuGameState.h"
#include "GameFlow/LobbyGameMode.h"
#include "GameFlow/LobbyGameState.h"
#include "GameFlow/CitRushGameMode.h"
#include "GameFlow/CitRushGameState.h"

DEFINE_LOG_CATEGORY_CLASS(FGameFlowUtility, GameFlowUtilityLog)


FGameFlowUtility::FGameFlowUtility()
{
}

bool FGameFlowUtility::SetSafeMainMenuGM(AGameModeBase* GM
	, const TFunction<bool(const FCustomizeSessionSettings&)>& CreateSession
	, const TFunction<bool(const FOnlineSessionSearchResult&)>& JoinSession
	, const TFunction<bool()>& FindSessions
	, const TFunction<bool(const FString&)>& FindSession
	, const TFunction<bool()>& CreateMatchMaking
	, const TFunction<bool()>& SearchMatchMaking
	, const TFunction<bool()>& CancelFindSession
	, const TDelegate<void(const TSharedPtr<FOnlineSessionSearch>&)>& OnCompleteFindSessions
	, const TDelegate<void(const FOnlineSessionSearchResult&)>& OnCompleteFindSessionByCode
) const
{
	if (AMainMenuGameMode* mGM = Cast<AMainMenuGameMode>(GM))
	{
		CITRUSH_LOG("Bind TFunctions In MainMenuGameMode.");
		mGM->CreateSession = CreateSession;
		mGM->JoinSession = JoinSession;
		mGM->FindSessions = FindSessions;
		mGM->FindSession = FindSession;

		mGM->OnCompleteFindSessions = OnCompleteFindSessions;
		mGM->OnCompleteFindSessionByCode = OnCompleteFindSessionByCode;
		
		mGM->CreateMatchMaking = CreateMatchMaking;
		mGM->SearchMatchMaking = SearchMatchMaking;

		mGM->CancelFindSession = CancelFindSession;
		return true;
	}
	return false;
}

bool FGameFlowUtility::SetSafeLobbyGM(AGameModeBase* GM, const FString& code, const TFunction<bool(FName&, const int32&)>& StartGameFunc) const
{
	if (ALobbyGameMode* lGM = Cast<ALobbyGameMode>(GM))
	{
		//https://unreal-garden.com/tutorials/tfunctionref/ => 아래의 Case에는 X
		lGM->StartGameFunc = StartGameFunc;
		lGM->sessionCode = code;
		return true;
	}
	return false;
}

bool FGameFlowUtility::SetSafeInGameGM(AGameModeBase* GM, const int32& participatedPlayersNum) const
{
	if (ACitRushGameMode* cGM = Cast<ACitRushGameMode>(GM))
	{
		cGM->playersNum = participatedPlayersNum;
		return true;
	}
	return false;
}