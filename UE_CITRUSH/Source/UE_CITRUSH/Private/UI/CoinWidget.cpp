// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CoinWidget.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Animation/WidgetAnimation.h"

void UCoinWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 루트 컨테이너 검증 (필수)
	if (!Root_Container)
	{
		UE_LOG(LogTemp, Error, TEXT("[CoinWidget] Root_Container (CanvasPanel or Overlay) is required! Please add it in Blueprint."));
	}
	
	// 초기 코인 값 표시
	SetCoinValue(CurrentCoinValue);
}

void UCoinWidget::SetCoinValue(int32 NewValue)
{
	if (Text_Coin)
	{
		Text_Coin->SetText(FText::AsNumber(NewValue));
	}

	// 코인 값이 변경되었을 때
	if (NewValue != CurrentCoinValue)
	{
		// 변경량 계산 및 팝업 표시
		int32 ChangeAmount = NewValue - CurrentCoinValue;
		if (ChangeAmount != 0)
		{
			ShowCoinChange(ChangeAmount);
		}

		// 바운스 애니메이션 재생
		if (Anim_Bounce)
		{
			PlayAnimation(Anim_Bounce, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		}

		// 경고 상태 체크
		SetWarningState(NewValue <= WarningThreshold);
	}

	CurrentCoinValue = NewValue;
}

void UCoinWidget::ShowCoinChange(int32 ChangeAmount)
{
	if (ChangeAmount == 0)
		return;

	if (Text_CoinGain)
	{
		// 양수면 +100, 음수면 -100 형태로 표시
		FText ChangeText;
		if (ChangeAmount > 0)
		{
			ChangeText = FText::Format(FText::FromString(TEXT("+{0}")), ChangeAmount);
		}
		else
		{
			ChangeText = FText::Format(FText::FromString(TEXT("{0}")), ChangeAmount); // 음수는 자동으로 - 부호 포함
		}
		
		Text_CoinGain->SetText(ChangeText);
		Text_CoinGain->SetVisibility(ESlateVisibility::Visible);

		// 코인 변경 팝업 애니메이션 (블루프린트에서 구현 가능)
		// 예: Fade In/Out, Slide Up 등
		// 블루프린트에서 Anim_CoinChange 같은 애니메이션을 만들고 여기서 재생할 수 있습니다
		
		// 일정 시간 후 자동으로 숨김 (애니메이션이 없을 경우 대비)
		if (UWorld* World = GetWorld())
		{
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(
				TimerHandle,
				[this]()
				{
					if (Text_CoinGain)
					{
						Text_CoinGain->SetVisibility(ESlateVisibility::Collapsed);
					}
				},
				2.0f, // 2초 후 숨김
				false
			);
		}
	}
}

void UCoinWidget::SetWarningState(bool bIsWarning)
{
	if (Anim_Warning)
	{
		if (bIsWarning)
		{
			// 경고 애니메이션 재생 (무한 반복: 0 = 무한)
			PlayAnimation(Anim_Warning, 0.0f, 0, EUMGSequencePlayMode::Forward, 1.0f);
		}
		else
		{
			// 경고 애니메이션 중지
			StopAnimation(Anim_Warning);
		}
	}

	// 블루프린트에서 색상 변경 등 추가 처리 가능
}

