// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CommenderCharacter.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Camera/CameraComponent.h"
#include "CommenderSystem/VendingItemActor.h"
#include "CommenderSystem/AVendingMachineBase.h"
#include "CommenderSystem/ItemInputMachine.h"
#include "UI/CommenderHUDWidget.h"
#include "UI/CommanderMessageType.h"
#include "UI/CrosshairWidget.h"
#include "Interaction/InteractableComponent.h"
#include "Player/Stats/MapInteractionActor.h"
#include "Player/CCTV/MonitorActor.h"
#include "Components/PhysicsGrabableComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFlow/CitRushGameState.h"
#include "Player/Stats/MapBoundsActor.h"
#include "Player/CitRushPlayerState.h"
#include "Player/AbstractRacer.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"

#include "Utility/WidgetBlueprintLoader.h"

// Sets default values
ACommenderCharacter::ACommenderCharacter()
{
	// Tick 비활성화 (타이머 기반으로 변경)
	PrimaryActorTick.bCanEverTick = false;

	// WidgetInteractionComponent 생성
	WidgetInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("WidgetInteraction"));
	WidgetInteraction->InteractionDistance = 500.f;
	WidgetInteraction->bShowDebug = true;

	// PhysicsHandle 생성 (물리 오브젝트 잡기용)
	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	// 아이템 홀드용 소켓 생성
	ItemHoldSocket = CreateDefaultSubobject<USceneComponent>(TEXT("ItemHoldSocket"));
	ItemHoldSocket->SetupAttachment(GetFirstPersonCameraComponent());

	const UWidgetBlueprintLoader* wbpLoader = UWidgetBlueprintLoader::Get();
	const UWidgetBlueprintDataAsset* wbp = wbpLoader->PDA_WBP.LoadSynchronous();
	CommenderHUDWidgetClass = wbp->GetWidgetBlueprintClassByKey(TEXT("CommanderUHD"));
}

// Called when the game starts or when spawned
void ACommenderCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// CommanderAnchor 태그 추가 (맵 위젯에서 Commander 위치 표시용)
	Tags.AddUnique(TEXT("CommanderAnchor"));
	
	// ItemHoldSocket Offset 위치 설정 (이미 Constructor에서 Camera에 Attach됨)
	if (ItemHoldSocket)
	{
		ItemHoldSocket->SetRelativeLocation(ItemHoldSocketOffset);
	}

	// GrabAction 상태 확인 (디버그 빌드에서만)
#if !UE_BUILD_SHIPPING
	if (!GrabAction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderCharacter] GrabAction이 NULL입니다! 블루프린트에서 설정하세요!"));
	}
#endif

	// WidgetInteraction을 카메라에 부착
	if (UCameraComponent* Camera = GetFirstPersonCameraComponent())
	{
		if (WidgetInteraction)
		{
			WidgetInteraction->AttachToComponent(Camera, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}

	if (APlayerController* pc = GetController<APlayerController>())
	{
		CommenderHUDWidget = CreateWidget<UCommenderHUDWidget>(pc, CommenderHUDWidgetClass);
		if (IsValid(CommenderHUDWidget))
		{
			CommenderHUDWidget->AddToViewport();
			UE_LOG(LogTemp, Log, TEXT("[CommenderCharacter] CommenderHUDWidget 생성 및 Viewport 추가 완료"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CommenderCharacter] CommenderHUDWidget 생성 실패! CommenderHUDWidgetClass가 설정되지 않았을 수 있습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderCharacter] PlayerController를 찾을 수 없습니다."));
	}

	// Grab 가능한 아이템 확인을 위한 타이머 설정 (Tick 대신)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GrabCheckTimerHandle,
			this,
			&ACommenderCharacter::CheckGrabableItem,
			LineTraceInterval,
			true // 반복
		);

		// PhysicsHandle 업데이트를 위한 타이머 설정 (Tick 대신, 짧은 간격으로)
		World->GetTimerManager().SetTimer(
			PhysicsHandleUpdateTimerHandle,
			this,
			&ACommenderCharacter::UpdatePhysicsHandle,
			0.01f, // 0.01초마다 업데이트 (100Hz)
			true // 반복
		);
	}
}

void ACommenderCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// 소켓 위치를 Offset 값으로 업데이트
	if (ItemHoldSocket)
	{
		ItemHoldSocket->SetRelativeLocation(ItemHoldSocketOffset);
	}
}

#if WITH_EDITOR
void ACommenderCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// ItemHoldSocketOffset이 변경되었을 때 실제 컴포넌트 위치도 업데이트
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ACommenderCharacter, ItemHoldSocketOffset))
	{
		if (ItemHoldSocket)
		{
			ItemHoldSocket->SetRelativeLocation(ItemHoldSocketOffset);
		}
	}
}
#endif

