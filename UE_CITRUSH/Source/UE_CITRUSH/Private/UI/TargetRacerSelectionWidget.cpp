// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/TargetRacerSelectionWidget.h"
#include "CommenderSystem/ItemInputMachine.h"
#include "Player/CommenderCharacter.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateBrush.h"

void UTargetRacerSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼 설정 및 이벤트 바인딩
	UButton* Buttons[] = { Racer1Button.Get(), Racer2Button.Get(), Racer3Button.Get() };
	for (int32 i = 0; i < 3; ++i)
	{
		if (Buttons[i])
		{
			SetupButtonContent(Buttons[i], i);
			
			switch (i)
			{
			case 0:
				Buttons[i]->OnClicked.AddDynamic(this, &UTargetRacerSelectionWidget::OnRacer1Clicked);
				Buttons[i]->OnHovered.AddDynamic(this, &UTargetRacerSelectionWidget::OnRacer1Hovered);
				Buttons[i]->OnUnhovered.AddDynamic(this, &UTargetRacerSelectionWidget::OnRacer1Unhovered);
				break;
			case 1:
				Buttons[i]->OnClicked.AddDynamic(this, &UTargetRacerSelectionWidget::OnRacer2Clicked);
				Buttons[i]->OnHovered.AddDynamic(this, &UTargetRacerSelectionWidget::OnRacer2Hovered);
				Buttons[i]->OnUnhovered.AddDynamic(this, &UTargetRacerSelectionWidget::OnRacer2Unhovered);
				break;
			case 2:
				Buttons[i]->OnClicked.AddDynamic(this, &UTargetRacerSelectionWidget::OnRacer3Clicked);
				Buttons[i]->OnHovered.AddDynamic(this, &UTargetRacerSelectionWidget::OnRacer3Hovered);
				Buttons[i]->OnUnhovered.AddDynamic(this, &UTargetRacerSelectionWidget::OnRacer3Unhovered);
				break;
			}
		}
	}

	// 레이서 이미지 업데이트
	UpdateRacerImages();
}

void UTargetRacerSelectionWidget::SetItemInputMachine(AItemInputMachine* InMachine)
{
	ItemInputMachine = InMachine;
}

void UTargetRacerSelectionWidget::SelectRacer(int32 RacerIndex)
{
	if (!ItemInputMachine)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TargetRacerSelectionWidget] ItemInputMachine이 설정되지 않았습니다."));
		return;
	}

	ItemInputMachine->SelectRacer(RacerIndex);
	CloseWidget();
}

void UTargetRacerSelectionWidget::CloseWidget()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());

		if (ACommenderCharacter* Commander = Cast<ACommenderCharacter>(PlayerController->GetPawn()))
		{
			if (UCharacterMovementComponent* MovementComp = Commander->GetCharacterMovement())
			{
				MovementComp->SetMovementMode(MOVE_Walking);
			}
		}
	}

	if (ItemInputMachine)
	{
		ItemInputMachine->CurrentSelectionWidget = nullptr;
	}

	RemoveFromParent();
}

// 헬퍼 함수 구현
UImage* UTargetRacerSelectionWidget::GetRacerImage(int32 RacerIndex) const
{
	switch (RacerIndex)
	{
	case 0: return Racer1Image.Get();
	case 1: return Racer2Image.Get();
	case 2: return Racer3Image.Get();
	default: return nullptr;
	}
}

UTextBlock* UTargetRacerSelectionWidget::GetRacerText(int32 RacerIndex) const
{
	switch (RacerIndex)
	{
	case 0: return Racer1Text.Get();
	case 1: return Racer2Text.Get();
	case 2: return Racer3Text.Get();
	default: return nullptr;
	}
}

UWidgetAnimation* UTargetRacerSelectionWidget::GetRacerAnimation(int32 RacerIndex) const
{
	switch (RacerIndex)
	{
	case 0: return Racer1HoveredAnimation.Get();
	case 1: return Racer2HoveredAnimation.Get();
	case 2: return Racer3HoveredAnimation.Get();
	default: return nullptr;
	}
}

void UTargetRacerSelectionWidget::SetRacerImage(int32 RacerIndex, UImage* Image)
{
	switch (RacerIndex)
	{
	case 0: Racer1Image = Image; break;
	case 1: Racer2Image = Image; break;
	case 2: Racer3Image = Image; break;
	}
}

void UTargetRacerSelectionWidget::SetRacerText(int32 RacerIndex, UTextBlock* TextBlock)
{
	switch (RacerIndex)
	{
	case 0: Racer1Text = TextBlock; break;
	case 1: Racer2Text = TextBlock; break;
	case 2: Racer3Text = TextBlock; break;
	}
}

// 통합된 이벤트 핸들러
void UTargetRacerSelectionWidget::OnRacerClicked(int32 RacerIndex)
{
	SelectRacer(RacerIndex);
}

