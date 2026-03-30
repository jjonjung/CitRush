// Copyright Epic Games, Inc. All Rights Reserved.


#include "Player/Car/VehicleDemoUITest.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "WheeledVehiclePawn.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Enemy/PixelEnemy.h"
#include "Player/Car/VehicleDemoCejCar.h"
#include "Player/AbstractRacer.h"
#include "GAS/AttributeSet/ASRacer.h"
#include "AbilitySystemComponent.h"
#include "GameFlow/CitRushGameState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Item/ItemData.h"
#include "Player/Car/ItemSlotWidget.h"
#include "Engine/Texture2D.h"
#include "Player/CitRushPlayerState.h"

void UVehicleDemoUITest::NativeConstruct()
{
	Super::NativeConstruct();

	// Tick 비활성화 (이벤트 기반으로 변경)
	SetIsFocusable(false);

	InitializeVehicleReferences();
	StartTimer();

	// 속도/기어 업데이트 타이머 시작 (0.1초 간격)
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			VehicleUpdateTimerHandle,
			this,
			&UVehicleDemoUITest::UpdateVehicleInfo,
			0.1f, // 100ms 간격
			true  // 반복
		);
	}
	
	// ItemSlotWidget이 Blueprint에서 설정되지 않았으면 동적으로 생성
	/*if (!ItemSlotWidget && ItemSlotWidgetClass)
	{
		ItemSlotWidget = CreateWidget<UItemSlotWidget>(this, ItemSlotWidgetClass);
		if (ItemSlotWidget)
		{
			// 루트 위젯에 추가 (Blueprint에서 설정된 컨테이너에 추가)
			if (UPanelWidget* RootWidget = Cast<UPanelWidget>(GetRootWidget()))
			{
				RootWidget->AddChild(ItemSlotWidget);
			}
			else
			{
				// 루트가 PanelWidget이 아니면 Viewport에 추가
				ItemSlotWidget->AddToViewport();
				ItemSlotWidget->AddToViewport(1000); 
			}
		}
	}*/
	
	// ItemSlotWidget이 Blueprint에서 설정되었는지 확인 및 Visibility 설정
	if (ItemSlotWidget)
	{
		// Blueprint에서 배치한 위젯이면 Visibility를 Visible로 설정
		ItemSlotWidget->SetVisibility(ESlateVisibility::Visible);
		
		// 위젯의 실제 화면 위치와 크기 확인 (다음 프레임에 업데이트되므로 타이머로 확인)
		if (UWorld* WidgetWorld = GetWorld())
		{
			FTimerHandle DebugTimerHandle;
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindLambda([this]()
			{
				if (ItemSlotWidget)
				{
					// 위젯의 실제 Geometry 가져오기
					const FGeometry WidgetGeometry = ItemSlotWidget->GetCachedGeometry();
					const FVector2D LocalSize = WidgetGeometry.GetLocalSize();
					const FVector2D AbsolutePosition = FVector2D(WidgetGeometry.AbsolutePosition);
					
					// 부모 위젯 정보 확인
					if (UPanelWidget* ParentWidget = ItemSlotWidget->GetParent())
					{
						const FGeometry ParentGeometry = ParentWidget->GetCachedGeometry();
						const FVector2D ParentLocalSize = ParentGeometry.GetLocalSize();
						const FVector2D ParentAbsolutePosition = FVector2D(ParentGeometry.AbsolutePosition);
							
							UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] ===== ItemSlotWidget 디버그 정보 ====="));
							UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] ItemSlotWidget - LocalSize: (%.1f, %.1f), AbsolutePosition: (%.1f, %.1f)"),
								LocalSize.X, LocalSize.Y, AbsolutePosition.X, AbsolutePosition.Y);
							UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] Parent (%s) - LocalSize: (%.1f, %.1f), AbsolutePosition: (%.1f, %.1f)"),
								*ParentWidget->GetClass()->GetName(), ParentLocalSize.X, ParentLocalSize.Y, ParentAbsolutePosition.X, ParentAbsolutePosition.Y);
							
							// 화면 경계 확인 (일반적인 해상도 기준)
							if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
							{
								UE_LOG(LogTemp, Error, TEXT("[VehicleDemoUITest] ⚠️ ItemSlotWidget 크기가 0입니다! Fill 설정이 제대로 작동하지 않을 수 있습니다."));
							}
							
							if (AbsolutePosition.X < 0.0f || AbsolutePosition.Y < 0.0f)
							{
								UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] ⚠️ ItemSlotWidget이 화면 왼쪽/상단 밖에 있습니다."));
							}
							
							// 화면 크기 가져오기
							if (APlayerController* PC = GetOwningPlayer())
							{
								int32 ViewportSizeX, ViewportSizeY;
								PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
								
								if (AbsolutePosition.X > ViewportSizeX || AbsolutePosition.Y > ViewportSizeY)
								{
									UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] ⚠️ ItemSlotWidget이 화면 오른쪽/하단 밖에 있습니다. Viewport: (%d, %d)"),
										ViewportSizeX, ViewportSizeY);
								}
								
								UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] Viewport 크기: (%d, %d)"), ViewportSizeX, ViewportSizeY);
							}
							
							UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] =========================================="));
						}
					}
				});
			WidgetWorld->GetTimerManager().SetTimer(DebugTimerHandle, TimerDelegate, 0.1f, false);
		}
		
		// Canvas Panel Slot인 경우 Anchor와 Size 설정
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ItemSlotWidget->Slot))
		{
			// Center 정렬로 설정 (Anchor를 중앙으로)
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f)); // 중앙 고정
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // 중앙 정렬
			
			// Size To Content 활성화 (내용물 크기에 맞춤)
			// Size를 Auto로 설정하면 내용물 크기에 맞춰집니다
			FVector2D CurrentSize = CanvasSlot->GetSize();
			
			// Size가 0이거나 매우 작으면 내용물 크기로 설정
			if (CurrentSize.X <= 0.0f || CurrentSize.Y <= 0.0f)
			{
				// 위젯의 DesiredSize를 사용하여 내용물 크기 확인
				FVector2D DesiredSize = ItemSlotWidget->GetDesiredSize();
				if (DesiredSize.X > 0.0f && DesiredSize.Y > 0.0f)
				{
					CanvasSlot->SetSize(DesiredSize);
					UE_LOG(LogTemp, Log, TEXT("[VehicleDemoUITest] ItemSlotWidget Size To Content: (%.1f, %.1f)"), DesiredSize.X, DesiredSize.Y);
				}
				else
				{
					// 내용물이 없으면 기본 크기 설정
					CanvasSlot->SetSize(FVector2D(300.0f, 150.0f));
					UE_LOG(LogTemp, Log, TEXT("[VehicleDemoUITest] ItemSlotWidget 기본 Size 설정: 300x150"));
				}
			}
			
			// Position은 Center로 설정했으므로 (0,0)으로 설정 (중앙 정렬)
			CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
			
			UE_LOG(LogTemp, Log, TEXT("[VehicleDemoUITest] ItemSlotWidget Canvas Slot 설정 완료 - Center 정렬, Size: (%.1f, %.1f)"),
				CanvasSlot->GetSize().X, CanvasSlot->GetSize().Y);
		}
		else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ItemSlotWidget->Slot))
		{
			// Overlay Slot의 경우 Center 정렬 설정
			OverlaySlot->SetHorizontalAlignment(HAlign_Center);
			OverlaySlot->SetVerticalAlignment(VAlign_Center);
			UE_LOG(LogTemp, Log, TEXT("[VehicleDemoUITest] ItemSlotWidget이 Overlay Slot에 있습니다. Center 정렬 설정 완료."));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] ItemSlotWidget의 Slot 타입을 확인할 수 없습니다. Blueprint에서 Canvas Panel이나 Overlay에 추가했는지 확인하세요."));
		}
		
		UE_LOG(LogTemp, Log, TEXT("[VehicleDemoUITest] ItemSlotWidget 발견 - Visibility: %d"), (int32)ItemSlotWidget->GetVisibility());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] ItemSlotWidget이 설정되지 않았습니다! Blueprint에서 ItemSlotWidget 변수에 WBP_ItemSlotWidget을 할당하거나 ItemSlotWidgetClass를 설정하세요."));
	}

	

	// GAS Attribute 델리게이트 바인딩
	BindAttributeDelegates();

	// 아이템 슬롯 델리게이트 바인딩
	BindItemSlotDelegates();

	// 초기 아이템 슬롯 업데이트
	UpdateItemSlots();
	
	// 플레이어 이름 업데이트
	UpdatePlayerName();
}

