// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenu/SearchingSessionsWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "GameFlow/MainMenuGameMode.h"
#include "Network/SteamSubsystem.h"
#include "UI/Lobby/SessionCodeWidget.h"
#include "UI/MainMenu/SessionItemWidget.h"

#include "Utility/DebugHelper.h"

void USearchingSessionsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	for (int32 i = 0; i < itemCountInEntry; ++i)
	{
		sessionItemDataArray.Add(NewObject<USessionItemData>(sessionsEntry));
	}
}

void USearchingSessionsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	refreshSessionButton->OnClicked.AddDynamic(this, &USearchingSessionsWidget::RefreshList);
	
	nextButton->OnClicked.AddDynamic(this, &USearchingSessionsWidget::OnClickNextButton);
	nextButton->SetIsEnabled(false);
	prevButton->OnClicked.AddDynamic(this, &USearchingSessionsWidget::OnClickPrevButton);
	prevButton->SetIsEnabled(false);

	sessionsEntry->SetSelectionMode(ESelectionMode::SingleToggle);
	sessionsEntry->OnItemClicked().AddUObject(this, &USearchingSessionsWidget::OnClickSessionItem);
	joinSessionButton->OnClicked.AddDynamic(this, &USearchingSessionsWidget::OnClickJoinButton);
	findSessionByCodeButton->OnClicked.AddDynamic(this, &USearchingSessionsWidget::OnClickFindSessionByCodeButton);
	sessionSearchByCodeInputText->OnTextCommitted.AddDynamic(this, &USearchingSessionsWidget::OnCommitInputCode);
	if (AMainMenuGameMode* mGM = GetWorld()->GetAuthGameMode<AMainMenuGameMode>())
	{
		CITRUSH_LOG("Success Bind Delegate");
		mGM->GetEventOnCompleteFindSessions().BindUObject(this, &USearchingSessionsWidget::OnSearchSessionCompleted);
		mGM->GetEventOnCompleteFindSessionByCode().BindUObject(this, &USearchingSessionsWidget::OnSearchSessionByCodeCompleted);
	}
	sessionSearchByCodeInputText->SetVisibility(ESlateVisibility::Collapsed);
}

void USearchingSessionsWidget::OnSearchSessionCompleted(const TSharedPtr<FOnlineSessionSearch>& search)
{
	if (!IsValid(sessionsEntry)) { return; }
	sessionSearchResults.Empty();
	sessionSearchResults = search->SearchResults;

	CITRUSH_LOG("Widget : SearchSession Completed");

	startPage = 0;
	endPage = sessionSearchResults.Num() / itemCountInEntry;
	
	SetPage(startPage);
}

void USearchingSessionsWidget::OnSearchSessionByCodeCompleted(const FOnlineSessionSearchResult& SearchResult)
{
	// TODO : 세션 정보 및 참가 UI
}

void USearchingSessionsWidget::SetPage(const int32& newPage)
{
	currentPage = newPage;
	pageCounter->SetText(FText::AsNumber(newPage + 1));
	
	prevButton->SetIsEnabled(currentPage > startPage);
	nextButton->SetIsEnabled(currentPage < endPage);
	
	PopulateSessionsList();
}

void USearchingSessionsWidget::ResetSessionsList()
{
	sessionSearchResults.Empty();
	for (int32 i = 0; i < itemCountInEntry; ++i)
	{
		sessionItemDataArray[i]->Initialize();
	}
	if (refreshListTimer.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(refreshListTimer);
	}
}

void USearchingSessionsWidget::RefreshList()
{
	if (AMainMenuGameMode* mGM = GetWorld()->GetAuthGameMode<AMainMenuGameMode>())
	{
		if (!mGM->SearchLobbies())
		{
			// TODO : 세션 검색 불가 UI
			CITRUSH_LOG("Cant Find Session");
		}
	}
}

void USearchingSessionsWidget::PopulateSessionsList()
{
	if (!IsValid(sessionsEntry)) { return; }
	for (int i = 0; i < itemCountInEntry; ++i)
	{
		sessionItemDataArray[i]->Initialize();
	}

	int32 index = currentPage * itemCountInEntry;
	int32 endIndex = index + itemCountInEntry;
	for (int32 i = 0; index < endIndex; ++index)
	{
		if (!sessionSearchResults.IsValidIndex(index)) {break;}
		
		const FOnlineSessionSearchResult& Result = sessionSearchResults[index];

		if (Result.Session.SessionSettings.bShouldAdvertise == false)
		{
			// Join 불가
			endIndex++;
			continue;
		}
		
		sessionItemDataArray[i++]->WriteData(Result, index);
	}
	sessionsEntry->SetListItems(sessionItemDataArray);

	/*
	GetWorld()->GetTimerManager().SetTimer(
		refreshListTimer, this, &USearchingSessionsWidget::RefreshList,
		10.f, true);
	*/
	
}

void USearchingSessionsWidget::OnClickNextButton()
{
	SetPage(currentPage + 1);
}

void USearchingSessionsWidget::OnClickPrevButton()
{
	SetPage(currentPage - 1);
}

void USearchingSessionsWidget::OnClickSessionItem(UObject* clickedItem)
{	
	if (USessionItemData* data = Cast<USessionItemData>(clickedItem);
		IsValid(data))
	{
		joinSessionButton->SetIsEnabled(data->CanJoinInSession());
		sessionsEntry->SetSelectedItem(clickedItem);
	}

}

void USearchingSessionsWidget::OnClickJoinButton()
{
	USessionItemData* item = sessionsEntry->GetSelectedItem<USessionItemData>();
	if (!IsValid(item)) {return;}

	if (!item->CanJoinInSession())
	{
		// TODO : 세션 참가 불가 UI
	}

	if (AMainMenuGameMode* gm = GetWorld()->GetAuthGameMode<AMainMenuGameMode>())
	{
		if (gm->JoinLobby(*item->GetSessionResult().Get()))
		{
			
		}
	}
}

void USearchingSessionsWidget::OnClickFindSessionByCodeButton()
{
	// TODO : 세션 Code 입력 UI
	sessionSearchByCodeInputText->SetVisibility(
		sessionSearchByCodeInputText->IsVisible()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible
	);
	
}

void USearchingSessionsWidget::OnCommitInputCode(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter && !Text.IsEmpty())
	{
		if (AMainMenuGameMode* gm = GetWorld()->GetAuthGameMode<AMainMenuGameMode>())
		{
			gm->SearchLobbyByCode(Text.ToString());
			sessionSearchByCodeInputText->SetText(FText::GetEmpty());
			sessionSearchByCodeInputText->SetHintText(FText::FromString(TEXT("찾는 중 입니다.")));
		}
	}
}
