// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenu/IndexedButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UIndexedButton::NativeConstruct()
{
	Super::NativeConstruct();
	indexedButton->OnClicked.AddDynamic(this, &UIndexedButton::OnDynamicButtonClicked);
}

void UIndexedButton::SetText(const FText& newText)
{
	buttonText->SetText(newText);
}

void UIndexedButton::OnDynamicButtonClicked()
{
	if (boundWidget != nullptr && boundWidget.IsValid())
	{
		OnBoundWidgetButtonClicked.ExecuteIfBound(boundWidget.Get());
		return;
	}
	if (widgetIndex < 0) {return;}
	
	OnIndexedButtonClicked.ExecuteIfBound(widgetIndex);
}
