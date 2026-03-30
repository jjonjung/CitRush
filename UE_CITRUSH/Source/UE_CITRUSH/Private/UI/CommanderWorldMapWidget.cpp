#include "UI/CommanderWorldMapWidget.h"
#include "UI/CommenderHUDWidget.h"
#include "Player/Stats/MapBoundsActor.h"
#include "Player/Controller/CitRushPlayerController.h"
#include "Player/CommenderCharacter.h"
#include "GameFlow/CitRushGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/Widget.h"
#include "Widgets/SWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Player/Stats/PingMarkerActor.h"
#include "UI/PingRadialMenuWidget.h"
#include "Engine/Texture2D.h"
#include "Slate/SlateBrushAsset.h"
#include "Player/Components/MinimapIconComponent.h"
#include "UI/RealtimeMapIcon.h"
#include "TimerManager.h"
#include "Player/CitRushPlayerState.h"

UCommanderWorldMapWidget::UCommanderWorldMapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DefaultPingType(ECommanderPingType::Objective)
	, SelectedPingType(ECommanderPingType::Objective)
	, MapWidgetSize(800.f, 600.f)
{
}

void UCommanderWorldMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	if (bAutoCalculateSize)
	{
		CalculateWidgetSizeAndOffset();
		if (bApplyAutoLayout)
		{
			ApplyCalculatedLayout();
		}
	}

	ApplyMapImageRotation();

	// CommanderAnchor 찾기 시작
	FindCommanderAnchor();

	// GameState 핑 업데이트 이벤트 구독
	if (UWorld* World = GetWorld())
	{
		if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
		{
			GameState->OnPingUpdated.AddUObject(this, &UCommanderWorldMapWidget::OnPingUpdated);
		}

		CollectIconComponents();
		LastComponentCollectionTime = World->GetTimeSeconds();

		World->GetTimerManager().SetTimer(
			RealtimeIconsTimerHandle,
			this,
			&UCommanderWorldMapWidget::UpdateRealtimeIcons,
			RealtimeIconsUpdateInterval,
			true
		);
	}

	if (PingRadialMenuClass && !PingRadialMenu)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			PingRadialMenu = CreateWidget<UPingRadialMenuWidget>(PC, PingRadialMenuClass);
			if (PingRadialMenu)
			{
				PingRadialMenu->SetVisibility(ESlateVisibility::Collapsed);
				if (UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget()))
				{
					RootCanvas->AddChild(PingRadialMenu);
				}
				else
				{
					PingRadialMenu->AddToViewport(200);
				}
			}
		}
	}
}

FVector UCommanderWorldMapWidget::GetCommanderWorldLocation() const
{
	if (const ACitRushPlayerController* PC = GetCitRushPlayerController())
	{
		if (const APawn* Pawn = PC->GetPawn())
		{
			return Pawn->GetActorLocation();
		}
	}

	return FVector::ZeroVector;
}

FVector UCommanderWorldMapWidget::GetCommanderStartLocation() const
{
	if (UWorld* World = GetWorld())
	{
		// GameMode를 통해 CommanderStart 찾기
		if (AGameModeBase* GameMode = World->GetAuthGameMode())
		{
			// PlayerController 가져오기
			APlayerController* PC = GetOwningPlayer();
			if (!PC)
			{
				PC = UGameplayStatics::GetPlayerController(World, 0);
			}

			if (PC)
			{
				// GameMode의 FindPlayerStart 사용
				AActor* CommanderStart = GameMode->FindPlayerStart(PC, TEXT("Commander"));
				if (CommanderStart)
				{
					return CommanderStart->GetActorLocation();
				}
			}
		}

		// Fallback: 모든 PlayerStart 중 "Commander" 태그가 있는 것 찾기
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			if (APlayerStart* PlayerStart = *It)
			{
				if (PlayerStart->PlayerStartTag == FName(TEXT("Commander")))
				{
					return PlayerStart->GetActorLocation();
				}
			}
		}
	}

	return FVector::ZeroVector;
}

void UCommanderWorldMapWidget::DebugShowWorldPingActorsOnMap()
{
	UWorld* World = GetWorld();
	if (!World || !MapImageWidget)
	{
		return;
	}

	AMapBoundsActor* MapBounds = FindMapBounds();
	if (!MapBounds)
	{
		return;
	}

	TSharedPtr<SWidget> MapImageSlateWidget = MapImageWidget->GetCachedWidget();
	if (!MapImageSlateWidget.IsValid())
	{
		return;
	}

	const FGeometry MapImageGeometry = MapImageSlateWidget->GetCachedGeometry();
	const FVector2D MapLocalSize = MapImageGeometry.GetLocalSize();

	if (MapLocalSize.X <= 0.f || MapLocalSize.Y <= 0.f)
	{
		return;
	}

	const FGeometry RootGeometry = GetCachedGeometry();

	TSet<APingMarkerActor*> CurrentPingActors;
	for (TActorIterator<APingMarkerActor> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			CurrentPingActors.Add(*It);
		}
	}

	TArray<APingMarkerActor*> ActorsToRemove;
	for (auto& Pair : DebugPingMarkers)
	{
		APingMarkerActor* Actor = Pair.Key;
		if (!IsValid(Actor) || !CurrentPingActors.Contains(Actor))
		{
			if (UImage* MarkerWidget = Pair.Value.Get())
			{
				MarkerWidget->RemoveFromParent();
			}
			ActorsToRemove.Add(Actor);
		}
	}

	for (APingMarkerActor* Actor : ActorsToRemove)
	{
		DebugPingMarkers.Remove(Actor);
	}

	for (APingMarkerActor* PingActor : CurrentPingActors)
	{
		if (!IsValid(PingActor))
		{
			continue;
		}

		const FVector WorldLoc = PingActor->GetActorLocation();
		const FVector2D WorldXY(WorldLoc.X, WorldLoc.Y);
		const FVector2D UV = MapBounds->WorldXYToUV(WorldXY);

		// UV 범위 체크
		if (UV.X < 0.f || UV.X > 1.f || UV.Y < 0.f || UV.Y > 1.f)
		{
			// 범위 밖이면 기존 마커 숨기기
			if (UImage* ExistingMarker = DebugPingMarkers.FindRef(PingActor).Get())
			{
				ExistingMarker->SetVisibility(ESlateVisibility::Hidden);
			}
			continue;
		}

		// 디버그 핑 마커는 회전 보정 없이 원본 UV 사용 (실제 레벨 위치 정확히 표시)
		const FVector2D ImageLocalPos(
			UV.X * MapLocalSize.X,
			UV.Y * MapLocalSize.Y
		);

		const FVector2D ScreenPos = MapImageGeometry.LocalToAbsolute(ImageLocalPos);
		const FVector2D LocalPos = RootGeometry.AbsoluteToLocal(ScreenPos);

		if (UImage* ExistingMarker = DebugPingMarkers.FindRef(PingActor).Get())
		{
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ExistingMarker->Slot))
			{
				CanvasSlot->SetPosition(LocalPos - PingMarkerSize * 0.5f);
				ExistingMarker->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else if (WidgetTree && GetRootWidget())
		{
			if (UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget()))
			{
				if (UImage* NewMarker = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
				{
					if (PingMarkerImage)
					{
						NewMarker->SetBrush(PingMarkerImage->GetBrush());
					}

					if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(NewMarker))
					{
						CanvasSlot->SetPosition(LocalPos - PingMarkerSize * 0.5f);
						CanvasSlot->SetSize(PingMarkerSize);
						CanvasSlot->SetZOrder(999);
						NewMarker->SetVisibility(ESlateVisibility::Visible);
						DebugPingMarkers.Add(PingActor, NewMarker);
					}
				}
			}
		}
	}
}

