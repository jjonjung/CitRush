// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/CommenderSystem/AVendingMachineBase.h"
#include "CommenderSystem/VendingItemActor.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "CommenderSystem/VendingItemListWidget.h"
#include "CommenderSystem/VendingSlotUIData.h"
#include "Interaction/InteractableComponent.h"
#include "Camera/CameraComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include "GameFlow/CitRushGameState.h"
#include "Player/CitRushPlayerState.h"

#include "Player/CommenderCharacter.h"
#include "Components/SceneComponent.h"

#include "Kismet/GameplayStatics.h"
#include "UI/CommenderHUDWidget.h"
#include "UI/CommanderMessageType.h"
#include "CommenderSystem/CommanderHelper.h"


// Sets default values
AVendingMachineBase::AVendingMachineBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root
	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	//본체
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyMesh->SetCollisionObjectType(ECC_WorldStatic);
	BodyMesh->SetCollisionResponseToAllChannels(ECR_Block);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMeshAsset(
	   TEXT("/Script/Engine.StaticMesh'/Game/CITRUSH/Commender/Apartment/Machine/VendingMachine_Body.VendingMachine_Body'")
	   );
		if (BodyMeshAsset.Succeeded())
		{
			BodyMesh->SetStaticMesh(BodyMeshAsset.Object);
		}

	
		PickupDoorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PickupDoorRoot"));
		PickupDoorRoot->SetupAttachment(BodyMesh);
		PickupDoorRoot->SetRelativeLocation(FVector(93.290196f, -107.208792f, 133.565723f));

		DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
		DoorMesh->SetupAttachment(PickupDoorRoot);
		DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DoorMesh->SetCollisionObjectType(ECC_WorldStatic);
		DoorMesh->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DoorMeshAsset(
	   TEXT("/Script/Engine.StaticMesh'/Game/CITRUSH/Commender/Apartment/Machine/VendingMachine_Door.VendingMachine_Door'")
	   );
		if (DoorMeshAsset.Succeeded())
		{
			DoorMesh->SetStaticMesh(DoorMeshAsset.Object);
		}

	GlassMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlassMesh"));
	GlassMesh->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GlassMeshAsset(
	   TEXT("/Script/Engine.StaticMesh'/Game/CITRUSH/Commender/Apartment/Machine/VendingMachine_glass.VendingMachine_glass'")
	   );
		if (GlassMeshAsset.Succeeded())
		{
			GlassMesh->SetStaticMesh(GlassMeshAsset.Object);
		}

	BarRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BarRoot"));
	BarRoot->SetupAttachment(RootComponent);

	BarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarMesh"));
	BarMesh->SetupAttachment(BarRoot);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BarMeshAsset(
	   TEXT("/Game/CITRUSH/Commender/Apartment/Machine/VendingMachine_Body.VendingMachine_Body")
	   );
		if (BarMeshAsset.Succeeded())
		{
			BarMesh->SetStaticMesh(BarMeshAsset.Object);
			BarMesh->SetRelativeLocation(FVector(-74.233422,0,-240.0));
			BarRoot->SetRelativeLocation(FVector(82.841206,0,-85.594222));
		}

	HandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandMesh"));
	HandMesh->SetupAttachment(BarMesh);

	HandAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HandAttachPoint"));
	HandAttachPoint->SetupAttachment(HandMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> HandMeshAsset(
	   TEXT("/Script/Engine.StaticMesh'/Game/CITRUSH/Commender/Apartment/Machine/VendingMachine_hand.VendingMachine_hand'")
	   );
	if (HandMeshAsset.Succeeded())
	{
		HandMesh->SetStaticMesh(HandMeshAsset.Object);
		HandAttachPoint->SetRelativeLocation(FVector(66.018722, 4.955141, 246.393496));
	}

	DropPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DropPoint"));
	DropPoint->SetupAttachment(RootComponent);
	DropPoint->SetRelativeLocation(FVector(70,-110,130));


	//UI
	ItemListWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("ItemListWidget"));
	ItemListWidgetComp->SetupAttachment(BodyMesh);
	
	ItemListWidgetComp->SetWidgetSpace(EWidgetSpace::World);
	ItemListWidgetComp->SetDrawSize(FVector2D(1000, 1000));
	ItemListWidgetComp->SetRelativeLocation(FVector(96,-101.0,360));
	ItemListWidgetComp->SetRelativeScale3D(FVector(1.0f, 0.11f, 0.1175f));
	ItemListWidgetComp->SetRelativeRotation(FRotator(0, 90, 0));
	
	// World Space 위젯에서 마우스 입력을 받기 위한 설정
	// UE 5.6에서는 UWidgetComponent가 기본적으로 World Space 입력을 지원합니다
	// Collision 설정으로 Visibility 채널에 대한 충돌만 활성화
	ItemListWidgetComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ItemListWidgetComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	ItemListWidgetComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// PickupBox - 아이템 픽업 영역
	PickupBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PickupBox"));
	PickupBox->SetupAttachment(RootComponent);
	PickupBox->SetRelativeLocation(FVector(70, -110, 130));  
	PickupBox->SetBoxExtent(FVector(30.f, 30.f, 30.f));
	PickupBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupBox->SetGenerateOverlapEvents(true);

	// DropPointCheckBox - 자판기 아이템 영역 체크용
	DropPointCheckBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DropPointCheckBox"));
	DropPointCheckBox->SetupAttachment(RootComponent);
	DropPointCheckBox->SetRelativeLocation(FVector(70, -110, 130)); // DropPoint와 같은 위치
	DropPointCheckBox->SetBoxExtent(FVector(40.f, 40.f, 40.f)); // 넉넉하게 설정
	DropPointCheckBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DropPointCheckBox->SetCollisionObjectType(ECC_WorldStatic);
	DropPointCheckBox->SetCollisionResponseToAllChannels(ECR_Ignore); // 모든 충돌 무시
	DropPointCheckBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap); // 아이템만 오버랩
	DropPointCheckBox->SetGenerateOverlapEvents(true);

	// InteractableComponent - 상호작용 시스템
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
	
	// Outline 효과 활성화
	InteractableComponent->feedbackSettings.EnableOutline(true);
	InteractableComponent->feedbackSettings.bUseOverlayMaterial = true;
}

