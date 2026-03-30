// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AbstractRacer.h"

#include "ChaosVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"

#include "GAS/AbilitySystemComponent/BaseASC.h"
#include "GAS/AttributeSet/ASRacer.h"
#include "Item/ItemData.h"
#include "Data/InputMappingsSettings.h"

#include "Player/AbstractCommander.h"
#include "Player/CCTV/CCTVCameraComponent.h"
#include "Player/Car/VehicleDemoCejCar.h"
#include "Player/Car/VehicleDemoUITest.h"
#include "VehicleTemplate/UE_CITRUSHPlayerController.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Player/CitRushPlayerState.h"
#include "GameFlow/CitRushGameState.h"
#include "GameFlow/LocalDataFlowSubsystem.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Player/Components/MinimapIconComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"

#if WITH_STEAMWORKS
	#include "Player/Components/SteamListenComponent.h"
	#include "Player/Components/SteamVoiceComponent.h"
#else
	#include "Player/Components/VoiceDonorComponent.h"
	#include "Player/Components/VoiceAcceptorComponent.h"
#endif

DEFINE_LOG_CATEGORY_CLASS(AAbstractRacer, RacerLog);

AAbstractRacer::AAbstractRacer()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

#if WITH_STEAMWORKS
	steamListenComponent = CreateDefaultSubobject<USteamListenComponent>(TEXT("SteamListenComponent"));
#else
	rawListenComponent = CreateDefaultSubobject<UVoiceAcceptorComponent>(TEXT("RawListenComponent"));
#endif

	// CCTV 카메라 컴포넌트 생성
	CCTVCameraComponent = CreateDefaultSubobject<UCCTVCameraComponent>(TEXT("CCTVCameraComponent"));
	
	// MinimapIconComponent 생성 (real ping 표시용)
	MinimapIconComponent = CreateDefaultSubobject<UMinimapIconComponent>(TEXT("MinimapIconComponent"));
	if (MinimapIconComponent)
	{
		MinimapIconComponent->bShowOnMap = true;
		MinimapIconComponent->IconId = ERealtimeMapIconId::Racer1; // 기본값, PlayerState에서 자동으로 업데이트됨
		MinimapIconComponent->bRotateWithActor = true;
	}

	if (const FInputMappingData* imc = UInputMappingsSettings::Get()->inputMappings.Find(TEXT("IMC_RacerItem")))
	{
		IMC_RacerItem = imc->inputMappingContext;
		IA_UseItem = imc->inputActions[TEXT("IA_UseItem")];
		IA_ToggleItem = imc->inputActions[TEXT("IA_ToggleItemSlot")];
	}

	restartTransforms = 
	{
		{FVector(13430.0, 86100.0, 120.0), FRotator(0,0,0)},
		{FVector(13920.0,50020.0,120.0), FRotator(0,0,0)},
		{FVector(91868.0, 50144.0, 120.0), FRotator(0.00,-180,0.0)},
		{FVector(2171.0,50414.0,120.0), FRotator(0,0,0)},
		{FVector(49912.0, 6336.0, 120.0), FRotator(0.00,-270,0.0)}
	};
	//attributeSet->InitMaxHealth(100.f);
	//attributeSet->InitHealth(100);
}

void AAbstractRacer::BeginPlay()
{
	Super::BeginPlay();
	TArray<FGameplayAttribute> attributes;
	GetAttributeSet()->GetAttributesFromSetClass(UASRacer::StaticClass(), attributes);
	for (FGameplayAttribute attribute : attributes)
	{
		if (attribute.AttributeName == TEXT("Health"))
		{
			// GS에 전달
		}
	}

	// 서버에서만 차량 움직임 감지 타이머 시작
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				MovementCheckTimer,
				this,
				&AAbstractRacer::CheckVehicleMovement,
				0.1f,  // 0.1초마다 체크
				true   // 반복
			);
			UE_LOG(RacerLog, Log, TEXT("[AbstractRacer] 차량 움직임 감지 타이머 시작 (0.1s 간격)"));
		}
	}
}