// PhysicsHandle 업데이트 (타이머 기반, Tick 대신)
void ACommenderCharacter::UpdatePhysicsHandle()
{
	// GrabbedItem 업데이트 (PhysicsHandle이 있을 때만)
	// Component Tag로 grab한 경우도 고려하여 PhysicsHandle이 있으면 GrabbedItem 업데이트
	if (PhysicsHandle && PhysicsHandle->GetGrabbedComponent())
	{
		// VendingItemActor인 경우에만 GrabbedItem 설정 (기존 호환성)
		GrabbedItem = GetGrabbedVendingItem();
	}
	else
	{
		// PhysicsHandle이 없으면 GrabbedItem도 nullptr로 설정
		GrabbedItem = nullptr;
	}

	// PhysicsHandle로 아이템을 잡고 있으면 목표 위치 업데이트
	if (PhysicsHandle && PhysicsHandle->GetGrabbedComponent() && ItemHoldSocket)
	{
		FVector TargetLocation = ItemHoldSocket->GetComponentLocation();
		FRotator TargetRotation = ItemHoldSocket->GetComponentRotation();
		PhysicsHandle->SetTargetLocationAndRotation(TargetLocation, TargetRotation);
		
		// 아이템 충돌 체크
		CheckItemCollision();
		
		// 아이템 위치가 변경되었으므로 크로스헤어 업데이트 (아이템 사용 가능 여부 체크)
		UpdateCrosshair(false); // LineTrace는 CheckGrabableItem에서 처리
	}
	else
	{
		// 아이템을 들고 있지 않으면 충돌 상태 리셋
		if (bItemColliding)
		{
			bItemColliding = false;
			CollisionNormal = FVector::ZeroVector;
		}
	}
}

// Called to bind functionality to input
void ACommenderCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Enhanced Input으로 아이템 집기/놓기 바인딩
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (GrabAction)
		{
			EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Started, this, &ACommenderCharacter::OnGrabItemStarted);
			EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Completed, this, &ACommenderCharacter::OnGrabItemCompleted);
		}

		if (UseItemAction)
		{
			EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Started, this, &ACommenderCharacter::OnUseItemPressed);
		}
#if !UE_BUILD_SHIPPING
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[CommenderCharacter] UseItemAction이 NULL입니다! 블루프린트에서 설정하세요!"));
		}
#endif
	}
#if !UE_BUILD_SHIPPING
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CommenderCharacter] EnhancedInputComponent를 찾을 수 없습니다!"));
	}
#endif
}

void ACommenderCharacter::OnGrabItemStarted(const FInputActionValue& Value)
{
	OnLeftClickPressed();
}

void ACommenderCharacter::OnLeftClickPressed()
{
	// PhysicsHandle로 잡고 있는 컴포넌트가 있으면 Drop
	if (PhysicsHandle && PhysicsHandle->GetGrabbedComponent())
	{
		// Drop 로직
		DropGrabbedItem();
	}
	else
	{
		// Grab 로직
		TryGrabItem();
	}
}

void ACommenderCharacter::TryGrabItem()
{
	if (!ItemHoldSocket || !PhysicsHandle)
	{
		// UI 클릭
		if (WidgetInteraction)
		{
			WidgetInteraction->PressPointerKey(EKeys::LeftMouseButton);
		}
		return;
	}

	// LineTrace로 Component Tag "Grabbable"이 있는 컴포넌트 찾기
	FVector CamLoc = GetFirstPersonCameraComponent()->GetComponentLocation();
	FRotator CamRot = GetFirstPersonCameraComponent()->GetComponentRotation();
	FVector TraceEnd = CamLoc + CamRot.Vector() * GrabDistance;
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, CamLoc, TraceEnd, ECC_Visibility, QueryParams))
	{
		UPrimitiveComponent* HitComp = HitResult.GetComponent();
		
		// Component Tag "Grabbable"이 있고 물리 시뮬레이션이 활성화되어 있으면 grab
		if (HitComp &&
			HitComp->IsSimulatingPhysics() &&
			HitComp->ComponentHasTag(TEXT("Grabbable")))
		{
			PhysicsHandle->GrabComponentAtLocationWithRotation(
				HitComp,
				NAME_None,
				HitComp->GetComponentLocation(),
				HitComp->GetComponentRotation()
			);

			// PhysicsHandle 설정
			PhysicsHandle->SetLinearDamping(200.0f);
			PhysicsHandle->SetLinearStiffness(1000.0f);
			PhysicsHandle->SetAngularDamping(200.0f);
			PhysicsHandle->SetAngularStiffness(1000.0f);
			PhysicsHandle->SetInterpolationSpeed(50.0f);

			// VendingItemActor인 경우 GrabbedItem 설정 (기존 호환성)
			AActor* HitActor = HitResult.GetActor();
			if (AVendingItemActor* VendingItem = Cast<AVendingItemActor>(HitActor))
			{
				GrabbedItem = VendingItem;
				if (FocusedVending && VendingItem->VendingMachine == FocusedVending)
				{
					HoldingVendingMachine = FocusedVending;
				}
				
				// 크로스헤어 업데이트
				UpdateCrosshair(false); // LineTrace는 CheckGrabableItem에서 처리
			}
			
			return;
		}
	}
	
	// 기존 VendingItemActor grab 로직 (하위 호환성 - Component Tag가 없는 경우)
	if (AimedItem && ItemHoldSocket)
	{
		// 자판기 아이템인지 확인
		if (FocusedVending && AimedItem->VendingMachine == FocusedVending)
		{
			HoldingVendingMachine = FocusedVending;
		}

		// PhysicsHandle로 잡기 - MeshComp를 찾아서 잡기
		UStaticMeshComponent* MeshComp = AimedItem->FindComponentByClass<UStaticMeshComponent>();
		if (MeshComp && PhysicsHandle)
		{
			FVector GrabLocation = MeshComp->GetComponentLocation();
			FRotator GrabRotation = MeshComp->GetComponentRotation();
			
			PhysicsHandle->GrabComponentAtLocationWithRotation(
				MeshComp,
				NAME_None,
				GrabLocation,
				GrabRotation
			);

			// PhysicsHandle 설정
			PhysicsHandle->SetLinearDamping(200.0f);
			PhysicsHandle->SetLinearStiffness(1000.0f);
			PhysicsHandle->SetAngularDamping(200.0f);
			PhysicsHandle->SetAngularStiffness(1000.0f);
			PhysicsHandle->SetInterpolationSpeed(50.0f);

			// ★ GrabbedItem 꼭 세팅
			GrabbedItem = AimedItem;
			
			// 크로스헤어 업데이트
			UpdateCrosshair(false); // LineTrace는 CheckGrabableItem에서 처리
			
			return;
		}
	}
	
	// 아이템을 조준하고 있지 않으면 UI 클릭
	if (WidgetInteraction)
	{
		WidgetInteraction->PressPointerKey(EKeys::LeftMouseButton);
	}
}

