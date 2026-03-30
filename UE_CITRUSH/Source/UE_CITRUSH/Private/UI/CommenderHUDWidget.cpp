// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CommenderHUDWidget.h"
#include "UI/CrosshairWidget.h"
#include "UI/CoinWidget.h"
#include "UI/CommanderMessageWidget.h"
#include "UI/CommanderMessageType.h"
#include "UI/CommanderWorldMapWidget.h"
#include "Components/VerticalBox.h"
#include "Components/Overlay.h"
#include "Components/Image.h"
#include "Player/CommenderCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Player/Controller/CitRushPlayerController.h"
#include "TimerManager.h"
#include "Components/TextBlock.h"
#include "GameFlow/CitRushGameState.h"

void UCommenderHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] NativeConstruct 호출"));
	UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] CrosshairWidget=%p, CoinWidget=%p, MessageStack=%p, MessageWidgetClass=%p"), 
		CrosshairWidget.Get(), CoinWidget.Get(), MessageStack.Get(), MessageWidgetClass.Get());
	
	// 크로스헤어 초기 상태를 Visible로 설정 (기본값)
	if (CrosshairWidget)
	{
		CrosshairWidget->SetVisibility(ESlateVisibility::Visible);
		if (UImage* CrosshairImage = CrosshairWidget->GetCrosshairImage())
		{
			CrosshairImage->SetVisibility(ESlateVisibility::Visible);
		}
	}
	
	// 메시지 맵 초기화
	InitializeMessageMap();
	
	FindAndInitializeCharacter();

	GetWorld()->GetTimerManager().SetTimer(GameTimer, this, &UCommenderHUDWidget::UpdateTimer, 0.5f, true);
}

void UCommenderHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 캐릭터를 아직 찾지 못한 경우에만 검색 시도
	// FindAndInitializeCharacter() 내부에서 이미 찾았으면 early return 처리됨
	if (!OwnerCharacter)
	{
		FindAndInitializeCharacter();
	}
}

void UCommenderHUDWidget::FindAndInitializeCharacter()
{
	if (OwnerCharacter)
		return;

	if (APlayerController* PC = GetOwningPlayer())
	{
		OwnerCharacter = Cast<ACommenderCharacter>(PC->GetPawn());
		
		if (OwnerCharacter)
		{
			// 코인 변경 이벤트 구독
			OwnerCharacter->OnCoinChanged.AddDynamic(this, &UCommenderHUDWidget::UpdateCoin);
			
			// 초기 코인 값 저장 (메시지 표시 방지)
			PreviousCoinValue = OwnerCharacter->CurrentCoin;
			
			// 초기 코인 표시
			UpdateCoin(OwnerCharacter->CurrentCoin);
		}
	}
}

void UCommenderHUDWidget::UpdateCoin(int32 NewCoin)
{
	if (CoinWidget)
	{
		CoinWidget->SetCoinValue(NewCoin);
	}

	// 코인이 0이 되었을 때 메시지 표시
	// 이전 코인이 0보다 크고, 현재 코인이 0이면 "코인이 부족합니다." 메시지 표시
	if (PreviousCoinValue > 0 && NewCoin == 0)
	{
		ShowInsufficientCoinMessage();
	}

	PreviousCoinValue = NewCoin;
}

void UCommenderHUDWidget::ShowMessage(const FText& InMessage, ECommanderMessageType Type, float Duration)
{
	UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] ShowMessage 호출: %s, Type=%d, Duration=%.2f"), 
		*InMessage.ToString(), (int32)Type, Duration);
	
	// MessageWidgetClass 확인
	if (!MessageWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] MessageWidgetClass가 설정되지 않았습니다! 블루프린트에서 설정해주세요."));
		// 대체: 화면에 직접 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				Duration > 0.0f ? Duration : 2.0f,
				FColor::Yellow,
				InMessage.ToString()
			);
		}
		return;
	}
	
	// MessageStack 확인
	if (!MessageStack)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] MessageStack이 바인딩되지 않았습니다! 블루프린트에서 MessageStack을 확인하세요."));
		// 대체: 화면에 직접 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				Duration > 0.0f ? Duration : 2.0f,
				FColor::Yellow,
				InMessage.ToString()
			);
		}
		return;
	}

	// 메시지 위젯 동적 생성
	UCommanderMessageWidget* MessageWidget = CreateWidget<UCommanderMessageWidget>(GetWorld(), MessageWidgetClass);
	if (!MessageWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[CommenderHUDWidget] MessageWidget 생성 실패! MessageWidgetClass=%p"), MessageWidgetClass.Get());
		// 대체: 화면에 직접 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				Duration > 0.0f ? Duration : 2.0f,
				FColor::Yellow,
				InMessage.ToString()
			);
		}
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] MessageWidget 생성 성공"));

	// 메시지 설정
	MessageWidget->Setup(InMessage, Type, Duration);

	// MessageStack에 추가
	if (MessageStack->AddChildToVerticalBox(MessageWidget))
	{
		UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] MessageWidget을 MessageStack에 추가 완료"));
		
		// 애니메이션 재생 (자동으로 제거됨)
		MessageWidget->PlayShowHideAnimation();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CommenderHUDWidget] MessageStack에 위젯 추가 실패!"));
		// 대체: 화면에 직접 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				Duration > 0.0f ? Duration : 2.0f,
				FColor::Yellow,
				InMessage.ToString()
			);
		}
	}
}

