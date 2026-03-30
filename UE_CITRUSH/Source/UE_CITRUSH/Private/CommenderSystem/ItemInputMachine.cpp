// Fill out your copyright notice in the Description page of Project Settings.

#include "CommenderSystem/ItemInputMachine.h"
#include "Interaction/InteractableComponent.h"
#include "Player/CommenderCharacter.h"
#include "UI/CommenderHUDWidget.h"
#include "UI/CommanderMessageType.h"
#include "UI/TargetRacerDisplayWidget.h"
#include "UI/TargetRacerSelectionWidget.h"
#include "CommenderSystem/VendingItemActor.h"
#include "GameFlow/CitRushGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/DataTypeObjectManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Item/ItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AbstractRacer.h"

AItemInputMachine::AItemInputMachine()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Root
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Mesh
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(Root);

	// DiscSlot (아이템 삽입 위치)
	DiscSlot = CreateDefaultSubobject<UBoxComponent>(TEXT("DiscSlot"));
	DiscSlot->SetupAttachment(Root);
	DiscSlot->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 오버랩 이벤트는 사용하지 않음

	// ItemUseOrigin (아이템 사용 기준 위치)
	ItemUseOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("ItemUseOrigin"));
	ItemUseOrigin->SetupAttachment(Root);
	ItemUseOrigin->SetRelativeLocation(FVector::ZeroVector);

	// Target Racer Widget
	TargetRacerWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TargetRacerWidgetComponent"));
	TargetRacerWidgetComponent->SetupAttachment(Root);
	TargetRacerWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TargetRacerWidgetComponent->SetDrawSize(FVector2D(200.0f, 100.0f));
	// 커맨더만 보이도록 설정 (레이서 클라이언트에서는 보이지 않음)
	TargetRacerWidgetComponent->SetOnlyOwnerSee(true);
	TargetRacerWidgetComponent->SetOwnerNoSee(false);

	// Interactable
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));

	// itemDataTable 초기화 시도 (생성자에서는 실패할 수 있음)
	TryFindItemDataTable();
}

void AItemInputMachine::TryFindItemDataTable()
{
	if (itemDataTable)
	{
		return; // 이미 설정되어 있으면 스킵
	}

	if (const UDataTypeObjectManager* datatables = UDataTypeObjectManager::Get())
	{
		itemDataTable = datatables->GetTableAsset(TEXT("Item"));
		if (itemDataTable)
		{
			UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] itemDataTable 찾기 성공: %s"), *itemDataTable->GetName());
		}
	}
}

void AItemInputMachine::BeginPlay()
{
	Super::BeginPlay();

	// 상호작용 이벤트 바인딩
	if (InteractableComponent)
	{
		InteractableComponent->OnMultiInteraction.AddDynamic(this, &AItemInputMachine::OnInteractionStarted);
		InteractableComponent->OnClientInteraction.AddDynamic(this, &AItemInputMachine::OnClientInteraction);
	}

	// 타겟 레이서 위젯 초기화
	if (TargetRacerWidgetComponent && TargetRacerWidgetClass)
	{
		UTargetRacerDisplayWidget* Widget = CreateWidget<UTargetRacerDisplayWidget>(GetWorld(), TargetRacerWidgetClass);
		if (Widget)
		{
			TargetRacerWidgetComponent->SetWidget(Widget);
			Widget->ClearTargetRacer();
		}
	}

	// 레이서 선택 요청 이벤트 자동 바인딩 (C++에서 처리)
	OnRacerSelectionRequested.AddDynamic(this, &AItemInputMachine::OnRacerSelectionRequestedHandler);
	UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] OnRacerSelectionRequested 델리게이트 바인딩 완료"));
	
	// itemDataTable이 null이면 BeginPlay에서 다시 시도
	TryFindItemDataTable();
	if (!itemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemInputMachine] BeginPlay에서도 itemDataTable을 찾을 수 없습니다! UDataTypeObjectManager에서 'Item' 테이블을 찾을 수 없습니다."));
	}
}

void AItemInputMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemInputMachine, TargetRacerIndex);
}