void ACommenderCharacter::DropGrabbedItem()
{
	// PhysicsHandle로 잡고 있는 컴포넌트가 있으면 Release
	if (PhysicsHandle && PhysicsHandle->GetGrabbedComponent())
	{
		ReleaseGrabPhysics();
		HoldingVendingMachine = nullptr;
		GrabbedItem = nullptr;
		
		// 크로스헤어 업데이트
		UpdateCrosshair(false); // LineTrace는 CheckGrabableItem에서 처리
	}
}

void ACommenderCharacter::OnUseItemPressed(const FInputActionValue& Value)
{
	// F키 입력 지점 확인 로그
	UE_LOG(LogTemp, Warning, TEXT("[F-Input] %s LocalRole=%d Pawn=%s"), 
		HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
		(int32)GetLocalRole(),
		*GetName());
	
#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Log, TEXT("[OnUseItemPressed] F키 입력 감지됨"));
#endif

	// 맵 상호작용 Actor 확인 (AbstractCommander의 interactionComponent 사용)
	AActor* FocusedActor = GetFocusedActor();
	if (AMapInteractionActor* MapInteractionActor = Cast<AMapInteractionActor>(FocusedActor))
	{
		// 맵 UI 열기/닫기 토글
		if (CommenderHUDWidget)
		{
			if (CommenderHUDWidget->IsMapUIOpen())
			{
				CommenderHUDWidget->CloseMapUI();
			}
			else
			{
				CommenderHUDWidget->OpenMapUI();
			}
		}
		return;
	}

	// CCTV 모니터 Actor 확인
	if (AMonitorActor* MonitorActor = Cast<AMonitorActor>(FocusedActor))
	{
		// CCTV UI 열기/닫기 토글 (현재 Commander의 PlayerController 전달)
		if (APlayerController* PC = GetController<APlayerController>())
		{
			MonitorActor->ToggleCCTV(PC);
		}
		return;
	}

	// ItemInputMachine을 바라보고 있는지 확인
	if (!FocusedInputMachine)
	{
		// ItemInputMachine을 바라보고 있지 않으면 아무것도 하지 않음
		return;
	}

	// 기존 아이템 사용 로직 (ItemInputMachine을 바라보고 있을 때만)
	if (!GrabbedItem)
	{
		if (CommenderHUDWidget)
		{
			CommenderHUDWidget->ShowMessageByID(ECommanderMessageID::Item_NotHolding);
		}
		return;
	}

	// 레이서가 선택되었는지 먼저 확인 (CanAcceptItem보다 먼저 체크)
	if (FocusedInputMachine->TargetRacerIndex < 0)
	{
		// 레이서가 선택되지 않음
		UE_LOG(LogTemp, Warning, TEXT("[OnUseItemPressed] 레이서가 선택되지 않았습니다! TargetRacerIndex: %d"), 
			FocusedInputMachine->TargetRacerIndex);
		
		// 레이서 선택 메시지 표시
		if (CommenderHUDWidget)
		{
			UE_LOG(LogTemp, Log, TEXT("[OnUseItemPressed] CommenderHUDWidget를 통해 메시지 표시 시도"));
			CommenderHUDWidget->ShowMessageByID(ECommanderMessageID::RacerSelection_NeedSelectFirst);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[OnUseItemPressed] CommenderHUDWidget가 null입니다! Fallback으로 화면 메시지 표시"));
			// Fallback: 화면에 직접 메시지 표시
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.0f,
					FColor::Yellow,
					TEXT("레이서를 먼저 선택해주세요.")
				);
			}
		}
		return;
	}

	// 레이서는 선택되었지만 실제로 연결되어 있는지 확인
	if (!FocusedInputMachine->IsTargetRacerValid())
	{
		// 레이서는 선택되었지만 연결되지 않음
		UE_LOG(LogTemp, Warning, TEXT("[OnUseItemPressed] 레이서가 선택되었지만 연결되지 않았습니다! TargetRacerIndex: %d"), 
			FocusedInputMachine->TargetRacerIndex);
		
		// 레이서 연결 안됨 메시지 표시
		if (CommenderHUDWidget)
		{
			CommenderHUDWidget->ShowMessageByID(ECommanderMessageID::RacerSelection_NotConnected);
		}
		else
		{
			// Fallback: 화면에 직접 메시지 표시
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.0f,
					FColor::Yellow,
					TEXT("레이서가 연결되지 않았습니다.")
				);
			}
		}
		return;
	}

	// 레이서가 선택되었으므로 CanAcceptItem 체크 (거리 등)
	if (!FocusedInputMachine->CanAcceptItem(GrabbedItem, this))
	{
		UE_LOG(LogTemp, Warning, TEXT("[F-Input] [%s] CanAcceptItem 실패 - Server RPC 호출 안 함"), 
			HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
		return;
	}

	// Server RPC로 서버에 아이템 지급 요청
	UE_LOG(LogTemp, Warning, TEXT("[F-Input] [%s] Server RPC 호출 시도: InputMachine=%s, Item=%s"), 
		HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
		*GetNameSafe(FocusedInputMachine),
		*GetNameSafe(GrabbedItem));
	
	Server_RequestGiveItemToRacer(FocusedInputMachine, GrabbedItem);
	
	// 크로스헤어 업데이트
	UpdateCrosshair(false); // LineTrace는 CheckGrabableItem에서 처리
}