void AVendingMachineBase::BeginPlay()
{
	Super::BeginPlay();

	// Commander 초기화 (나중에 스폰될 수 있으므로 헬퍼 함수로 처리)
	UpdateCachedCommander();

	// 슬롯 위치 찾기
	SlotTargets.Reset();

	TArray<UActorComponent*> Components;
	GetComponents(Components);

	static const FName SlotTag = TEXT("VendingSlot");

	for (UActorComponent* Comp : Components)
	{
		if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
		{
			if (SceneComp->ComponentHasTag(SlotTag))
			{
				SlotTargets.Add(SceneComp);
			}
		}
	}

	// 초기 위치 저장
	if (BarRoot)
	{
		BarBaseLocation = BarRoot->GetComponentLocation();
	}

	if (HandMesh)
	{
		HandBaseRelativeLocation = HandMesh->GetRelativeLocation();
	}

	SlotItemStacks.SetNum(SlotTargets.Num());

	// 아이템 스폰
	if (!DefaultItemClass)
		return;

	for (int32 SlotIdx = 0; SlotIdx < SlotTargets.Num(); ++SlotIdx)
	{
		USceneComponent* Slot = SlotTargets[SlotIdx];
		if (!Slot) continue;

		const FVector SlotLoc  = Slot->GetComponentLocation();
		const FRotator SlotRot = Slot->GetComponentRotation();

		const bool bHasConfig = SlotConfigs.IsValidIndex(SlotIdx);
		const FVendingSlotConfig* Config = bHasConfig ? &SlotConfigs[SlotIdx] : nullptr;

		TSubclassOf<AVendingItemActor> ClassToSpawn =
			(Config && Config->ItemClass) ? Config->ItemClass : DefaultItemClass;

		if (!ClassToSpawn)
			continue;

		const FVector ForwardDir = Slot->GetForwardVector();

		for (int32 i = 0; i < ItemsPerSlot; ++i)
		{
			const FVector SpawnLoc = SlotLoc + ForwardDir * (ItemSpacing * i);
			FRotator SpawnRot = SlotRot;
			if (Config)
				SpawnRot += Config->RotationOffset;

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AVendingItemActor* NewItem = GetWorld()->SpawnActor<AVendingItemActor>(
				ClassToSpawn, SpawnLoc, SpawnRot, Params);

			if (NewItem)
			{
				NewItem->InitItem(NewItem->ItemId, NewItem->Coin, SlotIdx, i);
				if (Config)
					NewItem->SetActorScale3D(Config->Scale);
				SlotItemStacks[SlotIdx].Items.Add(NewItem);
			}
		}
	}

	// UI 위젯 초기화
	if (UUserWidget* WidgetObj = ItemListWidgetComp->GetUserWidgetObject())
	{
		if (UVendingItemListWidget* ListWidget = Cast<UVendingItemListWidget>(WidgetObj))
		{
			TArray<UVendingSlotUIData*> UIDataArray;

			for (int32 SlotIdx = 0; SlotIdx < SlotItemStacks.Num(); ++SlotIdx)
			{
				int32 ItemCount = SlotItemStacks[SlotIdx].Items.Num();
				bool bIsEmpty = (ItemCount == 0);
				
				// 빈 슬롯도 UI에 표시하되 비활성화 상태로
				UVendingSlotUIData* Data = NewObject<UVendingSlotUIData>(this);
				Data->SlotIndex = SlotIdx;
				Data->bIsEmpty = bIsEmpty;
				Data->RemainingCount = ItemCount;
				
				if (!bIsEmpty)
				{
					AVendingItemActor* TopItem = SlotItemStacks[SlotIdx].Items.Last();
					if (TopItem)
					{
						Data->ItemName  = FText::FromName(TopItem->ItemId);
						Data->Coin      = TopItem->Coin;
						Data->Icon      = TopItem->ItemIcon;
					}
				}
				else
				{
					// 빈 슬롯 표시
					Data->ItemName = FText::FromString(TEXT("Empty"));
					Data->Coin = 0;
				}

				UIDataArray.Add(Data);
			}

			ListWidget->InitializeSlots(UIDataArray);
			ListWidget->OnSlotSelected.AddDynamic(this, &AVendingMachineBase::OnUISlotSelected);
		}
	}

	// PickupBox Overlap 이벤트 바인딩
	if (PickupBox)
	{
		PickupBox->OnComponentBeginOverlap.AddDynamic(this, &AVendingMachineBase::OnPickupBoxBeginOverlap);
		PickupBox->OnComponentEndOverlap.AddDynamic(this, &AVendingMachineBase::OnPickupBoxEndOverlap);
	}

	// InteractableComponent 초기화
	if (InteractableComponent)
	{
		// E키 또는 클릭으로 아이템 픽업 시도
		InteractableComponent->OnClientInteraction.AddDynamic(this, &AVendingMachineBase::PickupItem);
		
		// InteractionSphere를 PickupBox에 부착 (회전은 유지)
		TArray<USphereComponent*> SphereComponents;
		GetComponents<USphereComponent>(SphereComponents);
		for (USphereComponent* Sphere : SphereComponents)
		{
			if (Sphere && Sphere->GetName().Contains(TEXT("InteractionSphere")))
			{
				// 위치만 Snap하고 회전과 스케일은 유지
				FAttachmentTransformRules AttachRules(
					EAttachmentRule::SnapToTarget,  // Location
					EAttachmentRule::KeepWorld,      // Rotation (회전 유지)
					EAttachmentRule::KeepWorld,      // Scale (스케일 유지)
					false
				);
				Sphere->AttachToComponent(PickupBox, AttachRules);
				break;
			}
		}
		
		// 위치 업데이트
		UpdateInteractionSphereLocation();
		
		// UI 위젯 크기 및 회전 조정
		TArray<UWidgetComponent*> WidgetComponents;
		GetComponents<UWidgetComponent>(WidgetComponents);
		for (UWidgetComponent* Widget : WidgetComponents)
		{
			if (Widget && Widget->GetName().Contains(TEXT("GuideWidgetComponent")))
			{
				Widget->SetRelativeScale3D(FVector(InteractionUIScale));
		
				break;
			}
		}
	}

	if (bEnableTestMode)
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, 
			&AVendingMachineBase::AutoSetupTestItem, 0.5f, false);
	}
}

