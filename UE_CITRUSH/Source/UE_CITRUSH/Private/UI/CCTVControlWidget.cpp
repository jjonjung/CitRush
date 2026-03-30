// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CCTVControlWidget.h"
#include "Player/CCTV/CCTVFeedComponent.h"

// CCTV 로그 매크로 (모든 CCTV 관련 로그를 [CCTVLog] 태그로 통일)
#define CCTV_LOG(Verbosity, Format, ...) UE_LOG(LogCCTVFeed, Verbosity, TEXT("[CCTVLog] " Format), ##__VA_ARGS__)
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/GridPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Slate/SlateBrushAsset.h"
#include "Styling/SlateBrush.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "Player/CitRushPlayerState.h"
#include "GameFramework/Pawn.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Data/CitRushPlayerTypes.h"
#include "GameFramework/GameStateBase.h"
#include "Player/AbstractRacer.h"
#include "GAS/AttributeSet/ASRacer.h"
#include "AbilitySystemComponent.h"
#include "Enemy/AbstractEnemy.h"
#include "Player/AbstractCommander.h"

void UCCTVControlWidget::SetCCTVFeedComponent(UCCTVFeedComponent* InFeedComponent)
{
	// 기존 컴포넌트 이벤트 구독 해제
	if (CCTVFeedComponent)
	{
		CCTVFeedComponent->OnFocusIndexChanged.RemoveAll(this);
		CCTVFeedComponent->OnExpandedChanged.RemoveAll(this);
		CCTVFeedComponent->OnCameraSlotChanged.RemoveAll(this);
	}

	CCTVFeedComponent = InFeedComponent;
	CCTV_LOG(Log, "CCTVFeedComponent 설정: %s",
		CCTVFeedComponent ? *CCTVFeedComponent->GetName() : TEXT("nullptr"));
	
	// Feed Component가 설정되면 즉시 업데이트 및 이벤트 구독
	if (CCTVFeedComponent && IsConstructed())
	{
		// 초기 상태: 카메라 연결 여부 확인하여 텍스트 설정
		const TArray<UTextBlock*> RacerTexts = { Racer1Text, Racer2Text, Racer3Text, PixelEnemyText };
		const TArray<FString> DefaultNames = { TEXT("Racer1"), TEXT("Racer2"), TEXT("Racer3"), TEXT("Enemy-TEETH") };
		
		for (int32 i = 0; i < FeedImages.Num() && i < 4; ++i)
		{
			if (RacerTexts.IsValidIndex(i) && RacerTexts[i])
			{
				// Pawn 유효성 확인 (Enemy나 Racer가 죽었는지 확인)
				FCCTVFeedSlot FeedSlot = CCTVFeedComponent->GetFeedSlot(i);
				bool bIsPawnValid = IsValid(FeedSlot.TargetPawn);
				
				// 레이서인 경우 HP가 0인지 확인
				if (bIsPawnValid && i < 3)
				{
					if (AAbstractRacer* Racer = Cast<AAbstractRacer>(FeedSlot.TargetPawn))
					{
						if (UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet()))
						{
							if (RacerAS->GetHealth() <= 0.0f)
							{
								bIsPawnValid = false; // HP가 0이면 유효하지 않음
							}
						}
					}
				}
				
				// RenderTarget 유효성 확인
				UTextureRenderTarget2D* RT = CCTVFeedComponent->GetRenderTarget(i);
				const bool bHasValidRT = (RT && RT->SizeX > 0 && RT->SizeY > 0);
				
				// 레이서 이름 가져오기 (플레이어 아이디가 있으면 아이디, 없으면 기본 이름)
				FString RacerName;
				if (i < 3)
				{
					RacerName = GetPlayerDisplayName(i);
					if (RacerName.IsEmpty())
					{
						RacerName = DefaultNames[i];
					}
				}
				else
				{
					RacerName = DefaultNames[i];
				}
				
				FString DisplayText;
				FLinearColor TextColor;
				
				if (bIsPawnValid && bHasValidRT)
				{
					DisplayText = FString::Printf(TEXT("%s - ON AIR"), *RacerName);
					TextColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f); // 파란색
				}
				else
				{
					DisplayText = FString::Printf(TEXT("%s - NO SIGNAL"), *RacerName);
					TextColor = FLinearColor::White;
				}
				
				RacerTexts[i]->SetText(FText::FromString(DisplayText));
				RacerTexts[i]->SetColorAndOpacity(TextColor);
				RacerTexts[i]->SetVisibility(ESlateVisibility::Visible);
			}
		}
		
		CCTVFeedComponent->OnFocusIndexChanged.AddDynamic(this, &UCCTVControlWidget::OnFocusIndexChanged);
		CCTVFeedComponent->OnExpandedChanged.AddDynamic(this, &UCCTVControlWidget::OnExpandedChanged);
		CCTVFeedComponent->OnCameraSlotChanged.AddDynamic(this, &UCCTVControlWidget::OnCameraSlotChanged);
		
		// 초기 상태 설정 (이벤트가 브로드캐스트되면 자동으로 업데이트됨)
		bPreviousExpanded = CCTVFeedComponent->IsExpanded();
		
		// 첫 오픈 시 브러시 재세팅을 다음 프레임으로 지연 (캡처 완료 대기)
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick([this]()
			{
				UpdateExpandState();
				UpdateClickedOutline();
			});
		}
		else
		{
			// World가 없으면 즉시 업데이트 (에디터 등)
			UpdateExpandState();
			UpdateClickedOutline();
		}
	}
}

void UCCTVControlWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 위젯이 키보드 입력을 받을 수 있도록 설정
	SetIsFocusable(true);
	
	// FeedImage 위젯 배열 초기화
	this->FeedImages = { FeedImage_0, FeedImage_1, FeedImage_2, FeedImage_3 };
	
	// FeedImage 위젯들의 Clipping 설정 (Clip To Bounds = False)
	for (UImage* FeedImage : FeedImages)
	{
		if (FeedImage)
		{
			// Clip To Bounds를 비활성화하여 Grid 영역을 넘어서도 렌더링되도록 함
			FeedImage->SetClipping(EWidgetClipping::OnDemand);
			// 초기 Scale 설정
			FeedImage->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}
	
	// ExpandedFeedImage도 동일하게 설정
	if (ExpandedFeedImage)
	{
		ExpandedFeedImage->SetClipping(EWidgetClipping::OnDemand);
		ExpandedFeedImage->SetRenderScale(FVector2D(1.0f, 1.0f));
	}
	
	// OutlineBorder 초기화 (숨김)
	if (OutlineBorder)
	{
		// Brush 설정: 테두리만 표시되도록 Margin 설정
		FSlateBrush BorderBrush;
		BorderBrush.DrawAs = ESlateBrushDrawType::Border;
		BorderBrush.Margin = FMargin(OutlineThickness);
		BorderBrush.TintColor = OutlineColor;
		BorderBrush.ImageSize = FVector2D(1.0f, 1.0f); // 최소 크기
		OutlineBorder->SetBrush(BorderBrush);
		OutlineBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// BrainCam 위젯 초기화 (숨김)
	if (BrainCamWidget)
	{
		BrainCamWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// 호버 애니메이션 초기화
	HoveredFeedIndex = -1;
	PreviousHoveredFeedIndex = -1;
	ClickedFeedIndex = -1;
	HoverAnimationProgress = 0.0f;
	bHoverExpanding = true;
	
	// 노이즈 Material Instance 생성
	NoiseMaterialInstances.Empty();
	if (NoiseMaterial)
	{
		for (int32 i = 0; i < 4; ++i)
		{
			UMaterialInstanceDynamic* NoiseMI = UMaterialInstanceDynamic::Create(NoiseMaterial, this);
			if (NoiseMI)
			{
				// 각 인스턴스에 랜덤 오프셋 설정 (다른 노이즈 패턴)
				NoiseMI->SetScalarParameterValue(FName("TimeOffset"), FMath::RandRange(0.0f, 1000.0f));
				NoiseMaterialInstances.Add(NoiseMI);
			}
		}
	}
	
	// 초기 상태: 모든 슬롯에 노이즈 머티리얼 설정
	const TArray<UTextBlock*> RacerTexts = { Racer1Text, Racer2Text, Racer3Text, PixelEnemyText };
	const TArray<FString> DefaultNames = { TEXT("Racer1"), TEXT("Racer2"), TEXT("Racer3"), TEXT("Enemy-TEETH") };
	
	for (int32 i = 0; i < FeedImages.Num() && i < 4; ++i)
	{
		if (UImage* FeedImage = FeedImages[i])
		{
			SetFeedImageNoiseOrBackground(FeedImage, i);
			FeedImage->SetVisibility(ESlateVisibility::Visible);
		}
		
		// CCTVFeedComponent가 설정되어 있으면 카메라 연결 여부 확인, 없으면 기본 "No Signal"
		if (RacerTexts.IsValidIndex(i) && RacerTexts[i])
		{
			// 레이서 이름 가져오기 (플레이어 아이디가 있으면 아이디, 없으면 기본 이름)
			FString RacerName;
			if (i < 3)
			{
				RacerName = GetPlayerDisplayName(i);
				if (RacerName.IsEmpty())
				{
					RacerName = DefaultNames[i];
				}
			}
			else
			{
				RacerName = DefaultNames[i];
			}
			
			FString DisplayText;
			FLinearColor TextColor;
			
			if (CCTVFeedComponent)
			{
				// Pawn 유효성 확인 (Enemy나 Racer가 죽었는지 확인)
				FCCTVFeedSlot FeedSlot = CCTVFeedComponent->GetFeedSlot(i);
				bool bIsPawnValid = IsValid(FeedSlot.TargetPawn);
				
				// 레이서인 경우 HP가 0인지 확인
				if (bIsPawnValid && i < 3)
				{
					if (AAbstractRacer* Racer = Cast<AAbstractRacer>(FeedSlot.TargetPawn))
					{
						if (UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet()))
						{
							if (RacerAS->GetHealth() <= 0.0f)
							{
								bIsPawnValid = false; // HP가 0이면 유효하지 않음
							}
						}
					}
				}
				
				// 카메라 연결 여부 확인
				UTextureRenderTarget2D* RT = CCTVFeedComponent->GetRenderTarget(i);
				const bool bHasValidRT = (RT && RT->SizeX > 0 && RT->SizeY > 0);
				
				// Pawn이 유효하고 RenderTarget도 유효한 경우에만 "레이서이름 - On Air"
				if (bIsPawnValid && bHasValidRT)
				{
					DisplayText = FString::Printf(TEXT("%s - On Air"), *RacerName);
					TextColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f); // 파란색
				}
				else
				{
					DisplayText = FString::Printf(TEXT("%s - No Signal"), *RacerName);
					TextColor = FLinearColor::White;
				}
			}
			else
			{
				DisplayText = FString::Printf(TEXT("%s - No Signal"), *RacerName);
				TextColor = FLinearColor::White;
			}
			
			RacerTexts[i]->SetText(FText::FromString(DisplayText));
			RacerTexts[i]->SetColorAndOpacity(TextColor);
			RacerTexts[i]->SetVisibility(ESlateVisibility::Visible);
		}
	}
	
	// Feed Component가 이미 설정되어 있으면 초기 업데이트
	if (CCTVFeedComponent)
	{
		UpdateExpandState(); // 내부에서 UpdateAllFeeds 또는 UpdateExpandedFeed 호출
		UpdateClickedOutline();
		
		// CCTVFeedComponent 이벤트 구독
		CCTVFeedComponent->OnFocusIndexChanged.AddDynamic(this, &UCCTVControlWidget::OnFocusIndexChanged);
		CCTVFeedComponent->OnExpandedChanged.AddDynamic(this, &UCCTVControlWidget::OnExpandedChanged);
		CCTVFeedComponent->OnCameraSlotChanged.AddDynamic(this, &UCCTVControlWidget::OnCameraSlotChanged);
	}
	
	// 레이서 HP/연료 및 피드 상태 업데이트 타이머 설정 (0.5초마다 업데이트)
	if (UWorld* World = GetWorld())
	{
		// 초기 업데이트
		UpdateRacerStats();
		
		// 주기적 업데이트 타이머 설정 (HP/연료 + 피드 상태 모두 업데이트)
		World->GetTimerManager().SetTimer(
			RacerStatsUpdateTimerHandle,
			this,
			&UCCTVControlWidget::UpdateRacerStats,
			0.5f, // 0.5초마다 업데이트
			true  // 반복
		);
	}
	
	CCTV_LOG(Log, "NativeConstruct 완료 - Feed Images: %d/%d/%d/%d",
		FeedImage_0 ? 1 : 0,
		FeedImage_1 ? 1 : 0,
		FeedImage_2 ? 1 : 0,
		FeedImage_3 ? 1 : 0);
}

void UCCTVControlWidget::NativeDestruct()
{
	// CCTV UI 비활성화 (SceneCapture 중지, VRAM 절약)
	if (CCTVFeedComponent)
	{
		CCTVFeedComponent->SetCCTVUIActive(false);
		
		// CCTVFeedComponent 이벤트 구독 해제
		CCTVFeedComponent->OnFocusIndexChanged.RemoveAll(this);
		CCTVFeedComponent->OnExpandedChanged.RemoveAll(this);
		CCTVFeedComponent->OnCameraSlotChanged.RemoveAll(this);
	}

	// 호버 애니메이션 타이머 정리
	if (UWorld* World = GetWorld())
	{
		if (HoverAnimationTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(HoverAnimationTimerHandle);
		}
		
		// 레이서 HP/연료 업데이트 타이머 정리
		if (RacerStatsUpdateTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(RacerStatsUpdateTimerHandle);
		}
	}
	
	// Material Instance 정리
	NoiseMaterialInstances.Empty();
	
	Super::NativeDestruct();
}

void UCCTVControlWidget::UpdateAllFeeds()
{
	if (!CCTVFeedComponent)
	{
		return;
	}
	
	// 4개 Feed의 RenderTarget 업데이트 (FeedImages 멤버 배열 사용)
	// 4분할 화면에서는 항상 노이즈 맵만 표시하고 "레이서이름 - On Air"/"레이서이름 - No Signal" 텍스트 표시
	const TArray<UTextBlock*> RacerTexts = { Racer1Text, Racer2Text, Racer3Text, PixelEnemyText };
	const TArray<FString> DefaultNames = { TEXT("Racer1"), TEXT("Racer2"), TEXT("Racer3"), TEXT("Enemy-TEETH") };
	
	for (int32 i = 0; i < 4 && i < FeedImages.Num(); ++i)
	{
		UImage* FeedImage = FeedImages[i];
		if (!FeedImage)
		{
			continue;
		}
		
		FeedImage->SetVisibility(ESlateVisibility::Visible);
		
		// Pawn 유효성 확인 (Enemy나 Racer가 죽었는지 확인)
		FCCTVFeedSlot FeedSlot = CCTVFeedComponent->GetFeedSlot(i);
		bool bIsPawnValid = IsValid(FeedSlot.TargetPawn);
		
		// 레이서인 경우 HP가 0인지 확인
		if (bIsPawnValid && i < 3)
		{
			if (AAbstractRacer* Racer = Cast<AAbstractRacer>(FeedSlot.TargetPawn))
			{
				// AttributeSet이 있으면 HP 체크, 없으면 레이서가 존재하면 유효한 것으로 간주
				if (UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet()))
				{
					if (RacerAS->GetHealth() <= 0.0f)
					{
						bIsPawnValid = false; // HP가 0이면 유효하지 않음
					}
				}
				// AttributeSet이 없어도 레이서가 존재하면 유효한 것으로 간주 (초기화 중일 수 있음)
			}
		}
		
		// RenderTarget 유효성 확인 (카메라/SceneCapture/RT 준비 여부를 모두 포함)
		UTextureRenderTarget2D* RT = CCTVFeedComponent->GetRenderTarget(i);
		const bool bHasValidRT = (RT && RT->SizeX > 0 && RT->SizeY > 0);
		
		// 4분할 화면에서는 항상 노이즈맵만 표시 (확대했을 때만 RenderTarget 표시)
		SetFeedImageNoiseOrBackground(FeedImage, i);
		
		// 텍스트 표시
		if (RacerTexts.IsValidIndex(i) && RacerTexts[i])
		{
			FString DisplayText;
			FLinearColor TextColor;
			
			// 레이서 이름 가져오기 (플레이어 아이디가 있으면 아이디, 없으면 기본 이름)
			FString RacerName;
			if (i < 3)
			{
				RacerName = GetPlayerDisplayName(i);
				if (RacerName.IsEmpty())
				{
					RacerName = DefaultNames[i];
				}
			}
			else
			{
				RacerName = DefaultNames[i];
			}
			
				// 레이서인 경우: Pawn이 유효하면 "ON AIR" 표시 (RenderTarget 유효성과 무관)
				// Enemy인 경우: Pawn이 유효하고 RenderTarget도 유효한 경우만 "ON AIR" 표시
				if (i < 3)
				{
					// 레이서: Pawn이 유효하면 "ON AIR" (체력이 0이 아니면)
					if (bIsPawnValid)
					{
						DisplayText = FString::Printf(TEXT("%s - ON AIR"), *RacerName);
						TextColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f); // 파란색
					}
					else
					{
						// Pawn이 죽었으면 "NO SIGNAL" 표시
						DisplayText = FString::Printf(TEXT("%s - NO SIGNAL"), *RacerName);
						TextColor = FLinearColor::White;
					}
				}
				else
				{
					// Enemy: Pawn이 유효하고 RenderTarget도 유효한 경우만 "ON AIR"
					if (bIsPawnValid && bHasValidRT)
					{
						DisplayText = FString::Printf(TEXT("%s - ON AIR"), *RacerName);
						TextColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f); // 파란색
					}
					else
					{
						DisplayText = FString::Printf(TEXT("%s - NO SIGNAL"), *RacerName);
						TextColor = FLinearColor::White;
					}
				}
			
			RacerTexts[i]->SetText(FText::FromString(DisplayText));
			RacerTexts[i]->SetColorAndOpacity(TextColor);
			RacerTexts[i]->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void UCCTVControlWidget::UpdateFocusIndicator()
{
	// 클릭된 Feed의 아웃라인 업데이트 (호환성을 위해 유지)
	UpdateClickedOutline();
}