void AItemInputMachine::Server_InsertDisc_Implementation(ACommenderCharacter* Commander)
{
	UE_LOG(LogTemp, Warning, TEXT("[Server_InsertDisc] ===== 시작 [%s] ====="), 
		HasAuthority() ? TEXT("Server") : TEXT("Client"));
	
	if (!Commander)
	{
		UE_LOG(LogTemp, Error, TEXT("[Server_InsertDisc] [%s] Commander가 null입니다!"), 
			HasAuthority() ? TEXT("Server") : TEXT("Client"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
				TEXT("[ItemInputMachine] 아이템 사용 실패: Commander가 유효하지 않습니다."));
		}
		return;
	}

	if (InsertedItem || bIsInserting)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
				TEXT("[ItemInputMachine] 아이템 사용 실패: 이미 아이템이 삽입되어 있거나 삽입 중입니다."));
		}
		return;
	}

	AVendingItemActor* HeldItem = Commander->HeldDisc;
	if (!HeldItem)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
				TEXT("[ItemInputMachine] 아이템 사용 실패: 아이템을 들고 있지 않습니다."));
		}
		return;
	}

	bIsInserting = true;

	FDiscInsertResult Result;
	Result.bSuccess   = true;
	Result.EffectType = HeldItem->GetEffectType();
	Result.ItemId     = HeldItem->ItemId;
	Result.ItemName   = FText::FromName(HeldItem->ItemId);
	Result.ItemIcon   = HeldItem->ItemIcon;

	InsertedItem = HeldItem;
	InsertedItem->AttachToComponent(
		DiscSlot,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale
	);
	InsertedItem->SetActorEnableCollision(false);

	Commander->HeldDisc = nullptr;

	bIsInserting = false;

	OnDiscInserted.Broadcast(Result);

	if (GEngine)
	{
		const FString ItemNameStr = Result.ItemName.IsEmpty()
			? Result.ItemId.ToString()
			: Result.ItemName.ToString();

		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Cyan,
			FString::Printf(TEXT("아이템 삽입 완료: %s"), *ItemNameStr)
		);
	}

	if (Result.EffectType == ECommanderItemEffect::RacerItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server_InsertDisc] [%s] RacerItem 타입 감지, TargetRacerIndex: %d"), 
			HasAuthority() ? TEXT("Server") : TEXT("Client"), TargetRacerIndex);
		
		// 서버에서 레이서 수 확인
		if (HasAuthority())
		{
			if (UWorld* World = GetWorld())
			{
				if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
				{
					TArray<ACitRushPlayerState*> Racers = GameState->GetPlayerStatesByRole(EPlayerRole::Racer);
					UE_LOG(LogTemp, Warning, TEXT("[Server_InsertDisc] [Server] 현재 연결된 Racer 수: %d"), Racers.Num());
					
					for (int32 i = 0; i < Racers.Num(); i++)
					{
						if (IsValid(Racers[i]))
						{
							UE_LOG(LogTemp, Warning, TEXT("[Server_InsertDisc] [Server] Racer[%d]: %s"), 
								i, *Racers[i]->GetPlayerInfo().playerName);
						}
					}
				}
			}
		}
		
		if (TargetRacerIndex >= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Server_InsertDisc] [%s] TargetRacerIndex >= 0, 아이템 지급 시도"), 
				HasAuthority() ? TEXT("Server") : TEXT("Client"));
			
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.0f,
					FColor::Green,
					FString::Printf(TEXT("[ItemInputMachine] 아이템 사용 성공: 레이서 %d에게 전달"),
						TargetRacerIndex)
				);
			}
			OnRacerSelected.Broadcast(TargetRacerIndex);
			
			// 실제로 레이서에게 아이템 할당
			if (InsertedItem)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Server_InsertDisc] [%s] SupplyItemToRacer 호출: InsertedItem=%s, Commander=%s"), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"),
					*InsertedItem->GetName(), *Commander->GetName());
				SupplyItemToRacer(InsertedItem, Commander);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[Server_InsertDisc] [%s] InsertedItem이 null입니다!"), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"));
			}
		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.0f,
					FColor::Red,
					TEXT("[ItemInputMachine] 아이템 사용 실패: 레이서를 먼저 선택해주세요.")
				);
			}
			
			if (IsValid(Commander->CommenderHUDWidget.Get()))
			{
				Commander->CommenderHUDWidget->ShowMessageByID(ECommanderMessageID::RacerSelection_NeedSelectFirst);
			}

			OnRacerSelectionRequested.Broadcast();
		}
	}
	else if (Result.EffectType == ECommanderItemEffect::CommanderItem)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Green,
				TEXT("[ItemInputMachine] 아이템 사용 성공: 커맨더 아이템 사용 결정 UI 표시")
			);
		}
		OnCommanderItemUseDecided.Broadcast(Result);
	}
}

void AItemInputMachine::OnInteractionStarted(APawn* InteractingPawn)
{
	if (ACommenderCharacter* Commander = Cast<ACommenderCharacter>(InteractingPawn))
	{
		HandleInteraction(Commander);
	}
}

void AItemInputMachine::OnClientInteraction(APlayerController* PlayerController)
{
	if (!PlayerController) return;

	if (APawn* Pawn = PlayerController->GetPawn())
	{
		if (ACommenderCharacter* Commander = Cast<ACommenderCharacter>(Pawn))
		{
			HandleInteraction(Commander);
		}
	}
}

void AItemInputMachine::HandleInteraction(ACommenderCharacter* Commander)
{
	if (!Commander)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemInputMachine] HandleInteraction: Commander가 null입니다."));
		return;
	}

	// 서버에서 커맨더의 PlayerController를 Owner로 설정 (위젯이 커맨더에게만 보이도록)
	if (HasAuthority())
	{
		if (APlayerController* PC = Commander->GetController<APlayerController>())
		{
			SetOwner(PC);
			UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] HandleInteraction: Owner를 커맨더 PlayerController로 설정"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] HandleInteraction 호출: GrabbedItem=%p, HeldDisc=%p"), 
		Commander->GrabbedItem.Get(), Commander->HeldDisc);

	// LMB는 이제 Grab/Drop 전용이므로 HandleInteraction에서는 레이서 선택 UI만 처리
	// 아이템 사용은 F키로 처리됨 (CommenderCharacter::OnUseItemPressed)
	
	// 아이템을 들고 있지 않을 때만 레이서 선택 UI 표시 요청
	if (!Commander->GrabbedItem && !Commander->HeldDisc)
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] 레이서 선택 UI 요청: OnRacerSelectionRequested.IsBound()=%d"), 
			OnRacerSelectionRequested.IsBound());
		
		if (OnRacerSelectionRequested.IsBound())
		{
			OnRacerSelectionRequested.Broadcast();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemInputMachine] OnRacerSelectionRequested 델리게이트가 바인딩되지 않았습니다!"));
		}
	}
	else
	{
		// 아이템을 들고 있을 때는 레이서 선택 UI를 열 수 없음 - 메시지 표시
		UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] 아이템을 들고 있어서 메시지 표시: CommenderHUDWidget=%p, IsHoldingItem=%d"), 
			Commander->CommenderHUDWidget.Get(), Commander->IsHoldingItem() ? 1 : 0);
		
		if (IsValid(Commander->CommenderHUDWidget.Get()))
		{
			// 아이템을 들고 있으면 "아이템을 들고 있어서 레이서 선택이 불가능합니다" 메시지 표시
			ECommanderMessageID MessageID = Commander->IsHoldingItem()
				? ECommanderMessageID::RacerSelection_HoldingItem
				: ECommanderMessageID::RacerSelection_CannotSelect;
			
			Commander->CommenderHUDWidget->ShowMessageByID(MessageID);
			
			UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] 메시지 표시 완료: MessageID=%d"), (int32)MessageID);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemInputMachine] CommenderHUDWidget이 null입니다! 메시지를 표시할 수 없습니다."));
		}
	}
}

