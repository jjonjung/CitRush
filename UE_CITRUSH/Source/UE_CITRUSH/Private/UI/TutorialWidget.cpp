// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/TutorialWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "TimerManager.h"

UTutorialWidget::UTutorialWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentMessageIndex = -1;
}

void UTutorialWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TutorialTextBlock)
	{
		SetupTextBlockStyle();
	}

	if (BackgroundUpdateInterval > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				BackgroundUpdateTimerHandle,
				this,
				&UTutorialWidget::OnBackgroundUpdateTimer,
				BackgroundUpdateInterval,
				true
			);
		}
	}
}

void UTutorialWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BackgroundUpdateTimerHandle);
	}

	Super::NativeDestruct();
}

void UTutorialWidget::OnBackgroundUpdateTimer()
{
	if (BackgroundImage && TutorialTextBlock && TutorialTextBlock->GetVisibility() == ESlateVisibility::Visible)
	{
		UpdateBackgroundImageSizeToFitText();
	}
}

void UTutorialWidget::ShowMessageByName(const FName& MessageName)
{
	int32 FoundIndex = FindMessageIndexByName(MessageName);
	if (FoundIndex >= 0)
	{
		ShowMessage(FoundIndex);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialWidget] 멘트 이름을 찾을 수 없습니다: %s"), 
			*MessageName.ToString());
	}
}

void UTutorialWidget::ShowMessage(int32 MessageIndex)
{
	if (!Messages.IsValidIndex(MessageIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialWidget] 유효하지 않은 멘트 인덱스: %d (총 %d개)"), 
			MessageIndex, Messages.Num());
		return;
	}

	CurrentMessageIndex = MessageIndex;
	CurrentText = Messages[MessageIndex].MessageText;
	FName CurrentMessageName = Messages[MessageIndex].MessageName;

	OnMessageChangedInternal(CurrentText, CurrentMessageIndex, CurrentMessageName);
	OnMessageChanged(CurrentText, CurrentMessageIndex, CurrentMessageName);
}

int32 UTutorialWidget::FindMessageIndexByName(const FName& MessageName) const
{
	for (int32 i = 0; i < Messages.Num(); ++i)
	{
		if (Messages[i].MessageName == MessageName)
		{
			return i;
		}
	}
	return -1;
}

FName UTutorialWidget::GetCurrentMessageName() const
{
	if (Messages.IsValidIndex(CurrentMessageIndex))
	{
		return Messages[CurrentMessageIndex].MessageName;
	}
	return NAME_None;
}

void UTutorialWidget::SetTutorialText(const FText& InText)
{
	CurrentText = InText;
	CurrentMessageIndex = -1;

	OnMessageChangedInternal(CurrentText, CurrentMessageIndex, NAME_None);
	OnMessageChanged(CurrentText, CurrentMessageIndex, NAME_None);
}

void UTutorialWidget::OnMessageChangedInternal(const FText& NewText, int32 MessageIndex, const FName& MessageName)
{
	if (!TutorialTextBlock)
	{
		return;
	}

	TutorialTextBlock->SetText(NewText);
	TutorialTextBlock->SetAutoWrapText(true);
	TutorialTextBlock->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	SetupTextBlockStyle();

	if (CurrentWidgetScale > 0.0f)
	{
		UpdateTextBlockSizeForWidgetScale();
	}

	UpdateBackgroundImageSizeToFitText();
}

void UTutorialWidget::OnMessageChanged_Implementation(const FText& NewText, int32 MessageIndex, const FName& MessageName)
{
}

bool UTutorialWidget::HasDirectTextInput() const
{
	return TutorialTextBlock && !TutorialTextBlock->GetText().IsEmpty();
}

void UTutorialWidget::SetupTextBlockStyle()
{
	if (TutorialTextBlock)
	{
		TutorialTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)));
	}
}


