// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AbstractCommander.h"

#include "Player/CitRushPlayerState.h"
#include "Player/AbstractRacer.h"
#include "Player/CCTV/CCTVCameraComponent.h"

#include "GameplayTags.h"
#include "NiagaraValidationRule.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GAS/AbilitySystemComponent/BaseASC.h"
#include "GAS/AttributeSet/ASCommander.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/ViewComponent.h"
#include "Player/CCTV/MonitorActor.h"
#include "Player/CCTV/CCTVFeedComponent.h"
#include "UI/CCTVControlWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"

#include "Network/VoiceWebSocketComponent.h"
#include "Player/Components/DefaultVoiceComponent.h"
#include "Player/Components/VoiceCaptureComponent.h"
#if WITH_STEAMWORKS
	#include "Player/Components/SteamVoiceComponent.h"
#else
	#include "Player/Components/VoiceCaptureComponent.h"
	#include "Player/Components/VoiceDonorComponent.h"
#endif

DEFINE_LOG_CATEGORY_CLASS(AAbstractCommander, CommanderLog);

AAbstractCommander::AAbstractCommander()
{
	PrimaryActorTick.bCanEverTick = false;

	commanderInputMode = FInputModeGameOnly();
	commanderInputMode.SetConsumeCaptureMouseDown(false);
	

	viewComponent = CreateDefaultSubobject<UViewComponent>("View");
	interactionComponent = CreateDefaultSubobject<UInteractionComponent>("Interaction");
	
	#if WITH_STEAMWORKS
		steamVoiceComponent = CreateDefaultSubobject<USteamVoiceComponent>(TEXT("Voice"));
	#else
		rawVoiceCaptureComponent = CreateDefaultSubobject<UVoiceCaptureComponent>(TEXT("VoiceCapture"));
		rawVoiceDonorComponent = CreateDefaultSubobject<UVoiceDonorComponent>(TEXT("VoiceDonor"));
	#endif
}

void AAbstractCommander::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		InitInputMode(PC);
	}
}

void AAbstractCommander::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	InitAbilitySystem();
	InitVoiceInterface();
	InitInteractionSystem();
}

void AAbstractCommander::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		UE_LOG(CommanderLog, Error, TEXT("[SetupInput] PlayerInputComponent가 null입니다!"));
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		InitInputMode(PC);
	}

	// Tab 키는 CCTV 위젯에서 처리하므로 여기서는 바인딩하지 않음
}

void AAbstractCommander::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		InitInputMode(PC);
	}
	
	InitAbilitySystem();
	InitDefaultAttributes();
	InitVoiceInterface();
	InitInteractionSystem();
}

void AAbstractCommander::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	// Voice 컴포넌트 정리는 자동으로 처리됨
}


void AAbstractCommander::InitInputMode(APlayerController* playerController)
{
	playerController->SetInputMode(commanderInputMode);

	// 마우스 커서 및 마우스 관련 이벤트 활성화
	playerController->bShowMouseCursor = false;
	playerController->bEnableClickEvents = true;
	playerController->bEnableMouseOverEvents = true;
}

AActor* AAbstractCommander::GetFocusedActor()
{
	if (!IsValid(interactionComponent)) {return nullptr;}
	return interactionComponent->GetFocusedActor();
}

void AAbstractCommander::InitInteractionSystem()
{
	if (!IsLocallyControlled()) {return;}
	
	viewComponent->OnViewSthByLineTrace.AddDynamic(interactionComponent, &UInteractionComponent::FocusInteractableActor);
	viewComponent->EnableTrace(true);
}



void AAbstractCommander::InitVoiceInterface()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
	#if WITH_STEAMWORKS
		if (steamVoiceComponent)
		{
			steamVoiceComponent->RegisterLocalUser(PC, GetPlayerState());
		}
	#else
		if (rawVoiceCaptureComponent && voiceWebSocketComponent)
		{
			rawVoiceCaptureComponent->InitializeVoiceCapture();

			if (rawVoiceDonorComponent)
			{
				rawVoiceDonorComponent->RegisterVoiceCaptureComponent(rawVoiceCaptureComponent);
			}

			voiceWebSocketComponent->RegisterCaptureComponent(rawVoiceCaptureComponent);
		}
	#endif
	}
}

UAbilitySystemComponent* AAbstractCommander::GetAbilitySystemComponent() const
{
	return abilitySystemComponent;
}

UAttributeSet* AAbstractCommander::GetAttributeSet() const
{
	return attributeSet;
}

void AAbstractCommander::InitDefaultAttributes() const
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

bool AAbstractCommander::ActivateAbility(const TSubclassOf<UGameplayAbility>& ability)
{
	return false;
}

