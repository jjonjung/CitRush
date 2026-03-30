// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/LobbyWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFlow/LobbyGameMode.h"
#include "UI/Lobby/PlayerInfoWidget.h"

#include "GameFlow/LobbyGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"
#include "UI/HUD/LobbyHUD.h"

void ULobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	 // TODO : Test 환경에서는 끄기
	startButton->SetIsEnabled(false);
	readyButton->SetIsEnabled(true);
	
	readyButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnReadyButtonClicked);
	startButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnStartButtonClicked);
	exitButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnExitButtonClicked);
	
	if (ACitRushPlayerState* cPS = GetOwningPlayerState<ACitRushPlayerState>())
	{
		cPS->OnPlayerReadyChangedDelegate.AddUObject(this, &ULobbyWidget::OnReadyStatusChanged);
		cPS->OnPlayerRoleChangedDelegate.AddUObject(this, &ULobbyWidget::OnPlayerRoleChanged);
		//startButton->SetIsEnabled(cPS->GetPlayerRole() == EPlayerRole::Commander);
	}

	for (int32 i = 0; i < 4; ++i)
	{
		TObjectPtr<UPlayerInfoWidget> playerInfoWidget = CreateWidget<UPlayerInfoWidget>(playersContainer, playerInfoWidgetClass);
		playersContainer->AddChildToVerticalBox(playerInfoWidget);
		//playerInfoWidget->SetIsEnabled(false);
	}
	if (ALobbyGameState* lGS = GetWorld()->GetGameState<ALobbyGameState>())
	{
		TArray<ACitRushPlayerState*> cPSs = lGS->GetCitRushPlayers();
		if (cPSs.Num() > 0)
		{
			UpdatePlayerList(cPSs);
		}
	}
}

void ULobbyWidget::EnterLobby(TArray<ACitRushPlayerState*> players)
{
	for (ACitRushPlayerState* player : players)
	{
		AddPlayer(player);
	}
}

void ULobbyWidget::AddPlayer(ACitRushPlayerState* player)
{
	if (!IsValid(player)) {return;}
	int32 index = static_cast<int32>(player->GetPlayerInfo().targetIndex);
	UE_LOG(LogTemp, Display, TEXT("[Lobby Widget] Added Player index : %d"), index);
	TArray<UWidget*> children = playersContainer->GetAllChildren();
	if (children.IsValidIndex(index))
	{
		if (UPlayerInfoWidget* infoWidget = Cast<UPlayerInfoWidget>(children[index]))
		{
			infoWidget->InitializePlayerInfo(player, index);
			infoWidget->SetAuthority(false);
			if (player->GetPlayerController() == GetWorld()->GetFirstPlayerController())
			{
				myInfoIndex = index;
				infoWidget->SetAuthority(true);
			}
		}
	}

}

void ULobbyWidget::RemovePlayer(ACitRushPlayerState* player) const
{
	for (UWidget* childWidget : playersContainer->GetAllChildren())
	{
		if (UPlayerInfoWidget* infoWidget = Cast<UPlayerInfoWidget>(childWidget))
		{
			if (IsValid(player) && infoWidget->ComparePlayer(player->GetUniqueId()->GetTypeHash()))
			{
				player->OnPlayerRoleChangedDelegate.RemoveAll(this);
				
				infoWidget->InitializePlayerInfo(nullptr, -1);
				return;
			}
		}
	}
}

void ULobbyWidget::UpdatePlayerList(TArray<ACitRushPlayerState*> players)
{
	for (int32 i = 0; i < players.Num(); ++i)
	{
		ACitRushPlayerState* player = players[i];
		TArray<UWidget*> children = playersContainer->GetAllChildren();
		if (children.IsValidIndex(i))
		{
			if (UPlayerInfoWidget* infoWidget = Cast<UPlayerInfoWidget>(children[i]))
			{
				if (!IsValid(player))
				{
					infoWidget->InitializePlayerInfo(nullptr, -1);
				}
				else
				{
					infoWidget->InitializePlayerInfo(player, i);
					infoWidget->SetAuthority(false);
					if (player->GetPlayerController() == GetWorld()->GetFirstPlayerController())
					{
						myInfoIndex = i;
						infoWidget->SetAuthority(true);
					}
				}
			}
		}
		else{} // 퇴출
	}
}


void ULobbyWidget::OnReadyButtonClicked()
{
	ACitRushPlayerState* cPS = GetOwningPlayerState<ACitRushPlayerState>();
	if (!IsValid(cPS)) {return;}
	
	cPS->ServerRPC_SetReady(!cPS->IsReady());
}

void ULobbyWidget::OnStartButtonClicked()
{
	ACitRushPlayerState* cPS = GetOwningPlayerState<ACitRushPlayerState>();
	if (!IsValid(cPS)) {return;}
	// TODO : Test 끝나면 아래의 주석 활성화하기 
	//if (cPS->GetPlayerRole() != EPlayerRole::Commander) {return;}

	ALobbyGameMode* gm = GetWorld()->GetAuthGameMode<ALobbyGameMode>();
	if (!IsValid(gm)) {return;}
	gm->StartGame();
}

void ULobbyWidget::OnExitButtonClicked()
{
	/*TArray<UWidget*> children = playersContainer->GetAllChildren();
	if (myInfoIndex >= 0 && children.IsValidIndex(myInfoIndex))
	{
		if (UPlayerInfoWidget* infoWidget = Cast<UPlayerInfoWidget>(children[myInfoIndex]))
		{
			infoWidget->InitializePlayerInfo(nullptr, -1);
		}
	}*/
	
	if (ALobbyGameState* lGS = GetWorld()->GetGameState<ALobbyGameState>())
	{
		lGS->NetMulticastRPC_QuitLobby(GetOwningPlayerState<ACitRushPlayerState>());
	}
}

void ULobbyWidget::OnPlayerRoleChanged(const EPlayerRole& selectedRole)
{
	ACitRushPlayerState* cPS = GetOwningPlayerState<ACitRushPlayerState>();
	if (!IsValid(cPS)) {return;}
	// 무조건 Ready 상태 해제
	cPS->ServerRPC_SetReady(false);
}

void ULobbyWidget::OnReadyStatusChanged(bool bIsReady)
{
	ALobbyGameState* lgs = GetWorld()->GetGameState<ALobbyGameState>();
	if (lgs)
	{
		startButton->SetIsEnabled(lgs->CanStartGame());
	}
}