void ACommenderCharacter::SetFocusedInteractable(UInteractableComponent* NewInteractable)
{
		if (!NewInteractable)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(106, 0.0f, FColor::Cyan,
				TEXT("[SetFocusedInteractable] NewInteractable is NULL - FocusedInputMachine cleared"));
		}
		FocusedInputMachine = nullptr;
		
		// 크로스헤어 업데이트
		UpdateCrosshair(false); // LineTrace는 CheckGrabableItem에서 처리
		
		return;
	}

	AActor* InteractableOwner = NewInteractable->GetOwner();
	FocusedInputMachine = Cast<AItemInputMachine>(InteractableOwner);

	// 머신이 아니면 nullptr
	if (!FocusedInputMachine)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(107, 0.0f, FColor::Cyan,
				FString::Printf(TEXT("[SetFocusedInteractable] Owner is not ItemInputMachine: %s"), 
					InteractableOwner ? *InteractableOwner->GetName() : TEXT("NULL")));
		}
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(108, 0.0f, FColor::Green,
			FString::Printf(TEXT("[SetFocusedInteractable] ✓ FocusedInputMachine set: %s"), 
				*FocusedInputMachine->GetName()));
	}
	
	// 크로스헤어 업데이트
	UpdateCrosshair(false); // LineTrace는 CheckGrabableItem에서 처리
}

void ACommenderCharacter::OnGrabItemCompleted(const FInputActionValue& Value)
{
	if (WidgetInteraction)
	{
		WidgetInteraction->ReleasePointerKey(EKeys::LeftMouseButton);
	}
}

bool ACommenderCharacter::IsHoldingItem() const
{
	// PhysicsHandle로 아이템을 들고 있는지 확인
	return PhysicsHandle && PhysicsHandle->GetGrabbedComponent() != nullptr;
}

AVendingItemActor* ACommenderCharacter::GetGrabbedVendingItem() const
{
	if (!IsHoldingItem() || !PhysicsHandle || !PhysicsHandle->GetGrabbedComponent())
	{
		return nullptr;
	}
	
	AActor* GrabbedActor = PhysicsHandle->GetGrabbedComponent()->GetOwner();
	return Cast<AVendingItemActor>(GrabbedActor);
}

bool ACommenderCharacter::IsAimingAtItem() const
{
	return AimedItem != nullptr;
}

void ACommenderCharacter::CheckItemCollision()
{
	UPrimitiveComponent* GrabbedComp = PhysicsHandle ? PhysicsHandle->GetGrabbedComponent() : nullptr;
	if (!GrabbedComp)
	{
		bItemColliding = false;
		CollisionNormal = FVector::ZeroVector;
		return;
	}

	// 아이템의 현재 위치
	FVector ItemLocation = GrabbedComp->GetComponentLocation();
	
	// 목표 위치 (ItemHoldSocket 위치)
	FVector TargetLocation = ItemHoldSocket->GetComponentLocation();
	
	// 아이템이 목표 위치에서 너무 멀리 떨어져 있으면 충돌 중
	float Distance = FVector::Dist(ItemLocation, TargetLocation);
	
	if (Distance > ItemCollisionCheckDistance)
	{
		bItemColliding = true;
		
		// 충돌 방향 계산 (목표 → 아이템 방향)
		CollisionNormal = (ItemLocation - TargetLocation).GetSafeNormal();
	}
	else
	{
		bItemColliding = false;
		CollisionNormal = FVector::ZeroVector;
	}
}