void UTargetRacerSelectionWidget::OnRacerHovered(int32 RacerIndex)
{
	if (UWidgetAnimation* Animation = GetRacerAnimation(RacerIndex))
	{
		PlayAnimation(Animation, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

void UTargetRacerSelectionWidget::OnRacerUnhovered(int32 RacerIndex)
{
	if (UWidgetAnimation* Animation = GetRacerAnimation(RacerIndex))
	{
		PlayAnimation(Animation, 0.0f, 1, EUMGSequencePlayMode::Reverse, 1.0f);
	}
}

void UTargetRacerSelectionWidget::SetupButtonContent(UButton* Button, int32 RacerIndex)
{
	if (!Button || !WidgetTree)
	{
		return;
	}

	UWidget* CurrentContent = Button->GetContent();
	FText ExistingText = FText::GetEmpty();
	
	if (CurrentContent)
	{
		if (UTextBlock* ExistingTextBlock = Cast<UTextBlock>(CurrentContent))
		{
			ExistingText = ExistingTextBlock->GetText();
		}
	}

	UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(CurrentContent);
	
	if (!HorizontalBox)
	{
		HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (!HorizontalBox)
		{
			UE_LOG(LogTemp, Error, TEXT("[TargetRacerSelectionWidget] HorizontalBox 생성 실패"));
			return;
		}
		Button->SetContent(HorizontalBox);
	}
	else
	{
		// 기존 HorizontalBox의 자식 위젯들을 확인하여 바인딩
		for (UWidget* Child : HorizontalBox->GetAllChildren())
		{
			if (!Child) continue;
			
			if (UImage* ExistingImage = Cast<UImage>(Child))
			{
				if (!IsValid(GetRacerImage(RacerIndex)))
				{
					SetRacerImage(RacerIndex, ExistingImage);
				}
			}
			else if (UTextBlock* ExistingTextBlock = Cast<UTextBlock>(Child))
			{
				if (!IsValid(GetRacerText(RacerIndex)))
				{
					SetRacerText(RacerIndex, ExistingTextBlock);
					ExistingText = ExistingTextBlock->GetText();
				}
			}
		}
	}

	// Image 위젯 처리
	UImage* ImageWidget = GetRacerImage(RacerIndex);
	if (!ImageWidget)
	{
		ImageWidget = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (ImageWidget)
		{
			SetRacerImage(RacerIndex, ImageWidget);
		}
	}

	if (ImageWidget)
	{
		bool bAlreadyInHorizontalBox = (ImageWidget->GetParent() == HorizontalBox);
		
		if (!bAlreadyInHorizontalBox)
		{
			if (ImageWidget->GetParent())
			{
				ImageWidget->RemoveFromParent();
			}
			
			if (UHorizontalBoxSlot* ImageSlot = HorizontalBox->AddChildToHorizontalBox(ImageWidget))
			{
				ImageSlot->SetHorizontalAlignment(HAlign_Left);
				ImageSlot->SetVerticalAlignment(VAlign_Center);
				ImageSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			}
			ImageWidget->SetDesiredSizeOverride(FVector2D(64.0f, 64.0f));
		}
	}

	// TextBlock 처리
	UTextBlock* TextWidget = GetRacerText(RacerIndex);
	if (!TextWidget)
	{
		TextWidget = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (TextWidget)
		{
			if (!ExistingText.IsEmpty())
			{
				TextWidget->SetText(ExistingText);
			}
			else
			{
				FString DefaultText = FString::Printf(TEXT("Racer_%d"), RacerIndex + 1);
				TextWidget->SetText(FText::FromString(DefaultText));
			}
			SetRacerText(RacerIndex, TextWidget);
		}
	}
	else
	{
		if (TextWidget->GetParent())
		{
			TextWidget->RemoveFromParent();
		}
		if (!ExistingText.IsEmpty())
		{
			TextWidget->SetText(ExistingText);
		}
	}

	if (TextWidget)
	{
		bool bAlreadyInHorizontalBox = (TextWidget->GetParent() == HorizontalBox);
		
		if (!bAlreadyInHorizontalBox)
		{
			if (UHorizontalBoxSlot* TextSlot = HorizontalBox->AddChildToHorizontalBox(TextWidget))
			{
				TextSlot->SetHorizontalAlignment(HAlign_Left);
				TextSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}
}

void UTargetRacerSelectionWidget::UpdateRacerImages()
{
	for (int32 i = 0; i < 3; ++i)
	{
		if (UImage* ImageWidget = GetRacerImage(i))
		{
			if (RacerImages.IsValidIndex(i) && RacerImages[i])
			{
				ImageWidget->SetBrushFromTexture(RacerImages[i]);
			}
		}
	}
}

void UTargetRacerSelectionWidget::SetRacerText(int32 RacerIndex, const FText& Text)
{
	if (UTextBlock* TargetText = GetRacerText(RacerIndex))
	{
		TargetText->SetText(Text);
	}
	else if (RacerIndex >= 0 && RacerIndex < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TargetRacerSelectionWidget] 레이서 %d의 TextBlock이 없습니다."), RacerIndex);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TargetRacerSelectionWidget] 잘못된 레이서 인덱스: %d"), RacerIndex);
	}
}

void UTargetRacerSelectionWidget::SetRacerName(int32 RacerIndex, const FString& Name)
{
	SetRacerText(RacerIndex, FText::FromString(Name));
}