void UCommanderWorldMapWidget::ClearDebugPingMarkers()
{
	for (auto& Pair : DebugPingMarkers)
	{
		if (UImage* MarkerWidget = Pair.Value.Get())
		{
			MarkerWidget->RemoveFromParent();
		}
	}
	DebugPingMarkers.Empty();
}

TSharedRef<SWidget> UCommanderWorldMapWidget::RebuildWidget()
{
	TSharedRef<SWidget> Widget = Super::RebuildWidget();
	
	// 위젯이 재구성된 후 회전 적용
	ApplyMapImageRotation();
	
	return Widget;
}

FReply UCommanderWorldMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OpenRadialMenu(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		HandleMapClick(InGeometry, InMouseEvent);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCommanderWorldMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bRadialActive)
	{
		if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
		{
			UpdateRadialMenu(InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()));
		}
		else
		{
			CloseRadialMenu();
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UCommanderWorldMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bRadialActive)
	{
		CloseRadialMenu();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UCommanderWorldMapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::F)
	{
		if (OwningHUD)
		{
			OwningHUD->CloseMapUI();
			return FReply::Handled();
		}

		if (ACommenderCharacter* CommanderChar = GetCommanderCharacter())
		{
			CommanderChar->CloseMap();
		}

		RemoveFromParent();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCommanderWorldMapWidget::HandleMapClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector WorldLocation = ConvertClickToWorldLocation(InGeometry, InMouseEvent.GetScreenSpacePosition());
	if (WorldLocation == FVector::ZeroVector)
	{
		return;
	}

	ACommenderCharacter* CommanderChar = GetCommanderCharacter();
	if (!CommanderChar)
	{
		return;
	}

	float RemainingCooldown = 0.f;
	if (CommanderChar->IsPingOnGlobalCooldown(RemainingCooldown))
	{
		CommanderChar->ClientNotifyPingCooldown(RemainingCooldown);
		return;
	}

	if (MapImageWidget)
	{
		TSharedPtr<SWidget> MapImageSlateWidget = MapImageWidget->GetCachedWidget();
		if (MapImageSlateWidget.IsValid())
		{
			const FGeometry MapImageGeometry = MapImageSlateWidget->GetCachedGeometry();
			const FVector2D MapLocalSize = MapImageGeometry.GetLocalSize();
			
			if (MapLocalSize.X > 0.f && MapLocalSize.Y > 0.f)
			{
				const FVector2D ImageLocalPos = MapImageGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
				FVector2D RotatedUV = FVector2D(
					FMath::Clamp(ImageLocalPos.X / MapLocalSize.X, 0.f, 1.f),
					FMath::Clamp(ImageLocalPos.Y / MapLocalSize.Y, 0.f, 1.f)
				);
				
				FVector2D UV = RotateUVFromRotatedGeometry(RotatedUV);
				if (!CheckMapAlphaAtUV(UV, 0.5f))
				{
					return;
				}
			}
		}
	}

	FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	ShowPingMarkerAtPosition(LocalPosition, WorldLocation);
	CommanderChar->ServerPlacePing(WorldLocation, SelectedPingType);
}

bool UCommanderWorldMapWidget::GetMapImageScreenRect(const FGeometry& InGeometry, FVector2D& OutMin, FVector2D& OutMax) const
{
	if (!MapImageWidget)
	{
		return false;
	}

	TSharedPtr<SWidget> MapImageSlateWidget = MapImageWidget->GetCachedWidget();
	if (!MapImageSlateWidget.IsValid())
	{
		return false;
	}

	const FGeometry MapImageGeometry = MapImageSlateWidget->GetCachedGeometry();
	OutMin = FVector2D(MapImageGeometry.AbsolutePosition);
	OutMax = OutMin + FVector2D(MapImageGeometry.GetLocalSize());

	return true;
}

