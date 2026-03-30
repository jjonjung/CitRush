// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Car/ItemSlotWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Item/ItemData.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Front 아이템 슬롯 이미지 중앙 정렬 설정
	if (FrontItemSlotImage)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(FrontItemSlotImage->Slot))
		{
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // 중앙 정렬
		}
	}
	
	// Back 아이템 슬롯 이미지 중앙 정렬 설정
	if (BackItemSlotImage)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BackItemSlotImage->Slot))
		{
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // 중앙 정렬
		}
	}
}

UTexture2D* UItemSlotWidget::GetItemIcon(UItemData* Item) const
{
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemSlotWidget] GetItemIcon: Item이 null입니다"));
		return nullptr;
	}

	// racerIcon 우선 사용 (레이서 전용 아이콘)
	if (Item->racerIcon)
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemSlotWidget] GetItemIcon: 레이서 전용 아이콘 사용 - Item: %s (ID: %s), RacerIcon: %s"),
			*Item->GetName(), *Item->ID.ToString(), *Item->racerIcon->GetName());
		return Item->racerIcon;
	}

	// racerIcon이 없으면 기본 icon 사용 (Fallback)
	if (Item->icon)
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemSlotWidget] GetItemIcon: 기본 아이콘 사용 - Item: %s (ID: %s), Icon: %s"),
			*Item->GetName(), *Item->ID.ToString(), *Item->icon->GetName());
		return Item->icon;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ItemSlotWidget] GetItemIcon: 아이템 아이콘이 null입니다 - Item: %s (ID: %s)"),
		*Item->GetName(), *Item->ID.ToString());
	return nullptr;
}

