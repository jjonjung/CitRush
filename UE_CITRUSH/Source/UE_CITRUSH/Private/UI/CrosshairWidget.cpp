// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CrosshairWidget.h"
#include "Components/Image.h"
#include "Player/CommenderCharacter.h"
#include "GameFramework/PlayerController.h"

// NativeTick 제거 - SetCrosshairColor, ResetCrosshairToDefault, SetUseMode에서 직접 색상 설정하므로 불필요

void UCrosshairWidget::SetCrosshairColor(const FLinearColor& Color)
{
	if (Image_Crosshair && GetVisibility() != ESlateVisibility::Collapsed)
	{
		bUseCustomColor = true;
		CustomColor = Color;
		Image_Crosshair->SetColorAndOpacity(Color);
	}
}

void UCrosshairWidget::ResetCrosshairToDefault()
{
	if (Image_Crosshair && GetVisibility() != ESlateVisibility::Collapsed)
	{
		bUseCustomColor = false;
		Image_Crosshair->SetColorAndOpacity(DefaultColor);
	}
}

void UCrosshairWidget::SetUseMode(bool bUseMode)
{
	if (Image_Crosshair && GetVisibility() != ESlateVisibility::Collapsed)
	{
		if (bUseMode)
		{
			// 아이템 사용 가능 모드: AimingUsingMachineColor 사용
			bUseCustomColor = true;
			CustomColor = AimingUsingMachineColor;
			Image_Crosshair->SetColorAndOpacity(CustomColor);
		}
		else
		{
			// 기본 모드로 복원
			ResetCrosshairToDefault();
		}
	}
}