FVector UCommanderWorldMapWidget::ConvertClickToWorldLocation(const FGeometry& InGeometry, const FVector2D& ScreenPosition) const
{
	if (MapImageWidget)
	{
		TSharedPtr<SWidget> MapImageSlateWidget = MapImageWidget->GetCachedWidget();
		if (MapImageSlateWidget.IsValid())
		{
			const FGeometry MapImageGeometry = MapImageSlateWidget->GetCachedGeometry();
			const FVector2D MapLocalSize = MapImageGeometry.GetLocalSize();

			if (MapLocalSize.X > 0.f && MapLocalSize.Y > 0.f)
			{
				// 회전된 위젯에서 AbsoluteToLocal을 사용하면 회전된 좌표계의 로컬 좌표가 반환됨
				// 회전을 고려하여 원본 좌표계로 변환해야 함
				const FVector2D RotatedImageLocalPos = MapImageGeometry.AbsoluteToLocal(ScreenPosition);
				
				// 회전된 로컬 좌표가 범위를 벗어나는지 체크 (회전된 좌표계 기준)
				if (RotatedImageLocalPos.X < 0.f || RotatedImageLocalPos.X > MapLocalSize.X ||
					RotatedImageLocalPos.Y < 0.f || RotatedImageLocalPos.Y > MapLocalSize.Y)
				{
					return FVector::ZeroVector;
				}
				
				// 회전된 로컬 좌표를 UV로 변환 (회전된 좌표계 기준)
				FVector2D RotatedUV(
					RotatedImageLocalPos.X / MapLocalSize.X,
					RotatedImageLocalPos.Y / MapLocalSize.Y
				);

				// 회전된 UV를 원본 UV로 역변환 (회전 보정)
				// MapImageRotation이 -90도이면, 역회전은 +90도
				FVector2D OriginalUV = RotateUVFromRotatedGeometry(RotatedUV);
				
				return ConvertUVToWorldLocation(OriginalUV);
			}
		}
	}

	FVector2D MapImageRectMin, MapImageRectMax;
	if (!GetMapImageScreenRect(InGeometry, MapImageRectMin, MapImageRectMax))
	{
		FVector2D LocalPosition = InGeometry.AbsoluteToLocal(ScreenPosition);
		FVector2D ImageLocalPos = LocalPosition - MapImageOffset;
		
		// 회전된 Geometry의 UV로 변환 (MapImage 중심 기준)
		FVector2D RotatedUV = FVector2D(
			FMath::Clamp(ImageLocalPos.X / MapImageSize.X, 0.f, 1.f),
			FMath::Clamp(ImageLocalPos.Y / MapImageSize.Y, 0.f, 1.f)
		);
		
		// MapImage 중심(0.5, 0.5)을 기준으로 역회전하여 원본 UV 좌표로 변환
		FVector2D OriginalUV = RotateUVFromRotatedGeometry(RotatedUV);
		return ConvertUVToWorldLocation(OriginalUV);
	}

	FVector2D RectSize = MapImageRectMax - MapImageRectMin;
	if (RectSize.X <= 0.f || RectSize.Y <= 0.f)
	{
		return FVector::ZeroVector;
	}

	// 회전된 Geometry의 UV로 변환 (MapImage 중심 기준)
	FVector2D RotatedUV = FVector2D(
		FMath::Clamp((ScreenPosition.X - MapImageRectMin.X) / RectSize.X, 0.f, 1.f),
		FMath::Clamp((ScreenPosition.Y - MapImageRectMin.Y) / RectSize.Y, 0.f, 1.f)
	);

	// MapImage 중심(0.5, 0.5)을 기준으로 역회전하여 원본 UV 좌표로 변환
	FVector2D OriginalUV = RotateUVFromRotatedGeometry(RotatedUV);
	return ConvertUVToWorldLocation(OriginalUV);
}

FVector UCommanderWorldMapWidget::ConvertUVToWorldLocation(const FVector2D& UV) const
{
	AMapBoundsActor* MapBounds = FindMapBounds();
	if (!MapBounds)
	{
		return FVector::ZeroVector;
	}

	FVector2D WorldXY = MapBounds->UVToWorldXY(UV);

	FVector TraceStart = FVector(WorldXY.X, WorldXY.Y, 50000.f);
	FVector TraceEnd = FVector(WorldXY.X, WorldXY.Y, -50000.f);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		return FVector::ZeroVector;
	}

	FVector FinalLocation = FVector(WorldXY.X, WorldXY.Y, 0.f);
	if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		FinalLocation.Z = HitResult.Location.Z;
	}

	return FinalLocation;
}

AMapBoundsActor* UCommanderWorldMapWidget::FindMapBounds() const
{
	if (UWorld* World = GetWorld())
	{
		if (ACitRushGameState* GS = World->GetGameState<ACitRushGameState>())
		{
			if (AMapBoundsActor* Cached = GS->GetMapBounds())
			{
				return Cached;
			}
		}

		return Cast<AMapBoundsActor>(UGameplayStatics::GetActorOfClass(World, AMapBoundsActor::StaticClass()));
	}

	return nullptr;
}

void UCommanderWorldMapWidget::FindCommanderAnchor()
{
	if (!GetWorld())
	{
		return;
	}

	// CommanderAnchor Tag를 가진 Actor 찾기
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("CommanderAnchor"), FoundActors);

	if (FoundActors.Num() > 0 && IsValid(FoundActors[0]))
	{
		CachedCommanderAnchor = FoundActors[0];
		UE_LOG(LogTemp, Log, TEXT("[CommanderWorldMapWidget] CommanderAnchor 찾음: %s"), 
			*FoundActors[0]->GetName());
		
		// 찾았으면 즉시 업데이트
		UpdateCommanderMarker();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommanderWorldMapWidget] CommanderAnchor를 찾을 수 없습니다. 0.2초 후 재시도..."));
		
		// 0.2초 후 재시도
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				CommanderMarkerTimerHandle,
				this,
				&UCommanderWorldMapWidget::FindCommanderAnchor,
				0.2f,
				false
			);
		}
	}
}

