// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Data/CustomizeSessionSettings.h"
#include "SteamSubsystem.generated.h"


class UMapAssetLoader;
/**
 * Steam 온라인 세션 관리 Subsystem. 세션 생성/검색/참가/시작/종료 처리
 */
UCLASS()
class UE_CITRUSH_API USteamSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(SteamSubsystemLog, Warning, All);
	
public:
	/** 생성자 */
	USteamSubsystem();

protected:
	/** Subsystem 프로퍼티 초기화 */
	void InitializeProperties();

	/** Subsystem 초기화. SessionInterface 및 델리게이트 설정 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Subsystem 종료. 델리게이트 정리 */
	virtual void Deinitialize() override;

public:
	/** 네트워크 Identity 설정 */
	void SetNetIdentity(const FUniqueNetIdPtr& newNetId);

	/** Steam 닉네임 가져오기 */
	UFUNCTION(BlueprintCallable)
	FString GetSteamNickname();
	
	/** OnlineSession 인터페이스 가져오기 */
	IOnlineSessionPtr GetSessionInterface() const {return sessionInterface;}

	/** 현재 세션 정보 가져오기 */
	TSharedPtr<FOnlineSessionInfo> GetCurrentSessionInfo() const;
	
	/** 매치메이킹 세션 만들기 */
	bool CreateMatchMaking();
	/** 매치메이킹 세션 찾기 */
	bool SearchMatchMaking();

	/** 세션 생성 */
	bool CreateSession(const FCustomizeSessionSettings& requestedSessionInfo);

	//void InviteFriend();

	/** 모든 세션 검색 */
	bool FindSessions();

	/** 세션 코드로 특정 세션 검색 */
	bool FindSession(const FString& sessionCode);

	bool CancelFindSession();

	/** 세션 참가 (검색 결과 기반) */
	bool JoinSession(const FOnlineSessionSearchResult& session);

	/** 세션 참가 (인덱스 기반) */
	bool JoinSession(const int32& sessionIndex);

	/** 세션 시작 */
	bool StartSession(FName& outMapName, const int32& participatedPlayersNum);

	/** 세션 파괴 */
	bool DestroySession();

	void ErrorSession(const FUniqueNetId& id, ESessionFailure::Type type);

	/** 호스트 이름 세션 설정 키 가져오기 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE FName GetHostNameKey() const {return hostNamePair.Key;}

	/** 디스플레이 이름 세션 설정 키 가져오기 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE FName GetDisplayNameKey() const {return displayNamePair.Key;}

	/** 게임 맵 이름 세션 설정 키 가져오기 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE FName GetGameMapNameKey() const {return gameMapNamePair.Key;}

	/** 참가자 수 세션 설정 키 가져오기 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE FName GetParticipantsCounterNameKey() const {return participantsCountPair.Key;}

private:
	/** 세션 검색 성공 델리게이트 */
	DECLARE_DELEGATE_OneParam(FOnSuccessSearchSessions, const  TSharedPtr<FOnlineSessionSearch>&);
	FOnSuccessSearchSessions OnSuccessSearchSessions;
	/** 세션 코드 검색 성공 델리게이트 */
	DECLARE_DELEGATE_OneParam(FOnSuccessSearchSession, const  FOnlineSessionSearchResult&);
	FOnSuccessSearchSession OnSuccessSearchSessionByCode;
	
	void OnCompleteCreateSession(FName sessionName, bool bWasSuccess);
	void OnCompleteFindSessions(bool bWasSuccess);
	void OnCompleteCancelFindSession(bool bWasSuccess);
	void OnCompleteJoinSession(FName sessionName, EOnJoinSessionCompleteResult::Type result);
	void OnCompleteStartSession(FName sessionName, bool bWasSuccess);
	void ServerTravel();
	
	void OnCompleteDestroySession(FName sessionName, bool bWasSuccess);
	
	void OnWorldActorInitialized(const FActorsInitializedParams& inInitializationValues);

protected:
	FTimerHandle serverTravelTimer;
	
	/** 호스트 이름 세션 설정 Key-Value */
	TPair<FName, FString> hostNamePair = {"HOST", ""};

	/** 디스플레이 이름 세션 설정 Key-Value */
	TPair<FName, FString> displayNamePair = {"DISPLAY", ""};

	/** 참가자 수 세션 설정 Key-Value */
	TPair<FName, int32> participantsCountPair = {"COUNT", 0};

	/** 게임 맵 이름 세션 설정 Key-Value */
	TPair<FName, FName> gameMapNamePair = {"GAME_MAP", ""};

	/** 컨텐츠 모드 세션 설정 Key-Value */
	TPair<FName, int32> contentModePair = {"CONTENT_MODE", EContentMode::None};

	/** 로비 맵 이름 세션 설정 Key-Value */
	TPair<FName, FString> lobbyMapNamePair = {"LOBBY_MAP", ""};

	/** 메인 메뉴 맵 이름 세션 설정 Key-Value */
	TPair<FName, FString> mainMenuMapNamePair = {"MAIN_MENU_MAP", ""};
	
private:
	FName onlineSubsystemName = NAME_None;
	FName currentSessionName = NAME_None;
	FUniqueNetIdPtr netId;
	IOnlineSessionPtr sessionInterface;
	FOnlineSessionSettings sessionSettings;
	TSharedPtr<FOnlineSessionSearch> sessionSearch;

	UPROPERTY()
	const UMapDataAsset* mapAssets;

	bool bCancelFindSession = false;
};