FVector ACommenderCharacter::FilterMovementInput(const FVector& InputVector)
{
	if (!bItemColliding || InputVector.IsNearlyZero())
		return InputVector;

	// 입력 방향을 월드 공간으로 변환
	FVector WorldInput = GetActorRotation().RotateVector(InputVector);
	
	// 충돌 방향으로의 이동 성분 제거
	float DotProduct = FVector::DotProduct(WorldInput.GetSafeNormal(), CollisionNormal);
	
	// 충돌 방향으로 이동하려는 경우 (DotProduct > 0)
	if (DotProduct > 0.0f)
	{
		// 충돌 방향 성분 제거
		FVector FilteredWorld = WorldInput - (CollisionNormal * DotProduct * WorldInput.Size());
		
		// 다시 로컬 공간으로 변환
		return GetActorRotation().UnrotateVector(FilteredWorld);
	}
	
	return InputVector;
}

FVector2D ACommenderCharacter::FilterRotationInput(const FVector2D& InputVector)
{
	if (!bItemColliding || InputVector.IsNearlyZero())
		return InputVector;

	// 회전 입력에 의해 카메라가 충돌 방향으로 향하는지 체크
	// Yaw (좌우 회전)
	float YawInput = InputVector.X;
	FRotator TestYaw = GetFirstPersonCameraComponent()->GetComponentRotation();
	TestYaw.Yaw += YawInput;
	FVector TestForwardYaw = TestYaw.RotateVector(FVector::ForwardVector);
	
	float DotYaw = FVector::DotProduct(TestForwardYaw, CollisionNormal);
	if (DotYaw > 0.5f)  // 충돌 방향으로 회전하려는 경우
	{
		YawInput *= FMath::Max(0.0f, 1.0f - DotYaw);  // 회전 감소
	}
	
	// Pitch (상하 회전)
	float PitchInput = InputVector.Y;
	FRotator TestPitch = GetFirstPersonCameraComponent()->GetComponentRotation();
	TestPitch.Pitch += PitchInput;
	FVector TestForwardPitch = TestPitch.RotateVector(FVector::ForwardVector);
	
	float DotPitch = FVector::DotProduct(TestForwardPitch, CollisionNormal);
	if (DotPitch > 0.5f)  // 충돌 방향으로 회전하려는 경우
	{
		PitchInput *= FMath::Max(0.0f, 1.0f - DotPitch);  // 회전 감소
	}
	
	return FVector2D(YawInput, PitchInput);
}

void ACommenderCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 아이템 충돌 시 이동 제한 적용
		if (bItemColliding)
		{
			// 입력 벡터 생성
			FVector InputVector = FVector(MovementVector.X, MovementVector.Y, 0.0f);
			
			// 필터링
			FVector FilteredInput = FilterMovementInput(InputVector);
			
			// 필터링된 입력으로 이동
			AddMovementInput(GetActorForwardVector(), FilteredInput.Y);
			AddMovementInput(GetActorRightVector(), FilteredInput.X);
		}
		else
		{
			// 일반 이동
			AddMovementInput(GetActorForwardVector(), MovementVector.Y);
			AddMovementInput(GetActorRightVector(), MovementVector.X);
		}
	}
}

void ACommenderCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 아이템 충돌 시 회전 제한 적용
		if (bItemColliding)
		{
			FVector2D FilteredLook = FilterRotationInput(LookAxisVector);
			
			// 필터링된 입력으로 회전
			AddControllerYawInput(FilteredLook.X);
			AddControllerPitchInput(FilteredLook.Y);
		}
		else
		{
			// 일반 회전
			AddControllerYawInput(LookAxisVector.X);
			AddControllerPitchInput(LookAxisVector.Y);
		}
	}
}

void ACommenderCharacter::AddCoin(int32 Amount)
{
	if (Amount > 0)
	{
		CurrentCoin += Amount;
		OnCoinChanged.Broadcast(CurrentCoin);
	}
}

bool ACommenderCharacter::SpendCoin(int32 Amount)
{
	if (Amount > 0 && HasEnoughCoin(Amount))
	{
		CurrentCoin -= Amount;
		OnCoinChanged.Broadcast(CurrentCoin);
		return true;
	}
	return false;
}

bool ACommenderCharacter::HasEnoughCoin(int32 Amount) const
{
	return CurrentCoin >= Amount;
}

void ACommenderCharacter::ReleaseGrabPhysics()
{
	if (PhysicsHandle && PhysicsHandle->GetGrabbedComponent())
	{
		PhysicsHandle->ReleaseComponent();
	}
}

// ==================== 맵 시스템 ====================

void ACommenderCharacter::OpenMap()
{
	if (bMapOpen)
	{
		return; // 이미 열려있으면 무시
	}

	bMapOpen = true;

	// PlayerController 가져오기
	if (APlayerController* PC = GetController<APlayerController>())
	{
		// 입력 모드 변경: GameAndUI (마우스 커서 표시, UI 입력 가능)
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);

		// 마우스 커서 표시
		PC->bShowMouseCursor = true;

		// 게임 입력 차단: 이동/시점 모두 무시
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	// 이동 차단 (캐릭터 이동 자체를 멈춤)
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->DisableMovement();
	}

	UE_LOG(LogTemp, Log, TEXT("[CommenderCharacter] Map opened"));
}