void UCommanderWorldMapWidget::UpdateCommanderMarker()
{
	if (!CommanderMarkerImage)
	{
		return;
	}

	// CommanderAnchor가 없으면 업데이트 중단
	if (!CachedCommanderAnchor.IsValid())
	{
		CommanderMarkerImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	AMapBoundsActor* MapBounds = FindMapBounds();
	if (!MapBounds || !MapImageWidget)
	{
		CommanderMarkerImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	// MapImage 위젯의 실제 Geometry 가져오기 (UpdateRealtimeIcons와 동일)
	TSharedPtr<SWidget> MapImageSlateWidget = MapImageWidget->GetCachedWidget();
	if (!MapImageSlateWidget.IsValid())
	{
		CommanderMarkerImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const FGeometry MapImageGeometry = MapImageSlateWidget->GetCachedGeometry();
	const FVector2D MapLocalSize = MapImageGeometry.GetLocalSize();

	if (MapLocalSize.X <= 0.f || MapLocalSize.Y <= 0.f)
	{
		CommanderMarkerImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const FGeometry RootGeometry = GetCachedGeometry();

	// CommanderAnchor의 위치 사용 (Racer/Pacman과 동일한 방식)
	const FVector WorldLoc = CachedCommanderAnchor->GetActorLocation();
	const FVector2D WorldXY(WorldLoc.X, WorldLoc.Y);
	const FVector2D UV = MapBounds->WorldXYToUV(WorldXY);

	// UV 범위 체크 (UpdateRealtimeIcons와 동일)
	if (UV.X < 0.f || UV.X > 1.f || UV.Y < 0.f || UV.Y > 1.f)
	{
		CommanderMarkerImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	// Racer/Pacman과 완전히 동일한 좌표 변환
	FVector2D RotatedUV = RotateUVToRotatedGeometry(UV);
	const FVector2D ImageLocalPos(
		RotatedUV.X * MapLocalSize.X,
		RotatedUV.Y * MapLocalSize.Y
	);

	const FVector2D ScreenPos = MapImageGeometry.LocalToAbsolute(ImageLocalPos);
	const FVector2D LocalPos = RootGeometry.AbsoluteToLocal(ScreenPos);

	// CanvasPanelSlot에 직접 위치 설정 (Racer/Pacman과 동일한 부모 사용)
	if (UCanvasPanelSlot* CmdSlot = Cast<UCanvasPanelSlot>(CommanderMarkerImage->Slot))
	{
		// Racer/Pacman과 동일하게 Alignment를 (0,0)으로 설정
		CmdSlot->SetAlignment(FVector2D(0.f, 0.f));
		
		// 아이콘 크기의 절반만큼 오프셋을 빼서 중심점을 맞춤 (UpdatePosition과 동일한 로직)
		FVector2D IconSize = CmdSlot->GetSize();
		if (IconSize.IsNearlyZero())
		{
			IconSize = FVector2D(32.f, 32.f);
			CmdSlot->SetSize(IconSize);
		}
		
		FVector2D Offset = IconSize * 0.5f;
		FVector2D FinalPosition = LocalPos - Offset;
		CmdSlot->SetPosition(FinalPosition);
		
		CommanderMarkerImage->SetVisibility(ESlateVisibility::Visible);

		// 디버그 로그 (유효성 체크)
		FBox2D Bounds = MapBounds->GetXYBounds();
		FVector2D MapCenter = (Bounds.Min + Bounds.Max) * 0.5f;
		FVector2D MapExtent = (Bounds.Max - Bounds.Min) * 0.5f;
		UE_LOG(LogTemp, Verbose, TEXT("[CommanderWorldMapWidget] CommanderMarker 업데이트: WorldPos=(%.1f, %.1f, %.1f), MapBounds Center=(%.1f, %.1f), Extent=(%.1f, %.1f), MapPixel=(%.1f, %.1f)"),
			WorldLoc.X, WorldLoc.Y, WorldLoc.Z,
			MapCenter.X, MapCenter.Y,
			MapExtent.X, MapExtent.Y,
			FinalPosition.X, FinalPosition.Y);
	}
	else
	{
		CommanderMarkerImage->SetVisibility(ESlateVisibility::Hidden);
		UE_LOG(LogTemp, Warning, TEXT("[CommanderWorldMapWidget] CommanderMarkerImage의 CanvasPanelSlot을 찾을 수 없습니다!"));
	}
}

ACitRushPlayerController* UCommanderWorldMapWidget::GetCitRushPlayerController() const
{
	if (APlayerController* OwningPC = GetOwningPlayer())
	{
		if (ACitRushPlayerController* CitRushPC = Cast<ACitRushPlayerController>(OwningPC))
		{
			return CitRushPC;
		}
	}

	if (const ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (APlayerController* PC = LP->GetPlayerController(GetWorld()))
		{
			if (ACitRushPlayerController* CitRushPC = Cast<ACitRushPlayerController>(PC))
			{
				return CitRushPC;
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			return Cast<ACitRushPlayerController>(PC);
		}
	}

	return nullptr;
}

ACommenderCharacter* UCommanderWorldMapWidget::GetCommanderCharacter() const
{
	// 1) OwningPlayer의 Pawn 확인
	if (APlayerController* OwningPC = GetOwningPlayer())
	{
		if (APawn* Pawn = OwningPC->GetPawn())
		{
			if (ACommenderCharacter* CommanderChar = Cast<ACommenderCharacter>(Pawn))
			{
				return CommanderChar;
			}
		}
	}

	// 2) LocalPlayer → PlayerController → Pawn
	if (const ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (APlayerController* PC = LP->GetPlayerController(GetWorld()))
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (ACommenderCharacter* CommanderChar = Cast<ACommenderCharacter>(Pawn))
				{
					return CommanderChar;
				}
			}
		}
	}

	// 3) 최종 Fallback: 첫 번째 로컬 PlayerController의 Pawn
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (ACommenderCharacter* CommanderChar = Cast<ACommenderCharacter>(Pawn))
				{
					return CommanderChar;
				}
			}
		}
	}

	return nullptr;
}

bool UCommanderWorldMapWidget::CheckMapAlphaAtUV(const FVector2D& UV, float AlphaThreshold) const
{
	UTexture2D* Texture = AlphaMapTexture.Get();
	
	if (!Texture && MapImageWidget)
	{
		const FSlateBrush& Brush = MapImageWidget->GetBrush();
		if (UTexture2D* DirectTexture = Cast<UTexture2D>(Brush.GetResourceObject()))
		{
			Texture = DirectTexture;
		}
		else if (Brush.GetResourceObject())
		{
			return true;
		}
	}

	if (!Texture || !Texture->Source.IsValid())
	{
		return true;
	}

	const int32 TextureWidth = Texture->Source.GetSizeX();
	const int32 TextureHeight = Texture->Source.GetSizeY();
	
	if (TextureWidth <= 0 || TextureHeight <= 0)
	{
		return true;
	}

	const int32 PixelX = FMath::Clamp(FMath::FloorToInt(UV.X * TextureWidth), 0, TextureWidth - 1);
	const int32 PixelY = FMath::Clamp(FMath::FloorToInt(UV.Y * TextureHeight), 0, TextureHeight - 1);

	const uint8* MipData = Texture->Source.LockMip(0);
	if (!MipData)
	{
		return true;
	}

	const int32 BytesPerPixel = Texture->Source.GetBytesPerPixel();
	const int32 RowPitch = TextureWidth * BytesPerPixel;
	const int32 PixelOffset = (PixelY * RowPitch) + (PixelX * BytesPerPixel);
	
	FColor PixelColor;
	if (BytesPerPixel == 4)
	{
		PixelColor.B = MipData[PixelOffset + 0];
		PixelColor.G = MipData[PixelOffset + 1];
		PixelColor.R = MipData[PixelOffset + 2];
		PixelColor.A = MipData[PixelOffset + 3];
	}
	else if (BytesPerPixel == 1)
	{
		const uint8 GrayValue = MipData[PixelOffset];
		PixelColor.R = PixelColor.G = PixelColor.B = GrayValue;
		PixelColor.A = 255;
	}
	else
	{
		Texture->Source.UnlockMip(0);
		return true;
	}
	
	Texture->Source.UnlockMip(0);
	
	const float Brightness = (PixelColor.R + PixelColor.G + PixelColor.B) / 3.0f;
	return (Brightness / 255.0f) >= AlphaThreshold;
}

void UCommanderWorldMapWidget::ShowPingMarkerAtPosition(const FVector2D& LocalPosition, const FVector& WorldLocation)
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		if (PingMarkerImage)
		{
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PingMarkerImage->Slot))
			{
				CanvasSlot->SetPosition(LocalPosition - PingMarkerSize * 0.5f);
				CanvasSlot->SetSize(PingMarkerSize);
				PingMarkerImage->SetVisibility(ESlateVisibility::Visible);
			}
		}
		return;
	}

	if (UImage* NewMarker = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
	{
		if (PingRadialMenu)
		{
			const FSlateBrush RadialBrush = PingRadialMenu->GetIconBrushForType(SelectedPingType);
			if (RadialBrush.GetResourceObject() != nullptr)
			{
				NewMarker->SetBrush(RadialBrush);
			}
		}

		if (!NewMarker->GetBrush().GetResourceObject() && PingMarkerImage)
		{
			NewMarker->SetBrush(PingMarkerImage->GetBrush());
		}

		if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(NewMarker))
		{
			CanvasSlot->SetPosition(LocalPosition - PingMarkerSize * 0.5f);
			CanvasSlot->SetSize(PingMarkerSize);
			CanvasSlot->SetZOrder(999);
			NewMarker->SetVisibility(ESlateVisibility::Visible);

			// 월드 위치가 제공된 경우 마커 추적
			if (WorldLocation != FVector::ZeroVector)
			{
				MapPingMarkers.Add(WorldLocation, NewMarker);
			}
		}
	}
}