void AAbstractRacer::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAbstractRacer, frontItemSlot);
	DOREPLIFETIME(AAbstractRacer, backItemSlot);
	DOREPLIFETIME(AAbstractRacer, untouchableTime);
}

void AAbstractRacer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	/*if (APlayerController* pc = Cast<APlayerController>(NewController);
		pc->IsLocalController())
	{
		InitInputMode(pc);
	}*/
	
	InitAbilitySystem();
	InitVoiceInterface();
}

void AAbstractRacer::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilitySystem();
	InitVoiceInterface();
}

void AAbstractRacer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(RacerLog, Warning, TEXT("[Racer] SetupPlayerInputComponent called! HasAuthority: %d, IsLocallyControlled: %d"),
	       HasAuthority(), IsLocallyControlled());

	APlayerController* pc = GetController<APlayerController>();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(IMC_RacerItem, 1);
	}
	if (UEnhancedInputComponent* eic = Cast<UEnhancedInputComponent>(pc->InputComponent))
	{
		eic->BindAction(IA_UseItem, ETriggerEvent::Started, this, &AAbstractRacer::OnUseItemKeyInput);
		eic->BindAction(IA_ToggleItem, ETriggerEvent::Started, this, &AAbstractRacer::OnToggleItemKeyInput);
	}
	
	/*// InputMode 설정 (클라이언트)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		UE_LOG(RacerLog, Warning, TEXT("[Racer] InputMode set to GameOnly"));
	}*/
}

void AAbstractRacer::InitInputMode(APlayerController* playerController)
{
	playerController->SetInputMode(FInputModeGameOnly());
	playerController->SetShowMouseCursor(false);
}