void UCommenderHUDWidget::ShowInsufficientCoinMessage()
{
	ShowMessageByID(ECommanderMessageID::VendingMachine_InsufficientCoin);
}

void UCommenderHUDWidget::ShowMessageByID(ECommanderMessageID MessageID)
{
	if (MessageID == ECommanderMessageID::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] ShowMessageByID: MessageID가 None입니다."));
		return;
	}

	// 메시지 맵 초기화 확인
	EnsureMessageMapInitialized();

	// 메시지 맵에서 찾기
	if (const FCommanderMessageInfo* MessageInfo = MessageMap.Find(MessageID))
	{
		UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] ShowMessageByID: 메시지 찾음 (ID=%d, Text=%s)"), 
			(int32)MessageID, *MessageInfo->MessageText.ToString());
		ShowMessage(MessageInfo->MessageText, MessageInfo->MessageType, MessageInfo->Duration);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] ShowMessageByID: MessageID %d에 해당하는 메시지를 찾을 수 없습니다. MessageMap 크기=%d"), 
			(int32)MessageID, MessageMap.Num());
		
		// Fallback: 기본 메시지 텍스트 생성 및 표시
		FText FallbackText;
		ECommanderMessageType FallbackType = ECommanderMessageType::Warning;
		
		// MessageID에 따라 기본 메시지 설정
		switch (MessageID)
		{
		case ECommanderMessageID::RacerSelection_NeedSelectFirst:
			FallbackText = FText::FromString(TEXT("레이서를 먼저 선택해주세요."));
			break;
		case ECommanderMessageID::RacerSelection_NotConnected:
			FallbackText = FText::FromString(TEXT("레이서가 연결되지 않았습니다."));
			break;
		case ECommanderMessageID::Item_NotHolding:
			FallbackText = FText::FromString(TEXT("아이템을 들고 있지 않습니다."));
			break;
		case ECommanderMessageID::Item_SupplySuccess:
			FallbackText = FText::FromString(TEXT("아이템 할당 성공"));
			FallbackType = ECommanderMessageType::Success;
			break;
		case ECommanderMessageID::Item_SupplyFailed:
			FallbackText = FText::FromString(TEXT("아이템 할당 실패"));
			FallbackType = ECommanderMessageType::Error;
			break;
		case ECommanderMessageID::VendingMachine_InsufficientCoin:
			FallbackText = FText::FromString(TEXT("코인이 부족합니다."));
			break;
		case ECommanderMessageID::VendingMachine_Busy:
			FallbackText = FText::FromString(TEXT("자판기가 작동 중입니다."));
			break;
		case ECommanderMessageID::RacerSelection_CannotSelect:
			FallbackText = FText::FromString(TEXT("레이서를 선택할 수 없습니다."));
			break;
		case ECommanderMessageID::RacerSelection_HoldingItem:
			FallbackText = FText::FromString(TEXT("아이템을 들고 있어서 레이서를 선택할 수 없습니다."));
			break;
		default:
			FallbackText = FText::FromString(FString::Printf(TEXT("메시지 ID %d"), (int32)MessageID));
			break;
		}
		
		// Fallback 메시지 표시
		ShowMessage(FallbackText, FallbackType, 2.0f);
	}
}