// UpdateGridFocus와 UpdateExpandedFocus 함수는 제거됨 (더 이상 사용하지 않음)

void UCCTVControlWidget::UpdateExpandedFeed()
{
	if (!CCTVFeedComponent || !ExpandedContainer)
	{
		return;
	}
	
	// ExpandedFeedImage가 ExpandedContainer의 자식인지 확인하고 추가
	if (ExpandedFeedImage)
	{
		if (!ExpandedFeedImage->GetParent() || ExpandedFeedImage->GetParent() != ExpandedContainer)
		{
			if (ExpandedFeedImage->GetParent())
			{
				ExpandedFeedImage->RemoveFromParent();
			}
			
			if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ExpandedContainer))
			{
				CanvasPanel->AddChild(ExpandedFeedImage);
				
				// ExpandedFeedImage를 전체 화면으로 설정
				if (UCanvasPanelSlot* FeedSlot = Cast<UCanvasPanelSlot>(ExpandedFeedImage->Slot))
				{
					FAnchors Anchors(0.0f, 0.0f, 1.0f, 1.0f);
					FeedSlot->SetAnchors(Anchors);
					FeedSlot->SetAlignment(FVector2D(0.5f, 0.5f));
					FeedSlot->SetPosition(FVector2D::ZeroVector);
					FeedSlot->SetSize(FVector2D::ZeroVector); // Fill
					FeedSlot->SetZOrder(100);
				}
			}
		}
		
		int32 FocusIndex = CCTVFeedComponent->GetFocusIndex();
		
		// Pawn 유효성 확인 (Enemy나 Racer가 죽었는지 확인)
		FCCTVFeedSlot FeedSlot = CCTVFeedComponent->GetFeedSlot(FocusIndex);
		bool bIsPawnValid = IsValid(FeedSlot.TargetPawn);
		
		// 레이서인 경우 HP가 0인지 확인
		if (bIsPawnValid && FocusIndex < 3)
		{
			if (AAbstractRacer* Racer = Cast<AAbstractRacer>(FeedSlot.TargetPawn))
			{
				// AttributeSet이 있으면 HP 체크, 없으면 레이서가 존재하면 유효한 것으로 간주
				if (UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet()))
				{
					if (RacerAS->GetHealth() <= 0.0f)
					{
						bIsPawnValid = false; // HP가 0이면 유효하지 않음
					}
				}
				// AttributeSet이 없어도 레이서가 존재하면 유효한 것으로 간주 (초기화 중일 수 있음)
			}
		}
		
		// RenderTarget 유효성 확인 (카메라/SceneCapture/RT 준비 여부를 모두 포함)
		UTextureRenderTarget2D* RT = CCTVFeedComponent->GetRenderTarget(FocusIndex);
		const bool bHasValidRT = (RT && RT->SizeX > 0 && RT->SizeY > 0);
		
		// 확대 화면에서는 RenderTarget이 유효하면 표시
		if (bIsPawnValid && bHasValidRT)
		{
			// RenderTarget이 유효하면 표시
			SetBrushWithRenderTarget(ExpandedFeedImage, RT);
			ExpandedFeedImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// Pawn이 죽었거나 RenderTarget이 없거나 유효하지 않으면 노이즈 머티리얼 표시
			SetFeedImageNoiseOrBackground(ExpandedFeedImage, FocusIndex);
	}
	
	// 확대 상태에서 Border 표시 (ExpandedContainer 내부에 배치)
		if (OutlineBorder && FocusIndex >= 0 && FocusIndex < 4)
		{
			// ExpandedContainer의 자식이 아니면 추가
			if (!OutlineBorder->GetParent() || OutlineBorder->GetParent() != ExpandedContainer)
			{
				if (OutlineBorder->GetParent())
				{
					OutlineBorder->RemoveFromParent();
				}
				
				if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ExpandedContainer))
				{
					CanvasPanel->AddChild(OutlineBorder);
					
					if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(OutlineBorder->Slot))
					{
						FAnchors Anchors(0.0f, 0.0f, 1.0f, 1.0f);
						CanvasSlot->SetAnchors(Anchors);
						CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
						CanvasSlot->SetPosition(FVector2D::ZeroVector);
						CanvasSlot->SetSize(FVector2D::ZeroVector);
						CanvasSlot->SetZOrder(150);
					}
				}
			}
			
			FSlateBrush BorderBrush;
			BorderBrush.DrawAs = ESlateBrushDrawType::Border;
			BorderBrush.Margin = FMargin(OutlineThickness);
			BorderBrush.TintColor = OutlineColor;
			BorderBrush.ImageSize = FVector2D(1.0f, 1.0f);
			OutlineBorder->SetBrush(BorderBrush);
			OutlineBorder->SetVisibility(ESlateVisibility::Visible);
	}
	
	// 확대 화면 옆에 레이서 닉네임 또는 "Racer1/2/3" 표시
		if (ExpandText)
	{
		FString DisplayName;
		TArray<FString> DefaultNames = { TEXT("Racer1"), TEXT("Racer2"), TEXT("Racer3"), TEXT("Enemy-TEETH") };
		
		// 레이서 슬롯인 경우 (0~2) 플레이어 이름 가져오기
		if (FocusIndex >= 0 && FocusIndex < 3)
		{
			DisplayName = GetPlayerDisplayName(FocusIndex);
			if (DisplayName.IsEmpty())
			{
				// 멀티플레이에서 PlayerState가 아직 복제되지 않았을 수 있음
				// PlayerArray에서 targetIndex로 찾기 시도
				if (UWorld* World = GetWorld())
				{
					ETargetRacer TargetRacer = FeedIndexToTargetRacer(FocusIndex);
					if (ACitRushPlayerState* RacerPS = FindRacerPlayerStateByTargetIndex(World, TargetRacer))
					{
						DisplayName = RacerPS->GetPlayerInfo().playerName;
						if (DisplayName.IsEmpty())
						{
							DisplayName = RacerPS->GetPlayerName();
						}
					}
				}
				
				// 여전히 비어있으면 기본 이름 사용
				if (DisplayName.IsEmpty())
				{
					DisplayName = DefaultNames[FocusIndex];
				}
			}
		}
		else if (FocusIndex == 3)
		{
			DisplayName = DefaultNames[3];
		}
		else
		{
				DisplayName = DefaultNames[0];
		}
		
		// 위에서 이미 선언한 bIsPawnValid와 bHasValidRT 변수 재사용
		FString StatusText;
		FLinearColor TextColor;
		// 레이서인 경우: Pawn이 유효하면 "ON AIR" 표시 (RenderTarget 유효성과 무관)
		// Enemy인 경우: Pawn이 유효하고 RenderTarget도 유효한 경우만 "ON AIR" 표시
		if (FocusIndex < 3)
		{
			// 레이서: Pawn이 유효하면 "ON AIR" (체력이 0이 아니면)
			if (bIsPawnValid)
			{
				StatusText = FString::Printf(TEXT("%s - ON AIR"), *DisplayName);
				TextColor = FLinearColor::White;
			}
			else
			{
				StatusText = FString::Printf(TEXT("%s - NO SIGNAL"), *DisplayName);
				TextColor = FLinearColor::Red;
			}
		}
		else
		{
			// Enemy: Pawn이 유효하고 RenderTarget도 유효한 경우만 "ON AIR"
			if (bIsPawnValid && bHasValidRT)
			{
				StatusText = FString::Printf(TEXT("%s - ON AIR"), *DisplayName);
				TextColor = FLinearColor::White;
			}
			else
			{
				StatusText = FString::Printf(TEXT("%s - NO SIGNAL"), *DisplayName);
				TextColor = FLinearColor::Red;
			}
		}
		
		ExpandText->SetText(FText::FromString(StatusText));
		ExpandText->SetVisibility(ESlateVisibility::Visible);
		ExpandText->SetColorAndOpacity(TextColor);
		
			if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ExpandedContainer))
		{
		if (!ExpandText->GetParent() || ExpandText->GetParent() != ExpandedContainer)
		{
			if (ExpandText->GetParent())
			{
				ExpandText->RemoveFromParent();
			}
				CanvasPanel->AddChild(ExpandText);
		}
		
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ExpandText->Slot))
			{
				// Anchor를 Top-Center로 설정
				FAnchors Anchors(0.5f, 0.0f, 0.5f, 0.0f);
				CanvasSlot->SetAnchors(Anchors);
					CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
				
				FVector2D ContainerSize = CanvasPanel->GetDesiredSize();
				if (ContainerSize.X <= 0.0f || ContainerSize.Y <= 0.0f)
				{
					FGeometry ContainerGeometry = CanvasPanel->GetCachedGeometry();
					ContainerSize = ContainerGeometry.GetLocalSize();
				}
				
				if (ContainerSize.X <= 0.0f || ContainerSize.Y <= 0.0f)
				{
						ContainerSize = FVector2D(1920.0f, 1080.0f);
				}
				
				FVector2D TextSize = ExpandText->GetDesiredSize();
				if (TextSize.X <= 0.0f || TextSize.Y <= 0.0f)
				{
						TextSize = FVector2D(200.0f, 30.0f);
				}
				
				FVector2D Position(
						-TextSize.X * 0.5f,
						20.0f
				);
				CanvasSlot->SetPosition(Position);
					CanvasSlot->SetZOrder(200);
				}
			}
		}
	
	// 확대 상태일 때 포커스된 레이서의 HP/연료 표시
	if (FocusIndex >= 0 && FocusIndex < 3)
	{
		const TArray<UTextBlock*> StatsTexts = { Racer1StatsText, Racer2StatsText, Racer3StatsText };
		if (StatsTexts.IsValidIndex(FocusIndex) && StatsTexts[FocusIndex])
		{
			UTextBlock* ExpandedStatsText = StatsTexts[FocusIndex];
			FCCTVFeedSlot StatsFeedSlot = CCTVFeedComponent->GetFeedSlot(FocusIndex);
			
			// Enemy인지 확인 (Enemy는 HP/연료 표시 안 함)
			if (Cast<AAbstractEnemy>(StatsFeedSlot.TargetPawn))
			{
				ExpandedStatsText->SetVisibility(ESlateVisibility::Collapsed);
			}
			// 레이서 Pawn 확인
			else if (AAbstractRacer* Racer = Cast<AAbstractRacer>(StatsFeedSlot.TargetPawn))
			{
				if (IsValid(Racer))
				{
					// 레이서가 죽었는지 확인 (HP가 0인지 확인)
					bool bIsRacerAlive = true;
					if (UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet()))
					{
						if (RacerAS->GetHealth() <= 0.0f)
						{
							bIsRacerAlive = false;
						}
					}
					
					if (bIsRacerAlive)
					{
						// AttributeSet 가져오기
						UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet());
						if (RacerAS)
						{
							// HP와 연료 가져오기
							float Health = RacerAS->GetHealth();
							float MaxHealth = RacerAS->GetMaxHealth();
							float Fuel = RacerAS->GetFuel();
							float MaxFuel = RacerAS->GetMaxFuel();
							
							// % 계산
							float HealthPercent = (MaxHealth > 0.0f) ? (Health / MaxHealth * 100.0f) : 0.0f;
							float FuelPercent = (MaxFuel > 0.0f) ? (Fuel / MaxFuel * 100.0f) : 0.0f;
							
							// 텍스트 포맷: "HP: 85% | Fuel: 60%"
							FString StatsString = FString::Printf(TEXT("HP: %.0f%% | Fuel: %.0f%%"), HealthPercent, FuelPercent);
							ExpandedStatsText->SetText(FText::FromString(StatsString));
							
							// 색상 설정 (HP가 낮으면 빨간색, 연료가 낮으면 노란색)
							FLinearColor TextColor = FLinearColor::White;
							if (HealthPercent < 30.0f)
							{
								TextColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f); // 빨간색
							}
							else if (FuelPercent < 20.0f)
							{
								TextColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); // 노란색
							}
							
							ExpandedStatsText->SetColorAndOpacity(TextColor);
							
							// ExpandedContainer에 추가하고 위치 설정
							if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ExpandedContainer))
							{
								if (!ExpandedStatsText->GetParent() || ExpandedStatsText->GetParent() != ExpandedContainer)
								{
									if (ExpandedStatsText->GetParent())
									{
										ExpandedStatsText->RemoveFromParent();
									}
									CanvasPanel->AddChild(ExpandedStatsText);
								}
								
								if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ExpandedStatsText->Slot))
								{
									// Anchor를 Top-Right로 설정
									FAnchors Anchors(1.0f, 0.0f, 1.0f, 0.0f);
									CanvasSlot->SetAnchors(Anchors);
									CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
									
									FVector2D ContainerSize = CanvasPanel->GetDesiredSize();
									if (ContainerSize.X <= 0.0f || ContainerSize.Y <= 0.0f)
									{
										FGeometry ContainerGeometry = CanvasPanel->GetCachedGeometry();
										ContainerSize = ContainerGeometry.GetLocalSize();
									}
									
									if (ContainerSize.X <= 0.0f || ContainerSize.Y <= 0.0f)
									{
										ContainerSize = FVector2D(1920.0f, 1080.0f);
									}
									
									FVector2D TextSize = ExpandedStatsText->GetDesiredSize();
									if (TextSize.X <= 0.0f || TextSize.Y <= 0.0f)
									{
										TextSize = FVector2D(200.0f, 30.0f);
									}
									
									// 오른쪽 상단에 배치 (ExpandText 아래)
									FVector2D Position(
										-TextSize.X - 20.0f, // 오른쪽에서 20픽셀 여백
										60.0f // 위에서 60픽셀 (ExpandText 아래)
									);
									CanvasSlot->SetPosition(Position);
									CanvasSlot->SetZOrder(200);
								}
							}
							
							ExpandedStatsText->SetVisibility(ESlateVisibility::Visible);
						}
						else
						{
							ExpandedStatsText->SetVisibility(ESlateVisibility::Collapsed);
						}
					}
					else
					{
						ExpandedStatsText->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
				else
				{
					ExpandedStatsText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			else
			{
				ExpandedStatsText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	else
	{
		// 레이서가 아닌 경우 (Enemy 등) 모든 StatsText 숨김
		const TArray<UTextBlock*> StatsTexts = { Racer1StatsText, Racer2StatsText, Racer3StatsText };
		for (UTextBlock* StatsText : StatsTexts)
		{
			if (StatsText)
			{
				StatsText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	
	// BrainCam 위젯 처리 (Enemy 확대 시에만 표시)
	// 확대 상태이고 FocusIndex가 3(Enemy)일 때만 표시
	if (BrainCamWidget && FocusIndex == 3)
	{
		// ExpandedContainer의 자식이 아니면 추가
		if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ExpandedContainer))
		{
			if (!BrainCamWidget->GetParent() || BrainCamWidget->GetParent() != ExpandedContainer)
			{
				if (BrainCamWidget->GetParent())
				{
					BrainCamWidget->RemoveFromParent();
				}
				CanvasPanel->AddChild(BrainCamWidget);
			}
			
			// BrainCam 위젯을 화면에 배치 (우측 상단) - 확대할 때마다 항상 업데이트
			if (UCanvasPanelSlot* BrainCamSlot = Cast<UCanvasPanelSlot>(BrainCamWidget->Slot))
			{
				// Anchor를 Top-Right로 설정 (항상 업데이트)
				FAnchors Anchors(1.0f, 0.0f, 1.0f, 0.0f);
				BrainCamSlot->SetAnchors(Anchors);
				BrainCamSlot->SetAlignment(FVector2D(1.0f, 0.0f));
				
				// 컨테이너 크기 계산 (항상 최신 Geometry에서 가져오기)
				FVector2D ContainerSize = FVector2D::ZeroVector;
				FGeometry ContainerGeometry = CanvasPanel->GetCachedGeometry();
				if (ContainerGeometry.GetLocalSize().X > 0.0f && ContainerGeometry.GetLocalSize().Y > 0.0f)
				{
					ContainerSize = ContainerGeometry.GetLocalSize();
				}
				else
				{
					// Geometry가 아직 준비되지 않았으면 기본값 사용
					ContainerSize = FVector2D(1920.0f, 1080.0f);
				}
				
				// 위젯 크기 설정 (고정값 사용하여 위치 누적 방지)
				FVector2D WidgetSize = FVector2D(400.0f, 300.0f); // 고정 크기
				BrainCamSlot->SetSize(WidgetSize);
				
				// 위치 설정 (오른쪽 상단, 여백 20픽셀) - 항상 절대 위치로 설정
				FVector2D Position(
					-WidgetSize.X - 20.0f, // 오른쪽에서 20픽셀 여백 (Anchor가 1.0f이므로 음수 오프셋)
					20.0f // 위에서 20픽셀 여백
				);
				BrainCamSlot->SetPosition(Position);
				BrainCamSlot->SetZOrder(300); // ExpandText보다 위에 표시
				
				CCTV_LOG(Log, "BrainCamWidget 위치 설정 완료 - Position: (%.1f, %.1f), Size: (%.1f, %.1f), ContainerSize: (%.1f, %.1f)",
					Position.X, Position.Y, WidgetSize.X, WidgetSize.Y, ContainerSize.X, ContainerSize.Y);
			}
			
			// 확대할 때마다 항상 표시 (이미 추가되어 있어도 Visibility 재설정)
			BrainCamWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else if (BrainCamWidget)
	{
		// 확대 상태가 아니거나 Enemy가 아니면 숨김
		BrainCamWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	}
}

void UCCTVControlWidget::UpdateExpandState()
{
	if (!CCTVFeedComponent)
	{
		return;
	}
	
	bool bExpanded = CCTVFeedComponent->IsExpanded();
	
	// 확대 상태 변경 시에만 UI 업데이트
	if (bExpanded == bPreviousExpanded)
	{
		return;
	}
	
	bPreviousExpanded = bExpanded;
	
	// 4분할 컨테이너 표시/숨김
	if (CCTVContainer)
	{
		CCTVContainer->SetVisibility(bExpanded ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	
	// 확대 컨테이너 표시/숨김
	if (ExpandedContainer)
	{
		ExpandedContainer->SetVisibility(bExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	
	// StatsText 위치 업데이트
	UpdateStatsTextPosition(bExpanded);
	
	// 확대 상태일 때는 확대된 Feed 업데이트 (내부에서 ExpandText 설정)
	if (bExpanded)
	{
		UpdateExpandedFeed();
		// 확대 상태에서도 StatsText 위치 업데이트
		UpdateStatsTextPosition(true);
	}
	else
	{
		// 4분할 상태일 때는 ExpandText와 BrainCam 위젯 숨김
		if (ExpandText)
		{
			ExpandText->SetVisibility(ESlateVisibility::Hidden);
		}
		if (BrainCamWidget)
		{
			BrainCamWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		// 확대 상태에서 ExpandedContainer에 추가된 StatsText 제거 (4분할 화면으로 돌아갈 때)
		const TArray<UTextBlock*> StatsTexts = { Racer1StatsText, Racer2StatsText, Racer3StatsText };
		for (UTextBlock* StatsText : StatsTexts)
		{
			if (StatsText && StatsText->GetParent() == ExpandedContainer)
			{
				StatsText->RemoveFromParent();
			}
		}
		
		UpdateAllFeeds();
		// 4분할 상태에서도 StatsText 위치 업데이트
		UpdateStatsTextPosition(false);
	}
}

void UCCTVControlWidget::OnFocusIndexChanged(int32 OldIndex, int32 NewIndex)
{
	PreviousFocusIndex = NewIndex;
	UpdateClickedOutline();
	
	if (CCTVFeedComponent && CCTVFeedComponent->IsExpanded())
	{
		UpdateExpandedFeed();
	}
}

void UCCTVControlWidget::OnExpandedChanged(bool bExpanded)
{
	// UpdateExpandState()에서 변경 감지가 되도록 bPreviousExpanded를 반대로 설정
	// (UpdateExpandState() 내부에서 bPreviousExpanded를 업데이트하므로 여기서는 반대로 설정)
	bPreviousExpanded = !bExpanded;
	UpdateExpandState();
}

void UCCTVControlWidget::OnCameraSlotChanged(int32 FeedIndex, int32 CameraSlot)
{
	CCTV_LOG(Log, "[OnCameraSlotChanged] FeedIndex: %d, CameraSlot: %d", FeedIndex, CameraSlot);
	
	if (CCTVFeedComponent)
	{
		int32 CurrentFocusIndex = CCTVFeedComponent->GetFocusIndex();
		bool bIsExpanded = CCTVFeedComponent->IsExpanded();
		
		CCTV_LOG(Log, "[OnCameraSlotChanged] 현재 상태 - FocusIndex: %d, IsExpanded: %s", 
			CurrentFocusIndex, bIsExpanded ? TEXT("true") : TEXT("false"));
		
		if (bIsExpanded && FeedIndex == CurrentFocusIndex)
		{
			CCTV_LOG(Log, "[OnCameraSlotChanged] 확대 상태 - UpdateExpandedFeed 호출");
			UpdateExpandedFeed();
		}
		else
		{
			CCTV_LOG(Log, "[OnCameraSlotChanged] 4분할 상태 - UpdateAllFeeds 호출");
			UpdateAllFeeds();
		}
	}
	else
	{
		CCTV_LOG(Warning, "[OnCameraSlotChanged] CCTVFeedComponent가 null입니다");
	}
}

void UCCTVControlWidget::SetBrushWithRenderTarget(UImage* ImageWidget, UTextureRenderTarget2D* RenderTarget)
{
	if (!ImageWidget || !RenderTarget)
	{
		CCTV_LOG(Warning, "SetBrushWithRenderTarget: ImageWidget 또는 RenderTarget이 null입니다");
		return;
	}
	
	// RenderTarget이 제대로 초기화되었는지 확인
	if (RenderTarget->SizeX <= 0 || RenderTarget->SizeY <= 0)
	{
		CCTV_LOG(Error, "RenderTarget 크기가 잘못되었습니다: %dx%d", 
			RenderTarget->SizeX, RenderTarget->SizeY);
		return;
	}
	
	// RenderTarget 리소스 강제 업데이트
	RenderTarget->UpdateResourceImmediate(false);
	
	FSlateBrush Brush = ImageWidget->GetBrush();
	Brush.SetResourceObject(RenderTarget);
	
	// RenderTarget의 실제 크기를 ImageSize로 설정 (비율 유지)
	Brush.ImageSize = FVector2D(RenderTarget->SizeX, RenderTarget->SizeY);
	
	SetBrushImageProperties(Brush);
	ImageWidget->SetBrush(Brush);
	
	// Image 위젯의 색상과 불투명도 명시적으로 설정 (알파 1.0 = 완전 불투명)
	// SCS_FinalColorLDR는 알파를 포함하지 않으므로 UI에서 불투명하게 처리해야 함
	ImageWidget->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	ImageWidget->SetBrushTintColor(FSlateColor(FLinearColor::White));
	
	CCTV_LOG(Verbose, "RenderTarget 설정 완료 - RT: %s, 크기: %dx%d", 
		*RenderTarget->GetName(), RenderTarget->SizeX, RenderTarget->SizeY);
}

void UCCTVControlWidget::SetBrushWithMaterial(UImage* ImageWidget, UMaterialInstanceDynamic* Material)
{
	if (!ImageWidget || !Material)
	{
		return;
	}
	
	FSlateBrush Brush = ImageWidget->GetBrush();
	Brush.SetResourceObject(Material);
	// ImageSize는 설정하지 않음 - 위젯 크기에 맞춰 자동으로 조정됨
	
	SetBrushImageProperties(Brush);
	ImageWidget->SetBrush(Brush);
}

void UCCTVControlWidget::SetBrushImageProperties(FSlateBrush& Brush) const
{
	// CCTV / SceneCapture / Video는 무조건 Image 사용 (Box는 9-slice로 인해 잘림 발생)
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.Tiling = ESlateBrushTileType::NoTile;
	
	// 알파 채널이 없는 RenderTarget도 불투명하게 표시되도록 TintColor 명시적 설정
	// (SCS_FinalColorLDR는 알파를 포함하지 않으므로)
	if (!Brush.TintColor.IsColorSpecified())
	{
		Brush.TintColor = FSlateColor(FLinearColor::White);
	}
}

FReply UCCTVControlWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 왼쪽 마우스 버튼 클릭만 처리
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	if (!CCTVFeedComponent)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	// 확대 상태일 때는 클릭하면 복귀
	if (CCTVFeedComponent->IsExpanded())
	{
		CCTVFeedComponent->ToggleExpand(); // 확대 상태 해제
		ClickedFeedIndex = -1; // 클릭 해제
		UpdateClickedOutline();
		UpdateExpandState();
		return FReply::Handled();
	}
	
	// 4분할 상태일 때는 클릭한 Feed로 확대
	FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	int32 NewClickedFeedIndex = GetFeedIndexFromMousePosition(InGeometry, ScreenPosition);
	
	if (NewClickedFeedIndex >= 0 && NewClickedFeedIndex < 4)
	{
		// 클릭된 Feed 인덱스 저장
		ClickedFeedIndex = NewClickedFeedIndex;
		UpdateClickedOutline();
		
		// 해당 Feed로 포커스 이동 및 확대
		FocusAndExpandFeed(NewClickedFeedIndex);
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCCTVControlWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!CCTVFeedComponent || CCTVFeedComponent->IsExpanded())
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}
	
	// 마우스 위치에서 Feed 인덱스 찾기
	FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	int32 NewHoveredFeedIndex = GetFeedIndexFromMousePosition(InGeometry, ScreenPosition);
	
	// 호버 상태 변경 감지
	if (NewHoveredFeedIndex != HoveredFeedIndex)
	{
		PreviousHoveredFeedIndex = HoveredFeedIndex;
		HoveredFeedIndex = NewHoveredFeedIndex;
		
		// 이전 호버된 FeedImage는 원래 크기로 복원
		if (PreviousHoveredFeedIndex >= 0 && PreviousHoveredFeedIndex < 4)
		{
			if (UImage* PreviousImage = GetFeedImageByIndex(PreviousHoveredFeedIndex))
			{
				PreviousImage->SetRenderScale(FVector2D(1.0f, 1.0f));
			}
		}
		
		// 호버 애니메이션 시작/중지
		if (UWorld* World = GetWorld())
		{
			if (HoverAnimationTimerHandle.IsValid())
			{
				World->GetTimerManager().ClearTimer(HoverAnimationTimerHandle);
			}
			
			if (HoveredFeedIndex >= 0)
			{
				// 새로운 Feed에 호버 시작
				bHoverExpanding = true;
				HoverAnimationProgress = 0.0f;
				World->GetTimerManager().SetTimer(
					HoverAnimationTimerHandle,
					this,
					&UCCTVControlWidget::UpdateHoverAnimation,
					0.016f, // 약 60 FPS
					true
				);
			}
		}
	}
	
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UCCTVControlWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	// 호버 해제
	PreviousHoveredFeedIndex = HoveredFeedIndex;
	HoveredFeedIndex = -1;
	
	// 호버 애니메이션 중지 및 원래 크기로 복원
	if (UWorld* World = GetWorld())
	{
		if (HoverAnimationTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(HoverAnimationTimerHandle);
		}
	}
	
	if (PreviousHoveredFeedIndex >= 0 && PreviousHoveredFeedIndex < 4)
	{
		if (UImage* PreviousImage = GetFeedImageByIndex(PreviousHoveredFeedIndex))
		{
			PreviousImage->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}
	
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UCCTVControlWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Tab 키는 Commander일 때만 CCTV 카메라 전환용으로 처리
	if (InKeyEvent.GetKey() == EKeys::Tab)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (ACitRushPlayerState* PS = PC->GetPlayerState<ACitRushPlayerState>())
			{
				if (PS->GetPlayerRole() == EPlayerRole::Commander)
				{
					if (APawn* Pawn = PC->GetPawn())
					{
						if (AAbstractCommander* Commander = Cast<AAbstractCommander>(Pawn))
						{
							Commander->FindMonitorActor();
							Commander->HandleCCTVSwitchCameraTab();
							return FReply::Handled();
						}
					}
				}
			}
		}
	}
	
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

int32 UCCTVControlWidget::GetFeedIndexFromClickPosition(const FGeometry& InGeometry, const FVector2D& ScreenPosition) const
{
	// 호환성을 위해 유지 (내부적으로 GetFeedIndexFromMousePosition 호출)
	return GetFeedIndexFromMousePosition(InGeometry, ScreenPosition);
}

int32 UCCTVControlWidget::GetFeedIndexFromMousePosition(const FGeometry& InGeometry, const FVector2D& ScreenPosition) const
{
	if (!CCTVContainer)
	{
		return -1;
	}
	
	// Grid Panel과 Canvas Panel 모두 동일한 로직 사용
	FGeometry ContainerGeometry = CCTVContainer->GetCachedGeometry();
	FVector2D ContainerSize = ContainerGeometry.GetLocalSize();
	
	if (ContainerSize.X <= 0.0f || ContainerSize.Y <= 0.0f)
	{
		return -1;
	}
	
	// 컨테이너 내부의 로컬 좌표 계산
	FVector2D ContainerLocalPos = ContainerGeometry.AbsoluteToLocal(ScreenPosition);
	
	// 2x2 Grid이므로 각 셀 크기 계산
	FVector2D CellSize = ContainerSize * 0.5f;
	
	// 클릭된 셀 계산
	int32 Column = FMath::FloorToInt(ContainerLocalPos.X / CellSize.X);
	int32 Row = FMath::FloorToInt(ContainerLocalPos.Y / CellSize.Y);
	
	// 범위 체크
	if (Column >= 0 && Column < 2 && Row >= 0 && Row < 2)
	{
		return Row * 2 + Column;
	}
	
	return -1;
}

void UCCTVControlWidget::FocusAndExpandFeed(int32 FeedIndex)
{
	if (!CCTVFeedComponent)
	{
		return;
	}
	
	// 포커스 설정
	CCTVFeedComponent->SetFocusIndex(FeedIndex);
	
	// 확대 상태가 아니면 확대
	if (!CCTVFeedComponent->IsExpanded())
	{
		CCTVFeedComponent->ToggleExpand();
	}
	
	// UI 업데이트 (UpdateExpandState가 내부에서 UpdateExpandedFeed를 호출)
	UpdateExpandState();
	UpdateClickedOutline();
	
	CCTV_LOG(Log, "Feed %d로 포커스 이동 및 확대", FeedIndex);
}

UMaterialInstanceDynamic* UCCTVControlWidget::GetOrCreateNoiseMaterialInstance(int32 Index)
{
	if (!NoiseMaterial)
	{
		return nullptr;
	}
	
	// 배열 크기 확보
	if (NoiseMaterialInstances.Num() <= Index)
	{
		NoiseMaterialInstances.SetNum(Index + 1);
	}
	
	// 이미 있으면 반환
	if (NoiseMaterialInstances[Index])
	{
		return NoiseMaterialInstances[Index];
	}
	
	// 생성
	NoiseMaterialInstances[Index] = UMaterialInstanceDynamic::Create(NoiseMaterial, this);
	if (NoiseMaterialInstances[Index])
	{
		NoiseMaterialInstances[Index]->SetScalarParameterValue(TEXT("TimeOffset"), FMath::RandRange(0.0f, 1000.0f));
	}
	
	return NoiseMaterialInstances[Index];
}

void UCCTVControlWidget::SetFeedImageNoiseOrBackground(UImage* FeedImage, int32 FeedIndex)
{
	if (!FeedImage)
	{
		return;
	}
	
	FeedImage->SetVisibility(ESlateVisibility::Visible);
	
	UMaterialInstanceDynamic* NoiseMI = GetOrCreateNoiseMaterialInstance(FeedIndex);
	if (NoiseMI)
	{
		SetBrushWithMaterial(FeedImage, NoiseMI);
		FeedImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		// Material이 없으면 어두운 회색 배경
		FeedImage->SetColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
		FSlateBrush Brush = FeedImage->GetBrush();
		Brush.SetResourceObject(nullptr);
		SetBrushImageProperties(Brush);
		FeedImage->SetBrush(Brush);
	}
}

FString UCCTVControlWidget::GetPlayerDisplayName(int32 FeedIndex) const
{
	// 기본 이름 배열
	const TArray<FString> DefaultNames = { TEXT("Racer1"), TEXT("Racer2"), TEXT("Racer3"), TEXT("Enemy-TEETH") };
	
	if (!CCTVFeedComponent)
	{
		// FeedIndex가 유효하면 기본 이름 반환
		if (FeedIndex >= 0 && FeedIndex < 4)
		{
			return DefaultNames[FeedIndex];
		}
		return FString();
	}
	
	// FeedIndex 유효성 검사 (0~3: 레이서 3명 + Enemy)
	if (FeedIndex < 0 || FeedIndex >= 4)
	{
		return FString();
	}
	
	FCCTVFeedSlot FeedSlot = CCTVFeedComponent->GetFeedSlot(FeedIndex);
	if (!FeedSlot.TargetPawn)
	{
		// Pawn이 없으면 기본 이름 반환
		return DefaultNames[FeedIndex];
	}
	
	ACitRushPlayerState* PlayerState = FeedSlot.TargetPawn->GetPlayerState<ACitRushPlayerState>();
	if (!PlayerState && FeedIndex < 3)
	{
		// 멀티플레이에서 PlayerState가 아직 복제되지 않았을 수 있음
		// PlayerArray에서 targetIndex로 찾기 시도
		if (UWorld* World = GetWorld())
		{
			ETargetRacer TargetRacer = FeedIndexToTargetRacer(FeedIndex);
			PlayerState = FindRacerPlayerStateByTargetIndex(World, TargetRacer);
		}
		
		if (!PlayerState)
		{
			// PlayerState를 찾을 수 없으면 기본 이름 반환
			return DefaultNames[FeedIndex];
		}
	}
	
	// PlayerInfo에서 이름 가져오기 시도 (멀티플레이에서 더 안정적)
	FString PlayerName = PlayerState->GetPlayerInfo().playerName;
	if (!PlayerName.IsEmpty())
	{
		return PlayerName;
	}
	
	// 플레이어 이름이 있으면 사용
	PlayerName = PlayerState->GetPlayerName();
	if (!PlayerName.IsEmpty())
	{
		return PlayerName;
	}
	
	// 플레이어 이름이 없으면 UniqueId에서 스팀 아이디 추출
	FUniqueNetIdRepl UniqueId = PlayerState->GetUniqueId();
	if (UniqueId.IsValid())
	{
		TSharedPtr<const FUniqueNetId> UniqueNetId = UniqueId.GetUniqueNetId();
		if (UniqueNetId.IsValid())
		{
			FString UniqueIdString = UniqueNetId->ToString();
			if (!UniqueIdString.IsEmpty())
			{
				return UniqueIdString;
			}
		}
	}
	
	// 모든 방법으로 이름을 찾을 수 없으면 기본 이름 반환
	return DefaultNames[FeedIndex];
}

ETargetRacer UCCTVControlWidget::FeedIndexToTargetRacer(int32 FeedIndex)
{
	if (FeedIndex == 0) return ETargetRacer::Racer1;
	else if (FeedIndex == 1) return ETargetRacer::Racer2;
	else if (FeedIndex == 2) return ETargetRacer::Racer3;
	return ETargetRacer::None;
}

ACitRushPlayerState* UCCTVControlWidget::FindRacerPlayerStateByTargetIndex(UWorld* World, ETargetRacer TargetRacer)
{
	if (!World || TargetRacer == ETargetRacer::None)
	{
		return nullptr;
	}
	
	if (AGameStateBase* GameStateBase = World->GetGameState())
	{
		// PlayerArray를 순회하며 targetIndex가 일치하는 레이서 찾기
		for (APlayerState* PS : GameStateBase->PlayerArray)
		{
			if (ACitRushPlayerState* RacerPS = Cast<ACitRushPlayerState>(PS))
			{
				if (IsValid(RacerPS) && 
					RacerPS->GetPlayerRole() == EPlayerRole::Racer &&
					RacerPS->GetPlayerInfo().targetIndex == TargetRacer)
				{
					return RacerPS;
				}
			}
		}
	}
	
	return nullptr;
}

void UCCTVControlWidget::UpdateHoverAnimation()
{
	if (CCTVFeedComponent && CCTVFeedComponent->IsExpanded())
	{
		// 확대 상태일 때는 호버 애니메이션 중지
		if (UWorld* World = GetWorld())
		{
			if (HoverAnimationTimerHandle.IsValid())
			{
				World->GetTimerManager().ClearTimer(HoverAnimationTimerHandle);
			}
		}
		return;
	}
	
	// 호버 애니메이션 업데이트
	if (HoveredFeedIndex >= 0 && HoveredFeedIndex < 4)
	{
		// 확대 애니메이션
		if (bHoverExpanding)
		{
			HoverAnimationProgress += 0.016f * HoverAnimationSpeed; // Timer 간격 사용
			if (HoverAnimationProgress >= 1.0f)
			{
				HoverAnimationProgress = 1.0f;
				bHoverExpanding = false; // 확대 완료 후 축소 시작
			}
		}
		else
		{
			// 축소 애니메이션
			HoverAnimationProgress -= 0.016f * HoverAnimationSpeed; // Timer 간격 사용
			if (HoverAnimationProgress <= 0.0f)
			{
				HoverAnimationProgress = 0.0f;
				bHoverExpanding = true; // 축소 완료 후 확대 시작
			}
		}
		
		// Scale 계산 (1.0 ~ HoverScaleMax 사이를 부드럽게 이동)
		float CurrentScale = FMath::Lerp(1.0f, HoverScaleMax, HoverAnimationProgress);
		FVector2D ScaleVector(CurrentScale, CurrentScale);
		
		// 호버된 FeedImage에 Scale 적용
		if (UImage* HoveredImage = GetFeedImageByIndex(HoveredFeedIndex))
		{
			HoveredImage->SetRenderScale(ScaleVector);
		}
	}
	else
	{
		// 호버 해제 시 애니메이션 중지
		if (UWorld* World = GetWorld())
		{
			if (HoverAnimationTimerHandle.IsValid())
			{
				World->GetTimerManager().ClearTimer(HoverAnimationTimerHandle);
			}
		}
	}
}

UImage* UCCTVControlWidget::GetFeedImageByIndex(int32 Index) const
{
	if (Index >= 0 && Index < FeedImages.Num())
	{
		return FeedImages[Index];
	}
	return nullptr;
}


void UCCTVControlWidget::UpdateClickedOutline()
{
	if (!CCTVFeedComponent)
	{
		return;
	}
	
	bool bExpanded = CCTVFeedComponent->IsExpanded();
	
	if (!OutlineBorder)
	{
		return;
	}
	
	if (bExpanded)
	{
		// 확대 상태일 때는 UpdateExpandedFeed에서 처리되므로 여기서는 아무것도 하지 않음
		return;
	}
	
	// 4분할 상태일 때는 클릭된 Feed의 아웃라인만 표시
	if (ClickedFeedIndex >= 0 && ClickedFeedIndex < 4)
	{
		// Border가 CCTVContainer의 자식이 아니면 추가
		if (CCTVContainer && (!OutlineBorder->GetParent() || OutlineBorder->GetParent() != CCTVContainer))
		{
			if (OutlineBorder->GetParent())
			{
				OutlineBorder->RemoveFromParent();
			}
			
			if (UGridPanel* GridPanel = Cast<UGridPanel>(CCTVContainer))
			{
				GridPanel->AddChild(OutlineBorder);
				
				if (UGridSlot* GridSlot = Cast<UGridSlot>(OutlineBorder->Slot))
				{
					// 해당 FeedImage와 동일한 Grid 위치
					int32 Row = ClickedFeedIndex / 2;
					int32 Column = ClickedFeedIndex % 2;
					GridSlot->SetRow(Row);
					GridSlot->SetColumn(Column);
					GridSlot->SetRowSpan(1);
					GridSlot->SetColumnSpan(1);
				}
			}
			else if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(CCTVContainer))
			{
				CanvasPanel->AddChild(OutlineBorder);
				
				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(OutlineBorder->Slot))
				{
					// FeedImage와 동일한 위치 및 크기 설정
					if (UImage* FeedImage = GetFeedImageByIndex(ClickedFeedIndex))
					{
						if (UCanvasPanelSlot* FeedSlot = Cast<UCanvasPanelSlot>(FeedImage->Slot))
						{
							CanvasSlot->SetAnchors(FeedSlot->GetAnchors());
							CanvasSlot->SetAlignment(FeedSlot->GetAlignment());
							CanvasSlot->SetPosition(FeedSlot->GetPosition());
							CanvasSlot->SetSize(FeedSlot->GetSize());
							CanvasSlot->SetZOrder(100); // FeedImage 위에 표시
						}
					}
				}
			}
		}
		
		// 클릭된 Feed의 아웃라인 표시
		FSlateBrush BorderBrush;
		BorderBrush.DrawAs = ESlateBrushDrawType::Border;
		BorderBrush.Margin = FMargin(OutlineThickness);
		BorderBrush.TintColor = OutlineColor;
		BorderBrush.ImageSize = FVector2D(1.0f, 1.0f); // 최소 크기
		OutlineBorder->SetBrush(BorderBrush);
		OutlineBorder->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		// 클릭되지 않았으면 숨김
		OutlineBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCCTVControlWidget::UpdateRacerStats()
{
	if (!CCTVFeedComponent)
	{
		return;
	}

	// 피드 상태도 함께 업데이트 (레이서가 죽었을 때 "NO SIGNAL" 표시)
	bool bExpanded = CCTVFeedComponent->IsExpanded();
	if (bExpanded)
	{
		UpdateExpandedFeed();
	}
	else
	{
		UpdateAllFeeds();
	}

	// 각 레이서의 HP/연료 표시 (인덱스 0~2만 레이서)
	const TArray<UTextBlock*> StatsTexts = { Racer1StatsText, Racer2StatsText, Racer3StatsText };
	
	// 확대 상태일 때는 포커스된 레이서의 HP/연료만 표시
	if (bExpanded)
	{
		int32 FocusIndex = CCTVFeedComponent->GetFocusIndex();
		
		// 모든 StatsText 숨김
		for (UTextBlock* StatsText : StatsTexts)
		{
			if (StatsText)
			{
				StatsText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		
		// 포커스된 레이서(0~2)의 HP/연료만 표시 (Enemy는 표시 안 함)
		if (FocusIndex >= 0 && FocusIndex < 3 && StatsTexts.IsValidIndex(FocusIndex) && StatsTexts[FocusIndex])
		{
			UTextBlock* StatsText = StatsTexts[FocusIndex];
			FCCTVFeedSlot FeedSlot = CCTVFeedComponent->GetFeedSlot(FocusIndex);
			
			// Enemy인지 확인 (Enemy는 HP/연료 표시 안 함)
			if (Cast<AAbstractEnemy>(FeedSlot.TargetPawn))
			{
				StatsText->SetVisibility(ESlateVisibility::Collapsed);
			}
			// 레이서 Pawn 확인
			else if (AAbstractRacer* Racer = Cast<AAbstractRacer>(FeedSlot.TargetPawn))
			{
				if (IsValid(Racer))
				{
					// AttributeSet 가져오기 및 레이서가 죽었는지 확인 (HP가 0인지 확인)
					UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet());
					if (RacerAS && RacerAS->GetHealth() > 0.0f)
					{
						// HP와 연료 가져오기
						float Health = RacerAS->GetHealth();
						float MaxHealth = RacerAS->GetMaxHealth();
						float Fuel = RacerAS->GetFuel();
						float MaxFuel = RacerAS->GetMaxFuel();

						// % 계산
						float HealthPercent = (MaxHealth > 0.0f) ? (Health / MaxHealth * 100.0f) : 0.0f;
						float FuelPercent = (MaxFuel > 0.0f) ? (Fuel / MaxFuel * 100.0f) : 0.0f;

						// 텍스트 포맷: "HP: 85% | Fuel: 60%"
						FString StatsString = FString::Printf(TEXT("HP: %.0f%% | Fuel: %.0f%%"), HealthPercent, FuelPercent);
						StatsText->SetText(FText::FromString(StatsString));

						// 색상 설정 (HP가 낮으면 빨간색, 연료가 낮으면 노란색)
						FLinearColor TextColor = FLinearColor::White;
						if (HealthPercent < 30.0f)
						{
							TextColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
						}
						else if (FuelPercent < 20.0f)
						{
							TextColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
						}
						
						StatsText->SetColorAndOpacity(TextColor);
						StatsText->SetVisibility(ESlateVisibility::Visible);
					}
					else
					{
						StatsText->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
				else
				{
					StatsText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			else
			{
				StatsText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	// 4분할 화면일 때는 각 분할 화면에 해당하는 레이서의 HP/연료만 표시
	else
	{
		for (int32 i = 0; i < 3; ++i)
		{
			if (!StatsTexts.IsValidIndex(i) || !StatsTexts[i])
			{
				continue;
			}

			UTextBlock* StatsText = StatsTexts[i];
			FCCTVFeedSlot FeedSlot = CCTVFeedComponent->GetFeedSlot(i);
			
			// Enemy인지 확인 (Enemy는 HP/연료 표시 안 함)
			if (Cast<AAbstractEnemy>(FeedSlot.TargetPawn))
			{
				StatsText->SetVisibility(ESlateVisibility::Collapsed);
				continue;
			}
			
			// 레이서 Pawn 확인
			AAbstractRacer* Racer = Cast<AAbstractRacer>(FeedSlot.TargetPawn);
			if (!Racer || !IsValid(Racer))
			{
				StatsText->SetVisibility(ESlateVisibility::Collapsed);
				continue;
			}
			
			// AttributeSet 가져오기 및 레이서가 죽었는지 확인 (HP가 0인지 확인)
			UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet());
			if (!RacerAS || RacerAS->GetHealth() <= 0.0f)
			{
				// 레이서가 죽었거나 AttributeSet이 없으면 숨김
				StatsText->SetVisibility(ESlateVisibility::Collapsed);
				continue;
			}

			// HP와 연료 가져오기
			float Health = RacerAS->GetHealth();
			float MaxHealth = RacerAS->GetMaxHealth();
			float Fuel = RacerAS->GetFuel();
			float MaxFuel = RacerAS->GetMaxFuel();

			// % 계산
			float HealthPercent = (MaxHealth > 0.0f) ? (Health / MaxHealth * 100.0f) : 0.0f;
			float FuelPercent = (MaxFuel > 0.0f) ? (Fuel / MaxFuel * 100.0f) : 0.0f;

			// 텍스트 포맷: "HP: 85% | Fuel: 60%"
			FString StatsString = FString::Printf(TEXT("HP: %.0f%% | Fuel: %.0f%%"), HealthPercent, FuelPercent);
			StatsText->SetText(FText::FromString(StatsString));

			// 색상 설정 (HP가 낮으면 빨간색, 연료가 낮으면 노란색)
			FLinearColor TextColor = FLinearColor::White;
			if (HealthPercent < 30.0f)
			{
				TextColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
			}
			else if (FuelPercent < 20.0f)
			{
				TextColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
			}
			
			StatsText->SetColorAndOpacity(TextColor);
			StatsText->SetVisibility(ESlateVisibility::Visible);
		}
	}
	
	// 확대/축소 상태에 따라 위치 업데이트
	UpdateStatsTextPosition(bExpanded);
}

void UCCTVControlWidget::UpdateStatsTextPosition(bool bExpanded)
{
	const TArray<UTextBlock*> StatsTexts = { Racer1StatsText, Racer2StatsText, Racer3StatsText };
	UPanelWidget* TargetContainer = bExpanded ? ExpandedContainer : CCTVContainer;
	
	if (!TargetContainer)
	{
		return;
	}
	
	for (int32 i = 0; i < 3; ++i)
	{
		if (!StatsTexts.IsValidIndex(i) || !StatsTexts[i])
		{
			continue;
		}
		
		UTextBlock* StatsText = StatsTexts[i];
		
		// 현재 부모가 TargetContainer가 아니면 이동
		if (!StatsText->GetParent() || StatsText->GetParent() != TargetContainer)
		{
			if (StatsText->GetParent())
			{
				StatsText->RemoveFromParent();
			}
			
			if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(TargetContainer))
			{
				CanvasPanel->AddChild(StatsText);
				
				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(StatsText->Slot))
				{
					if (bExpanded)
					{
						// 확대 상태: 우측 상단에 세로로 배치
						FAnchors Anchors(1.0f, 0.0f, 1.0f, 0.0f);
						CanvasSlot->SetAnchors(Anchors);
						CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
						
						FVector2D ContainerSize = CanvasPanel->GetDesiredSize();
						if (ContainerSize.X <= 0.0f || ContainerSize.Y <= 0.0f)
						{
							FGeometry ContainerGeometry = CanvasPanel->GetCachedGeometry();
							ContainerSize = ContainerGeometry.GetLocalSize();
						}
						if (ContainerSize.X <= 0.0f || ContainerSize.Y <= 0.0f)
						{
							ContainerSize = FVector2D(1920.0f, 1080.0f);
						}
						
						FVector2D TextSize = StatsText->GetDesiredSize();
						if (TextSize.X <= 0.0f || TextSize.Y <= 0.0f)
						{
							TextSize = FVector2D(200.0f, 30.0f);
						}
						
						FVector2D Position(
							-TextSize.X - 20.0f,
							20.0f + (i * (TextSize.Y + 10.0f))
						);
						CanvasSlot->SetPosition(Position);
						CanvasSlot->SetZOrder(250);
					}
					else
					{
						// 4분할 상태: 각 FeedImage의 우측 상단에 배치
						if (UImage* FeedImage = GetFeedImageByIndex(i))
						{
							if (UCanvasPanelSlot* FeedSlot = Cast<UCanvasPanelSlot>(FeedImage->Slot))
							{
								FAnchors FeedAnchors = FeedSlot->GetAnchors();
								FVector2D FeedPosition = FeedSlot->GetPosition();
								FVector2D FeedSize = FeedSlot->GetSize();
								
								FAnchors Anchors(FeedAnchors.Minimum.X, FeedAnchors.Minimum.Y, FeedAnchors.Maximum.X, FeedAnchors.Maximum.Y);
								CanvasSlot->SetAnchors(Anchors);
								CanvasSlot->SetAlignment(FVector2D(1.0f, 0.0f));
								
								FVector2D TextSize = StatsText->GetDesiredSize();
								if (TextSize.X <= 0.0f || TextSize.Y <= 0.0f)
								{
									TextSize = FVector2D(150.0f, 25.0f);
								}
								
								FVector2D Position(
									FeedPosition.X + FeedSize.X - TextSize.X - 10.0f,
									FeedPosition.Y + 10.0f
								);
								CanvasSlot->SetPosition(Position);
								CanvasSlot->SetZOrder(50);
							}
						}
					}
				}
			}
			else if (UGridPanel* GridPanel = Cast<UGridPanel>(TargetContainer))
			{
				GridPanel->AddChild(StatsText);
				
				if (UGridSlot* GridSlot = Cast<UGridSlot>(StatsText->Slot))
				{
					int32 Row = i / 2;
					int32 Column = i % 2;
					GridSlot->SetRow(Row);
					GridSlot->SetColumn(Column);
					GridSlot->SetRowSpan(1);
					GridSlot->SetColumnSpan(1);
					GridSlot->SetHorizontalAlignment(HAlign_Right);
					GridSlot->SetVerticalAlignment(VAlign_Top);
					GridSlot->SetPadding(FMargin(0.0f, 10.0f, 10.0f, 0.0f));
				}
			}
		}
	}
}