void AAbstractRacer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (voiceInterface.IsValid())
	{
		if (ACitRushGameState* gs = GetWorld()->GetGameState<ACitRushGameState>())
		{
			TArray<ACitRushPlayerState*> commanders = gs->GetPlayerStatesByRole(EPlayerRole::Commander);
			if (commanders.Num() > 0 && IsValid(commanders[0]))
			{
				TSharedPtr<const FUniqueNetId> CommanderNetId = commanders[0]->GetUniqueId().GetUniqueNetId();

				if (CommanderNetId.IsValid())
				{
					voiceInterface->UnregisterRemoteTalker(*CommanderNetId);
					UE_LOG(RacerLog, Warning, TEXT("[Racer] Unregistered Commander voice"));
				}
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AAbstractRacer::InitVoiceInterface()
{
	ACitRushGameState* gs = GetWorld()->GetGameState<ACitRushGameState>();
	if (!IsValid(gs)) {return;}
	TArray<ACitRushPlayerState*> commanders = gs->GetPlayerStatesByRole(EPlayerRole::Commander);
	if (commanders.Num() == 0 || !IsValid(commanders[0])) {return;}
	AAbstractCommander* commander = commanders[0]->GetPawn<AAbstractCommander>();
	if (!IsValid(commander)) {return;}
	
#if WITH_STEAMWORKS
	USteamVoiceComponent* voiceComponent = commander->GetSteamVoiceComponent();
	FString netMode = ToString(GetNetMode());
	if (!IsValid(voiceComponent) || bRegisterVoiceComponents) {return;}
	if (steamListenComponent && steamListenComponent->RegisterLocalUser(GetPlayerState()))
	{
		if (steamListenComponent->RegisterCommanderVoiceComponent(voiceComponent))
		{
			bRegisterVoiceComponents = true;
		}
		
	} else {UE_LOG(RacerLog, Error, TEXT("Cant Register ListenComponent : %s"), *netMode)}
	
#else
	if (IOnlineSubsystem* oss = Online::GetSubsystem(GetWorld()))
	{
		voiceInterface = oss->GetVoiceInterface();
	}
	if (rawListenComponent)
	{
		TSharedPtr<const FUniqueNetId> commanderNetId = commanders[0]->GetUniqueId().GetUniqueNetId();
		if (commanderNetId.IsValid())
		{
			voiceInterface->RegisterRemoteTalker(*commanderNetId);
		}
		//...
	}
#endif
	

	
}

UAbilitySystemComponent* AAbstractRacer::GetAbilitySystemComponent() const
{
	return abilitySystemComponent;
}

UAttributeSet* AAbstractRacer::GetAttributeSet() const
{
	return attributeSet;
}

void AAbstractRacer::InitDefaultAttributes() const
{
	if (!abilitySystemComponent || !defaultAttributeEffect) {return;}
	
	FGameplayEffectContextHandle effectContext = abilitySystemComponent->MakeEffectContext();
	effectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle specHandle = abilitySystemComponent->MakeOutgoingSpec(defaultAttributeEffect, 1.f, effectContext);
	if (specHandle.IsValid())
	{
		abilitySystemComponent->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
	}
}

void AAbstractRacer::AcquireItem(UItemData* NewItem)
{
	UE_LOG(LogTemp, Warning, TEXT("[AbstractRacer::AcquireItem] ===== 시작 ====="));
	UE_LOG(LogTemp, Warning, TEXT("[AbstractRacer::AcquireItem] Racer: %s, NewItem: %s"), 
		*GetName(), NewItem ? *NewItem->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("[AbstractRacer::AcquireItem] LocalRole: %d, Authority: %d"), 
		(int32)GetLocalRole(), HasAuthority() ? 1 : 0);
	
	if (GetLocalRole() < ROLE_Authority)
	{
		UE_LOG(LogTemp, Error, TEXT("[AbstractRacer::AcquireItem] 권한 없음! (LocalRole: %d < ROLE_Authority)"), (int32)GetLocalRole());
		return;
	}
	
	if (!IsValid(NewItem))
	{
		UE_LOG(LogTemp, Error, TEXT("[AbstractRacer::AcquireItem] NewItem이 유효하지 않습니다!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AbstractRacer::AcquireItem] frontItemSlot: %s, backItemSlot: %s"), 
		frontItemSlot ? *frontItemSlot->GetName() : TEXT("NULL"),
		backItemSlot ? *backItemSlot->GetName() : TEXT("NULL"));

	GetWorld()->GetGameState<ACitRushGameState>()->NetMulticastRPC_CollectFloatData(TEXT("GivingItem"), 1);
	if (NewItem->tag.GetTagLeafName() == TEXT("Refuel"))
	{
		attributeSet->SetFuel(attributeSet->GetFuel() + 60.f);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Green,
				FString::Printf(TEXT("✓ 연료 충전 : %s (%.2f)"), *NewItem->GetName(), attributeSet->GetFuel())
			);
		}
		ClientNotifyItemAcquired(NewItem->ID.ToString());
		return;
	}
	if (frontItemSlot == nullptr)
	{
		frontItemSlot = NewItem;
		UE_LOG(LogTemp, Warning, TEXT("[AbstractRacer::AcquireItem] ✓ frontItemSlot에 아이템 할당 완료 - Racer: %s, Item: %s"), 
			*GetName(), *NewItem->GetName());
		
		// 서버에서도 즉시 델리게이트 브로드캐스트 (해당 레이서의 UI만 업데이트)
		OnItemSlotChanged.Broadcast(frontItemSlot, backItemSlot);
		
		// 클라이언트에 아이템 부여 알림
		ClientNotifyItemAcquired(NewItem->ID.ToString());
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Green,
				FString::Printf(TEXT("[AbstractRacer] ✓ 아이템 획득: %s (Front Slot)"), *NewItem->GetName())
			);
		}
	}
	else if (backItemSlot == nullptr)
	{
		backItemSlot = NewItem;
		UE_LOG(LogTemp, Warning, TEXT("[AbstractRacer::AcquireItem] ✓ backItemSlot에 아이템 할당 완료 - Racer: %s, Item: %s"), 
			*GetName(), *NewItem->GetName());
		
		// 서버에서도 즉시 델리게이트 브로드캐스트 (해당 레이서의 UI만 업데이트)
		OnItemSlotChanged.Broadcast(frontItemSlot, backItemSlot);
		
		// 클라이언트에 아이템 부여 알림
		ClientNotifyItemAcquired(NewItem->ID.ToString());
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Green,
				FString::Printf(TEXT("[AbstractRacer] ✓ 아이템 획득: %s (Back Slot)"), *NewItem->GetName())
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AbstractRacer::AcquireItem] 아이템 슬롯이 모두 가득 참! (아이템 할당 실패)"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Yellow,
				TEXT("[AbstractRacer] 아이템 슬롯이 모두 가득 참!")
			);
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[AbstractRacer::AcquireItem] ===== 완료 ====="));
}