void UCommanderWorldMapWidget::SetSelectedPingType(ECommanderPingType NewType)
{
	SelectedPingType = NewType;
}

void UCommanderWorldMapWidget::OpenRadialMenu(const FVector2D& ScreenPos)
{
	if (!PingRadialMenu)
	{
		return;
	}
	bRadialActive = true;
	RadialMenuClickPosition = ScreenPos;

	const FGeometry Geometry = GetCachedGeometry();
	PingRadialMenu->ShowAtScreenPosition(Geometry.AbsoluteToLocal(ScreenPos));
}

void UCommanderWorldMapWidget::UpdateRadialMenu(const FVector2D& ScreenPos)
{
	if (!bRadialActive || !PingRadialMenu)
	{
		return;
	}
	PingRadialMenu->UpdateSelectionByScreenPosition(ScreenPos);
}

void UCommanderWorldMapWidget::CloseRadialMenu()
{
	if (!bRadialActive || !PingRadialMenu)
	{
		return;
	}
	
	ECommanderPingType SelectedType = PingRadialMenu->GetCurrentSelectedType();
	bRadialActive = false;
	PingRadialMenu->HideMenu();
	
	int32 SelectedIndex = PingRadialMenu->HighlightIndex;
	if (SelectedIndex != INDEX_NONE && PingRadialMenu->PingTypes.IsValidIndex(SelectedIndex))
	{
		FGeometry WidgetGeometry = GetCachedGeometry();
		FVector WorldLocation = ConvertClickToWorldLocation(WidgetGeometry, RadialMenuClickPosition);
		
		if (WorldLocation != FVector::ZeroVector)
		{
			if (ACommenderCharacter* CommanderChar = GetCommanderCharacter())
			{
				float RemainingCooldown = 0.f;
				if (CommanderChar->IsPingOnGlobalCooldown(RemainingCooldown))
				{
					CommanderChar->ClientNotifyPingCooldown(RemainingCooldown);
					return;
				}

				CommanderChar->ServerPlacePing(WorldLocation, SelectedType);
				ShowPingMarkerAtPosition(WidgetGeometry.AbsoluteToLocal(RadialMenuClickPosition), WorldLocation);
			}
		}
	}
	
	SelectedPingType = SelectedType;
}
void UCommanderWorldMapWidget::CalculateWidgetSizeAndOffset()
{
	FVector2D ViewportSize = FVector2D::ZeroVector;
	
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize2D;
		GEngine->GameViewport->GetViewportSize(ViewportSize2D);
		ViewportSize = ViewportSize2D;
	}
	
	if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f)
	{
		ViewportSize = FVector2D(1920.f, 1080.f);
	}

	if (MapImageSize.X <= 0.f || MapImageSize.Y <= 0.f)
	{
		return;
	}

	const float ImageAspectRatio = MapImageSize.X / MapImageSize.Y;
	const float AvailableAspectRatio = ViewportSize.X / ViewportSize.Y;

	FVector2D CalculatedWidgetSize;
	if (AvailableAspectRatio > ImageAspectRatio)
	{
		CalculatedWidgetSize.Y = ViewportSize.Y;
		CalculatedWidgetSize.X = ViewportSize.Y * ImageAspectRatio;
	}
	else
	{
		CalculatedWidgetSize.X = ViewportSize.X;
		CalculatedWidgetSize.Y = ViewportSize.X / ImageAspectRatio;
	}

	MapWidgetSize = CalculatedWidgetSize;
	MapImageOffset.X = (ViewportSize.X - CalculatedWidgetSize.X) * 0.5f;
	MapImageOffset.Y = (ViewportSize.Y - CalculatedWidgetSize.Y) * 0.5f;
}