void UVehicleDemoUITest::NativeDestruct()
{
	// 타이머 정리
	UWorld* World = GetWorld();
	if (World)
	{
		if (VehicleUpdateTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(VehicleUpdateTimerHandle);
		}

		if (TimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}

	// GAS 델리게이트 언바인딩
	UnbindAttributeDelegates();

	// 아이템 슬롯 델리게이트 언바인딩
	UnbindItemSlotDelegates();

	Super::NativeDestruct();
}

void UVehicleDemoUITest::InitializeVehicleReferences()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	APawn* ControlledPawn = PlayerController->GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	CarInfo = Cast<AWheeledVehiclePawn>(ControlledPawn);
	if (!CarInfo)
	{
		return;
	}

	VehicleMovementComponent = CarInfo->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
	if (!VehicleMovementComponent)
	{
		VehicleMovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(
			CarInfo->GetMovementComponent()
		);
        
	}
}

int32 UVehicleDemoUITest::GetCurrentGear() const
{
	if (!VehicleMovementComponent)
	{
		return 0;
	}
	return VehicleMovementComponent->GetCurrentGear();
}

// ========== 이벤트 기반 업데이트 함수 ==========
void UVehicleDemoUITest::UpdateVehicleInfo()
{
	// VehicleMovementComponent가 없으면 재초기화 시도
	if (!VehicleMovementComponent || !CarInfo)
	{
		InitializeVehicleReferences();
		return;
	}

	// 차량 속도 및 기어 업데이트
	if (VehicleMovementComponent)
	{
		float CurrentSpeed = VehicleMovementComponent->GetForwardSpeed();
		UpdateSpeed(CurrentSpeed);

		int32 CurrentGear = GetCurrentGear();
		UpdateGear(CurrentGear);

		UpdateCompass();
	}
	
	// 플레이어 이름이 아직 설정되지 않았으면 업데이트 시도 (멀티플레이 대응)
	if (PlayerNameText && PlayerNameText->GetText().IsEmpty())
	{
		UpdatePlayerName();
	}
}