void AAbstractRacer::ConsumeItem(UItemData* ItemToConsume)
{
	if (!HasAuthority()) return;

	bool bChanged = false;
	if (frontItemSlot == ItemToConsume)
	{
		frontItemSlot = nullptr;
		bChanged = true;
	}
	else if (backItemSlot == ItemToConsume)
	{
		backItemSlot = nullptr;
		bChanged = true;
	}

	if (bChanged)
	{
		OnItemSlotChanged.Broadcast(frontItemSlot, backItemSlot);
		UE_LOG(RacerLog, Log, TEXT("[AbstractRacer] Item Consumed: %s"), ItemToConsume ? *ItemToConsume->GetName() : TEXT("NULL"));
	}
}

void AAbstractRacer::ClientNotifyItemAcquired_Implementation(const FString& ItemName)
{
	UE_LOG(LogTemp, Warning, TEXT("[AbstractRacer] [Client] ✓ 아이템 부여됨: %s"), *ItemName);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Cyan,
			FString::Printf(TEXT("[레이서] ✓ 커맨더가 아이템을 부여했습니다: %s"), *ItemName)
		);
	}

	// VehicleDemoUITest에 메시지 표시
	if (APlayerController* PC = GetController<APlayerController>())
	{
		if (PC->IsLocalController())
		{
			// VehicleDemoCejCar를 통해 찾기 (this가 VehicleDemoCejCar인 경우)
			if (AVehicleDemoCejCar* CejCar = Cast<AVehicleDemoCejCar>(this))
			{
				if (UVehicleDemoUITest* CarHUDWidget = CejCar->GetCarHUDWidget())
				{
					CarHUDWidget->ShowItemNotification(ItemName);
					return;
				}
			}

			// PlayerController에서 VehicleDemoUITest 찾기
			// UE_CITRUSHPlayerController의 VehicleUI 참조 확인
			if (AUE_CITRUSHPlayerController* CITRUSHPC = Cast<AUE_CITRUSHPlayerController>(PC))
			{
				if (UVehicleDemoUITest* VehicleUI = CITRUSHPC->GetVehicleUI())
				{
					VehicleUI->ShowItemNotification(ItemName);
					return;
				}
			}
		}
	}
}

bool AAbstractRacer::ReceiveAbility(const TSubclassOf<UGameplayAbility>& ability)
{
	if (frontItemSlot == nullptr)
	{
		frontItemSlot->grantedAbilityClass = ability;
		return true;
	}
	if (backItemSlot == nullptr)
	{
		backItemSlot->grantedAbilityClass = ability;
		return true;
	}
	return false;
}

bool AAbstractRacer::ReceiveAbility(const FName& gameplayTagName)
{
	return false;
}

