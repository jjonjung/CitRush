// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

/**
 * 로비용 GameMode. 세션 코드 관리 및 게임 시작 처리
 */
UCLASS()
class UE_CITRUSH_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
	friend class FGameFlowUtility;

public:
	/** 생성자 */
	ALobbyGameMode();

	/** 플레이어 로그인 시 호출 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 플레이어 로그아웃 시 호출 */
	virtual void Logout(AController* Exiting) override;

protected:
	/** 게임 시작 시 호출 */
	virtual void BeginPlay() override;

	virtual APlayerController* ProcessClientTravel(FString& URL, bool bSeamless, bool bAbsolute) override;
	virtual void ProcessServerTravel(const FString& URL, bool bAbsolute = false) override;

	/** 게임 종료 시 호출 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma region GameDoorway
public:
	/** 게임 시작. StartGameFunc 콜백 호출 */
	bool StartGame();

	/** 세션 코드 반환 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE FString GetSessionCode() const {return sessionCode;}

private:
	/** 세션 코드 (참가용) */
	FString sessionCode;

	/** 게임 시작 콜백 (SteamSubsystem에서 설정) */
	TFunction<bool(FName&, const int32&)> StartGameFunc;

#pragma endregion
};