bool AItemInputMachine::GetDiscItemInfo(FDiscInsertResult& OutResult) const
{
	if (!InsertedItem)
	{
		OutResult.bSuccess = false;
		return false;
	}

	OutResult.bSuccess   = true;
	OutResult.EffectType = InsertedItem->GetEffectType();
	OutResult.ItemId     = InsertedItem->ItemId;
	OutResult.ItemName   = FText::FromName(InsertedItem->ItemId);
	OutResult.ItemIcon   = InsertedItem->ItemIcon;

	return true;
}


void AItemInputMachine::Server_SetTargetRacer_Implementation(int32 RacerIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("[Server_SetTargetRacer] [%s] RacerIndex: %d"), 
		HasAuthority() ? TEXT("Server") : TEXT("Client"), RacerIndex);
	
	TargetRacerIndex = RacerIndex;
	OnRep_CurrentTargetRacer();
}

void AItemInputMachine::OnRep_CurrentTargetRacer()
{
	UE_LOG(LogTemp, Warning, TEXT("[OnRep_CurrentTargetRacer] [%s] TargetRacerIndex: %d"), 
		HasAuthority() ? TEXT("Server") : TEXT("Client"), TargetRacerIndex);
	
	UpdateTargetRacerWidget();
	OnTargetRacerChanged();
	
	// 레이서가 연결되었고 InsertedItem이 있으면 자동으로 할당 시도 (서버에서만)
	if (HasAuthority() && TargetRacerIndex >= 0 && InsertedItem && InsertedItem->GetEffectType() == ECommanderItemEffect::RacerItem)
	{
		UE_LOG(LogTemp, Log, TEXT("[OnRep_CurrentTargetRacer] [Server] 레이서 연결됨, InsertedItem 자동 할당 시도"));
		
		// Commander 찾기
		ACommenderCharacter* Commander = nullptr;
		if (UWorld* World = GetWorld())
		{
			if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
			{
				TArray<ACitRushPlayerState*> Commanders = GameState->GetPlayerStatesByRole(EPlayerRole::Commander);
				if (Commanders.Num() > 0 && IsValid(Commanders[0]))
				{
					Commander = Commanders[0]->GetPawn<ACommenderCharacter>();
				}
			}
		}
		
		if (Commander)
		{
			UE_LOG(LogTemp, Log, TEXT("[OnRep_CurrentTargetRacer] [Server] Commander 찾기 성공, SupplyItemToRacer 호출"));
			SupplyItemToRacer(InsertedItem, Commander);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[OnRep_CurrentTargetRacer] [Server] Commander를 찾을 수 없어 자동 할당 실패"));
		}
	}
}

void AItemInputMachine::UpdateTargetRacerWidget()
{
	if (!TargetRacerWidgetComponent) return;

		if (UTargetRacerDisplayWidget* Widget =
		Cast<UTargetRacerDisplayWidget>(TargetRacerWidgetComponent->GetWidget()))
	{
		if (TargetRacerIndex >= 0)
		{
			// 레이서 이미지와 색상 가져오기
			UTexture2D* RacerImage = nullptr;
			FLinearColor RacerColor = FLinearColor::White;
			
			// CurrentSelectionWidget에서 레이서 이미지 가져오기
			if (IsValid(CurrentSelectionWidget))
			{
				TArray<TObjectPtr<UTexture2D>> RacerImages = CurrentSelectionWidget->RacerImages;
				if (RacerImages.IsValidIndex(TargetRacerIndex) && IsValid(RacerImages[TargetRacerIndex]))
				{
					RacerImage = RacerImages[TargetRacerIndex].Get();
					
					// 레이서 인덱스에 따른 기본 색상 (블루, 그린, 오렌지)
					switch (TargetRacerIndex)
					{
					case 0:
						RacerColor = FLinearColor(0.2f, 0.4f, 1.0f, 1.0f); // 파란색
						break;
					case 1:
						RacerColor = FLinearColor(0.2f, 1.0f, 0.4f, 1.0f); // 초록색
						break;
					case 2:
						RacerColor = FLinearColor(1.0f, 0.6f, 0.2f, 1.0f); // 오렌지색
						break;
					default:
						RacerColor = FLinearColor::White;
						break;
					}
				}
			}
			
			Widget->SetTargetRacer(TargetRacerIndex, RacerImage, RacerColor);
		}
		else
		{
			Widget->ClearTargetRacer();
		}
	}
}

void AItemInputMachine::SelectRacer(int32 RacerIndex)
{
	Server_SetTargetRacer(RacerIndex);
	OnRacerSelected.Broadcast(RacerIndex);
}

