// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenu/HomeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "UI/MainMenu/IndexedButton.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Network/SteamSubsystem.h"

#include "Data/WidgetDataAsset.h"
#include "Utility/DebugHelper.h"

void UHomeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (IsValid(exitButton))
	{
		exitButton->OnClicked.AddDynamic(this, &UHomeWidget::OnClickExitButton);
	}
}

void UHomeWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UHomeWidget::InitializeButtons(const TMap<int32, FText>& dictTextPerIndex)
{
	if (IsValid(buttonContainer))
	{
		buttonContainer->ClearChildren();
	}
	if (!IsValid(dynamicButtonFactory))
	{
		dynamicButtonFactory = widgetDataAsset->GetWidgetClass<UIndexedButton>(UIndexedButton::StaticClass());
		if (dynamicButtonFactory == nullptr) {CITRUSH_LOG("Cant Init Dynamic Button Factory");}
	}
	TArray<int32> indices;
	dictTextPerIndex.GetKeys(indices);
	for (const int32& index: indices)
	{
		UIndexedButton* button = CreateWidget<UIndexedButton>(this, dynamicButtonFactory);
		button->SetIndex(index);
		button->SetText(dictTextPerIndex[index]);
		button->OnIndexedButtonClicked.BindUObject(this, &UHomeWidget::OnButtonClickedIndex);
		buttonContainer->AddChild(button);
		button->SetPadding(buttonMargin);
	}
}

void UHomeWidget::InitializeButtons(const TMap<UWidget*, FText>& dictTextPerWidget)
{
	if (IsValid(buttonContainer))
	{
		buttonContainer->ClearChildren();
	}
	if (!IsValid(dynamicButtonFactory))
	{
		dynamicButtonFactory = widgetDataAsset->GetWidgetClass<UIndexedButton>(UIndexedButton::StaticClass());
		if (dynamicButtonFactory == nullptr) {CITRUSH_LOG("Cant Init Dynamic Button Factory");}
	}
	TArray<UWidget*> widgets;
	dictTextPerWidget.GetKeys(widgets);
	for (UWidget* widget : widgets)
	{
		UIndexedButton* button = CreateWidget<UIndexedButton>(this, dynamicButtonFactory);
		button->SetWidget(widget);
		button->SetText(dictTextPerWidget[widget]);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *dictTextPerWidget[widget].ToString());
		button->OnBoundWidgetButtonClicked.BindUObject(this, &UHomeWidget::OnButtonClickedWidget);
		buttonContainer->AddChild(button);
		button->SetPadding(buttonMargin);
	}
}

void UHomeWidget::OnButtonClickedIndex(const int32& index)
{
	OnScreenButtonClicked.ExecuteIfBound(index);
}

void UHomeWidget::OnButtonClickedWidget(UWidget* widget)
{
	OnWidgetSelected.ExecuteIfBound(widget);
}

void UHomeWidget::OnClickExitButton()
{
	UWorld* world = GetWorld();
	if (!IsValid(world)) {return;}
	
	if (world->GetNetDriver())
	{
		world->GetNetDriver()->Shutdown();
	}
	UKismetSystemLibrary::QuitGame(world, world->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

void UHomeWidget::OnMarginChanged()
{
	for (UWidget* child : buttonContainer->GetAllChildren())
	{
		if (UUserWidget* widget = Cast<UUserWidget>(child))
		{
			widget->SetPadding(buttonMargin);
		}
	}
}