void AAbstractRacer::InitAbilitySystem()
{
	UE_LOG(RacerLog, Log, TEXT("[Racer] InitAbilitySystem() called - Actor: %s"), *GetNameSafe(this));

	if (ACitRushPlayerState* cPS = GetPlayerState<ACitRushPlayerState>())
	{
		UE_LOG(RacerLog, Log, TEXT("[Racer] PlayerState found: %s"), *GetNameSafe(cPS));

		abilitySystemComponent = Cast<UBaseASC>(cPS->GetAbilitySystemComponent());
		if (abilitySystemComponent)
		{
			UE_LOG(RacerLog, Log, TEXT("[Racer] AbilitySystemComponent found: %s"), *GetNameSafe(abilitySystemComponent));
			abilitySystemComponent->InitAbilityActorInfo(cPS, this);

			attributeSet = Cast<UASRacer>(cPS->GetAttributeSet(EPlayerRole::Racer));
			if (attributeSet)
			{
				UE_LOG(RacerLog, Log, TEXT("[Racer] AttributeSet found: %s"), *GetNameSafe(attributeSet));
				attributeSet->InitMaxHealth(100.f);
				attributeSet->InitHealth(100.f);
				attributeSet->InitMaxFuel(100.f);
				attributeSet->InitFuel(100.f);
			}
			else
			{
				UE_LOG(RacerLog, Error, TEXT("[Racer] AttributeSet is NULL! GetAttributeSet(Racer) failed"));
			}
		}
		else
		{
			UE_LOG(RacerLog, Error, TEXT("[Racer] AbilitySystemComponent is NULL! GetAbilitySystemComponent() failed"));
		}
	}
	else
	{
		UE_LOG(RacerLog, Error, TEXT("[Racer] PlayerState is NULL! GetPlayerState<ACitRushPlayerState>() failed"));
	}
}

void AAbstractRacer::OnRep_FrontItemSlot()
{
	// 아이템 슬롯 변경 델리게이트 브로드캐스트
	OnItemSlotChanged.Broadcast(frontItemSlot, backItemSlot);
}

void AAbstractRacer::OnRep_BackItemSlot()
{
	// 아이템 슬롯 변경 델리게이트 브로드캐스트
	OnItemSlotChanged.Broadcast(frontItemSlot, backItemSlot);
}

void AAbstractRacer::OnUseItemKeyInput()
{
	ServerRPC_UseItem();
}

void AAbstractRacer::ServerRPC_UseItem_Implementation()
{
	if (!frontItemSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] Front 아이템 슬롯이 비어있습니다."));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] ===== 시작 ====="));
	UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] Front 아이템: %s"), 
		*frontItemSlot->GetName());
	
	TSubclassOf<UGameplayAbility> ability = frontItemSlot->grantedAbilityClass;
	if (!ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] Front 아이템에 Ability가 없습니다."));
		return;
	}

	if (abilitySystemComponent && abilitySystemComponent->TryActivateAbilityByClass(ability))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] ✓ Ability 활성화 성공"));
		
		// Front 아이템 사용 완료 - Front 슬롯 아이콘 삭제
		UItemData* UsedItem = frontItemSlot;
		frontItemSlot = nullptr;
		
		UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] Front 아이템 삭제: %s"), 
			*UsedItem->GetName());
		
		// Back 아이템이 있으면 Front로 승급
		if (backItemSlot)
		{
			UItemData* PromotedItem = backItemSlot;
			frontItemSlot = PromotedItem;
			backItemSlot = nullptr;
			
			UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] ✓ Back 아이템을 Front로 승급: %s"), 
				*PromotedItem->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] Back 아이템이 없어서 승급할 아이템이 없습니다."));
		}
		
		// 아이템 슬롯 변경 델리게이트 브로드캐스트 (UI 업데이트)
		OnItemSlotChanged.Broadcast(frontItemSlot, backItemSlot);
		
		UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] ===== 완료 ====="));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ServerRPC_UseItem] ✗ Ability 활성화 실패"));
	}
	/*// TODO: UItemData에 CooldownEffectClass 이용 할지 말지
	if (FrontItemSlot->CooldownEffectClass)
    {
		abilitySystemComponent->ApplyGameplayEffectToSelf(
			FrontItemSlot->CooldownEffectClass.GetDefaultObject()
			, 1
			, abilitySystemComponent->MakeEffectContext()
		);
	}
	*/
}