void ACommenderCharacter::CloseMap()
{
	if (!bMapOpen)
	{
		return; // 이미 닫혀있으면 무시
	}

	bMapOpen = false;

	// PlayerController 가져오기
	if (APlayerController* PC = GetController<APlayerController>())
	{
		// 입력 모드 복원: GameOnly
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);

		// 마우스 커서 숨김
		PC->bShowMouseCursor = false;

		// 게임 입력 허용: 이동/시점 모두 복원
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}

	// 이동 활성화 (캐릭터 이동 모드 복원)
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->SetMovementMode(MOVE_Walking);
	}

	UE_LOG(LogTemp, Log, TEXT("[CommenderCharacter] Map closed"));
}

// ==================== 핑 시스템 ====================

void ACommenderCharacter::ServerPlacePing_Implementation(const FVector& WorldLocation, ECommanderPingType Type)
{
	UE_LOG(LogTemp, Warning, TEXT("[CommenderCharacter] ServerPlacePing_Implementation 호출 - WorldLocation: %s, Type: %d, HasAuthority: %d"), 
		*WorldLocation.ToString(), (int32)Type, HasAuthority() ? 1 : 0);
	
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderCharacter] ServerPlacePing_Implementation: 권한 없음, 무시"));
		return;
	}

	// PlayerState 가져오기
	ACitRushPlayerState* PS = GetPlayerState<ACitRushPlayerState>();
	if (!PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderCharacter] ServerPlacePing: PlayerState not found"));
		return;
	}

	// GameState 가져오기
	ACitRushGameState* GS = GetWorld()->GetGameState<ACitRushGameState>();
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderCharacter] ServerPlacePing: GameState not found"));
		return;
	}

	// MapBounds로 위치 검증 및 보정
	AMapBoundsActor* MapBounds = GS->GetMapBounds();
	FVector ValidatedLocation = WorldLocation;

	if (MapBounds)
	{
		FBox2D Bounds = MapBounds->GetXYBounds();
		
		// XY를 Bounds 안으로 클램프
		ValidatedLocation.X = FMath::Clamp(ValidatedLocation.X, Bounds.Min.X, Bounds.Max.X);
		ValidatedLocation.Y = FMath::Clamp(ValidatedLocation.Y, Bounds.Min.Y, Bounds.Max.Y);
	}

	// Z 좌표 라인트레이스로 보정 (치트/오차 방지)
	FVector TraceStart = ValidatedLocation + FVector(0.f, 0.f, 50000.f);
	FVector TraceEnd = ValidatedLocation + FVector(0.f, 0.f, -50000.f);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		ValidatedLocation.Z = HitResult.Location.Z;
	}
	else
	{
		// 라인트레이스 실패 시 기본 Z 사용
		UE_LOG(LogTemp, Warning, TEXT("[CommenderCharacter] ServerPlacePing: LineTrace failed, using original Z"));
	}

	// 서버 시간 가져오기 (핑 스팸 / 글로벌 쿨다운에 사용)
	const float CurrentTime = GS->GetServerWorldTimeSeconds();

	// 먼저 글로벌 쿨다운 여부 확인
	if (CurrentTime < GlobalPingCooldownEndTime)
	{
		const float Remaining = GlobalPingCooldownEndTime - CurrentTime;
		ClientNotifyPingCooldown(Remaining);
		return;
	}

	// 코인 5 차감
	constexpr int32 PingCost = 5;
	if (!SpendCoin(PingCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CommenderCharacter] ServerPlacePing: 코인 부족 (필요: %d, 보유: %d)"), PingCost, CurrentCoin);
		return;
	}

	// 기본 지속 시간: GameState 에서 설정한 값 사용 (기본 15초)
	float Duration = GS->GetDefaultPingDurationSeconds();
	if (Duration <= 0.f)
	{
		Duration = 15.f; // 안전한 기본값
	}

	// GameState에 핑 설정
	GS->SetActivePing(ValidatedLocation, Type, Duration, PS);

	// 최근 5초 이내에 찍은 핑 횟수 기록
	// 1) 오래된 기록 제거 (5초 이전 것)
	constexpr float SpamWindowSeconds = 5.f;
	for (int32 Index = RecentPingTimes.Num() - 1; Index >= 0; --Index)
	{
		if (CurrentTime - RecentPingTimes[Index] > SpamWindowSeconds)
		{
			RecentPingTimes.RemoveAt(Index);
		}
	}

	// 2) 현재 핑 추가
	RecentPingTimes.Add(CurrentTime);

	// 3) 너무 많이 찍었으면 (예: 5회 이상) 전체 핑 글로벌 쿨다운 15초 부여
	constexpr int32 SpamThreshold = 5;
	constexpr float GlobalCooldownSeconds = 15.f;

	if (RecentPingTimes.Num() >= SpamThreshold)
	{
		GlobalPingCooldownEndTime = CurrentTime + GlobalCooldownSeconds;

		// 다음 스팸 감지를 위해 최근 기록 초기화
		RecentPingTimes.Reset();

		// 클라이언트에 글로벌 쿨다운 시작 알림
		ClientNotifyPingCooldown(GlobalCooldownSeconds);
	}

	UE_LOG(LogTemp, Log, TEXT("[CommenderCharacter] ServerPlacePing: Ping placed at %s by %s"), 
		*ValidatedLocation.ToString(), *PS->GetPlayerName());
}

