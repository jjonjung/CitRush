#pragma once

#include "ObjectLifeCycleSingleton.h"

class AGameModeBase;
class APlayerState;
class FOnlineSessionSearchResult;
class FOnlineSessionSearch;
struct FCustomizeSessionSettings;

/**
 * GameMode 전환 및 설정 유틸리티. MainMenu, Lobby, InGame GM 설정 관리
 */
class FGameFlowUtility : public TObjectLifeCycleSingleton<FGameFlowUtility>
{
	DECLARE_LOG_CATEGORY_CLASS(GameFlowUtilityLog, Warning, All);
	friend TObjectLifeCycleSingleton<FGameFlowUtility>;

	/** 생성자 (private) */
	FGameFlowUtility();

public:
	/** MainMenu GameMode 설정. CreateSession 및 JoinSession 콜백 등록 */
	bool SetSafeMainMenuGM(AGameModeBase* GM
		, const TFunction<bool(const FCustomizeSessionSettings&)>& CreateSession
		, const TFunction<bool(const FOnlineSessionSearchResult&)>& JoinSession
		, const TFunction<bool()>& FindSessions
		, const TFunction<bool(const FString&)>& FindSession
		, const TFunction<bool()>& CreateMatchMaking
		, const TFunction<bool()>& SearchMatchMaking
		, const TFunction<bool()>& CancelFindSession
		, const TDelegate<void(const TSharedPtr<FOnlineSessionSearch>&)>& OnCompleteFindSessions
		, const TDelegate<void(const FOnlineSessionSearchResult&)>& OnCompleteFindSessionByCode
	) const;

	/** Lobby GameMode 설정. 세션 코드 및 StartGame 콜백 등록 */
	bool SetSafeLobbyGM(AGameModeBase* GM
		, const FString& code
		, const TFunction<bool(FName&, const int32&)>& StartGameFunc
	) const;

	/** InGame GameMode 설정 */
	bool SetSafeInGameGM(AGameModeBase* GM
		, const int32& participatedPlayersNum
	) const;

protected:
	/** Getter 방지 여부 (항상 false 반환하여 Get() 허용) */
	virtual bool PreventGetter() const override {return false;}
};