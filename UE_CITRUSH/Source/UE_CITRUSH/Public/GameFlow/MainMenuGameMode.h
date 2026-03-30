// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

class UMapDataAsset;
class ACitRushPlayerState;
class FOnlineSessionSearch;
struct FCustomizeSessionSettings;

/**
 * 메인메뉴용 GameMode. 로비 생성 및 참가 기능 제공
 */
UCLASS()
class UE_CITRUSH_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
	friend class FGameFlowUtility;

public:
	/** 생성자 */
	AMainMenuGameMode();

protected:
	/** 게임 시작 시 호출 */
	virtual void BeginPlay() override;

	/** 플레이어 로그인 시 호출 */
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	/** 매치 메이킹 시작. MatchMake 콜백 호출 */
	bool StartMatchMaking() const;
	
	/** 로비 생성. CreateSession 콜백 호출 */
	bool CreateLobby(const FCustomizeSessionSettings& settings) const;

	/** 로비 검색. FindSessions 콜백 호출 */
	bool SearchLobbies() const;
	/** 로비 코드 검색. FindSession 콜백 호출 */
	bool SearchLobbyByCode(const FString& sessionCode) const;
	
	/** 로비 참가. JoinSession 콜백 호출 */
	bool JoinLobby(const FOnlineSessionSearchResult& session) const;

	/** 로비 참가. JoinSession 콜백 호출 */
	bool CancelSearchSession() const;

	TDelegate<void(const TSharedPtr<FOnlineSessionSearch>&)>& GetEventOnCompleteFindSessions();
	TDelegate<void(const FOnlineSessionSearchResult&)>& GetEventOnCompleteFindSessionByCode();


private:
	/** 세션 생성 콜백 (SteamSubsystem에서 설정) */
	TFunction<bool(const FCustomizeSessionSettings&)> CreateSession;
	/** 세션 검색 콜백 (SteamSubsystem에서 설정) */
	TFunction<bool()> FindSessions;
	/** 세션 코드 검색 콜백 (SteamSubsystem에서 설정) */
	TFunction<bool(const FString&)> FindSession;
	/** 세션 참가 콜백 (SteamSubsystem에서 설정) */
	TFunction<bool(const FOnlineSessionSearchResult&)> JoinSession;

	/** 세션 검색 결과 이벤트 구독 (SteamSubsystem에서 설정) */
	TDelegate<void(const TSharedPtr<FOnlineSessionSearch>&)> OnCompleteFindSessions;
	/** 세션 코드 검색 결과 이벤트 구독 (SteamSubsystem에서 설정) */
	TDelegate<void(const FOnlineSessionSearchResult&)> OnCompleteFindSessionByCode;
	
	/** 매치 메이킹 생성 콜백 (SteamSubsystem에서 설정) */
	TFunction<bool()> CreateMatchMaking;
	/** 매치 메이킹 검색 콜백 (SteamSubsystem에서 설정) */
	TFunction<bool()> SearchMatchMaking;

	/** 세션 검색 취소 콜백 (SteamSubsystem에서 설정) */
	TFunction<bool()> CancelFindSession;
	
	UPROPERTY()
	UMapDataAsset* mapDataAsset;
	
	
};