void AAbstractRacer::OnToggleItemKeyInput()
{
	ServerRPC_ToggleItem();
}

void AAbstractRacer::OnCollectDamagedEvent(const FOnAttributeChangeData& changeData)
{
	float delta = changeData.OldValue - changeData.NewValue;
	if (delta > 0)
	{
		GetWorld()->GetGameState<ACitRushGameState>()->NetMulticastRPC_CollectFloatData(TEXT("Damaged"), delta);
	}
}

void AAbstractRacer::ServerRPC_ToggleItem_Implementation()
{
	// Front와 Back 아이템 슬롯 교체
	UItemData* tempItemData = frontItemSlot;
	frontItemSlot = backItemSlot;
	backItemSlot = tempItemData;
	
	// 서버에서도 즉시 델리게이트 브로드캐스트 (UI 업데이트)
	OnItemSlotChanged.Broadcast(frontItemSlot, backItemSlot);
}

bool AAbstractRacer::ActivateAbility(const TSubclassOf<UGameplayAbility>& ability)
{
	check(abilitySystemComponent);
	if (!HasAuthority()) {return false;}
	if (!IsValid(frontItemSlot)) {return false;}

	FGameplayAbilitySpec abilitySpec(frontItemSlot->grantedAbilityClass, 1);  // Level도 정할 수 있음.
	TSharedPtr<FGameplayEventData> gameplayEventData = MakeShareable<FGameplayEventData>(new FGameplayEventData());
	abilitySystemComponent->GiveAbilityAndActivateOnce(abilitySpec, gameplayEventData.Get());
	
	if (IsValid(backItemSlot))
	{
		frontItemSlot = backItemSlot;
		backItemSlot = nullptr;
	}
	
	return true;
}

bool AAbstractRacer::ActivateAbility(const FName& gameplayTagName)
{
	return false;
}

void AAbstractRacer::NetMulticastRPC_TryAttack_Implementation(bool bSuccess, FVector repelLocation, FVector repelDirection)
{
	FVector resultRepelDirection = (GetActorLocation() - repelLocation).GetSafeNormal2D();
	float delta = GetWorld()->GetDeltaSeconds();
	if (delta > 1.f)
	{
		resultRepelDirection /= delta; 
	}
	if (bSuccess)
	{
		GetMesh()->AddImpulse(resultRepelDirection * repulsive_SuccessHit, NAME_None, true);
	}
	else
	{
		GetMesh()->AddImpulse(resultRepelDirection * repulsive_FailedHit, NAME_None, true);
	}
}

