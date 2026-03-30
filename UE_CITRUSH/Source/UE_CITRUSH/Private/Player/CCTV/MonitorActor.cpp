// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/CCTV/MonitorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Player/CCTV/CCTVFeedComponent.h"
#include "UI/CCTVControlWidget.h"
#include "Interaction/InteractableComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Framework/Application/SlateApplication.h"

DEFINE_LOG_CATEGORY_STATIC(LogMonitorActor, Log, All);

// CCTV 로그 매크로 (모든 CCTV 관련 로그를 [CCTVLog] 태그로 통일)
#define CCTV_LOG(Verbosity, Format, ...) UE_LOG(LogMonitorActor, Verbosity, TEXT("[CCTVLog] " Format), ##__VA_ARGS__)

AMonitorActor::AMonitorActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// Monitor Mesh (Root Component로 설정)
	MonitorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MonitorMesh"));
	RootComponent = MonitorMesh;

	// InteractableComponent 생성 (UActorComponent이므로 Attach 불필요)
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));

	// CCTV Feed Component 생성 (UActorComponent이므로 Attach 불필요)
	CCTVFeedComponent = CreateDefaultSubobject<UCCTVFeedComponent>(TEXT("CCTVFeedComponent"));

	bCCTVOpen = false;
}

void AMonitorActor::BeginPlay()
{
	Super::BeginPlay();

	// Feed 초기화는 Match Start 이벤트에서 자동으로 호출됨 (CCTVFeedComponent::OnMatchStarted)
	// MonitorActor에서는 타이머로 호출하지 않음

	// 초기 상태: CCTV 닫힘
	CloseCCTV();
}

void AMonitorActor::ToggleCCTV(APlayerController* InteractingPC)
{
	if (bCCTVOpen)
	{
		CloseCCTV(InteractingPC);
	}
	else
	{
		OpenCCTV(InteractingPC);
	}
}

void AMonitorActor::OpenCCTV(APlayerController* InteractingPC)
{
	if (bCCTVOpen) return;

	bCCTVOpen = true;

	// 상호작용한 Commander의 PlayerController 사용 (멀티플레이 지원)
	APlayerController* PC = InteractingPC;
	if (!PC)
	{
		// InteractingPC가 없으면 로컬 컨트롤러 찾기 (단일 플레이어용)
		PC = GetWorld()->GetFirstPlayerController();
	}
	
	if (!PC)
	{
		CCTV_LOG(Warning, "PlayerController를 찾을 수 없습니다!");
		return;
	}

	// Screen Space Widget (화면에 표시)
	if (!ScreenCCTVWidget && CCTVWidgetClass)
	{
		ScreenCCTVWidget = CreateWidget<UCCTVControlWidget>(PC, CCTVWidgetClass);
		if (ScreenCCTVWidget)
		{
			ScreenCCTVWidget->SetCCTVFeedComponent(CCTVFeedComponent);
			ScreenCCTVWidget->AddToViewport();
			// 위젯이 포커스를 받을 수 있도록 설정
			ScreenCCTVWidget->SetIsFocusable(true);
		}
	}
	
	if (ScreenCCTVWidget)
	{
		ScreenCCTVWidget->SetVisibility(ESlateVisibility::Visible);
		ScreenCCTVWidget->SetIsFocusable(true);
		
		// Input Mode 변경 (UI 상호작용 가능하도록)
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		
		// 위젯에 포커스 설정 (Tab 키 입력을 받기 위해)
		TSharedPtr<SWidget> WidgetToFocus = ScreenCCTVWidget->TakeWidget();
		if (WidgetToFocus.IsValid())
		{
			InputMode.SetWidgetToFocus(WidgetToFocus);
		}
		
		PC->SetInputMode(InputMode);
		ScreenCCTVWidget->SetKeyboardFocus();
		
		// 마우스 커서 표시
		PC->SetShowMouseCursor(true);
		
		// 게임 입력 차단: 이동/시점 모두 무시
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}
	
	// 이동 차단 (캐릭터 이동 자체를 멈춤)
	if (ACharacter* Character = PC->GetCharacter())
	{
		if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
		{
			MovementComp->DisableMovement();
		}
	}

	// CCTV Feed Component 활성화 및 초기화
	if (CCTVFeedComponent)
	{
		// CCTV 상태를 초기 상태로 리셋 (4분할 화면, 포커스 0)
		// 위젯이 생성된 후에 호출하여 UI가 이벤트를 받을 수 있도록 함
		CCTVFeedComponent->ResetToDefaultState();
		CCTVFeedComponent->SetCCTVUIActive(true);
	}
}

void AMonitorActor::CloseCCTV(APlayerController* InteractingPC)
{
	// early return 제거: 초기화 시에도 비활성화 로직 실행
	bCCTVOpen = false;

	// CCTV Feed Component 비활성화 (SceneCapture 중지)
	if (CCTVFeedComponent)
	{
		CCTVFeedComponent->SetCCTVUIActive(false);
	}

	// Screen Space Widget 숨김
	if (ScreenCCTVWidget)
	{
		ScreenCCTVWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 상호작용한 Commander의 PlayerController 사용 (멀티플레이 지원)
	APlayerController* PC = InteractingPC;
	if (!PC)
	{
		// InteractingPC가 없으면 로컬 컨트롤러 찾기 (단일 플레이어용)
		PC = GetWorld()->GetFirstPlayerController();
	}

	// Input Mode 복원
	if (PC)
	{
		// 입력 모드 복원: GameOnly
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		
		// 마우스 커서 숨김
		PC->SetShowMouseCursor(false);
		
		// 게임 입력 허용: 이동/시점 모두 복원
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		
		// 이동 활성화 (캐릭터 이동 모드 복원)
		if (ACharacter* Character = PC->GetCharacter())
		{
			if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
			{
				MovementComp->SetMovementMode(MOVE_Walking);
			}
		}
	}

	CCTV_LOG(Log, "CCTV 닫기 완료");
}

void AMonitorActor::ServerSwitchCameraSlot_Implementation(bool bNext)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (CCTVFeedComponent)
	{
		CCTV_LOG(Log, "ServerSwitchCameraSlot 호출 - bNext: %s", bNext ? TEXT("true") : TEXT("false"));
		CCTVFeedComponent->SwitchCameraSlot(bNext);
	}
}

