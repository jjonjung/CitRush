// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/TargetRacerDisplayWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "GameFlow/CitRushGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateColor.h"

void UTargetRacerDisplayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 루트 컨테이너 검증
	if (!Root_Container)
	{
		UE_LOG(LogTemp, Error, TEXT("[TargetRacerDisplayWidget] Root_Container (CanvasPanel or Overlay) is required! Please add it in Blueprint."));
	}

	// 초기 상태: 타겟 없음
	ClearTargetRacer();
}

void UTargetRacerDisplayWidget::SetTargetRacer(int32 RacerIndex, UTexture2D* RacerImage, FLinearColor RacerColor)
{
	CurrentRacerIndex = RacerIndex;

	// 레이서 아이콘 설정
	if (Image_RacerPortrait)
	{
		if (RacerImage)
		{
			Image_RacerPortrait->SetBrushFromTexture(RacerImage);
		}
		else
		{
			// 이미지가 없으면 기본 이미지 사용 (선택사항)
			Image_RacerPortrait->SetBrushFromTexture(nullptr);
		}
	}

	// 레이서 이름 설정 및 색상 적용
	if (Text_RacerName)
	{
		FString RacerName = TEXT("Racer_") + FString::FromInt(RacerIndex + 1);
		
		// GameState에서 실제 레이서 이름 가져오기 (targetIndex 기반)
		if (UWorld* World = GetWorld())
		{
			if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
			{
				TArray<ACitRushPlayerState*> Racers = GameState->GetPlayerStatesByRole(EPlayerRole::Racer);
				
				// RacerIndex를 ETargetRacer로 변환
				ETargetRacer TargetRacerEnum = ETargetRacer::None;
				if (RacerIndex == 0) TargetRacerEnum = ETargetRacer::Racer1;
				else if (RacerIndex == 1) TargetRacerEnum = ETargetRacer::Racer2;
				else if (RacerIndex == 2) TargetRacerEnum = ETargetRacer::Racer3;
				
				// targetIndex가 일치하는 레이서 찾기
				for (ACitRushPlayerState* RacerPS : Racers)
				{
					if (IsValid(RacerPS) && RacerPS->GetPlayerInfo().targetIndex == TargetRacerEnum)
					{
						RacerName = RacerPS->GetPlayerInfo().playerName.IsEmpty() 
							? RacerPS->GetPlayerName() 
							: RacerPS->GetPlayerInfo().playerName;
						break;
					}
				}
			}
		}
		
		Text_RacerName->SetText(FText::FromString(RacerName));
		
		// 텍스트 색상을 레이서 색상으로 설정
		Text_RacerName->SetColorAndOpacity(RacerColor);
	}

	// 블루프린트에서 레이서 정보 업데이트
	OnTargetRacerUpdated(RacerIndex);

	// 위젯 표시
	SetVisibility(ESlateVisibility::Visible);
}

void UTargetRacerDisplayWidget::ClearTargetRacer()
{
	CurrentRacerIndex = -1;

	// 위젯 숨김 또는 기본 상태로 설정
	SetVisibility(ESlateVisibility::Collapsed);

	// 텍스트 초기화
	if (Text_RacerName)
	{
		Text_RacerName->SetText(FText::GetEmpty());
	}

	// 이미지 초기화
	if (Image_RacerPortrait)
	{
		Image_RacerPortrait->SetBrushFromTexture(nullptr);
	}
}