void AAbstractRacer::NetMulticastRPC_Damaged_Implementation(FVector repelLocation, FVector repelDirection)
{
	if (untouchableTime > 0.f) {return;}
	APlayerController* pc = GetController<APlayerController>();
	ACitRushGameState* lGS = GetWorld()->GetGameState<ACitRushGameState>();
	if (!lGS) {return;}
	TArray<ACitRushPlayerState*> commanders = lGS->GetPlayerStatesByRole(EPlayerRole::Commander);
	
	if (HasAuthority())
	{
		pc->GetPlayerState<ACitRushPlayerState>()->ServerRPC_SetDied_Implementation(true);
		attributeSet->SetHealth(attributeSet->GetHealth() - 40.f);
		untouchableTime = 1.5f;
		
		int32 index = FMath::RandRange(0, 4);
		SetActorLocationAndRotation(restartTransforms[index].Key, restartTransforms[index].Value,
			false, nullptr, ETeleportType::ResetPhysics);

		if (attributeSet->GetHealth() <= 0)
		{
			if (commanders.Num() <= 0)
			{
				lGS->Lose();
				return;
			}
			if (commanders.IsValidIndex(0))
			{
				if (AAbstractCommander* commander = commanders[0]->GetPawn<AAbstractCommander>())
				{
					commander->BecomeViewTarget(pc);
				}
			}
		}
	}
	
	/*
	FVector resultRepelDirection = (GetActorLocation() - repelLocation).GetSafeNormal2D();
	float delta = GetWorld()->GetDeltaSeconds();
	if (delta > 1.f)
	{
		resultRepelDirection /= delta; 
	}*/
	/*FRotator rot = GetActorRotation();
	rot.Roll = 0.0;
	FVector loc = GetActorLocation() + resultRepelDirection * repulsive_Damaged ;
	SetActorLocationAndRotation(loc, rot, true, nullptr, ETeleportType::ResetPhysics);*/
	/*GetMesh()->AddImpulse(resultRepelDirection * repulsive_Damaged, NAME_None, true);*/
	UE_LOG(RacerLog, Warning, TEXT("==================== Damaged : %.1f"), attributeSet->GetHealth());
	if (attributeSet->GetHealth() <= 0)
	{
		GEngine->AddOnScreenDebugMessage(313, 5.f, FColor::Red, TEXT("!!!Destroy!!!"));
		if (pc && pc->IsLocalController())
		{
			if (pc->InputComponent)
			{
				pc->InputComponent->Deactivate();
			}
		}
		
	}	
}

UCameraComponent* AAbstractRacer::GetCCTVCamera_Implementation(int32 SlotIndex)
{
	return CCTVCameraComponent ? CCTVCameraComponent->GetCCTVCamera(SlotIndex) : nullptr;
}


bool AAbstractRacer::IsCCTVCameraInUse_Implementation(int32 SlotIndex)
{
	// 기본 구현: 항상 사용 중이 아님 (하위 클래스에서 오버라이드 필요)
	return false;
}

USceneCaptureComponent2D* AAbstractRacer::GetCCTVSceneCaptureComponent() const
{
	// PixelEnemy와 동일한 방식: FindComponentByClass를 사용하여 SceneCaptureComponent 찾기
	return FindComponentByClass<USceneCaptureComponent2D>();
}

void AAbstractRacer::CheckAndNotifyFirstInput()
{
	// 이미 첫 입력을 알렸으면 리턴
	if (bFirstInputSent)
	{
		return;
	}

	// 서버에서만 처리
	if (!HasAuthority())
	{
		return;
	}

	// GameState에 첫 입력 알림
	if (UWorld* World = GetWorld())
	{
		if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
		{
			bFirstInputSent = true;
			GameState->NotifyFirstPlayerInput();
			UE_LOG(RacerLog, Warning, TEXT("[AbstractRacer] 첫 번째 플레이어 입력 감지! GameState에 알림 전송"));
		}
	}
}

void AAbstractRacer::CheckVehicleMovement()
{
	// 서버에서만 처리
	if (!HasAuthority())
	{
		return;
	}

	// 이미 첫 움직임을 감지했으면 타이머 정지
	if (bFirstInputSent)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(MovementCheckTimer);
			UE_LOG(RacerLog, Log, TEXT("[AbstractRacer] 첫 움직임 감지 완료 - 타이머 정지"));
		}
		return;
	}

	// 차량 속도 확인 (GetVelocity().Size() 사용)
	const float CurrentSpeed = GetVelocity().Size();

	// 속도가 10.0 이상이면 "의미 있는 움직임"으로 판단
	if (CurrentSpeed > 10.0f)
	{
		UE_LOG(RacerLog, Warning, TEXT("[AbstractRacer] 차량 움직임 감지! 속도: %.2f"), CurrentSpeed);
		CheckAndNotifyFirstInput();
	}
}