void UItemSlotWidget::UpdateItemSlots(UItemData* FrontItem, UItemData* BackItem)
{
	// 전방 아이템 슬롯 업데이트
	// FrontItemSlotImage는 배경으로 항상 표시
	/*if (FrontItemSlotImage)
	{
		FrontItemSlotImage->SetVisibility(ESlateVisibility::Visible);
	}*/
	
		// FrontItemIconImage는 아이템이 있을 때만 표시
	if (FrontItemIconImage)
	{
		if (FrontItemSlotImage)
		{
			FrontItemSlotImage->SetVisibility(ESlateVisibility::Visible);
		}
		
		// 새 아이템이 추가되었는지 확인 (애니메이션 트리거)
		bool bNewItemAdded = (FrontItem && FrontItem != PreviousFrontItem && PreviousFrontItem == nullptr);
		
		// 아이템 아이콘 가져오기 (Data Asset의 icon 사용)
		UTexture2D* ItemIcon = GetItemIcon(FrontItem);

		// [수정] 아이템이 추가되면 특정 부스트 이미지로 교체 (요청 사항)
		if (FrontItem)
		{
			// 요청된 특정 부스트 텍스처 로드
			UTexture2D* BoostTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/CITRUSH/Commender/Tex/Boost.Boost"));
			
			if (BoostTexture)
			{
				ItemIcon = BoostTexture;
				UE_LOG(LogTemp, Log, TEXT("[ItemSlotWidget] Boost Texture 로드 성공: %s"), *BoostTexture->GetPathName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[ItemSlotWidget] Boost Texture 로드 실패! 경로 확인 필요: /Game/CITRUSH/Commender/Tex/Boost"));
			}
		}
		
		if (FrontItem && ItemIcon)
		{
			// 브러시를 직접 설정하는 대신 SetBrushFromTexture를 사용하여 기본 크기 매칭
			FrontItemIconImage->SetBrushFromTexture(ItemIcon, true);
			FrontItemIconImage->SetVisibility(ESlateVisibility::Visible);
			
			// 새 아이템이 추가되었으면 애니메이션 시작
			if (bNewItemAdded)
			{
				StartIconAnimation(FrontItemIconImage, true);
			}
			
			UE_LOG(LogTemp, Log, TEXT("[ItemSlotWidget] FrontItemIcon 업데이트 완료 - Item: %s, Texture: %s"), 
				*FrontItem->GetName(), *ItemIcon->GetName());
		}
		else
		{
			FrontItemIconImage->SetBrushFromTexture(nullptr);
			FrontItemIconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		PreviousFrontItem = FrontItem;
	}

	// 후방 아이템 슬롯 업데이트
	// BackItemSlotImage는 배경으로 항상 표시
	if (BackItemSlotImage)
	{
		BackItemSlotImage->SetVisibility(ESlateVisibility::Visible);
	}
	
	// BackItemIconImage는 아이템이 있을 때만 표시
	if (BackItemIconImage)
	{
		// 새 아이템이 추가되었는지 확인 (애니메이션 트리거)
		bool bNewItemAdded = (BackItem && BackItem != PreviousBackItem && PreviousBackItem == nullptr);
		
		// 아이템 아이콘 가져오기 (Data Asset의 icon 사용)
		UTexture2D* ItemIcon = GetItemIcon(BackItem);
		
		if (BackItem && ItemIcon)
		{
			// Canvas Panel Slot 또는 Overlay Slot에서 위젯 크기 가져오기
			FVector2D ImageSize(120.0f, 120.0f); // 기본값
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BackItemIconImage->Slot))
			{
				FVector2D SlotSize = CanvasSlot->GetSize();
				if (SlotSize.X > 0.0f && SlotSize.Y > 0.0f)
				{
					ImageSize = SlotSize;
				}
			}
			else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(BackItemIconImage->Slot))
			{
				// Overlay Slot의 경우 부모 위젯 크기 사용
				if (BackItemSlotImage)
				{
					FVector2D SlotSize = BackItemSlotImage->GetDesiredSize();
					if (SlotSize.X > 0.0f && SlotSize.Y > 0.0f)
					{
						ImageSize = SlotSize;
					}
				}
			}
			
			// Brush 설정: 위젯 크기에 맞게 이미지 스케일링
			FSlateBrush Brush;
			Brush.SetResourceObject(ItemIcon);
			Brush.ImageSize = ImageSize; // 위젯 크기에 맞게 설정
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.Tiling = ESlateBrushTileType::NoTile;
			Brush.Mirroring = ESlateBrushMirrorType::NoMirror;
			Brush.ImageType = ESlateBrushImageType::FullColor;
			
			BackItemIconImage->SetBrush(Brush);
			BackItemIconImage->SetVisibility(ESlateVisibility::Visible);
			
			// 새 아이템이 추가되었으면 애니메이션 시작
			if (bNewItemAdded)
			{
				StartIconAnimation(BackItemIconImage, false);
			}
			
			UE_LOG(LogTemp, Log, TEXT("[ItemSlotWidget] BackItemIcon 업데이트 - Item: %s (ID: %s), Icon: %s, Size: %.1f x %.1f"), 
				*BackItem->GetName(), *BackItem->ID.ToString(), ItemIcon ? *ItemIcon->GetName() : TEXT("NULL"), ImageSize.X, ImageSize.Y);
		}
		else
		{
			BackItemIconImage->SetBrushFromTexture(nullptr);
			BackItemIconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		PreviousBackItem = BackItem;
	}
}

void UItemSlotWidget::StartIconAnimation(UImage* IconImage, bool bIsFront)
{
	if (!IconImage || !GetWorld())
	{
		return;
	}
	
	// 기존 애니메이션 중지
	if (bIsFront)
	{
		if (FrontIconAnimationTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(FrontIconAnimationTimerHandle);
		}
		bIsFrontIconAnimating = true;
		FrontIconAnimationProgress = 0.0f;
		
		// 초기 Scale 설정 (1.0)
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(IconImage->Slot))
		{
			CanvasSlot->SetSize(FVector2D(120.0f, 120.0f));
		}
	}
	else
	{
		if (BackIconAnimationTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(BackIconAnimationTimerHandle);
		}
		bIsBackIconAnimating = true;
		BackIconAnimationProgress = 0.0f;
		
		// 초기 Scale 설정 (1.0)
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(IconImage->Slot))
		{
			CanvasSlot->SetSize(FVector2D(120.0f, 120.0f));
		}
	}
	
	// 애니메이션 업데이트 시작 (0.016초마다 업데이트, 약 60fps)
	FTimerDelegate TimerDelegate;
	if (bIsFront)
	{
		TimerDelegate.BindUFunction(this, FName("UpdateFrontIconAnimation"));
		GetWorld()->GetTimerManager().SetTimer(
			FrontIconAnimationTimerHandle,
			TimerDelegate,
			0.016f, // 약 60fps
			true // 반복
		);
	}
	else
	{
		TimerDelegate.BindUFunction(this, FName("UpdateBackIconAnimation"));
		GetWorld()->GetTimerManager().SetTimer(
			BackIconAnimationTimerHandle,
			TimerDelegate,
			0.016f, // 약 60fps
			true // 반복
		);
	}
}