void AVendingMachineBase::OnUISlotSelected(int32 SlotIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[VendingMachine] OnUISlotSelected 호출: SlotIndex=%d"), SlotIndex);
	
	// Commander가 없거나 HUD 위젯이 초기화되지 않았으면 업데이트 시도
	if (!IsValid(CachedCommander) || !IsValid(CachedCommander->CommenderHUDWidget))
	{
		UE_LOG(LogTemp, Log, TEXT("[VendingMachine] Commander 또는 HUD 위젯이 없어서 업데이트 시도"));
		UpdateCachedCommander();
	}

	// Commander가 여전히 없으면 로그 출력 후 종료 (HUD 위젯은 선택적)
	if (!IsValid(CachedCommander))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingMachine] Commander를 찾을 수 없습니다. 자판기 기능을 사용할 수 없습니다."));
		return;
	}
	
	// HUD 위젯이 없어도 작동은 가능하도록 (메시지 표시만 안 됨)
	if (!IsValid(CachedCommander->CommenderHUDWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingMachine] HUD 위젯이 초기화되지 않았습니다. 메시지 표시는 안 되지만 작동은 계속합니다."));
	}

	// 자판기 작동 중인지 확인 (Bar 이동, Hand 이동, 아이템 드롭 중이면 구매 불가)
	if (bMoveBar || bMoveHand || bDroppingItem || bWaitingToGrabItem || GrabbedItem != nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("[VendingMachine] 자판기가 작동 중입니다. bMoveBar=%d, bMoveHand=%d, bDroppingItem=%d, bWaitingToGrabItem=%d, GrabbedItem=%p"), 
			bMoveBar, bMoveHand, bDroppingItem, bWaitingToGrabItem, GrabbedItem.Get());
		// 작동 중 - UI 메시지 표시
		if (IsValid(CachedCommander->CommenderHUDWidget))
		{
			CachedCommander->CommenderHUDWidget->ShowMessageByID(ECommanderMessageID::VendingMachine_Busy);
		}
		return;
	}

	// 슬롯 유효성 체크
	if (!SlotItemStacks.IsValidIndex(SlotIndex))
		return;

	const auto& Stack = SlotItemStacks[SlotIndex];
	if (Stack.Items.Num() == 0)
		return;

	// 아이템 가격 확인
	AVendingItemActor* TopItem = Stack.Items.Last();
	if (!TopItem)
		return;

	int32 ItemPrice = TopItem->Coin;

	// 코인 체크 및 차감
	if (!CachedCommander->HasEnoughCoin(ItemPrice))
	{
		// 코인 부족 - UI 메시지 표시
		if (IsValid(CachedCommander->CommenderHUDWidget))
		{
			CachedCommander->CommenderHUDWidget->ShowMessageByID(ECommanderMessageID::VendingMachine_InsufficientCoin);
		}
		return; // 코인 부족 시 구매 중단
	}

	// 코인 차감
	CachedCommander->SpendCoin(ItemPrice);
	
	UE_LOG(LogTemp, Log, TEXT("[VendingMachine] 코인 차감 완료. MoveToSlot 호출: SlotIndex=%d"), SlotIndex);
	
	// 구매 성공 - 아이템 배출
	MoveToSlot(SlotIndex);
}

void AVendingMachineBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// InteractionSphere 위치 업데이트
	UpdateInteractionSphereLocation();
	
	// UI 위젯 스케일 업데이트
	TArray<UWidgetComponent*> WidgetComponents;
	GetComponents<UWidgetComponent>(WidgetComponents);
	for (UWidgetComponent* Widget : WidgetComponents)
	{
		if (Widget && Widget->GetName().Contains(TEXT("GuideWidgetComponent")))
		{
			Widget->SetRelativeScale3D(FVector(InteractionUIScale));
			break;
		}
	}
}

#if WITH_EDITOR
void AVendingMachineBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// InteractionSphereOffset이 변경되었을 때만 업데이트
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AVendingMachineBase, InteractionSphereOffset))
	{
		UpdateInteractionSphereLocation();
	}
}
#endif

void AVendingMachineBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Bar 이동
	if (bMoveBar && BarRoot)
	{
		const FVector Current = BarRoot->GetComponentLocation();
		const float DistanceSq = FVector::DistSquared(Current, BarTargetLocation);
		const float ToleranceSq = BarMoveTolerance * BarMoveTolerance;
		
		if (DistanceSq <= ToleranceSq)
		{
			BarRoot->SetWorldLocation(BarTargetLocation);
			bMoveBar = false;

			if (bWaitingToGrabItem && !bDroppingItem && !bMoveHand)
				GrabItemFromSlot();
			else if (bDroppingItem && GrabbedItem)
				ReleaseItemAtDropPoint();
		}
		else
		{
			const float Distance = FMath::Sqrt(DistanceSq);
			const float AdaptiveSpeed = Distance < BarMoveNearDistance 
				? BarMoveSpeed * BarMoveNearSpeedMultiplier 
				: BarMoveSpeed;
			
			const FVector NewLoc = FMath::VInterpTo(Current, BarTargetLocation, DeltaSeconds, AdaptiveSpeed);
			BarRoot->SetWorldLocation(NewLoc);
		}
	}

	// Hand 이동
	if (bMoveHand && HandMesh)
	{
		const FVector CurRel = HandMesh->GetRelativeLocation();
		const float DistanceSq = FVector::DistSquared(CurRel, HandTargetRelativeLocation);
		const float ToleranceSq = HandMoveTolerance * HandMoveTolerance;
		
		if (DistanceSq <= ToleranceSq)
		{
			HandMesh->SetRelativeLocation(HandTargetRelativeLocation);
			bMoveHand = false;

			if (bWaitingToGrabItem && !bDroppingItem)
				GrabItemFromSlot();
			else if (bDroppingItem)
				ReleaseItemAtDropPoint();
			else if (!bWaitingToGrabItem && !bDroppingItem && !GrabbedItem)
				bIsProcessing = false;
		}
		else
		{
			const float Distance = FMath::Sqrt(DistanceSq);
			float CurrentSpeed;
			const float FinalDistance = HandMoveTolerance * HandMoveFinalDistanceMultiplier;
			
			if (Distance < FinalDistance)
				CurrentSpeed = HandMoveDecelerationSpeed * HandMoveFinalSpeedMultiplier;
			else if (Distance < HandMoveDecelerationDistance)
				CurrentSpeed = HandMoveDecelerationSpeed;
			else
				CurrentSpeed = HandMoveSpeed;

			const FVector NewRel = FMath::VInterpTo(CurRel, HandTargetRelativeLocation, DeltaSeconds, CurrentSpeed);
			HandMesh->SetRelativeLocation(NewRel);
		}
	}
}

