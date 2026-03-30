#include "UI/RacerPingHUDWidget.h"
#include "GameFlow/CitRushGameState.h"
#include "GameFramework/Pawn.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

URacerPingHUDWidget::URacerPingHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CompassHalfFOV(90.f)
	, CompassWidth(400.f)
	, MiniMapRadiusWorld(2000.f)
	, UpdateInterval(0.05f)
	, LastUpdateTime(0.f)
{
}

void URacerPingHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// GameState 찾기
	UWorld* World = GetWorld();
	if (World)
	{
		GameState = World->GetGameState<ACitRushGameState>();
		if (GameState)
		{
			// 핑 업데이트 이벤트 구독
			GameState->OnPingUpdated.AddUObject(this, &URacerPingHUDWidget::OnPingUpdated);
		}
	}

	// Player Pawn 찾기
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		PlayerPawn = PC->GetPawn();
	}
}

void URacerPingHUDWidget::NativeDestruct()
{
	// 이벤트 구독 해제
	if (GameState)
	{
		GameState->OnPingUpdated.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void URacerPingHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 업데이트 주기 체크
	if (UpdateInterval > 0.f)
	{
		LastUpdateTime += InDeltaTime;
		if (LastUpdateTime < UpdateInterval)
		{
			return;
		}
		LastUpdateTime = 0.f;
	}

	// 활성 핑이 있을 때만 업데이트
	if (CurrentPing.IsValid() && GameState && GameState->HasActivePing())
	{
		UpdateCompass();
		UpdateMinimap();
	}
}

void URacerPingHUDWidget::OnPingUpdated(const FPingData& NewPing)
{
	CurrentPing = NewPing;
	
	if (CurrentPing.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[RacerPingHUDWidget] Ping updated at %s"), 
			*CurrentPing.WorldLocation.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[RacerPingHUDWidget] Ping cleared"));
		// UI 숨기기
		if (CompassIcon)
		{
			CompassIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (DistanceText)
		{
			DistanceText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void URacerPingHUDWidget::UpdateCompass()
{
	if (!PlayerPawn || !CurrentPing.IsValid())
	{
		return;
	}

	FVector PlayerLoc = PlayerPawn->GetActorLocation();
	FVector PingLoc = CurrentPing.WorldLocation;

	// 2D 거리 및 방향 계산
	FVector ToPing = PingLoc - PlayerLoc;
	FVector2D ToPing2D(ToPing.X, ToPing.Y);
	float Distance2D = ToPing2D.Size();

	// Yaw 계산
	float YawToPing = FMath::RadiansToDegrees(FMath::Atan2(ToPing2D.Y, ToPing2D.X));
	
	// 플레이어 Yaw
	FRotator PlayerRot = PlayerPawn->GetActorRotation();
	float PlayerYaw = PlayerRot.Yaw;

	// 각도 차이 계산
	float DeltaYaw = FMath::FindDeltaAngleDegrees(PlayerYaw, YawToPing);

	// 방위계 위치 계산 (-HalfFOV ~ +HalfFOV를 -HalfWidth ~ +HalfWidth로 매핑)
	float Normalized = FMath::Clamp(DeltaYaw / CompassHalfFOV, -1.f, 1.f);
	float XOffset = Normalized * (CompassWidth * 0.5f);

	// CompassIcon 위치 업데이트
	if (CompassIcon && CompassPanel)
	{
		CompassIcon->SetVisibility(ESlateVisibility::Visible);
		
		UCanvasPanelSlot* CompassSlot = Cast<UCanvasPanelSlot>(CompassIcon->Slot);
		if (CompassSlot)
		{
			FVector2D CurrentPos = CompassSlot->GetPosition();
			CompassSlot->SetPosition(FVector2D(XOffset, CurrentPos.Y));
		}
	}

	// 거리 텍스트 업데이트
	if (DistanceText)
	{
		DistanceText->SetVisibility(ESlateVisibility::Visible);
		FText DistanceTextValue = FText::AsNumber(FMath::RoundToInt(Distance2D / 100.f)); // 미터 단위로 변환
		DistanceText->SetText(DistanceTextValue);
	}
}

void URacerPingHUDWidget::UpdateMinimap()
{
	// 미니맵 업데이트는 Blueprint에서 구현하거나 필요시 추가
	// 기본적으로는 방위계만 사용
}