void UItemSlotWidget::UpdateFrontIconAnimation()
{
	if (!FrontItemIconImage || !bIsFrontIconAnimating)
	{
		return;
	}
	
	// 애니메이션 진행도 업데이트 (0.0 ~ 1.0)
	FrontIconAnimationProgress += 0.016f / 0.3f; // 0.3초 동안 애니메이션
	
	if (FrontIconAnimationProgress >= 1.0f)
	{
		FrontIconAnimationProgress = 1.0f;
		bIsFrontIconAnimating = false;
		
		// 타이머 정리
		if (GetWorld() && FrontIconAnimationTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(FrontIconAnimationTimerHandle);
		}
	}
	
	// Scale 계산: 1.0 -> 1.2 -> 1.0 (Ease Out)
	float Scale = 1.0f;
	if (FrontIconAnimationProgress < 0.5f)
	{
		// 0.0 ~ 0.5: 1.0 -> 1.2 (커지는 부분)
		float t = FrontIconAnimationProgress * 2.0f; // 0.0 ~ 1.0
		Scale = FMath::Lerp(1.0f, 1.2f, t);
	}
	else
	{
		// 0.5 ~ 1.0: 1.2 -> 1.0 (작아지는 부분)
		float t = (FrontIconAnimationProgress - 0.5f) * 2.0f; // 0.0 ~ 1.0
		Scale = FMath::Lerp(1.2f, 1.0f, t);
	}
	
	// Canvas Panel Slot 또는 Overlay Slot의 Size를 변경하여 Scale 효과 구현
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(FrontItemIconImage->Slot))
	{
		FVector2D BaseSize(120.0f, 120.0f);
		CanvasSlot->SetSize(BaseSize * Scale);
	}
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(FrontItemIconImage->Slot))
	{
		// Overlay Slot의 경우 Render Transform 사용
		FrontItemIconImage->SetRenderScale(FVector2D(Scale, Scale));
	}
}

void UItemSlotWidget::UpdateBackIconAnimation()
{
	if (!BackItemIconImage || !bIsBackIconAnimating)
	{
		return;
	}
	
	// 애니메이션 진행도 업데이트 (0.0 ~ 1.0)
	BackIconAnimationProgress += 0.016f / 0.3f; // 0.3초 동안 애니메이션
	
	if (BackIconAnimationProgress >= 1.0f)
	{
		BackIconAnimationProgress = 1.0f;
		bIsBackIconAnimating = false;
		
		// 타이머 정리
		if (GetWorld() && BackIconAnimationTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(BackIconAnimationTimerHandle);
		}
	}
	
	// Scale 계산: 1.0 -> 1.2 -> 1.0 (Ease Out)
	float Scale = 1.0f;
	if (BackIconAnimationProgress < 0.5f)
	{
		// 0.0 ~ 0.5: 1.0 -> 1.2 (커지는 부분)
		float t = BackIconAnimationProgress * 2.0f; // 0.0 ~ 1.0
		Scale = FMath::Lerp(1.0f, 1.2f, t);
	}
	else
	{
		// 0.5 ~ 1.0: 1.2 -> 1.0 (작아지는 부분)
		float t = (BackIconAnimationProgress - 0.5f) * 2.0f; // 0.0 ~ 1.0
		Scale = FMath::Lerp(1.2f, 1.0f, t);
	}
	
	// Canvas Panel Slot 또는 Overlay Slot의 Size를 변경하여 Scale 효과 구현
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BackItemIconImage->Slot))
	{
		FVector2D BaseSize(120.0f, 120.0f);
		CanvasSlot->SetSize(BaseSize * Scale);
	}
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(BackItemIconImage->Slot))
	{
		// Overlay Slot의 경우 Render Transform 사용
		BackItemIconImage->SetRenderScale(FVector2D(Scale, Scale));
	}
}

