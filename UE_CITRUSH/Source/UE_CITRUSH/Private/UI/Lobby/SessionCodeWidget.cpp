// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Lobby/SessionCodeWidget.h"

//#include "GenericPlatform/GenericApplication.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

#include "GameFlow/LobbyGameMode.h"
#include "Windows/WindowsPlatformApplicationMisc.h"


void USessionCodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	sessionCodeText->SetText(FText::GetEmpty());
	if (ALobbyGameMode* lGM = GetWorld()->GetAuthGameMode<ALobbyGameMode>())
	{
		sessionCodeText->SetText(FText::FromString(lGM->GetSessionCode()));
	}
	copySessionCodeButton->OnClicked.AddDynamic(this, &USessionCodeWidget::OnClickCopyButton);
}

void USessionCodeWidget::OnClickCopyButton()
{
	if (!sessionCodeText->GetText().IsEmpty())
	{
		FPlatformApplicationMisc::ClipboardCopy(*sessionCodeText->GetText().ToString());
		UE_LOG(LogTemp, Display, TEXT("Copy Session Code Success! : %s"), *sessionCodeText->GetText().ToString());
	}
}