void UVehicleDemoUITest::BindAttributeDelegates()
{
	// 이미 바인딩되어 있으면 언바인딩 먼저
	UnbindAttributeDelegates();

	if (!CarInfo)
	{
		return;
	}

	// Racer인 경우 GAS Attribute 델리게이트 바인딩
	AAbstractRacer* Racer = Cast<AAbstractRacer>(CarInfo);
	if (!Racer)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Racer->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet());
	if (!RacerAS)
	{
		return;
	}

	// Fuel Attribute 변경 델리게이트 바인딩
	/*FGameplayAttribute FuelAttribute = RacerAS->GetFuelAttribute();
	FuelAttributeChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(FuelAttribute).AddUObject(
		this,
		&UVehicleDemoUITest::OnFuelChanged
	);*/

	float CurrentFuel = RacerAS->GetFuel();
	float MaxFuel = RacerAS->GetMaxFuel();
}

void UVehicleDemoUITest::UnbindAttributeDelegates()
{
	if (!CarInfo)
	{
		return;
	}

	AAbstractRacer* Racer = Cast<AAbstractRacer>(CarInfo);
	if (!Racer)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Racer->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	UASRacer* RacerAS = Cast<UASRacer>(Racer->GetAttributeSet());
	if (!RacerAS)
	{
		return;
	}

	// Fuel 델리게이트 언바인딩
	if (FuelAttributeChangedHandle.IsValid())
	{
		FGameplayAttribute FuelAttribute = RacerAS->GetFuelAttribute();
		ASC->GetGameplayAttributeValueChangeDelegate(FuelAttribute).Remove(FuelAttributeChangedHandle);
		FuelAttributeChangedHandle.Reset();
	}
}