void AVendingMachineBase::MoveToSlot(int32 SlotIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[VendingMachine] MoveToSlot 호출: SlotIndex=%d, bIsProcessing=%d"), SlotIndex, bIsProcessing);
	
	if (bIsProcessing)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingMachine] 이미 처리 중입니다. MoveToSlot 취소"));
		return;
	}

	if (!SlotTargets.IsValidIndex(SlotIndex) || !SlotTargets[SlotIndex])
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingMachine] 유효하지 않은 SlotIndex: %d (SlotTargets.Num()=%d)"), SlotIndex, SlotTargets.Num());
		return;
	}

	if (SlotItemStacks.IsValidIndex(SlotIndex))
	{
		const auto& Stack = SlotItemStacks[SlotIndex];
		if (Stack.Items.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[VendingMachine] 슬롯 %d에 아이템이 없습니다."), SlotIndex);
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[VendingMachine] MoveToSlot 시작: SlotIndex=%d"), SlotIndex);
	
	bIsProcessing = true;
	CurrentSlotIndex = SlotIndex;
	bWaitingToGrabItem = true;
	bDroppingItem = false;

	USceneComponent* SlotComp = SlotTargets[SlotIndex];
	const FVector SlotWorld = SlotComp->GetComponentLocation();

	// Bar 이동 설정
	bool bBarImmediateMove = false;
	if (BarRoot)
	{
		const FVector BarWorld = BarRoot->GetComponentLocation();
		BarTargetLocation = FVector(BarWorld.X, BarWorld.Y, SlotWorld.Z);
		const float BarDistance = FVector::Dist(BarWorld, BarTargetLocation);

		if (BarDistance <= ImmediateMoveDistance)
		{
			BarRoot->SetWorldLocation(BarTargetLocation);
			bMoveBar = false;
			bBarImmediateMove = true;
		}
		else
		{
			bMoveBar = true;
			UE_LOG(LogTemp, Log, TEXT("[VendingMachine] Bar 이동 시작: 거리=%.2f"), BarDistance);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingMachine] BarRoot가 없습니다!"));
	}

	// Hand 이동 설정
	bool bHandImmediateMove = false;
	if (HandMesh && BarMesh)
	{
		const FTransform BarMeshTransform = BarMesh->GetComponentTransform();
		const FVector SlotInBarSpace = BarMeshTransform.InverseTransformPosition(SlotWorld);

		HandTargetRelativeLocation = HandBaseRelativeLocation;
		HandTargetRelativeLocation.Y = SlotInBarSpace.Y;

		const FVector CurrentHandRel = HandMesh->GetRelativeLocation();
		const float HandDistance = FVector::Dist(CurrentHandRel, HandTargetRelativeLocation);

		if (HandDistance <= ImmediateMoveDistance)
		{
			HandMesh->SetRelativeLocation(HandTargetRelativeLocation);
			bMoveHand = false;
			bHandImmediateMove = true;
		}
		else
		{
			bMoveHand = true;
			UE_LOG(LogTemp, Log, TEXT("[VendingMachine] Hand 이동 시작: 거리=%.2f"), HandDistance);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingMachine] HandMesh 또는 BarMesh가 없습니다! HandMesh=%p, BarMesh=%p"), HandMesh.Get(), BarMesh.Get());
	}

	if (bBarImmediateMove && bHandImmediateMove && bWaitingToGrabItem)
	{
		UE_LOG(LogTemp, Log, TEXT("[VendingMachine] 즉시 이동 완료. GrabItemFromSlot 호출"));
		GrabItemFromSlot();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[VendingMachine] 이동 대기 중: bBarImmediateMove=%d, bHandImmediateMove=%d, bWaitingToGrabItem=%d"), 
			bBarImmediateMove, bHandImmediateMove, bWaitingToGrabItem);
	}
}