void ACommenderCharacter::ResetPingCooldown()
{
	if (!HasAuthority())
	{
		return;
	}

	GlobalPingCooldownEndTime = 0.f;
	RecentPingTimes.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("[CommenderCharacter] 핑 쿨타임 초기화"));
}

void ACommenderCharacter::ClientNotifyPingCooldown_Implementation(float RemainingTime)
{
	// 클라이언트 기준 글로벌 쿨다운 종료 시각 갱신
	if (UWorld* World = GetWorld())
	{
		const float Now = World->GetTimeSeconds();
		ClientGlobalPingCooldownEndTime = Now + FMath::Max(0.f, RemainingTime);
	}

	// 간단한 디버그 메시지 출력 (필요시 HUD 위젯 연동으로 교체 가능)
	if (GEngine)
	{
		const FString Msg = FString::Printf(
			TEXT("핑 글로벌 쿨다운: %.1f초 후 다시 사용 가능"),
			FMath::Max(0.f, RemainingTime));

		GEngine->AddOnScreenDebugMessage(
			/*Key*/ -1,
			/*TimeToDisplay*/ 2.0f,
			FColor::Yellow,
			Msg);
	}
}

bool ACommenderCharacter::IsPingOnGlobalCooldown(float& OutRemainingTime) const
{
	OutRemainingTime = 0.f;

	if (const UWorld* World = GetWorld())
	{
		const float Now = World->GetTimeSeconds();
		if (Now < ClientGlobalPingCooldownEndTime)
		{
			OutRemainingTime = ClientGlobalPingCooldownEndTime - Now;
			return true;
		}
	}

	return false;
}

// ==================== 크로스헤어 업데이트 (이벤트 기반) ====================

void ACommenderCharacter::UpdateCrosshair(bool bCanGrabItemFromTrace)
{
	// 맵이 열려있으면 크로스헤어 업데이트를 건너뜀
	if (bMapOpen || !CommenderHUDWidget || !CommenderHUDWidget->CrosshairWidget)
	{
		return;
	}

	// 조건 체크
	const bool bHasGrabbedItem = (GrabbedItem != nullptr);
	const bool bHasFocusedMachine = (FocusedInputMachine != nullptr);
	bool bCanUseItem = false;
	bool bCanGrabItem = bCanGrabItemFromTrace && !bHasGrabbedItem; // LineTrace 결과 사용 (중복 방지)
	
	// 아이템 사용 가능 여부 확인
	if (bHasGrabbedItem && bHasFocusedMachine)
	{
		const bool bRacerSet = (FocusedInputMachine->TargetRacerIndex >= 0);
		
		if (bRacerSet && FocusedInputMachine->ItemUseOrigin && GrabbedItem)
		{
			const FVector Origin = FocusedInputMachine->ItemUseOrigin->GetComponentLocation();
			const FVector ItemLoc = GrabbedItem->GetActorLocation();
			const float DistSquared = FVector::DistSquared(Origin, ItemLoc);
			const float RadiusSquared = FMath::Square(FocusedInputMachine->ItemUseRadius);
			bCanUseItem = (DistSquared <= RadiusSquared);
		}
	}

	// 상태가 변경되지 않았으면 업데이트하지 않음 (효율성)
	if (bCanGrabItem == bPreviousCanGrabItem && bCanUseItem == bPreviousCanUseItem && !bCanGrabItem && !bCanUseItem)
	{
		// 둘 다 false이고 이전 상태도 같으면 기본 색상으로 한 번만 설정
		if (!bPreviousCanUseItem && !bPreviousCanGrabItem)
		{
			return; // 이미 기본 상태이므로 업데이트 불필요
		}
	}

	// Grab 가능한 아이템을 조준 중이면 빨간색으로 표시 (우선순위 높음)
	if (bCanGrabItem)
	{
		if (!bPreviousCanGrabItem) // 상태가 변경되었을 때만 업데이트
		{
			CommenderHUDWidget->CrosshairWidget->SetCrosshairColor(FLinearColor::Red);
			bPreviousCanGrabItem = true;
			bPreviousCanUseItem = false; // 리셋
		}
	}
	// 아이템 사용 가능 모드 (초록색)
	else if (bCanUseItem)
	{
		if (bCanUseItem != bPreviousCanUseItem) // 상태가 변경되었을 때만 업데이트
		{
			CommenderHUDWidget->CrosshairWidget->SetUseMode(bCanUseItem);
			bPreviousCanUseItem = bCanUseItem;
			bPreviousCanGrabItem = false; // 리셋
		}
	}
	// 둘 다 아니면 기본 색상으로 복원
	else
	{
		if (bPreviousCanUseItem || bPreviousCanGrabItem) // 이전에 다른 상태였을 때만 복원
		{
			if (bPreviousCanUseItem)
			{
				CommenderHUDWidget->CrosshairWidget->SetUseMode(false);
			}
			CommenderHUDWidget->CrosshairWidget->ResetCrosshairToDefault();
			bPreviousCanUseItem = false;
			bPreviousCanGrabItem = false;
		}
	}
}