void AItemInputMachine::DecideCommanderItemUse(bool bUse)
{
	if (!bUse || !InsertedItem)
		return;

	FDiscInsertResult Result;
	if (GetDiscItemInfo(Result))
	{
		OnCommanderItemUseDecided.Broadcast(Result);
	}


}

void AItemInputMachine::OnRacerSelectionRequestedHandler()
{
	UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] OnRacerSelectionRequestedHandler 호출"));
	
	// 현재 로컬 플레이어가 Commander인지 확인
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	APlayerController* LocalPC = World->GetFirstPlayerController();
	if (!LocalPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemInputMachine] 로컬 PlayerController를 찾을 수 없습니다."));
		return;
	}
	
	// PlayerState를 통해 역할 확인
	ACitRushPlayerState* LocalPS = LocalPC->GetPlayerState<ACitRushPlayerState>();
	if (!LocalPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemInputMachine] 로컬 PlayerState를 찾을 수 없습니다."));
		return;
	}
	
	// Commander가 아닌 경우 (Racer 등) 위젯을 표시하지 않음
	if (LocalPS->GetPlayerRole() != EPlayerRole::Commander)
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] 현재 플레이어가 Commander가 아니므로 위젯을 표시하지 않습니다. Role: %d"), 
			(int32)LocalPS->GetPlayerRole());
		return;
	}
	
	// 이미 위젯이 열려있으면 무시 (딱 한 번만 띄우기)
	if (IsValid(CurrentSelectionWidget))
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] 이미 위젯이 열려있어서 무시"));
		return;
	}

	// LocalPC의 Pawn이 Commander임 (위에서 이미 확인함)
	ACommenderCharacter* Commander = Cast<ACommenderCharacter>(LocalPC->GetPawn());
	if (!Commander)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemInputMachine] Commander Pawn을 찾을 수 없습니다."));
		return;
	}
	
	// 아이템을 들고 있거나 GRAB 상태이면 레이서 선택 UI를 열 수 없음
	// 아이템을 들고 있는지 확인 (HeldDisc 또는 GRAB 상태)
	if (Commander->HeldDisc || Commander->IsHoldingItem())  // 둘이 같은 Actor인데..?
	{
		if (IsValid(Commander->CommenderHUDWidget.Get()))
		{
			// 아이템을 들고 있으면 "아이템을 들고 있어서 레이서 선택이 불가능합니다" 메시지 표시
			ECommanderMessageID MessageID = Commander->IsHoldingItem()
				? ECommanderMessageID::RacerSelection_HoldingItem
				: ECommanderMessageID::RacerSelection_CannotSelect;
			
			Commander->CommenderHUDWidget->ShowMessageByID(MessageID);
			UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] OnRacerSelectionRequestedHandler: 메시지 표시 완료 (MessageID=%d)"), (int32)MessageID);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemInputMachine] OnRacerSelectionRequestedHandler: CommenderHUDWidget이 null입니다!"));
		}
		return;
	}

	// 레이서 선택 위젯 클래스가 설정되지 않았으면 경고
	if (!TargetRacerSelectionWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemInputMachine] TargetRacerSelectionWidgetClass가 설정되지 않았습니다. 블루프린트에서 설정해주세요."));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Red,
				TEXT("[ItemInputMachine] ⚠️ TargetRacerSelectionWidgetClass가 설정되지 않았습니다. 블루프린트에서 설정해주세요.")
			);
		}
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] 레이서 선택 위젯 생성 시도: PlayerController=%p, WidgetClass=%p"), 
		LocalPC, TargetRacerSelectionWidgetClass.Get());

	// 레이서 선택 위젯 생성
	UTargetRacerSelectionWidget* SelectionWidget = CreateWidget<UTargetRacerSelectionWidget>(
		LocalPC,
		TargetRacerSelectionWidgetClass
	);

	if (SelectionWidget)
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] 레이서 선택 위젯 생성 성공"));
		// 현재 위젯 저장
		CurrentSelectionWidget = SelectionWidget;

		// ItemInputMachine 참조 설정
		SelectionWidget->SetItemInputMachine(this);

		// 캐릭터 이동 정지
		if (UCharacterMovementComponent* MovementComp = Commander->GetCharacterMovement())
		{
			MovementComp->DisableMovement();
		}

		// 마우스 커서 활성화 및 카메라 고정
		LocalPC->SetShowMouseCursor(true);
		LocalPC->SetInputMode(FInputModeUIOnly());
		
		// Viewport에 추가
		SelectionWidget->AddToViewport();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Green,
				TEXT("[ItemInputMachine] 레이서 선택 위젯이 표시되었습니다.")
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemInputMachine] 레이서 선택 위젯 생성 실패"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Red,
				TEXT("[ItemInputMachine] 레이서 선택 위젯 생성 실패")
			);
		}
	}
}

