// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CommanderMessageWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Animation/WidgetAnimation.h"
#include "TimerManager.h"

void UCommanderMessageWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UCommanderMessageWidget::Setup(const FText& InMessage, ECommanderMessageType InType, float InDuration)
{
	if (Text_Message)
	{
		Text_Message->SetText(InMessage);
	}

	MessageType = InType;
	MessageDuration = InDuration > 0.0f ? InDuration : 2.0f;

	ApplyTypeSettings();
}

void UCommanderMessageWidget::PlayShowHideAnimation()
{
	if (Anim_ShowHide)
	{
		// 애니메이션 재생
		PlayAnimation(Anim_ShowHide, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		
		
	}
	
	// 메시지 지속 시간 후 제거를 위한 타이머 설정
	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&UCommanderMessageWidget::OnMessageAnimationFinished,
			MessageDuration,
			false
		);
	}
}

void UCommanderMessageWidget::OnMessageAnimationFinished()
{
	// 부모 위젯에서 제거
	if (UPanelWidget* Parent = GetParent())
	{
		RemoveFromParent();
	}
}

void UCommanderMessageWidget::ApplyTypeSettings()
{
	
	switch (MessageType)
	{
		case ECommanderMessageType::Warning:
			// 노란색 계열
			break;
		case ECommanderMessageType::Error:
			// 빨간색 계열 
			break;
		case ECommanderMessageType::Success:
			// 초록색 계열
			break;
		case ECommanderMessageType::Info:
		default:
			// 기본 색상 
			break;
	}
}