void UCommenderHUDWidget::ShowMessageByIDWithFormat(ECommanderMessageID MessageID, const TArray<FString>& FormatArgs)
{
	if (MessageID == ECommanderMessageID::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] ShowMessageByIDWithFormat: MessageID가 None입니다."));
		return;
	}

	// 메시지 맵 초기화 확인
	EnsureMessageMapInitialized();

	// 메시지 맵에서 찾기
	if (const FCommanderMessageInfo* MessageInfo = MessageMap.Find(MessageID))
	{
		FText FormattedText = MessageInfo->MessageText;
		
		// 포맷 인자가 있으면 포맷팅
		if (FormatArgs.Num() > 0)
		{
			FString FormattedString = MessageInfo->MessageText.ToString();
			
			// {0}, {1}, {2} 형식으로 포맷팅
			for (int32 i = 0; i < FormatArgs.Num(); ++i)
			{
				FString Placeholder = FString::Printf(TEXT("{%d}"), i);
				FormattedString = FormattedString.Replace(*Placeholder, *FormatArgs[i], ESearchCase::CaseSensitive);
			}
			
			FormattedText = FText::FromString(FormattedString);
		}
		
		UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] ShowMessageByIDWithFormat: 메시지 찾음 (ID=%d, FormattedText=%s)"), 
			(int32)MessageID, *FormattedText.ToString());
		ShowMessage(FormattedText, MessageInfo->MessageType, MessageInfo->Duration);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CommenderHUDWidget] ShowMessageByIDWithFormat: MessageID %d에 해당하는 메시지를 찾을 수 없습니다. (MessageMap 크기: %d)"), 
			(int32)MessageID, MessageMap.Num());
		
		// Fallback: 화면에 직접 메시지 표시
		if (GEngine)
		{
			FString FallbackMessage = FString::Printf(TEXT("아이템 할당 성공 (메시지 ID %d를 찾을 수 없음)"), (int32)MessageID);
			if (FormatArgs.Num() >= 2)
			{
				FallbackMessage = FString::Printf(TEXT("%s에게 %s 전달됨"), *FormatArgs[0], *FormatArgs[1]);
			}
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Green,
				FallbackMessage
			);
		}
	}
}

void UCommenderHUDWidget::EnsureMessageMapInitialized()
{
	if (MessageMap.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] EnsureMessageMapInitialized: MessageMap 초기화"));
		InitializeMessageMap();
	}
}

void UCommenderHUDWidget::InitializeMessageMap()
{
	// 메시지 맵이 이미 초기화되어 있으면 스킵 (블루프린트에서 수정 가능하도록)
	if (MessageMap.Num() > 0)
	{
		return;
	}

	// 기본 메시지들 설정
	MessageMap.Add(ECommanderMessageID::VendingMachine_Busy, FCommanderMessageInfo(
		FText::FromString(TEXT("자판기가 작동 중입니다. 잠시 후 다시 시도해주세요.")),
		ECommanderMessageType::Warning,
		2.0f
	));

	MessageMap.Add(ECommanderMessageID::VendingMachine_InsufficientCoin, FCommanderMessageInfo(
		FText::FromString(TEXT("코인이 부족합니다.")),
		ECommanderMessageType::Warning,
		2.0f
	));

	MessageMap.Add(ECommanderMessageID::Item_NotHolding, FCommanderMessageInfo(
		FText::FromString(TEXT("아이템이 없습니다.")),
		ECommanderMessageType::Warning,
		2.0f
	));

	MessageMap.Add(ECommanderMessageID::Item_SupplySuccess, FCommanderMessageInfo(
		FText::FromString(TEXT("{0} <{1}> 부여되었습니다.")),
		ECommanderMessageType::Success,
		4.0f
	));

	MessageMap.Add(ECommanderMessageID::Item_SupplyFailed, FCommanderMessageInfo(
		FText::FromString(TEXT("아이템 부여에 실패했습니다. 아이템 데이터 테이블이 설정되지 않았습니다.")),
		ECommanderMessageType::Error,
		3.0f
	));

	MessageMap.Add(ECommanderMessageID::RacerSelection_NeedSelectFirst, FCommanderMessageInfo(
		FText::FromString(TEXT("레이서를 먼저 선택해주세요.")),
		ECommanderMessageType::Warning,
		2.0f
	));

	MessageMap.Add(ECommanderMessageID::RacerSelection_CannotSelect, FCommanderMessageInfo(
		FText::FromString(TEXT("레이서를 선택할 수 없습니다.")),
		ECommanderMessageType::Warning,
		3.0f
	));

	MessageMap.Add(ECommanderMessageID::RacerSelection_HoldingItem, FCommanderMessageInfo(
		FText::FromString(TEXT("아이템을 들고 있어서 레이서 선택이 불가능합니다.")),
		ECommanderMessageType::Warning,
		3.0f
	));

	UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] 메시지 맵 초기화 완료: %d개 메시지"), MessageMap.Num());
}

void UCommenderHUDWidget::AddOrUpdateMessage(ECommanderMessageID MessageID, const FText& MessageText, ECommanderMessageType MessageType, float Duration)
{
	if (MessageID == ECommanderMessageID::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] AddOrUpdateMessage: MessageID가 None입니다."));
		return;
	}

	// 메시지 맵에 추가 또는 업데이트
	MessageMap.Add(MessageID, FCommanderMessageInfo(MessageText, MessageType, Duration));
	
	UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] 메시지 추가/수정: ID=%d, Text=%s, Type=%d, Duration=%.2f"), 
		(int32)MessageID, *MessageText.ToString(), (int32)MessageType, Duration);
}