bool AItemInputMachine::CanAcceptItem(AVendingItemActor* Item, ACommenderCharacter* Commander) const
{
	UE_LOG(LogTemp, Warning, TEXT("[CanAcceptItem] [%s] ===== 시작 ====="), 
		HasAuthority() ? TEXT("Server") : TEXT("Client"));
	
	if (!Item || !Commander)
	{
		UE_LOG(LogTemp, Error, TEXT("[CanAcceptItem] [%s] Item or Commander is NULL"), 
			HasAuthority() ? TEXT("Server") : TEXT("Client"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(101, 0.0f, FColor::Red,
				TEXT("[CanAcceptItem] Item or Commander is NULL"));
		}
		return false;
	}

	// 1) 레이서가 세팅되어 있어야 함
	if (TargetRacerIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanAcceptItem] [%s] TargetRacerIndex < 0 (현재: %d)"), 
			HasAuthority() ? TEXT("Server") : TEXT("Client"), TargetRacerIndex);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(102, 0.0f, FColor::Yellow,
				FString::Printf(TEXT("[CanAcceptItem] TargetRacerIndex < 0 (현재: %d)"), TargetRacerIndex));
		}
		return false;
	}

	// 2) 아이템이 지정 범위 안에 있는지
	if (!ItemUseOrigin)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(103, 0.0f, FColor::Red,
				TEXT("[CanAcceptItem] ItemUseOrigin is NULL"));
		}
		return false;
	}

	const FVector Origin = ItemUseOrigin->GetComponentLocation();
	const FVector ItemLoc = Item->GetActorLocation();
	const float Dist = FVector::Dist(Origin, ItemLoc);

	if (Dist > ItemUseRadius)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanAcceptItem] [%s] 거리 초과: %.1f > %.1f"), 
			HasAuthority() ? TEXT("Server") : TEXT("Client"), Dist, ItemUseRadius);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(104, 0.0f, FColor::Yellow,
				FString::Printf(TEXT("[CanAcceptItem] 거리 초과: %.1f > %.1f"), Dist, ItemUseRadius));
		}
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CanAcceptItem] [%s] ✓ 사용 가능! 거리: %.1f"), 
		HasAuthority() ? TEXT("Server") : TEXT("Client"), Dist);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(105, 0.0f, FColor::Green,
			FString::Printf(TEXT("[CanAcceptItem] ✓ 사용 가능! 거리: %.1f"), Dist));
	}

	return true;
}