void UCommanderWorldMapWidget::ApplyCalculatedLayout()
{
	SetDesiredSizeInViewport(MapWidgetSize);
	SetPositionInViewport(MapImageOffset);
	SetAlignmentInViewport(FVector2D(0.f, 0.f));
}

void UCommanderWorldMapWidget::NativeDestruct()
{
	// GameState 핑 업데이트 이벤트 구독 해제
	if (UWorld* World = GetWorld())
	{
		if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
		{
			GameState->OnPingUpdated.RemoveAll(this);
		}

		World->GetTimerManager().ClearTimer(RealtimeIconsTimerHandle);
	}

	// 맵 핑 마커 제거
	RemoveMapPingMarkers();

	CleanupRealtimeIcons();
	Super::NativeDestruct();
}

void UCommanderWorldMapWidget::CollectIconComponents()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommanderWorldMapWidget] CollectIconComponents: World가 없습니다!"));
		return;
	}

	CachedIconComponents.Empty();

	int32 FoundCount = 0;
	int32 ValidCount = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (AActor* Actor = *It; IsValid(Actor))
		{
			if (UMinimapIconComponent* IconComp = Actor->FindComponentByClass<UMinimapIconComponent>())
			{
				FoundCount++;
				
				// 멀티플레이에서 PlayerState가 나중에 복제될 수 있으므로
				// PlayerState가 설정되어 있으면 아이콘을 다시 업데이트
				if (APawn* OwnerPawn = Cast<APawn>(Actor))
				{
					ACitRushPlayerState* CitRushPS = OwnerPawn->GetPlayerState<ACitRushPlayerState>();
					
					// PlayerState가 없으면 GameState의 PlayerArray에서 찾기 시도
					if (!CitRushPS && World)
					{
						if (AGameStateBase* GameStateBase = World->GetGameState())
						{
							for (APlayerState* PS : GameStateBase->PlayerArray)
							{
								if (ACitRushPlayerState* FoundPS = Cast<ACitRushPlayerState>(PS))
								{
									if (FoundPS->GetPawn() == OwnerPawn)
									{
										CitRushPS = FoundPS;
										break;
									}
								}
							}
						}
					}
					
					if (CitRushPS)
					{
						// PlayerState가 있으면 아이콘 업데이트 시도 (아이콘 ID 재설정)
						IconComp->ApplyIconIdFromPlayerState();
					}
				}
				
				// bShowOnMap 확인 (PlayerState 업데이트 후)
				if (IconComp->bShowOnMap)
				{
					CachedIconComponents.Add(IconComp);
					ValidCount++;
					
					UE_LOG(LogTemp, Log, TEXT("[CommanderWorldMapWidget] CollectIconComponents: Actor=%s, IconId=%d, bShowOnMap=%d, TeamId=%d"),
						*Actor->GetName(), (int32)IconComp->IconId, IconComp->bShowOnMap ? 1 : 0, IconComp->TeamId);
				}
				else
				{
					UE_LOG(LogTemp, Verbose, TEXT("[CommanderWorldMapWidget] CollectIconComponents: Actor=%s는 bShowOnMap=false로 스킵됨"),
						*Actor->GetName());
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[CommanderWorldMapWidget] CollectIconComponents 완료: 총 %d개 컴포넌트 발견, %d개 유효 (bShowOnMap=true)"),
		FoundCount, ValidCount);
}

void UCommanderWorldMapWidget::UpdateRealtimeIcons()
{
	if (!RealtimeIconsLayer || !MapImageWidget)
	{
		return;
	}

	AMapBoundsActor* MapBounds = FindMapBounds();
	if (!MapBounds)
	{
		return;
	}

	// MapImage 위젯의 실제 Geometry 가져오기
	TSharedPtr<SWidget> MapImageSlateWidget = MapImageWidget->GetCachedWidget();
	if (!MapImageSlateWidget.IsValid())
	{
		return;
	}

	const FGeometry MapImageGeometry = MapImageSlateWidget->GetCachedGeometry();
	const FVector2D MapLocalSize = MapImageGeometry.GetLocalSize();

	if (MapLocalSize.X <= 0.f || MapLocalSize.Y <= 0.f)
	{
		return;
	}

	const FGeometry RootGeometry = GetCachedGeometry();

	if (UWorld* World = GetWorld())
	{
		const float CurrentTime = World->GetTimeSeconds();
		if (CurrentTime - LastComponentCollectionTime >= ComponentCollectionInterval)
		{
			LastComponentCollectionTime = CurrentTime;
			
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (AActor* Actor = *It; IsValid(Actor))
				{
					if (UMinimapIconComponent* IconComp = Actor->FindComponentByClass<UMinimapIconComponent>())
					{
				// 멀티플레이에서 PlayerState가 나중에 복제될 수 있으므로
				// PlayerState가 설정되어 있으면 아이콘을 다시 업데이트
				if (APawn* OwnerPawn = Cast<APawn>(Actor))
				{
					ACitRushPlayerState* CitRushPS = OwnerPawn->GetPlayerState<ACitRushPlayerState>();
					
					// PlayerState가 없으면 GameState의 PlayerArray에서 찾기 시도
					if (!CitRushPS && World)
					{
						if (AGameStateBase* GameStateBase = World->GetGameState())
						{
							for (APlayerState* PS : GameStateBase->PlayerArray)
							{
								if (ACitRushPlayerState* FoundPS = Cast<ACitRushPlayerState>(PS))
								{
									if (FoundPS->GetPawn() == OwnerPawn)
									{
										CitRushPS = FoundPS;
										break;
									}
								}
							}
						}
					}
					
					if (CitRushPS)
					{
						// PlayerState가 있으면 아이콘 업데이트 시도 (아이콘 ID 재설정)
						IconComp->ApplyIconIdFromPlayerState();
					}
				}
						
						// bShowOnMap 확인 (PlayerState 업데이트 후)
						if (IconComp->bShowOnMap)
						{
							bool bAlreadyCached = false;
							for (const TWeakObjectPtr<UMinimapIconComponent>& CachedComp : CachedIconComponents)
							{
								if (CachedComp.Get() == IconComp)
								{
									bAlreadyCached = true;
									break;
								}
							}

							if (!bAlreadyCached)
							{
								CachedIconComponents.Add(IconComp);
								UE_LOG(LogTemp, Log, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: 새 컴포넌트 추가 - Actor=%s, IconId=%d, bShowOnMap=%d"),
									*Actor->GetName(), (int32)IconComp->IconId, IconComp->bShowOnMap ? 1 : 0);
							}
						}
					}
				}
			}
		}
	}

	TArray<TWeakObjectPtr<UMinimapIconComponent>> ComponentsToRemove;
	for (TWeakObjectPtr<UMinimapIconComponent>& WeakComp : CachedIconComponents)
	{
		if (!WeakComp.IsValid())
		{
			ComponentsToRemove.Add(WeakComp);
		}
	}

	for (const TWeakObjectPtr<UMinimapIconComponent>& WeakComp : ComponentsToRemove)
	{
		if (UMinimapIconComponent* Comp = WeakComp.Get())
		{
			if (URealtimeMapIcon* IconWidget = IconWidgetMap.FindRef(Comp).Get())
			{
				IconWidget->RemoveFromParent();
				IconWidgetMap.Remove(Comp);
			}
		}
		CachedIconComponents.Remove(WeakComp);
	}

	int32 ProcessedCount = 0;
	int32 VisibleCount = 0;
	int32 HiddenCount = 0;
	int32 CreatedCount = 0;

	for (TWeakObjectPtr<UMinimapIconComponent>& WeakComp : CachedIconComponents)
	{
		UMinimapIconComponent* IconComp = WeakComp.Get();
		if (!IconComp || !IsValid(IconComp))
		{
			UE_LOG(LogTemp, Verbose, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: 컴포넌트가 유효하지 않음"));
			continue;
		}

		ProcessedCount++;

		if (!IconComp->bShowOnMap)
		{
			if (URealtimeMapIcon* IconWidget = IconWidgetMap.FindRef(IconComp).Get())
			{
				IconWidget->SetVisibility(ESlateVisibility::Hidden);
				HiddenCount++;
			}
			UE_LOG(LogTemp, Verbose, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: Actor=%s는 bShowOnMap=false로 숨김"),
				IconComp->GetOwner() ? *IconComp->GetOwner()->GetName() : TEXT("None"));
			continue;
		}

		AActor* OwnerActor = IconComp->GetOwner();
		if (!IsValid(OwnerActor))
		{
			UE_LOG(LogTemp, Warning, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: Actor가 유효하지 않음"));
			continue;
		}

		const FVector WorldLoc = OwnerActor->GetActorLocation();
		const FVector2D WorldXY(WorldLoc.X, WorldLoc.Y);
		const FVector2D UV = MapBounds->WorldXYToUV(WorldXY);

		if (UV.X < 0.f || UV.X > 1.f || UV.Y < 0.f || UV.Y > 1.f)
		{
			if (URealtimeMapIcon* IconWidget = IconWidgetMap.FindRef(IconComp).Get())
			{
				IconWidget->SetVisibility(ESlateVisibility::Hidden);
				HiddenCount++;
			}
			UE_LOG(LogTemp, Verbose, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: Actor=%s는 맵 범위 밖 (UV=%.3f, %.3f)"),
				*OwnerActor->GetName(), UV.X, UV.Y);
			continue;
		}

		FVector2D RotatedUV = RotateUVToRotatedGeometry(UV);
		const FVector2D ImageLocalPos(
			RotatedUV.X * MapLocalSize.X,
			RotatedUV.Y * MapLocalSize.Y
		);

		const FVector2D ScreenPos = MapImageGeometry.LocalToAbsolute(ImageLocalPos);
		const FVector2D LocalPos = RootGeometry.AbsoluteToLocal(ScreenPos);

		URealtimeMapIcon* IconWidget = IconWidgetMap.FindRef(IconComp).Get();
		if (!IconWidget)
		{
			if (!RealtimeMapIconClass)
			{
				UE_LOG(LogTemp, Error, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: RealtimeMapIconClass가 설정되지 않았습니다! Blueprint에서 설정해주세요."));
				continue;
			}

			if (APlayerController* PC = GetOwningPlayer())
			{
				IconWidget = CreateWidget<URealtimeMapIcon>(PC, RealtimeMapIconClass);
				if (IconWidget)
				{
					IconWidget->SetupIcon(IconComp);
					
					if (UCanvasPanelSlot* CanvasSlot = RealtimeIconsLayer->AddChildToCanvas(IconWidget))
					{
						CanvasSlot->SetZOrder(100);
						IconWidgetMap.Add(IconComp, IconWidget);
						CreatedCount++;
						UE_LOG(LogTemp, Log, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: 아이콘 위젯 생성 - Actor=%s, IconId=%d, TeamId=%d"),
							*OwnerActor->GetName(), (int32)IconComp->IconId, IconComp->TeamId);
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: RealtimeIconsLayer에 아이콘 추가 실패 - Actor=%s"),
							*OwnerActor->GetName());
						IconWidget->RemoveFromParent();
						continue;
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: 아이콘 위젯 생성 실패 - Actor=%s"),
						*OwnerActor->GetName());
					continue;
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons: PlayerController를 찾을 수 없습니다!"));
				continue;
			}
		}

		if (IconWidget)
		{
			IconWidget->SetVisibility(ESlateVisibility::Visible);
			IconWidget->UpdatePosition(LocalPos);
			
			// PlayerState가 나중에 복제되면 아이콘 ID가 변경될 수 있으므로
			// 매 프레임마다 아이콘 ID와 팀 색상을 업데이트
			IconWidget->UpdateIconId(IconComp->IconId);
			IconWidget->UpdateTeamColor(IconComp->TeamId);
			
			VisibleCount++;

			if (IconComp->bRotateWithActor)
			{
				// Actor의 +X축이 정면(북쪽)을 향하도록 회전 계산
				// Actor의 Yaw는 +X 방향이 정면일 때 0도
				// 맵 회전도 고려하여 아이콘 회전 보정
				float ActorYaw = IconComp->GetActorYaw();
				float IconRotation = ActorYaw + MapImageRotation;
				IconWidget->UpdateRotation(IconRotation);
			}
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("[CommanderWorldMapWidget] UpdateRealtimeIcons 완료: 처리=%d, 표시=%d, 숨김=%d, 생성=%d"),
		ProcessedCount, VisibleCount, HiddenCount, CreatedCount);

	// CommanderMarkerImage도 함께 업데이트
	UpdateCommanderMarker();
}

void UCommanderWorldMapWidget::CleanupRealtimeIcons()
{
	for (auto& Pair : IconWidgetMap)
	{
		if (URealtimeMapIcon* IconWidget = Pair.Value.Get())
		{
			IconWidget->RemoveFromParent();
		}
	}

	IconWidgetMap.Empty();
	CachedIconComponents.Empty();
}

void UCommanderWorldMapWidget::ApplyMapImageRotation()
{
	const float RotationAngle = FMath::IsNearlyZero(MapImageRotation) ? 0.f : -MapImageRotation;
	
	if (MapImageWidget)
	{
		MapImageWidget->SetRenderTransformAngle(RotationAngle);
		MapImageWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}
	if (AlphaMapImage)
	{
		// AlphaMapImage는 별도의 AlphaMapRotation 프로퍼티 사용
		const float AlphaMapRotationAngle = FMath::IsNearlyZero(AlphaMapRotation) ? 0.f : -AlphaMapRotation;
		AlphaMapImage->SetRenderTransformAngle(AlphaMapRotationAngle);
		AlphaMapImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		// 회전 적용 확인을 위해 강제로 업데이트
		AlphaMapImage->InvalidateLayoutAndVolatility();
	}
	if (RealtimeIconsLayer)
	{
		RealtimeIconsLayer->SetRenderTransformAngle(RotationAngle);
		RealtimeIconsLayer->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}
}

FVector2D UCommanderWorldMapWidget::RotateUVToRotatedGeometry(const FVector2D& OriginalUV) const
{
	if (FMath::IsNearlyZero(MapImageRotation))
	{
		return OriginalUV;
	}

	const FVector2D Center(0.5f, 0.5f);
	FVector2D Offset = OriginalUV - Center;
	
	const float Radians = FMath::DegreesToRadians(MapImageRotation);
	const float Cos = FMath::Cos(Radians);
	const float Sin = FMath::Sin(Radians);
	
	FVector2D RotatedOffset(
		Offset.X * Cos - Offset.Y * Sin,
		Offset.X * Sin + Offset.Y * Cos
	);
	
	FVector2D RotatedUV = RotatedOffset + Center;
	RotatedUV.X = FMath::Clamp(RotatedUV.X, 0.f, 1.f);
	RotatedUV.Y = FMath::Clamp(RotatedUV.Y, 0.f, 1.f);
	
	return RotatedUV;
}

FVector2D UCommanderWorldMapWidget::RotateUVFromRotatedGeometry(const FVector2D& RotatedUV) const
{
	// 회전 보정 없이 그대로 반환 (0도 회전)
	return RotatedUV;
}

void UCommanderWorldMapWidget::TestUVRotation(const FVector2D& TestUV) const
{
	FVector2D RotatedUV = RotateUVToRotatedGeometry(TestUV);
	FVector2D BackToOriginal = RotateUVFromRotatedGeometry(RotatedUV);
	
	UE_LOG(LogTemp, Warning, TEXT("=== UV Rotation Test ==="));
	UE_LOG(LogTemp, Warning, TEXT("MapImageRotation: %.1f degrees"), MapImageRotation);
	UE_LOG(LogTemp, Warning, TEXT("Original UV: (%.3f, %.3f)"), TestUV.X, TestUV.Y);
	UE_LOG(LogTemp, Warning, TEXT("Rotated UV: (%.3f, %.3f)"), RotatedUV.X, RotatedUV.Y);
	UE_LOG(LogTemp, Warning, TEXT("Back to Original UV: (%.3f, %.3f)"), BackToOriginal.X, BackToOriginal.Y);
	UE_LOG(LogTemp, Warning, TEXT("Difference: (%.6f, %.6f)"), FMath::Abs(TestUV.X - BackToOriginal.X), FMath::Abs(TestUV.Y - BackToOriginal.Y));
	UE_LOG(LogTemp, Warning, TEXT("========================"));
}

void UCommanderWorldMapWidget::OnPingUpdated(const FPingData& NewPing)
{
	// GameState의 활성 핑 목록과 Map UI의 마커를 동기화
	if (UWorld* World = GetWorld())
	{
		if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
		{
			const TArray<FPingData>& ActivePings = GameState->GetActivePings();
			
			// Map UI의 마커 중 활성 핑 목록에 없는 것들 제거
			// (15초 자동 삭제, 레이서 충돌 삭제 모두 즉시 반영)
			TArray<FVector> MarkersToRemove;
			for (const auto& Pair : MapPingMarkers)
			{
				const FVector& MarkerWorldLoc = Pair.Key;
				bool bFound = false;
				
				// 활성 핑 목록에서 해당 위치 찾기 (거리 기반 매칭, 100cm 이내)
				for (const FPingData& Ping : ActivePings)
				{
					if (FVector::DistSquared(Ping.WorldLocation, MarkerWorldLoc) < 100.0f * 100.0f)
					{
						bFound = true;
						break;
					}
				}
				
				if (!bFound)
				{
					MarkersToRemove.Add(MarkerWorldLoc);
				}
			}
			
			// Map UI에서 마커 제거 (레벨의 PingActor는 이미 GameState에서 삭제됨)
			for (const FVector& WorldLoc : MarkersToRemove)
			{
				if (TWeakObjectPtr<UImage>* MarkerPtr = MapPingMarkers.Find(WorldLoc))
				{
					if (UImage* Marker = MarkerPtr->Get())
					{
						Marker->RemoveFromParent();
					}
					MapPingMarkers.Remove(WorldLoc);
				}
			}
			
			// 모든 핑이 삭제된 경우 모든 마커 제거
			if (ActivePings.Num() == 0)
			{
				RemoveMapPingMarkers();
			}
		}
	}
}

void UCommanderWorldMapWidget::RemoveMapPingMarkers()
{
	// Map UI의 모든 핑 마커 제거
	// 참고: 레벨의 PingActor는 GameState의 ActivePings 변경 시 
	// OnRep_ActivePings -> UpdatePingMarkerManager -> RefreshPingMarkers에서 자동으로 삭제됨
	for (auto& Pair : MapPingMarkers)
	{
		if (UImage* Marker = Pair.Value.Get())
		{
			Marker->RemoveFromParent();
		}
	}
	MapPingMarkers.Empty();
}