void UTutorialWidget::SetFontSizeByWidgetScale(float WidgetScale)
{
	CurrentWidgetScale = WidgetScale;

	if (TutorialTextBlock)
	{
		const float NewFontSize = FMath::Clamp(16.0f * WidgetScale, 8.0f, 72.0f);
		FSlateFontInfo FontInfo = TutorialTextBlock->GetFont();
		FontInfo.Size = FMath::RoundToInt(NewFontSize);
		TutorialTextBlock->SetFont(FontInfo);
	}
	
	UpdateTextBlockSizeForWidgetScale();
	UpdateBackgroundImageSizeToFitText();
}

void UTutorialWidget::UpdateTextBlockSizeForWidgetScale()
{
	if (!TutorialTextBlock || CurrentWidgetScale <= 0.0f)
	{
		return;
	}

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TutorialTextBlock->Slot))
	{
		static TMap<UTextBlock*, FVector2D> BaseSizes;
		FVector2D BaseSize;

		if (BaseSizes.Contains(TutorialTextBlock))
		{
			BaseSize = BaseSizes[TutorialTextBlock];
		}
		else
		{
			FVector2D CurrentSize = CanvasSlot->GetSize();
			if (CurrentWidgetScale != 1.0f && CurrentSize.X > 0.0f && CurrentSize.Y > 0.0f)
			{
				BaseSize = CurrentSize / CurrentWidgetScale;
			}
			else
			{
				BaseSize = CurrentSize;
			}
			
			if (BaseSize.X <= 0.0f || BaseSize.Y <= 0.0f)
			{
				BaseSize = FVector2D(360.0f, 410.0f);
			}
			BaseSizes.Add(TutorialTextBlock, BaseSize);
		}
		
		FVector2D NewSize = BaseSize * CurrentWidgetScale;
		NewSize.Y = FMath::Max(NewSize.Y, 1000.0f * CurrentWidgetScale);
		
		CanvasSlot->SetSize(NewSize);
		TutorialTextBlock->SetWrapTextAt(NewSize.X > 0.0f ? NewSize.X : 0.0f);
	}
}

void UTutorialWidget::UpdateBackgroundImageSizeToFitText()
{
	if (!BackgroundImage || !TutorialTextBlock || TutorialTextBlock->GetVisibility() != ESlateVisibility::Visible)
	{
		return;
	}

	TSharedPtr<SWidget> SlateWidget = TutorialTextBlock->GetCachedWidget();
	if (!SlateWidget.IsValid())
	{
		return;
	}

	FVector2D TextBlockSize = SlateWidget->GetDesiredSize();
	
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TutorialTextBlock->Slot))
	{
		FVector2D SlotSize = CanvasSlot->GetSize();
		if (SlotSize.X > 0.0f)
		{
			TextBlockSize.X = SlotSize.X;
		}
		if (SlotSize.Y < TextBlockSize.Y)
		{
			CanvasSlot->SetSize(FVector2D(SlotSize.X, TextBlockSize.Y));
		}
	}

	TextBlockSize.Y = FMath::Max(TextBlockSize.Y, 50.0f * CurrentWidgetScale);

	if (TextBlockSize.X <= 0.0f || TextBlockSize.Y <= 0.0f)
	{
		TextBlockSize = (BaseBackgroundImageSize - (TextPadding * 2.0f)) * CurrentWidgetScale;
	}

	FVector2D BackgroundSize = ((TextBlockSize / CurrentWidgetScale) + (TextPadding * 2.0f)) * CurrentWidgetScale;
	FVector2D MinSize = BaseBackgroundImageSize * CurrentWidgetScale;
	BackgroundSize.X = FMath::Max(BackgroundSize.X, MinSize.X);
	BackgroundSize.Y = FMath::Max(BackgroundSize.Y, MinSize.Y);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BackgroundImage->Slot))
	{
		FVector2D CurrentSize = CanvasSlot->GetSize();
		if (!FMath::IsNearlyEqual(CurrentSize.X, BackgroundSize.X, 1.0f) ||
			!FMath::IsNearlyEqual(CurrentSize.Y, BackgroundSize.Y, 1.0f))
		{
			CanvasSlot->SetSize(BackgroundSize);
		}
	}
}