bool AItemInputMachine::SupplyItemToRacer(AVendingItemActor* Item, ACommenderCharacter* Commander)
{
	// 서버에서만 실행되어야 함
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] 클라이언트에서 호출됨 - 서버에서만 실행 가능"));
		return false;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] ===== 시작 [Server] ====="));
	UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [Server] Item: %s, TargetRacerIndex: %d"), 
		Item ? *Item->GetName() : TEXT("NULL"), TargetRacerIndex);
	
	// 디버그 모드: 시작 정보 표시
	if (bDebugItemAssignment && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			300,
			10.0f,
			FColor::Cyan,
			FString::Printf(TEXT("[DEBUG] 아이템 할당 시작 - Item: %s, TargetRacerIndex: %d"), 
				Item ? *Item->GetName() : TEXT("NULL"), TargetRacerIndex)
		);
	}
	
	if (!CanAcceptItem(Item, Commander))
	{
		UE_LOG(LogTemp, Error, TEXT("[SupplyItemToRacer] [Server] CanAcceptItem 실패!"));
		
		// 디버그 모드: 실패 정보 표시
		if (bDebugItemAssignment && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				301,
				10.0f,
				FColor::Red,
				TEXT("[DEBUG] ❌ CanAcceptItem 실패! 아이템이 기계 범위 밖에 있습니다.")
			);
		}
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [Server] CanAcceptItem 통과"));
	
	// 디버그 모드: CanAcceptItem 통과 표시
	if (bDebugItemAssignment && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			302,
			10.0f,
			FColor::Green,
			TEXT("[DEBUG] ✓ CanAcceptItem 통과")
		);
	}

	// 아이템 이름 가져오기
	FString ItemName = Item->ItemId.ToString();
	UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [Server] ItemName: %s"), *ItemName);

	// 레이서 찾기: GameState를 통해 PlayerInfo의 targetIndex 사용 (멀티플레이 지원)
	ACitRushPlayerState* targetRacer = nullptr;
	
	// TargetRacerIndex 유효성 검사
	if (TargetRacerIndex < 0 || TargetRacerIndex >= 3)
	{
		UE_LOG(LogTemp, Error, TEXT("[SupplyItemToRacer] [Server] 잘못된 TargetRacerIndex: %d (0-2 범위여야 함)"), TargetRacerIndex);
		return false;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	
	ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>();
	if (!GameState)
	{
		UE_LOG(LogTemp, Error, TEXT("[SupplyItemToRacer] [Server] GameState를 찾을 수 없습니다!"));
		if (bDebugItemAssignment && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(306, 10.0f, FColor::Red, TEXT("[DEBUG] ❌ GameState를 찾을 수 없습니다!"));
		}
		return false;
	}
	
	// 레이서 목록 가져오기
	TArray<ACitRushPlayerState*> Racers = GameState->GetPlayerStatesByRole(EPlayerRole::Racer);
	UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [Server] 찾은 레이서 수: %d, TargetRacerIndex: %d"), 
		Racers.Num(), TargetRacerIndex);
	
	// TargetRacerIndex를 ETargetRacer로 변환 (0=Racer1, 1=Racer2, 2=Racer3)
	ETargetRacer TargetRacerEnum = ETargetRacer::None;
	if (TargetRacerIndex == 0) TargetRacerEnum = ETargetRacer::Racer1;
	else if (TargetRacerIndex == 1) TargetRacerEnum = ETargetRacer::Racer2;
	else if (TargetRacerIndex == 2) TargetRacerEnum = ETargetRacer::Racer3;
	
	// PlayerInfo의 targetIndex가 일치하는 레이서 찾기
	for (ACitRushPlayerState* RacerPS : Racers)
	{
		if (!IsValid(RacerPS))
		{
			continue;
		}
		
		if (RacerPS->GetPlayerInfo().targetIndex == TargetRacerEnum)
		{
			targetRacer = RacerPS;
			UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [Server] 레이서 찾기 성공 (targetIndex: %d): %s"), 
				(int32)TargetRacerEnum, *targetRacer->GetPlayerInfo().playerName);
			
			if (bDebugItemAssignment && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(304, 10.0f, FColor::Green,
					FString::Printf(TEXT("[DEBUG] ✓ 레이서 찾기 성공 (targetIndex: %d): %s"), 
						(int32)TargetRacerEnum, *targetRacer->GetPlayerInfo().playerName));
			}
			break;
		}
	}
	
	if (!targetRacer)
	{
		// 레이서를 찾지 못한 경우 디버그 정보 출력
		UE_LOG(LogTemp, Error, TEXT("[SupplyItemToRacer] [Server] targetIndex %d에 해당하는 레이서를 찾을 수 없습니다! (총 레이서 수: %d)"), 
			(int32)TargetRacerEnum, Racers.Num());
		
		if (bDebugItemAssignment && GEngine)
		{
			FString RacerList = FString::Printf(TEXT("[DEBUG] 현재 레이서 목록 (%d명): "), Racers.Num());
			for (ACitRushPlayerState* RacerPS : Racers)
			{
				if (IsValid(RacerPS))
				{
					ETargetRacer RacerTargetIndex = RacerPS->GetPlayerInfo().targetIndex;
					RacerList += FString::Printf(TEXT("%s[targetIndex:%d] "), 
						*RacerPS->GetPlayerInfo().playerName, (int32)RacerTargetIndex);
				}
			}
			GEngine->AddOnScreenDebugMessage(303, 10.0f, FColor::Yellow, RacerList);
			GEngine->AddOnScreenDebugMessage(305, 10.0f, FColor::Red,
				FString::Printf(TEXT("[DEBUG] ❌ targetIndex %d에 해당하는 레이서를 찾을 수 없습니다!"), (int32)TargetRacerEnum));
		}
	}
	
	if (!targetRacer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [Server] targetRacer가 null입니다! 레이서가 아직 연결되지 않았을 수 있습니다."));
		
		// 디버그 모드: 레이서 없음 상세 표시
		if (bDebugItemAssignment && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(307, 10.0f, FColor::Yellow,
				FString::Printf(TEXT("[DEBUG] ⚠ 레이서 없음 - 아이템 보관 중 (인덱스: %d)"), TargetRacerIndex));
		}
		
		// Commander에게 메시지 표시 (연결된 레이서가 없다는 메시지)
		ShowCommanderMessage(Commander, ECommanderMessageID::RacerSelection_NeedSelectFirst);
		
		// 아이템을 Destroy하지 않고 return (InsertedItem에 남아있음)
		// 레이서가 연결되면 다시 시도할 수 있도록 함
		// PhysicsHandle은 Release하지 않아서 grab 상태 유지
		return false;
	}

	// itemDataTable 확인 및 재시도
	TryFindItemDataTable();
	if (!itemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[SupplyItemToRacer] [%s] itemDataTable을 찾을 수 없습니다!"), 
			HasAuthority() ? TEXT("Server") : TEXT("Client"));
		
		if (bDebugItemAssignment && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(313, 10.0f, FColor::Red, TEXT("[DEBUG] ❌ itemDataTable이 null입니다! 아이템 할당 실패"));
		}
		
		// Commander에게 메시지 표시
		ShowCommanderMessage(Commander, ECommanderMessageID::Item_SupplyFailed);
		
		// itemDataTable이 없으면 아이템을 Destroy하지 않고 grab 상태 유지
		// PhysicsHandle은 Release하지 않아서 grab 상태 유지
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] itemDataTable 찾기 시도: ItemId=%s"), 
		HasAuthority() ? TEXT("Server") : TEXT("Client"), *Item->ItemId.ToString());
	
	if (FItemTableRow* itemDataRow = itemDataTable->FindRow<FItemTableRow>(Item->ItemId, TEXT("Item Supply Lookup")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] 아이템 테이블 행 찾기 성공"),
			HasAuthority() ? TEXT("Server") : TEXT("Client"));

		// 디버그 모드: 아이템 테이블 행 찾기 성공 표시
		if (bDebugItemAssignment && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				314,
				10.0f,
				FColor::Green,
				FString::Printf(TEXT("[DEBUG] ✓ 아이템 테이블 행 찾기 성공: %s"), *Item->ItemId.ToString())
			);
		}

		if (UItemData* itemData = itemDataRow->itemData)
		{
			// Data Table의 racerIcon을 UItemData에 복사 (레이서 전용 아이콘)
			if (itemDataRow->racerIcon && !itemData->racerIcon)
			{
				itemData->racerIcon = itemDataRow->racerIcon;
				UE_LOG(LogTemp, Log, TEXT("[SupplyItemToRacer] racerIcon 복사 완료: %s -> %s"),
					*itemDataRow->racerIcon->GetName(), *itemData->GetName());
			}
			UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] itemData 찾기 성공"), 
				HasAuthority() ? TEXT("Server") : TEXT("Client"));
			
			// 디버그 모드: itemData 찾기 성공 표시
			if (bDebugItemAssignment && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					315,
					10.0f,
					FColor::Green,
					FString::Printf(TEXT("[DEBUG] ✓ itemData 찾기 성공: %s"), *itemData->ID.ToString())
				);
			}
			
			AAbstractRacer* RacerPawn = targetRacer->GetPawn<AAbstractRacer>();
			if (RacerPawn)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] 레이서 Pawn 찾기 성공: %s"), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"), *RacerPawn->GetName());
				UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] AcquireItem 호출 전 - LocalRole: %d, Authority: %d"), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"),
					(int32)GetLocalRole(), HasAuthority() ? 1 : 0);
				
				// 디버그 모드: AcquireItem 호출 전 정보 표시
				if (bDebugItemAssignment && GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						308,
						10.0f,
						FColor::Cyan,
						FString::Printf(TEXT("[DEBUG] 레이서 Pawn 찾기 성공: %s"), *RacerPawn->GetName())
					);
					GEngine->AddOnScreenDebugMessage(
						309,
						10.0f,
						FColor::Cyan,
						FString::Printf(TEXT("[DEBUG] 아이템 할당 시도: %s -> %s"), 
							*itemData->ID.ToString(), *targetRacer->GetPlayerInfo().playerName)
					);
				}
				
				UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] AcquireItem 호출 시도... TargetRacerIndex: %d, Racer: %s"), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"),
					TargetRacerIndex, *RacerPawn->GetName());
				RacerPawn->AcquireItem(itemData);
				
				UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] ✓ AcquireItem 호출 완료! TargetRacerIndex: %d, Racer: %s"), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"),
					TargetRacerIndex, *RacerPawn->GetName());
				
				// 디버그 모드: AcquireItem 호출 완료 표시
				if (bDebugItemAssignment && GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						310,
						10.0f,
						FColor::Green,
						FString::Printf(TEXT("[DEBUG] ✓✓✓ 아이템 할당 성공! %s에게 %s 전달됨"), 
							*targetRacer->GetPlayerInfo().playerName, *itemData->ID.ToString())
					);
				}
				
				// Commander Message 표시 - 레이서 이름과 아이템 이름 포함
				FString RacerName = targetRacer->GetPlayerInfo().playerName;
				FString FinalItemName = ItemName;
				
				// 아이템 데이터에서 ID가 있으면 사용 (더 명확한 이름)
				if (itemData && !itemData->ID.IsNone())
				{
					FinalItemName = itemData->ID.ToString();
				}
				
				UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] 메시지 표시 준비: RacerName=%s, ItemName=%s"), 
					*RacerName, *FinalItemName);
				
				// Commander에게 성공 메시지 표시
				TArray<FString> FormatArgs;
				FormatArgs.Add(RacerName);  // {0} - 레이서 이름
				FormatArgs.Add(FinalItemName);  // {1} - 아이템 이름
				
				if (!ShowCommanderMessage(Commander, ECommanderMessageID::Item_SupplySuccess, FormatArgs))
				{
					// Fallback: 화면 디버그 메시지
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(
							-1,
							5.0f,
							FColor::Green,
							FString::Printf(TEXT("✓ 아이템 할당 성공: %s에게 %s 전달"), *RacerName, *FinalItemName)
						);
					}
				}
				
				// 아이템이 성공적으로 할당되었으므로 PhysicsHandle Release 후 Destroy
				// PhysicsHandle로 붙잡고 있다면 Release (아이템 할당 성공 후)
				if (Commander->PhysicsHandle && Commander->PhysicsHandle->GetGrabbedComponent())
				{
					Commander->PhysicsHandle->ReleaseComponent();
					UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] PhysicsHandle Release 완료 (아이템 할당 성공 후)"));
				}
				
				// 아이템 Destroy
				UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] 아이템 할당 성공, Destroy: %s"), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"), *Item->GetName());
				
				// 디버그 모드: 아이템 Destroy 표시
				if (bDebugItemAssignment && GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						311,
						10.0f,
						FColor::Magenta,
						FString::Printf(TEXT("[DEBUG] 아이템 Destroy: %s"), *Item->GetName())
					);
				}
				
				Item->Destroy();
				
				// InsertedItem도 초기화
				InsertedItem = nullptr;
				
				UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] ===== 성공 완료 ====="), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"));
				
				// 디버그 모드: 최종 성공 표시
				if (bDebugItemAssignment && GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						312,
						10.0f,
						FColor::Green,
						TEXT("[DEBUG] ===== 아이템 할당 프로세스 완료 (성공) =====")
					);
				}
				return true;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[SupplyItemToRacer] [%s] 레이서 Pawn을 찾을 수 없습니다!"), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"));
				
				// 디버그 모드: 레이서 Pawn 없음 표시
				if (bDebugItemAssignment && GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						316,
						10.0f,
						FColor::Red,
						FString::Printf(TEXT("[DEBUG] ❌ 레이서 Pawn을 찾을 수 없습니다! (PlayerState: %s)"), 
							*targetRacer->GetName())
					);
					GEngine->AddOnScreenDebugMessage(
						317,
						10.0f,
						FColor::Yellow,
						TEXT("[DEBUG] ⚠ 아이템 보관 중 (레이서 Pawn 연결 대기)")
					);
				}
				
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						5.0f,
						FColor::Red,
						FString::Printf(TEXT("[ItemInputMachine] 레이서 Pawn을 찾을 수 없습니다! (PlayerState: %s)"), 
							*targetRacer->GetName())
					);
				}
				// 레이서 Pawn이 없으면 아이템을 Destroy하지 않고 보관 (나중에 다시 시도)
				// PhysicsHandle은 Release하지 않아서 grab 상태 유지
				UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] 레이서 Pawn 없음, 아이템 보관"), 
					HasAuthority() ? TEXT("Server") : TEXT("Client"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[SupplyItemToRacer] [%s] itemDataRow->itemData가 null입니다!"), 
				HasAuthority() ? TEXT("Server") : TEXT("Client"));
			
			// 디버그 모드: itemData null 표시
			if (bDebugItemAssignment && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					318,
					10.0f,
					FColor::Red,
					FString::Printf(TEXT("[DEBUG] ❌ itemDataRow->itemData가 null입니다! ItemId: %s"), *Item->ItemId.ToString())
				);
			}
			
			GEngine->AddOnScreenDebugMessage(
				201,
				5.0f,
				FColor::Red,
				FString::Printf(TEXT("[ItemInputMachine] 테이블엔 존재하나 내부에 아이템 데이터 없음 : %s"), *Item->ItemId.ToString())
			);
			// 아이템 데이터가 없으면 Destroy
			Item->Destroy();
			InsertedItem = nullptr;
			return false;
		}
	}
	else
	{
		// itemDataRow를 찾지 못한 경우
		UE_LOG(LogTemp, Error, TEXT("[SupplyItemToRacer] [%s] 아이템 테이블 행을 찾을 수 없습니다! ItemId: %s"), 
			HasAuthority() ? TEXT("Server") : TEXT("Client"), *Item->ItemId.ToString());
		
		// 디버그 모드: 테이블 행 없음 표시
		if (bDebugItemAssignment && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				319,
				10.0f,
				FColor::Red,
				FString::Printf(TEXT("[DEBUG] ❌ 아이템 테이블 행을 찾을 수 없습니다! ItemId: %s"), *Item->ItemId.ToString())
			);
		}
		
		GEngine->AddOnScreenDebugMessage(
			202,
			5.0f,
			FColor::Red,
			FString::Printf(TEXT("[ItemInputMachine] 테이블에 아이템 속성이 없음 : %s"), *Item->ItemId.ToString())
		);
		// 아이템 테이블 행이 없으면 Destroy
		Item->Destroy();
		InsertedItem = nullptr;
		return false;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[SupplyItemToRacer] [%s] ===== 완료 ====="), 
		HasAuthority() ? TEXT("Server") : TEXT("Client"));
	
	// 디버그 모드: 예상치 못한 종료 표시
	if (bDebugItemAssignment && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			320,
			10.0f,
			FColor::Red,
			TEXT("[DEBUG] ⚠ 예상치 못한 종료 - 정상적인 할당 경로를 거치지 않았습니다!")
		);
	}
	
	// 함수 끝에 도달한 경우 (예상치 못한 경우)
	return false;
}