void UVehicleDemoUITest::BindItemSlotDelegates()
{
	// 이미 바인딩되어 있으면 언바인딩 먼저
	UnbindItemSlotDelegates();

	if (!CarInfo)
	{
		return;
	}

	AAbstractRacer* Racer = Cast<AAbstractRacer>(CarInfo);
	if (!Racer)
	{
		return;
	}

	// 아이템 슬롯 변경 델리게이트 바인딩 (해당 레이서의 이벤트만 구독)
	Racer->OnItemSlotChanged.AddDynamic(this, &UVehicleDemoUITest::OnItemSlotChanged);
	
	UE_LOG(LogTemp, Log, TEXT("[VehicleDemoUITest] ItemSlot 델리게이트 바인딩 완료 - Racer: %s"), 
		*Racer->GetName());
}

void UVehicleDemoUITest::UnbindItemSlotDelegates()
{
	if (!CarInfo)
	{
		return;
	}

	AAbstractRacer* Racer = Cast<AAbstractRacer>(CarInfo);
	if (!Racer)
	{
		return;
	}

	// 아이템 슬롯 변경 델리게이트 언바인딩
	Racer->OnItemSlotChanged.RemoveDynamic(this, &UVehicleDemoUITest::OnItemSlotChanged);
}

void UVehicleDemoUITest::OnItemSlotChanged(UItemData* FrontItem, UItemData* BackItem)
{
	// 현재 레이서의 아이템 슬롯만 업데이트
	if (!CarInfo)
	{
		return;
	}

	AAbstractRacer* Racer = Cast<AAbstractRacer>(CarInfo);
	if (!Racer)
	{
		return;
	}

	// 이벤트를 발생시킨 레이서가 현재 레이서인지 확인
	// (OnItemSlotChanged는 해당 레이서에서만 발생하므로 항상 일치해야 함)
	UE_LOG(LogTemp, Log, TEXT("[VehicleDemoUITest] OnItemSlotChanged - Racer: %s, FrontItem: %s, BackItem: %s"), 
		*Racer->GetName(),
		FrontItem ? *FrontItem->GetName() : TEXT("NULL"),
		BackItem ? *BackItem->GetName() : TEXT("NULL"));

	UpdateItemSlots();
}

void UVehicleDemoUITest::UpdateSpeed(float NewSpeed)
{
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	OnSpeedUpdate(FormattedSpeed);

	if (SpeedLabel)
	{
		FString SpeedString = FString::Printf(TEXT("%.0f"), FormattedSpeed);
		SpeedLabel->SetText(FText::FromString(SpeedString));
	}

	if (ImgPin)
	{
		float ClampedSpeed = FMath::Clamp(FormattedSpeed, 0.0f, MaxSpeed);
		float SpeedRatio = ClampedSpeed / MaxSpeed;
		// 각도 계산: MinAngle(-135) ~ MaxAngle(90)
		float CurrentAngle = FMath::Lerp(MinAngle, MaxAngle, SpeedRatio);
		FWidgetTransform Transform = ImgPin->GetRenderTransform();
		Transform.Angle = CurrentAngle;
		ImgPin->SetRenderTransform(Transform);
	}
}

void UVehicleDemoUITest::UpdateGear(int32 NewGear)
{
	OnGearUpdate(NewGear);
}

//연료 부족 
void UVehicleDemoUITest::ShowFuelWarning()
{
	FString Message = FString::Printf(TEXT("연료가 부족하여 속도를 제한합니다."));
	if (!StateText) return;
	StateText->SetText(FText::FromString(Message));
	StateText->SetVisibility(ESlateVisibility::Visible);

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(WarningHideTimerHandle, this, &UVehicleDemoUITest::HideFuelWarning, 3.0f, false);
	}
}