void UCommenderHUDWidget::RemoveMessage(ECommanderMessageID MessageID)
{
	if (MessageID == ECommanderMessageID::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] RemoveMessage: MessageID가 None입니다."));
		return;
	}

	int32 RemovedCount = MessageMap.Remove(MessageID);
	if (RemovedCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] 메시지 제거: ID=%d"), (int32)MessageID);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] RemoveMessage: ID=%d에 해당하는 메시지를 찾을 수 없습니다."), (int32)MessageID);
	}
}

bool UCommenderHUDWidget::HasMessage(ECommanderMessageID MessageID) const
{
	if (MessageID == ECommanderMessageID::None)
	{
		return false;
	}

	return MessageMap.Contains(MessageID);
}

void UCommenderHUDWidget::SetCrosshairColor(const FLinearColor& Color)
{
	if (CrosshairWidget)
	{
		CrosshairWidget->SetCrosshairColor(Color);
	}
}

void UCommenderHUDWidget::ResetCrosshairToDefault()
{
	if (CrosshairWidget)
	{
		CrosshairWidget->ResetCrosshairToDefault();
	}
}

void UCommenderHUDWidget::OpenMapUI()
{
	// 이미 열려있으면 "닫기"로 동작하여 F 키 한 번으로 토글되도록 처리
	if (IsMapUIOpen())
	{
		UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] OpenMapUI 호출 but map already open -> CloseMapUI (토글)"));
		CloseMapUI();
		return;
	}

	// MapWidgetClass 확인
	if (!MapWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] MapWidgetClass가 설정되지 않았습니다! 블루프린트에서 설정해주세요."));
		return;
	}

	// PlayerController 가져오기
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderHUDWidget] PlayerController를 찾을 수 없습니다."));
		return;
	}

	// 맵 위젯 생성
	MapWidgetInstance = CreateWidget<UCommanderWorldMapWidget>(PC, MapWidgetClass);
	if (!MapWidgetInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[CommenderHUDWidget] MapWidgetInstance 생성 실패! MapWidgetClass=%p"), MapWidgetClass.Get());
		return;
	}

	// 맵 위젯에 이 HUD 자신을 알려준다 (맵 위에서 F 키로 닫을 때 사용)
	MapWidgetInstance->SetOwningHUD(this);

	// Viewport에 추가
	MapWidgetInstance->AddToViewport(100); // 높은 Z-Order로 추가 (다른 UI 위에 표시)

	// 맵이 열릴 때 현재 커맨더 위치로 마커 한 번 갱신
	MapWidgetInstance->UpdateCommanderMarker();

	// 크로스헤어 숨기기 (맵이 열려 있는 동안에는 보이지 않도록)
	if (CrosshairWidget)
	{
		CrosshairWidget->SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] 크로스헤어 숨김 (맵 UI 활성화)"));
	}

	// CommanderCharacter에서 입력 모드/이동 차단을 일괄 처리
	if (OwnerCharacter)
	{
		OwnerCharacter->OpenMap();
	}

	UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] 맵 UI 열기 완료"));
}

void UCommenderHUDWidget::CloseMapUI()
{
	// 열려있지 않으면 스킵
	if (!IsMapUIOpen())
	{
		UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] 맵 UI가 열려있지 않습니다."));
		return;
	}

	// 위젯 제거
	if (MapWidgetInstance)
	{
		MapWidgetInstance->RemoveFromParent();
		MapWidgetInstance = nullptr;
	}

	// 크로스헤어 다시 표시 (기본값: Visible)
	if (CrosshairWidget)
	{
		CrosshairWidget->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] 크로스헤어 표시 (맵 UI 비활성화)"));
	}

	// CommanderCharacter에서 입력 모드/이동 복원을 일괄 처리
	if (OwnerCharacter)
	{
		OwnerCharacter->CloseMap();
	}

	UE_LOG(LogTemp, Log, TEXT("[CommenderHUDWidget] 맵 UI 닫기 완료"));
}

bool UCommenderHUDWidget::IsMapUIOpen() const
{
	return IsValid(MapWidgetInstance) && MapWidgetInstance->IsInViewport();
}

void UCommenderHUDWidget::UpdateTimer()
{
	if (!GameTimer.IsValid()) {return;}
	
	float RemainingTime = 0.f;
	if (ACitRushGameState* CitGameState = GetWorld()->GetGameState<ACitRushGameState>())
	{
		RemainingTime = CitGameState->GetRemainingTime();
	}

	if (RemainingTime <= 0.0f)
	{
		if (GameTimer.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(GameTimer);
		}
		FString TimerString = FString::Printf(TEXT("00:00"));
		TimerText->SetText(FText::FromString(TimerString));
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

void UCommenderHUDWidget::ToggleMapUI()
{
	// F 키 한 번으로 열기/닫기 모두 처리하기 위한 토글 함수
	if (IsMapUIOpen())
	{
		CloseMapUI();
	}
	else
	{
		OpenMapUI();
	}
}

