#include "Interaction/InteractionComponent.h"

#include "Data/InputMappingsSettings.h"
#include "Net/UnrealNetwork.h"
#include "Interaction/InteractableComponent.h"
#include "Engine/Engine.h"


UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
	
	if (const FInputMappingData* data = UInputMappingsSettings::Get()->inputMappings.Find("IMC_Interaction"))
	{
		IMC_Interaction = data->inputMappingContext;
		IA_Interaction = data->inputActions["IA_Interaction"];
	}
	

	ComponentTags.Add("Interaction");
}

void UInteractionComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UInteractionComponent, focusingActor);
}


void UInteractionComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
	
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (APawn* owner = GetOwner<APawn>())
	{
		if (owner->GetController<APlayerController>() && owner->IsLocallyControlled())
		{
			ownerPlayerController = owner->GetController<APlayerController>();
		}
	}
	if (IsValid(ownerPlayerController) && ownerPlayerController->IsLocalController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(ownerPlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(IMC_Interaction, 1);
		}
		if (UEnhancedInputComponent* eic = Cast<UEnhancedInputComponent>(ownerPlayerController->InputComponent))
		{
			eic->BindAction(IA_Interaction, ETriggerEvent::Started, this, &UInteractionComponent::InteractKeyInput);
		}
	}
}

void UInteractionComponent::InteractKeyInput(const FInputActionValue& value)
{
	UE_LOG(LogTemp, Display, TEXT("[InteractionComponent] InteractKeyInput 호출됨"));
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Magenta,
			TEXT("[InteractionComponent] E 키 입력 감지!")
		);
	}

	if (!IsValid(focusingActor))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Red,
				TEXT("[InteractionComponent] focusingActor가 유효하지 않습니다!")
			);
		}
		UE_LOG(LogTemp, Warning, TEXT("[InteractionComponent] focusingActor가 유효하지 않음"));
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Cyan,
			FString::Printf(TEXT("[InteractionComponent] focusingActor: %s"), *focusingActor->GetActorNameOrLabel())
		);
	}

	if (UInteractableComponent* interactable = focusingActor->FindComponentByClass<UInteractableComponent>())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Cyan,
				FString::Printf(TEXT("[InteractionComponent] InteractableComponent 찾음. IsMultiPlayable: %s"), 
					interactable->IsMultiPlayable() ? TEXT("True") : TEXT("False"))
			);
		}

		if (interactable->IsMultiPlayable())
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					2.0f,
					FColor::Blue,
					TEXT("[InteractionComponent] Server_Interact 호출")
				);
			}
			Server_Interact(interactable);
		}
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Blue,
				TEXT("[InteractionComponent] TryInteract 호출")
			);
		}
		interactable->TryInteract(ownerPlayerController);
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Red,
				TEXT("[InteractionComponent] InteractableComponent를 찾을 수 없습니다!")
			);
		}
		UE_LOG(LogTemp, Warning, TEXT("[InteractionComponent] InteractableComponent를 찾을 수 없음"));
	}
}

void UInteractionComponent::FocusInteractableActor(const FHitResult& hitResult)
{
	AActor* hitActor = hitResult.GetActor();
	if (hitActor == focusingActor) {return;}
	
	if (focusingActor)
	{
		if (UInteractableComponent* interactable = focusingActor->FindComponentByTag<UInteractableComponent>("Interactable"))
		{
			interactable->TryChangeState(GetOwner<APawn>()->GetController<APlayerController>(), EInteractableState::UnFocused);
		}
	}
	
	UE_LOG(LogTemp, Display, TEXT("    Client : %s - %s"), *GetOwner()->GetActorNameOrLabel(), *(hitActor ? hitActor->GetActorNameOrLabel() : "NULL"));
	Server_Focus(hitActor);
	
	if (hitActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Actor Name : %s"), *hitActor->GetActorNameOrLabel());
		if (UInteractableComponent* interactable = hitActor->FindComponentByClass<UInteractableComponent>())
		{
			interactable->TryChangeState(ownerPlayerController, EInteractableState::Focused);
		}
	}
}

void UInteractionComponent::Server_Focus_Implementation(AActor* focusedActor)
{
	//UE_LOG(LogTemp, Display, TEXT("    Server - %s"), *(focusedActor ? focusedActor->GetActorNameOrLabel() : "NULL"));
	focusingActor = focusedActor;
}

void UInteractionComponent::Server_Interact_Implementation(UInteractableComponent* interactable)
{
	if (!IsValid(interactable) || !IsValid(GetOwner<APawn>())) {return;}
	interactable->Multicast_TryInteract(GetOwner<APawn>());
}

void UInteractionComponent::Server_FinishInteraction_Implementation()
{
	if (!IsValid(GetOwner<APawn>())) {return;}
	focusingActor = nullptr;
}