void UVehicleDemoUITest::ShowBoostEmptyWarning()
{
	if (!StateText) return;

	StateText->SetText(FText::FromString(TEXT("사용가능한 부스터가 없습니다.")));
	StateText->SetVisibility(ESlateVisibility::Visible);

	UWorld* World = GetWorld();
	if (World)
	{
		FTimerHandle BoostWarningTimerHandle;
		World->GetTimerManager().SetTimer(BoostWarningTimerHandle, [this]()
		{
			if (StateText)
			{
				StateText->SetVisibility(ESlateVisibility::Hidden);
			}
		}, 2.0f, false);
	}
}

//방위계
void UVehicleDemoUITest::UpdateCompass()
{
	if (!CarInfo || !IMG_Compass)
	{
		return;
	}

	FRotator ActorRotation = CarInfo->GetActorRotation();
	float Yaw = ActorRotation.Yaw;

	float NormalizedYaw = Yaw / 360.0f;
	if (UMaterialInstanceDynamic* MID = IMG_Compass->GetDynamicMaterial())
	{
		MID->SetScalarParameterValue(TEXT("Heading"), NormalizedYaw);
	}
}

void UVehicleDemoUITest::HideFuelWarning()
{
	if (ItemNotificationText)
	{
		ItemNotificationText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UVehicleDemoUITest::ShowStateMessage(const FString& Message)
{
	if (!StateText) return;

	StateText->SetText(FText::FromString(Message));
	StateText->SetVisibility(ESlateVisibility::Visible);
	StateText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(StateHideTimerHandle, this, &UVehicleDemoUITest::HideStateMessage, 2.0f, false);
	}
}

void UVehicleDemoUITest::HideStateMessage()
{
	if (StateText)
	{
		StateText->SetVisibility(ESlateVisibility::Hidden);
	}
}

// ========== 타이머 관련 함수 구현 ==========
void UVehicleDemoUITest::StartTimer()
{
	if (bIsTimerRunning)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	bIsTimerRunning = true;

	if (World)
	{
		World->GetTimerManager().SetTimer(TimerHandle, this, &UVehicleDemoUITest::UpdateTimer, 0.5f, true);
		UpdateTimer();
	}
}

void UVehicleDemoUITest::UpdateTimer()
{
	if (!bIsTimerRunning)
	{
		return;
	}
	
	if (ACitRushGameState* CitGameState = GetWorld()->GetGameState<ACitRushGameState>())
	{
		RemainingTime = CitGameState->GetRemainingTime();
	}

	if (RemainingTime <= 0.0f)
	{
		RemainingTime = 0.0f;
		FString TimerString = FString::Printf(TEXT("00:00"));
		TimerText->SetText(FText::FromString(TimerString));
		OnTimerEnd();
		return;
	}

	if (TimerText)
	{
		int32 Minutes = FMath::FloorToInt(RemainingTime / 60.0f);
		int32 Seconds = FMath::FloorToInt(RemainingTime) % 60;

		FString TimerString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		TimerText->SetText(FText::FromString(TimerString));

		if (RemainingTime <= 30.0f)
		{
			TimerText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		}
		else if (RemainingTime <= 60.0f)
		{
			TimerText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
		}
		else
		{
			TimerText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
	}
}

void UVehicleDemoUITest::OnTimerEnd()
{
	bIsTimerRunning = false;

	UWorld* World = GetWorld();
	if (World && TimerHandle.IsValid())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}

	if (TimerText)
	{
		TimerText->SetText(FText::FromString(TEXT("00:00")));
		TimerText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
	}

}

void UVehicleDemoUITest::UpdateItemSlots()
{
	if (!CarInfo)
	{
		return;
	}

	AAbstractRacer* Racer = Cast<AAbstractRacer>(CarInfo);
	if (!Racer)
	{
		return;
	}

	// ItemSlotWidget이 있으면 업데이트
	if (ItemSlotWidget)
	{
		// 현재 레이서의 아이템 슬롯만 가져오기
		UItemData* FrontItem = Racer->GetFrontItemSlot();
		UItemData* BackItem = Racer->GetBackItemSlot();
		
		UE_LOG(LogTemp, VeryVerbose, TEXT("[VehicleDemoUITest] UpdateItemSlots - Racer: %s, FrontItem: %s, BackItem: %s"), 
			*Racer->GetName(),
			FrontItem ? *FrontItem->GetName() : TEXT("NULL"),
			BackItem ? *BackItem->GetName() : TEXT("NULL"));
		
		ItemSlotWidget->UpdateItemSlots(FrontItem, BackItem);
	}
}

void UVehicleDemoUITest::UpdatePlayerName()
{
	if (!PlayerNameText)
	{
		return;
	}
	
	// PlayerController에서 PlayerState 가져오기
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}
	
	ACitRushPlayerState* PlayerState = PlayerController->GetPlayerState<ACitRushPlayerState>();
	if (!PlayerState)
	{
		// PlayerState가 아직 복제되지 않았을 수 있음 (멀티플레이)
		// 다음 프레임에 다시 시도
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick([this]()
			{
				UpdatePlayerName();
			});
		}
		return;
	}
	
	// PlayerInfo에서 이름 가져오기 (멀티플레이에서 더 안정적)
	FString PlayerName = PlayerState->GetPlayerInfo().playerName;
	if (PlayerName.IsEmpty())
	{
		// PlayerInfo에 이름이 없으면 기본 PlayerName 사용
		PlayerName = PlayerState->GetPlayerName();
	}
	
	if (PlayerName.IsEmpty())
	{
		PlayerName = TEXT("Racer");
	}
	
	PlayerNameText->SetText(FText::FromString(PlayerName));
	PlayerNameText->SetVisibility(ESlateVisibility::Visible);
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("[VehicleDemoUITest] 플레이어 이름 업데이트: %s"), *PlayerName);
}