bool AAbstractCommander::ActivateAbility(const FName& gameplayTagName)
{
	return false;
}

bool AAbstractCommander::ReceiveAbility(const TSubclassOf<UGameplayAbility>& ability)
{
	return false;
}

bool AAbstractCommander::ReceiveAbility(const FName& gameplayTagName)
{
	return false;
}

void AAbstractCommander::InitAbilitySystem()
{
	if (ACitRushPlayerState* cPS = GetPlayerState<ACitRushPlayerState>())
	{
		abilitySystemComponent = CastChecked<UBaseASC>(cPS->GetAbilitySystemComponent());
		abilitySystemComponent->InitAbilityActorInfo(cPS, this);
		attributeSet = Cast<UASCommander>(cPS->GetAttributeSet(EPlayerRole::Commander));
		if (attributeSet)
		{
			attributeSet->InitMaxCommandPoints(10000.f);
			attributeSet->InitCommandPoints(1000.f);
		}
	}
}


// ========== CCTV ==========

bool AAbstractCommander::IsLocalController() const
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		return PC->IsLocalController();
	}
	return false;
}

void AAbstractCommander::HandleCCTVSwitchCameraTab()
{
	UE_LOG(CommanderLog, Log, TEXT("[HandleCCTVSwitchCameraTab] Tab 키 입력 감지"));
	
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		UE_LOG(CommanderLog, Verbose, TEXT("[HandleCCTVSwitchCameraTab] 로컬 컨트롤러가 아닙니다"));
		return;
	}
	
	FindMonitorActor();
	if (!ActiveMonitorActor || !ActiveMonitorActor->IsCCTVOpen())
	{
		UE_LOG(CommanderLog, Verbose, TEXT("[HandleCCTVSwitchCameraTab] MonitorActor가 없거나 CCTV가 열려있지 않습니다 - ActiveMonitorActor: %p, IsCCTVOpen: %s"), 
			ActiveMonitorActor.Get(), ActiveMonitorActor ? (ActiveMonitorActor->IsCCTVOpen() ? TEXT("true") : TEXT("false")) : TEXT("null"));
		return;
	}
	
	if (UCCTVFeedComponent* FeedComponent = ActiveMonitorActor->GetCCTVFeedComponent())
	{
		if (!FeedComponent->IsExpanded())
		{
			UE_LOG(CommanderLog, Verbose, TEXT("[HandleCCTVSwitchCameraTab] 확대 화면이 아닙니다"));
			return;
		}
		
		int32 FocusIndex = FeedComponent->GetFocusIndex();
		if (FocusIndex < 0 || FocusIndex >= 3)
		{
			UE_LOG(CommanderLog, Warning, TEXT("[HandleCCTVSwitchCameraTab] 유효하지 않은 FocusIndex: %d"), FocusIndex);
			return;
		}
		
		FCCTVFeedSlot FeedSlot = FeedComponent->GetFeedSlot(FocusIndex);
		if (!FeedSlot.TargetPawn)
		{
			UE_LOG(CommanderLog, Warning, TEXT("[HandleCCTVSwitchCameraTab] TargetPawn이 null입니다 - FocusIndex: %d"), FocusIndex);
			return;
		}
		
		AAbstractRacer* Racer = Cast<AAbstractRacer>(FeedSlot.TargetPawn);
		if (!Racer || !Racer->CCTVCameraComponent)
		{
			UE_LOG(CommanderLog, Warning, TEXT("[HandleCCTVSwitchCameraTab] Racer 또는 CCTVCameraComponent가 null입니다 - Racer: %s"), 
				Racer ? *Racer->GetName() : TEXT("null"));
			return;
		}
		
		UE_LOG(CommanderLog, Log, TEXT("[HandleCCTVSwitchCameraTab] 확대 화면에서 Tab 키 입력 - FocusIndex: %d, Racer: %s"), 
			FocusIndex, *Racer->GetName());
		
		// 멀티플레이에서 서버로 전달하기 위해 MonitorActor의 Server RPC 호출
		ActiveMonitorActor->ServerSwitchCameraSlot(true);
	}
	else
	{
		UE_LOG(CommanderLog, Warning, TEXT("[HandleCCTVSwitchCameraTab] CCTVFeedComponent를 찾을 수 없습니다"));
	}
}

void AAbstractCommander::FindMonitorActor()
{
	if (ActiveMonitorActor && IsValid(ActiveMonitorActor))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundMonitors;
	UGameplayStatics::GetAllActorsOfClass(World, AMonitorActor::StaticClass(), FoundMonitors);

	if (FoundMonitors.Num() > 0)
	{
		ActiveMonitorActor = Cast<AMonitorActor>(FoundMonitors[0]);
	}
}