void AVendingMachineBase::GrabItemFromSlot()
{
	bWaitingToGrabItem = false;

	if (GrabbedItem)
		return;

	if (!SlotItemStacks.IsValidIndex(CurrentSlotIndex))
		return;

	auto& Stack = SlotItemStacks[CurrentSlotIndex];

	if (Stack.Items.Num() == 0)
	{
		bIsProcessing = false;
		return;
	}

	GrabbedItem = Stack.Items.Pop();
	if (!GrabbedItem)
		return;

	// 슬롯이 비었는지 확인하고 UI 업데이트
	int32 RemainingCount = Stack.Items.Num();
	bool bSlotEmpty = (RemainingCount == 0);
	
	// UI 위젯에 상태 업데이트
	if (UUserWidget* WidgetObj = ItemListWidgetComp->GetUserWidgetObject())
	{
		if (UVendingItemListWidget* ListWidget = Cast<UVendingItemListWidget>(WidgetObj))
		{
			ListWidget->UpdateSlotStatus(CurrentSlotIndex, bSlotEmpty, RemainingCount);
		}
	}

	// 물리 비활성화
	TArray<UStaticMeshComponent*> MeshComps;
	GrabbedItem->GetComponents<UStaticMeshComponent>(MeshComps);
	if (MeshComps.Num() > 0 && MeshComps[0])
	{
		UStaticMeshComponent* MeshComp = MeshComps[0];
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetEnableGravity(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Hand에 부착
	GrabbedItem->AttachToComponent(
		HandAttachPoint,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale
	);

	// 드롭 포인트로 이동
	if (DropPoint && BarRoot)
	{
		bDroppingItem = true;
		const FVector DropWorld = DropPoint->GetComponentLocation();
		const FVector BarCurrent = BarRoot->GetComponentLocation();
		BarTargetLocation = FVector(BarCurrent.X, BarCurrent.Y, DropWorld.Z);
		bMoveBar = true;
	}
}

void AVendingMachineBase::ReleaseItemAtDropPoint()
{
	bDroppingItem = false;

	if (!GrabbedItem)
		return;

	// Hand에서 분리
	const FVector CurrentWorldLocation = GrabbedItem->GetActorLocation();

	USceneComponent* RootComp = GrabbedItem->GetRootComponent();
	if (RootComp && RootComp->GetAttachParent())
		RootComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	else
		GrabbedItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 물리 활성화 및 충돌 설정
	TArray<UStaticMeshComponent*> MeshComps;
	GrabbedItem->GetComponents<UStaticMeshComponent>(MeshComps);
	if (MeshComps.Num() > 0 && MeshComps[0])
	{
		UStaticMeshComponent* MeshComp = MeshComps[0];
		
		// 1. 먼저 물리 비활성화 (충돌 설정 전에)
		MeshComp->SetSimulatePhysics(false);
		
		// 2. Collision 설정 (바닥과 충돌하도록 명시적으로 설정)
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
		MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 캐릭터와 충돌 무시
		MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // 바닥과 충돌 (명시적)
		MeshComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // 동적 오브젝트와 충돌
		
		// 3. 충돌 업데이트 강제 (충돌이 제대로 적용되도록)
		MeshComp->UpdateBounds();
		MeshComp->UpdateOverlaps();
		
		// 4. 드롭 포인트에 배치 (Sweep을 사용하여 충돌 체크)
		FVector TargetLocation = DropPoint ? DropPoint->GetComponentLocation() : CurrentWorldLocation;
		FHitResult HitResult;
		GrabbedItem->SetActorLocation(TargetLocation, true, &HitResult, ETeleportType::None);
		
		// 5. 물리 활성화 (충돌 설정 후)
		MeshComp->SetSimulatePhysics(true);
		MeshComp->SetEnableGravity(true);
		
		// 6. 물리 업데이트 강제 (충돌이 제대로 적용되도록)
		MeshComp->WakeAllRigidBodies();
		MeshComp->UpdateBounds();
	}

	DroppedItem = GrabbedItem;
	if (DroppedItem)
		DroppedItem->SetVendingMachine(this);

	GrabbedItem = nullptr;

	// 원위치로 복귀
	if (HandMesh)
	{
		HandTargetRelativeLocation = HandBaseRelativeLocation;
		bMoveHand = true;
	}
	else
	{
		bIsProcessing = false;
	}

	if (BarRoot)
	{
		BarTargetLocation = BarBaseLocation;
		bMoveBar = true;
	}
}

void AVendingMachineBase::OnItemHitGround(AVendingItemActor* Item)
{
	if (!Item || Item != DroppedItem)
		return;

	Item->Destroy();
	DroppedItem = nullptr;
	SpawnItemAtDropPoint();
}

void AVendingMachineBase::SpawnItemAtDropPoint()
{
	if (!DropPoint || !DefaultItemClass)
		return;

	const FVector DropWorld = DropPoint->GetComponentLocation();
	const FRotator DropRot = DropPoint->GetComponentRotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AVendingItemActor* NewItem = GetWorld()->SpawnActor<AVendingItemActor>(
		DefaultItemClass, DropWorld, DropRot, Params);

	if (NewItem)
	{
		NewItem->InitItem(NewItem->ItemId, NewItem->Coin, INDEX_NONE, INDEX_NONE);
		NewItem->SetVendingMachine(this);

		TArray<UStaticMeshComponent*> MeshComps;
		NewItem->GetComponents<UStaticMeshComponent>(MeshComps);
		if (MeshComps.Num() > 0 && MeshComps[0])
		{
			UStaticMeshComponent* MeshComp = MeshComps[0];
			
			// 1. 먼저 물리 비활성화 (충돌 설정 전에)
			MeshComp->SetSimulatePhysics(false);
			
			// 2. Collision 설정 (바닥과 충돌하도록 명시적으로 설정)
			MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
			MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
			MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 캐릭터와 충돌 무시
			MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // 바닥과 충돌 (명시적)
			MeshComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // 동적 오브젝트와 충돌
			
			// 3. 충돌 업데이트 강제 (충돌이 제대로 적용되도록)
			MeshComp->UpdateBounds();
			MeshComp->UpdateOverlaps();
			
			// 4. 위치 재설정 (Sweep을 사용하여 충돌 체크)
			FHitResult HitResult;
			NewItem->SetActorLocation(DropWorld, true, &HitResult, ETeleportType::None);
			
			// 5. 물리 활성화 (충돌 설정 후)
			MeshComp->SetSimulatePhysics(true);
			MeshComp->SetEnableGravity(true);
			
			// 6. 물리 업데이트 강제 (충돌이 제대로 적용되도록)
			MeshComp->WakeAllRigidBodies();
			MeshComp->UpdateBounds();
		}

		DroppedItem = NewItem;
		if (InteractableComponent && !InteractableComponent->IsActive())
		{
			InteractableComponent->SetActive(true);
		}
	}
}

void AVendingMachineBase::PickupItem(APlayerController* PlayerController)
{
	// 상호작용 시 호출됨 (현재는 비활성화)
}

AVendingItemActor* AVendingMachineBase::FindItemInDropPointBox() const
{
	// 우선 DroppedItem이 있으면 우선 사용
	if (IsValid(DroppedItem))
	{
		return DroppedItem;
	}

	if (!DropPointCheckBox)
	{
		return nullptr;
	}

	// DropPointCheckBox 안에 오버랩 중인 VendingItemActor 찾기
	TArray<AActor*> OverlappingActors;
	DropPointCheckBox->GetOverlappingActors(OverlappingActors, AVendingItemActor::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (AVendingItemActor* Item = Cast<AVendingItemActor>(Actor))
		{
			return Item;
		}
	}

	return nullptr;
}

void AVendingMachineBase::UpdateInteractionSphereLocation()
{
	// InteractionSphere를 찾아서 위치 업데이트
	TArray<USphereComponent*> SphereComponents;
	GetComponents<USphereComponent>(SphereComponents);
	for (USphereComponent* Sphere : SphereComponents)
	{
		if (Sphere && Sphere->GetName().Contains(TEXT("InteractionSphere")))
		{
			Sphere->SetRelativeLocation(InteractionSphereOffset);
			break;
		}
	}
}

void AVendingMachineBase::UpdateCachedCommander()
{
	// 이미 유효한 Commander가 캐시되어 있으면 업데이트하지 않음
	if (IsValid(CachedCommander))
	{
		return;
	}

	// 공통 헬퍼 함수 사용 (최적화)
	CachedCommander = UCommanderHelper::FindCommander(GetWorld(), false);
	
#if !UE_BUILD_SHIPPING
	if (CachedCommander)
	{
		UE_LOG(LogTemp, Log, TEXT("[VendingMachine] Commander 캐시 완료. HUD 위젯=%p"), CachedCommander->CommenderHUDWidget.Get());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VendingMachine] Commander를 찾을 수 없습니다."));
	}
#endif
}

void AVendingMachineBase::AutoSetupTestItem()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVendingItemActor::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		AVendingItemActor* TestItem = Cast<AVendingItemActor>(FoundActors[0]);
		if (TestItem)
		{
			SetTestItem(TestItem);
		}
	}
}