void UVehicleDemoUITest::ShowItemNotification(const FString& ItemName)
{
	if (!ItemNotificationText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VehicleDemoUITest] ItemNotificationText가 설정되지 않았습니다! Blueprint에서 ItemNotificationText 변수에 TextBlock을 할당하세요."));
		return;
	}

	// 메시지 텍스트 설정
	FString NotificationText = FString::Printf(TEXT("커맨더가 아이템을 부여했습니다: %s"), *ItemName);
	ItemNotificationText->SetText(FText::FromString(NotificationText));
	ItemNotificationText->SetVisibility(ESlateVisibility::Visible);
	ItemNotificationText->SetColorAndOpacity(FLinearColor::White);

	UE_LOG(LogTemp, Log, TEXT("[VehicleDemoUITest] 아이템 알림 표시: %s"), *NotificationText);

	// 3초 후 자동으로 숨김
	UWorld* World = GetWorld();
	if (World)
	{
		FTimerHandle NotificationTimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindLambda([this]()
		{
			if (ItemNotificationText)
			{
				ItemNotificationText->SetVisibility(ESlateVisibility::Collapsed);
			}
		});
		World->GetTimerManager().SetTimer(NotificationTimerHandle, TimerDelegate, 3.0f, false);
	}
}

/*
void UVehicleDemoUITest::UpdateItemSlots()
{
	// CarInfo가 AbstractRacer가 아니면 아이템이 없음
	//AAbstractRacer* Racer = Cast<AAbstractRacer>(CarInfo);
	//if (!Racer)
	//{
	//	return;
	//}

	//// 전방 아이템 슬롯 업데이트
	//if (FrontItemImage && Racer->frontItemSlot && Racer->frontItemSlot->icon)
	//{
	//	FrontItemImage->SetBrushFromTexture(Racer->frontItemSlot->icon);
	//	FrontItemImage->SetVisibility(ESlateVisibility::Visible);
	//}
	//else if (FrontItemImage)
	//{
	//	// 아이템이 없으면 숨김
	//	FrontItemImage->SetVisibility(ESlateVisibility::Hidden);
	//}

	//// 후방 아이템 슬롯 업데이트
	//if (BackItemImage && Racer->backItemSlot && Racer->backItemSlot->icon)
	//{
	//	BackItemImage->SetBrushFromTexture(Racer->backItemSlot->icon);
	//	BackItemImage->SetVisibility(ESlateVisibility::Visible);
	//}
	//else if (BackItemImage)
	//{
	//	// 아이템이 없으면 숨김
	//	BackItemImage->SetVisibility(ESlateVisibility::Hidden);
	//}
}*/