bool AItemInputMachine::ShowCommanderMessage(ACommenderCharacter* Commander, ECommanderMessageID MessageID)
{
	if (!IsValid(Commander))
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemInputMachine] ShowCommanderMessage: Commander가 null입니다!"));
		return false;
	}

	if (!IsValid(Commander->CommenderHUDWidget))
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemInputMachine] ShowCommanderMessage: CommenderHUDWidget이 null입니다! Commander=%s"), *Commander->GetName());
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] ShowCommanderMessage: 일반 메시지 표시 (ID=%d)"), (int32)MessageID);
	Commander->CommenderHUDWidget->ShowMessageByID(MessageID);
	return true;
}

bool AItemInputMachine::ShowCommanderMessage(ACommenderCharacter* Commander, ECommanderMessageID MessageID, const TArray<FString>& FormatArgs)
{
	if (!IsValid(Commander))
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemInputMachine] ShowCommanderMessage: Commander가 null입니다!"));
		return false;
	}

	if (!IsValid(Commander->CommenderHUDWidget))
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemInputMachine] ShowCommanderMessage: CommenderHUDWidget이 null입니다! Commander=%s"), *Commander->GetName());
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[ItemInputMachine] ShowCommanderMessage: 포맷 메시지 표시 (ID=%d, Args=%d)"), 
		(int32)MessageID, FormatArgs.Num());
	Commander->CommenderHUDWidget->ShowMessageByIDWithFormat(MessageID, FormatArgs);
	return true;
}

