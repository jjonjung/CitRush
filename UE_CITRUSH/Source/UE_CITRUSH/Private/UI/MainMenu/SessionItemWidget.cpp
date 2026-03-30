// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenu/SessionItemWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Utility/Base64Converter.h"
#include "Network/SteamSubsystem.h"

DEFINE_LOG_CATEGORY(SessionItemLog);

#pragma region Data

void USessionItemData::Initialize()
{
	searchResultIndex = -1;
	sessionSearchResult = nullptr;
}

void USessionItemData::WriteData(const FOnlineSessionSearchResult& session, const int32& inSearchResultIndex)
{
	if (!session.IsSessionInfoValid()) {return;}
	searchResultIndex = inSearchResultIndex;
	sessionSearchResult = MakeShared<FOnlineSessionSearchResult>(session);
}

bool USessionItemData::CanJoinInSession()
{
	if (!IsValid(GetWorld())) {return false;}
	USteamSubsystem* sss = GetWorld()->GetGameInstance()->GetSubsystem<USteamSubsystem>();
	if (! (IsValid(sss) && sessionSearchResult->IsSessionInfoValid()) ) {return false;}

	FOnlineSessionSettings sessionSettings = sessionSearchResult->Session.SessionSettings;
	int32 participantsCounter;
	sessionSettings.Get(sss->GetParticipantsCounterNameKey(), participantsCounter);
	
	return sessionSettings.NumPublicConnections > participantsCounter
		&& searchResultIndex > 0
	;
}
#pragma endregion Data


void USessionItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void USessionItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	SetVisibility(ESlateVisibility::Hidden);
	SetIsEnabled(false);
	USessionItemData* data = Cast<USessionItemData>(ListItemObject);
	if (!IsValid(data)) {return;}
	if (data->GetSearchResultIndex() < 0) {return;}
	
	USteamSubsystem* sss = GetGameInstance()->GetSubsystem<USteamSubsystem>();
	if (!IsValid(sss)) {return;}
	
	TSharedPtr<FOnlineSessionSearchResult> result = data->GetSessionResult();
	if (!result.IsValid()) {return;}

	int32 ping = result->PingInMs;
	if (ping < 0) {return;}
	
	hostPingText->SetText(FText::AsNumber(ping));
	if (ping > 100) {hostPingText->SetColorAndOpacity(FColor::Red);}
	else if (ping > 60) {hostPingText->SetColorAndOpacity(FColor::Orange);}
	else if (ping > 30) {hostPingText->SetColorAndOpacity(FColor::White);}
	else {hostPingText->SetColorAndOpacity(FColor::Green);}
	
	FString hostName;
	result->Session.SessionSettings.Get(sss->GetHostNameKey(), hostName);
	hostNameText->SetText(FText::FromString(UBase64Converter::Base64ToString(hostName)));
	FString displayName;
	result->Session.SessionSettings.Get(sss->GetDisplayNameKey(), displayName);
	displayNameText->SetText(FText::FromString(UBase64Converter::Base64ToString(displayName)));
	FString gameMapName;
	result->Session.SessionSettings.Get(sss->GetGameMapNameKey(), gameMapName);
	mapNameText->SetText(FText::FromString(UBase64Converter::Base64ToString(gameMapName)));

	maxPlayerCounterText->SetText(FText::AsNumber(result->Session.SessionSettings.NumPublicConnections));
	int32 participantsCounter;
	result->Session.SessionSettings.Get(sss->GetParticipantsCounterNameKey(), participantsCounter);
	currentPlayerCounterText->SetText(FText::AsNumber(participantsCounter));

	UE_LOG(SessionItemLog, Warning, TEXT("\n"
			"=========== Item ===========\n"
			"   Host : %s / Ping : %i\n   Session : %s\n   Map : %s\n"
			"============================"
		),
		*hostName, ping, *displayName, *gameMapName
	);

	if (data->CanJoinInSession())
	{
		SetVisibility(ESlateVisibility::Visible);
		SetIsEnabled(true);
		//backgroundButton->SetIsEnabled(true);
	}
}