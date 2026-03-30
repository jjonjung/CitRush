// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/EnemyBrainCam.h"
#include "Subsystems/EnemyAISubsystem.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Enemy/PixelEnemy.h"
#include "GAS/AttributeSet/ASEnemy.h"

void UEnemyBrainCam::NativeConstruct()
{
	Super::NativeConstruct();

	// 콤팩트 모드 처리 (PixelEnemy용)
	if (bIsCompactMode)
	{
		if (SummaryTextBlock)
		{
			SummaryTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (ReasoningTextBlock)
		{
			ReasoningTextBlock->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		AISubsystem = GameInstance->GetSubsystem<UEnemyAISubsystem>();

		if (AISubsystem)
		{
			AISubsystem->OnBrainCamDataReceived.AddDynamic(this, &UEnemyBrainCam::OnBrainCamDataReceived);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[AISubsystem] S000"));
		}
	}
}

void UEnemyBrainCam::NativeDestruct()
{
	if (AISubsystem)
	{
		AISubsystem->OnBrainCamDataReceived.RemoveDynamic(this, &UEnemyBrainCam::OnBrainCamDataReceived);
	}

	Super::NativeDestruct();
}

void UEnemyBrainCam::UpdateBrainCamData(const FString& Summary, const FString& FinalChoice, const TArray<FString>& ReasoningDocs)
{
	// 1. 상황 요약 (Perception)
	if (SummaryTextBlock)
	{
		SummaryTextBlock->SetText(FText::FromString(Summary));
	}

	// 2. 참고 전술 (Reasoning)
	if (ReasoningTextBlock)
	{
		FString ReasoningText;
		for (const FString& Doc : ReasoningDocs)
		{
			ReasoningText += FString::Printf(TEXT("\u2022 %s\n"), *Doc); // Bullet point 추가
		}
		
		// 내용이 없으면 기본 텍스트 표시
		if (ReasoningText.IsEmpty())
		{
			ReasoningText = TEXT("-");
		}

		ReasoningTextBlock->SetText(FText::FromString(ReasoningText));
	}

	// 3. AI 최종 판단 (Decision)
	if (FinalChoiceTextBlock)
	{
		FinalChoiceTextBlock->SetText(FText::FromString(FinalChoice));
	}

	// 4. 현재 체력 업데이트 (TEETHcurrentHP)
	if (TEETHcurrentHP && AISubsystem)
	{
		// pacman_main ID를 가진 적을 찾아 체력 정보를 가져옴
		TScriptInterface<IAIDecisionReceiver> EnemyInterface = AISubsystem->FindEnemyByID(TEXT("pacman_main"));
		if (AActor* EnemyActor = Cast<AActor>(EnemyInterface.GetObject()))
		{
			if (APixelEnemy* PixelEnemy = Cast<APixelEnemy>(EnemyActor))
			{
				if (UASEnemy* AS = Cast<UASEnemy>(PixelEnemy->GetAttributeSet()))
				{
					float CurrentHP = AS->GetHealth();
					TEETHcurrentHP->SetText(FText::AsNumber(FMath::RoundToInt(CurrentHP)));
				}
			}
		}
	}
}

void UEnemyBrainCam::OnBrainCamDataReceived(const FString& Summary, const FString& FinalChoice, const TArray<FString>& ReasoningDocs)
{
	UpdateBrainCamData(Summary, FinalChoice, ReasoningDocs);
}