void AVendingMachineBase::SetTestItem(AVendingItemActor* TestItem)
{
	if (!IsValid(TestItem))
		return;

	if (DroppedItem)
	{
		DroppedItem->Destroy();
	}

	DroppedItem = TestItem;
	DroppedItem->SetVendingMachine(this);

	// 드랍 포인트 근처인지 확인 후 뷰포트에 표시
	if (DropPoint && GEngine)
	{
		const float Dist = FVector::Dist(
			DroppedItem->GetActorLocation(),
			DropPoint->GetComponentLocation()
		);

		if (Dist < 60.0f)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Cyan,
				TEXT("아이템이 DropPoint 영역에 있습니다")
			);
		}
	}
}

void AVendingMachineBase::DebugPrintStatus()
{
	// 디버그 출력 제거됨
}

void AVendingMachineBase::OnPickupBoxBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (ACommenderCharacter* Commander = Cast<ACommenderCharacter>(OtherActor))
	{
		Commander->FocusedVending = this;

		// Commander가 상호작용 영역에 들어왔을 때 직접 캐시 (가장 확실한 방법)
		if (IsValid(Commander))
		{
			CachedCommander = Commander;
			UE_LOG(LogTemp, Log, TEXT("[VendingMachine] OnPickupBoxBeginOverlap에서 Commander 캐시 완료. HUD 위젯=%p"), Commander->CommenderHUDWidget.Get());
		}
		else if (!IsValid(CachedCommander))
		{
			// 백업: UpdateCachedCommander 호출
			UpdateCachedCommander();
		}

		// 캐릭터가 PickupBox 안으로 들어왔음을 뷰포트에 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				1.5f,
				FColor::Cyan,
				TEXT("Pickup 영역에 들어왔습니다")
			);
		}
	}
}

void AVendingMachineBase::OnPickupBoxEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (ACommenderCharacter* Commander = Cast<ACommenderCharacter>(OtherActor))
	{
		if (Commander->FocusedVending == this)
		{
			// PickupBox에서 나감
			Commander->FocusedVending = nullptr;
				
				
		}
	}
}
