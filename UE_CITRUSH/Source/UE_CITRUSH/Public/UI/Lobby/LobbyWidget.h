// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

class USessionCodeWidget;
enum class EPlayerRole : uint8;
class UPlayerInfoWidget;
class UVerticalBox;
class UButton;
class UTextBlock;
class ACitRushPlayerState;

/**
 * 로비 Widget. 플레이어 목록/Ready/Start 버튼 관리. 세션 코드 표시
 */
UCLASS()
class UE_CITRUSH_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 로비 진입. 기존 플레이어 목록 초기화 */
	void EnterLobby(TArray<ACitRushPlayerState*> players);

	/** 플레이어 추가. PlayerInfoWidget 생성 및 목록 추가 */
	void AddPlayer(ACitRushPlayerState* player);

	/** 플레이어 제거. PlayerInfoWidget 찾아서 삭제 */
	void RemovePlayer(ACitRushPlayerState* player) const;

	void UpdatePlayerList(TArray<ACitRushPlayerState*> players);

protected:
	/** Widget 생성 시 호출. 버튼 클릭 델리게이트 바인딩 */
	virtual void NativeConstruct() override;

private:
	/*/** 시작 버튼 활성화 조건 체크. Host만 활성화, CanStartGame 확인 #1#
	void CheckStartButtonEnabled() const;*/

	/** Ready 버튼 클릭 처리. PlayerState에 Ready 상태 설정 */
	UFUNCTION()
	void OnReadyButtonClicked();

	/** Start 버튼 클릭 처리 (Host 전용). 게임 시작 */
	UFUNCTION()
	void OnStartButtonClicked();

	/** Exit 버튼 클릭 처리. 로비 퇴장 */
	UFUNCTION()
	void OnExitButtonClicked();

	/** 플레이어 역할 변경 콜백. UI 갱신 */
	UFUNCTION()
	void OnPlayerRoleChanged(const EPlayerRole& selectedRole);

	/** Ready 상태 변경 콜백. UI 갱신 */
	UFUNCTION()
	void OnReadyStatusChanged(bool bIsReady);

protected:
	/** Ready 버튼 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> readyButton;

	/** Start 버튼 (Host 전용) */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> startButton;

	/** 플레이어 목록 컨테이너 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> playersContainer;

	/** 플레이어 정보 Widget 클래스 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UPlayerInfoWidget> playerInfoWidgetClass;

	int32 myInfoIndex = -1;

	/** 세션 코드 Widget */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USessionCodeWidget> sessionCodeWidget;

	/** 나가기 버튼 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> exitButton;
};