void ACommenderCharacter::CheckGrabableItem()
{
	// LineTrace로 아이템 조준 확인 (크로스헤어용)
	AimedItem = nullptr;
	bool bCanGrabItem = false;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		FVector TraceEnd = CamLoc + CamRot.Vector() * GrabDistance;

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, CamLoc, TraceEnd, ECC_Visibility, QueryParams))
		{
			AActor* HitActor = HitResult.GetActor();
			UPrimitiveComponent* HitComp = HitResult.GetComponent();
			
			// Component Tag "Grabbable"이 있거나 VendingItemActor인 경우 AimedItem 설정
			if (HitComp && HitComp->ComponentHasTag(TEXT("Grabbable")))
			{
				// Grab 가능 여부 확인 (물리 시뮬레이션 중인 컴포넌트만)
				if (HitComp->IsSimulatingPhysics())
				{
					bCanGrabItem = true;
				}
				
				// VendingItemActor인 경우 AimedItem 설정 (기존 호환성)
				if (AVendingItemActor* ItemActor = Cast<AVendingItemActor>(HitActor))
				{
					AimedItem = ItemActor;
				}
			}
			else if (AVendingItemActor* ItemActor = Cast<AVendingItemActor>(HitActor))
			{
				AimedItem = ItemActor;
			}
		}
	}

	// 크로스헤어 업데이트 (LineTrace 결과 전달하여 중복 방지)
	UpdateCrosshair(bCanGrabItem);
}

void ACommenderCharacter::Server_RequestGiveItemToRacer_Implementation(AItemInputMachine* InputMachine, AVendingItemActor* Item)
{
	// Server RPC 도착 로그
	UE_LOG(LogTemp, Warning, TEXT("[F-RPC] ARRIVED %s"), 
		HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	
	if (!InputMachine)
	{
		UE_LOG(LogTemp, Error, TEXT("[F-RPC] [SERVER] InputMachine이 null입니다!"));
		return;
	}
	
	if (!Item)
	{
		UE_LOG(LogTemp, Error, TEXT("[F-RPC] [SERVER] Item이 null입니다!"));
		return;
	}
	
	// 서버에서 레이서 스캔 로그 (핵심)
	int32 RacerCount = 0;
	TArray<AAbstractRacer*> FoundRacers;
	
	// 방법 1: GetAllActorsOfClass로 스캔
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAbstractRacer::StaticClass(), FoundActors);
	
	for (AActor* Actor : FoundActors)
	{
		if (AAbstractRacer* Racer = Cast<AAbstractRacer>(Actor))
		{
			RacerCount++;
			FoundRacers.Add(Racer);
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[F-Scan] SERVER RacerCount=%d (GetAllActorsOfClass)"), RacerCount);
	
	// 방법 2: GameState를 통한 레이서 목록 확인
	if (ACitRushGameState* GameState = GetWorld()->GetGameState<ACitRushGameState>())
	{
		TArray<ACitRushPlayerState*> Racers = GameState->GetPlayerStatesByRole(EPlayerRole::Racer);
		UE_LOG(LogTemp, Warning, TEXT("[F-Scan] SERVER RacerCount (GameState)=%d"), Racers.Num());
		
		for (int32 i = 0; i < Racers.Num(); i++)
		{
			if (IsValid(Racers[i]))
			{
				AAbstractRacer* RacerPawn = Racers[i]->GetPawn<AAbstractRacer>();
				UE_LOG(LogTemp, Warning, TEXT("[F-Scan] SERVER Racer[%d]: PlayerState=%s, Pawn=%s"), 
					i, 
					*Racers[i]->GetPlayerInfo().playerName,
					RacerPawn ? *RacerPawn->GetName() : TEXT("NULL"));
			}
		}
	}
	
	// 실제 아이템 지급 실행 (서버에서만)
	UE_LOG(LogTemp, Warning, TEXT("[F-RPC] [SERVER] SupplyItemToRacer 호출: InputMachine=%s, Item=%s"), 
		*InputMachine->GetName(), *Item->GetName());
	
	bool bSuccess = InputMachine->SupplyItemToRacer(Item, this);
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("[F-RPC] [SERVER] ✓ 아이템 지급 성공!"));
		
		// 성공했을 때만 PhysicsHandle Release
		if (PhysicsHandle && PhysicsHandle->GetGrabbedComponent())
		{
			PhysicsHandle->ReleaseComponent();
			HoldingVendingMachine = nullptr;
		}
		
		GrabbedItem = nullptr;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[F-FAIL] SERVER 아이템 지급 실패! InputMachine=%s, Item=%s"), 
			*InputMachine->GetName(), *Item->GetName());
		
		// 실패 시 아이템은 grab 상태로 유지됨
		UE_LOG(LogTemp, Warning, TEXT("[F-RPC] [SERVER] 아이템 할당 실패 - grab 상태 유지"));
	}
}