bool AItemInputMachine::IsTargetRacerValid() const
{
	// TargetRacerIndex가 유효한 범위에 있는지 확인
	if (TargetRacerIndex < 0 || TargetRacerIndex >= 3)
	{
		return false;
	}
	
	// 실제로 레이서가 존재하는지 확인
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	
	ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>();
	if (!GameState)
	{
		return false;
	}
	
	// 레이서 목록 가져오기
	TArray<ACitRushPlayerState*> Racers = GameState->GetPlayerStatesByRole(EPlayerRole::Racer);
	if (Racers.Num() == 0)
	{
		return false;
	}
	
	// TargetRacerIndex를 ETargetRacer로 변환 (0=Racer1, 1=Racer2, 2=Racer3)
	ETargetRacer TargetRacerEnum = ETargetRacer::None;
	if (TargetRacerIndex == 0) TargetRacerEnum = ETargetRacer::Racer1;
	else if (TargetRacerIndex == 1) TargetRacerEnum = ETargetRacer::Racer2;
	else if (TargetRacerIndex == 2) TargetRacerEnum = ETargetRacer::Racer3;
	
	// PlayerInfo의 targetIndex가 일치하는 레이서 찾기
	for (ACitRushPlayerState* RacerPS : Racers)
	{
		if (!IsValid(RacerPS))
		{
			continue;
		}
		
		if (RacerPS->GetPlayerInfo().targetIndex == TargetRacerEnum)
		{
			return true; // 레이서를 찾았음
		}
	}
	
	return false; // 레이서를 찾지 못함
}
