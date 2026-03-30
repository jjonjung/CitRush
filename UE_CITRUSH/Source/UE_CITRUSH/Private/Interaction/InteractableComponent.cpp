#include "Interaction/InteractableComponent.h"

#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Engine/Engine.h"
#include "Particles/ParticleSystem.h"
#include "Utility/DebugHelper.h"
#include "CommenderSystem/ItemInputMachine.h"
#include "Player/CommenderCharacter.h"
#include "UI/CommenderHUDWidget.h"
#include "UI/CrosshairWidget.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "CommenderSystem/VendingItemActor.h"


UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Tick 사용 안 함 - 이벤트 기반으로 처리
		
	ComponentTags.Add(TEXT("Interactable"));

	bWantsInitializeComponent = true;
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void UInteractableComponent::InitializeComponent()
{
	Super::InitializeComponent();

	AActor* owner = GetOwner();
	if (!IsValid(owner)) { return; }

	interactionSphere = NewObject<USphereComponent>(owner, USphereComponent::StaticClass(), TEXT("InteractionSphere"));
	if (!IsValid(interactionSphere)) { return; }
	
	owner->AddInstanceComponent(interactionSphere);
	if (IsValid(owner->GetRootComponent()))
	{
		interactionSphere->AttachToComponent(owner->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		interactionSphere->SetSphereRadius(interactionRadius);
		interactionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		interactionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		interactionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		interactionSphere->SetGenerateOverlapEvents(true);
		interactionSphere->RegisterComponent();
		interactionSphere->OnComponentBeginOverlap.AddDynamic(this, &UInteractableComponent::OnInteractionSphereBeginOverlap);
		interactionSphere->OnComponentEndOverlap.AddDynamic(this, &UInteractableComponent::OnInteractionSphereEndOverlap);
	}
	
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();
	
	clientState = EInteractableState::OutOfBound;

	if (IsValid(interactionSphere))
	{
		interactionSphere->SetSphereRadius(interactionRadius);
		interactionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		interactionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		interactionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	}

	// MeshComponent 확인만 수행 (아웃라인 초기화 제거)
	UpdateAvailablePrimitiveComponents();
	
	if (feedbackSettings.IsWidgetOn() && feedbackSettings.interactionGuideWidgetClass)
	{
		interactionGuideComponent = NewObject<UWidgetComponent>(GetOwner(), UWidgetComponent::StaticClass(), TEXT("GuideWidgetComponent"));
		if (IsValid(interactionGuideComponent))
		{
			GetOwner()->AddInstanceComponent(interactionGuideComponent);
			
			UUserWidget* widget = CreateWidget(GetWorld(), feedbackSettings.interactionGuideWidgetClass, FName("WidgetForGuide"));
			if (IsValid(widget))
			{
				// 항상 상대 회전으로 고정 (카메라 방향 무시)
				FAttachmentTransformRules attachRules(
				EAttachmentRule::SnapToTarget,  // Location
				EAttachmentRule::KeepRelative,  // Rotation (항상 상대 회전 유지)
				EAttachmentRule::KeepRelative,  // Scale
				false 
				);
				attachRules.bWeldSimulatedBodies = false;
				if (feedbackSettings.widgetSocketName != NAME_None)
				{
					TArray<USceneComponent*> components;
					GetOwner()->GetComponents(components);
					for (USceneComponent* component : components)
					{
						if (component->DoesSocketExist(feedbackSettings.widgetSocketName))
						{
							interactionGuideComponent->AttachToComponent(component, attachRules, feedbackSettings.widgetSocketName);
							break;
						}
					}
				}
				if (interactionGuideComponent->GetAttachSocketName() == NAME_None)
				{
					interactionGuideComponent->AttachToComponent(GetOwner()->GetRootComponent(), attachRules);	
				}
				interactionGuideComponent->SetWidget(widget);
				interactionGuideComponent->SetWidgetSpace(EWidgetSpace::World);
				interactionGuideComponent->SetAbsolute(false, true, true);
				
				// Widget 크기 설정 (글씨 잘림 방지) - 충분한 크기로 설정
				interactionGuideComponent->SetDrawSize(FVector2D(300.0f, 150.0f));
				
				// 위젯에 상호작용 텍스트 전달 (블루프린트 위젯이 SetInteractionText 함수를 구현해야 함)
				if (widget && !feedbackSettings.interactionText.IsEmpty())
				{
					// 블루프린트에서 호출 가능한 함수로 텍스트 전달 시도
					FString FunctionName = TEXT("SetInteractionText");
					UFunction* SetTextFunction = widget->FindFunction(*FunctionName);
					if (SetTextFunction && SetTextFunction->NumParms == 1)
					{
						FText TextToSet = feedbackSettings.interactionText;
						widget->ProcessEvent(SetTextFunction, &TextToSet);
					}
				}
				
				// 🔥 위젯 회전 오프셋 적용 (에디터에서 설정한 값으로 고정 회전)
				// 항상 오프셋 값으로 상대 회전 설정 (카메라 방향 무시)
				interactionGuideComponent->SetRelativeRotation(feedbackSettings.widgetRotationOffset);
				
				// 거리 기반 표시는 UpdateVisuals에서 처리
				
				interactionGuideComponent->RegisterComponent();

				interactionGuideComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				interactionGuideComponent->SetVisibility(false);
				interactionGuideComponent->SetCastShadow(false);
				interactionGuideComponent->SetReceivesDecals(false);
			}
		}
	}

	if (feedbackSettings.IsNetworkOn())
	{
	}
}

void UInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	interactionSphere->OnComponentBeginOverlap.Clear();
	interactionSphere->OnComponentEndOverlap.Clear();
	playerInRange = nullptr;
	TryChangeState(GetWorld()->GetFirstPlayerController(), EInteractableState::Default);
	OnClientInteraction.Clear();
	OnMultiInteraction.Clear();
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void UInteractableComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UInteractableComponent, interactionRadius))
	{
		if (IsValid(interactionSphere))
		{
			interactionSphere->SetSphereRadius(interactionRadius);
		}
	}
}
#endif //WITH_EDITOR

// TickComponent 제거됨 - UpdateVisuals에서 이벤트 기반으로 처리

void UInteractableComponent::TryChangeState(APlayerController* playerController, EInteractableState newState)
{
	if (!LIKELY(IsValid(playerController))) {return;}
	if (!playerController->IsLocalController()) {return;}
	if (clientState == EInteractableState::Default) {return;}
	
	switch (newState)
	{
	case EInteractableState::Default:
		clientState = EInteractableState::Default;
		break;
		
	case EInteractableState::UnFocused:
	case EInteractableState::InRange:
		{
			if (!IsValid(playerInRange)) {return TryChangeState(playerController, EInteractableState::OutOfBound);}
			UInteractionComponent* interactionComp = playerInRange->FindComponentByTag<UInteractionComponent>("Interaction");
			if (IsValid(interactionComp))
			{
				clientState = EInteractableState::InRange;			
				if (interactionComp->GetFocusedActor() != GetOwner()) {break;}
			}
			else {return TryChangeState(playerController, EInteractableState::OutOfBound);}
		}

	case EInteractableState::Focused:
		{
			if (!EnumHasAnyFlags(clientState, EInteractableState::InRange)) {return;}
			clientState = EInteractableState::Focused;
			break;
		}
	
	case EInteractableState::OutOfBound:
		{
			playerInRange = nullptr;
			clientState = EInteractableState::OutOfBound;
			break;
		}
	}

	// 상태 변경 이벤트 브로드캐스트
	if (OnStateChanged.IsBound())
	{
		OnStateChanged.Broadcast(playerController, clientState);
	}

	UpdateVisuals(playerController);
}

void UInteractableComponent::TryInteract(APlayerController* playerController)
{
	if (!LIKELY(IsValid(playerController))) {return;}
	if (!playerController->IsLocalController()) {return;}
	if (clientState != EInteractableState::Focused) {return;}

	if (OnClientInteraction.IsBound())
	{
		OnClientInteraction.Broadcast(playerController);
		PlayEffects(true);
	}
}

void UInteractableComponent::Multicast_TryInteract_Implementation(APawn* player)
{
	if (!IsValid(player) || !feedbackSettings.IsNetworkOn()) {return;}
	
	if (OnMultiInteraction.IsBound())
	{
		OnMultiInteraction.Broadcast(player);
		if (feedbackSettings.bInteractionEffectsInNetwork) {PlayEffects(true);}
	}
}

void UInteractableComponent::FinishInteracting(APlayerController* playerController, bool bSuccess)
{
	if (!LIKELY(IsValid(playerController))) {return;}
	if (!playerController->IsLocalController()) {return;}
	
	if (bSuccess)
	{
		TryChangeState(playerController, EInteractableState::OutOfBound);
		return;
	}
	TryChangeState(playerController, EInteractableState::InRange);
}

void UInteractableComponent::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                                             AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                                             const FHitResult& SweepResult)
{
	APawn* player = Cast<APawn>(OtherActor);
	if (!IsValid(player)) {return;}
	
	APlayerController* pc = player->GetController<APlayerController>();
	if (IsValid(pc) && pc->IsLocalController())
	{
		if (playerInRange == player) {return;}
		playerInRange = player;
		TryChangeState(pc, EInteractableState::InRange);
	}
	
}

void UInteractableComponent::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* player = Cast<APawn>(OtherActor);
	if (!IsValid(player)) {return;}
	
	APlayerController* pc = player->GetController<APlayerController>();
	if (IsValid(pc) && pc->IsLocalController())
	{
		if (playerInRange != player) {return;}
		playerInRange = nullptr;
		TryChangeState(pc, EInteractableState::OutOfBound);
	}
}

bool UInteractableComponent::UpdateAvailablePrimitiveComponents()
{
	if (feedbackSettings.ownerMeshComponent.IsValid()) {return true;}
	
	AActor* owner = GetOwner();
	if (!IsValid(owner)) {return false;}
	
	TArray<UMeshComponent*> activeComponents;
	owner->GetComponents<UMeshComponent>(activeComponents);
	if (activeComponents.Num() == 0) {return false;}
	feedbackSettings.ownerMeshComponent = activeComponents[0];
	return feedbackSettings.ownerMeshComponent.IsValid();
}

void UInteractableComponent::UpdateVisuals(APlayerController* playerController)
{
	if (!LIKELY(IsValid(playerController))) {return;}
	APlayerController* pc = GetWorld()->GetFirstPlayerController();
	if (pc != playerController) {return;}

	bool bShouldShowOutline = (clientState == EInteractableState::Focused);

	// 커스텀 머티리얼 아웃라인
	if (feedbackSettings.IsOutlineOn())
	{
		UpdateCustomOutline(bShouldShowOutline);
	}

	// Widget - 거리 기반 표시 및 회전 업데이트
	if (feedbackSettings.IsWidgetOn() && IsValid(interactionGuideComponent))
	{
		// 플레이어와의 거리 계산
		APawn* PlayerPawn = playerController->GetPawn();
		if (IsValid(PlayerPawn) && IsValid(GetOwner()))
		{
			float DistanceToPlayer = FVector::Dist(GetOwner()->GetActorLocation(), PlayerPawn->GetActorLocation());
			
			// 거리 기반 Visibility: 범위 내에 있을 때만 표시
			// 멀리 있을 때는 안 보이고, 가까이 다가가면 보임
			bool bShouldShowWidget = DistanceToPlayer <= interactionRadius;
			
			interactionGuideComponent->SetVisibility(bShouldShowWidget);
		}
		else
		{
			interactionGuideComponent->SetVisibility(false);
		}
	}

	// Interactable 시스템에서 Commander에게 포커스 정보 전달
	if (ACommenderCharacter* Commander = Cast<ACommenderCharacter>(playerController->GetPawn()))
	{
		if (bShouldShowOutline) // Focused 상태일 때
		{
			Commander->SetFocusedInteractable(this);
		}
		else
		{
			Commander->SetFocusedInteractable(nullptr);
		}
	}
}

void UInteractableComponent::PlayEffects(bool bSuccess)
{
	if (feedbackSettings.IsSoundOn()) {PlaySound(feedbackSettings.interactedSound);}
	if (feedbackSettings.IsNiagaraOn()) {PlayEffect(feedbackSettings.interactedNiagaraVFX);}
	if (feedbackSettings.IsParticleOn()) {PlayEffect(feedbackSettings.interactedParticleVFX);}
}

void UInteractableComponent::PlaySound(USoundBase* sound)
{
	if (!IsValid(sound)) {return;}
	
	if (sound->IsPlayable())
	{
		UGameplayStatics::SpawnSoundAtLocation(GetWorld(), sound, GetOwner()->GetActorLocation());
	}
}

void UInteractableComponent::PlayEffect(UParticleSystem* effect)
{
	if (!IsValid(effect)) {return;}

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), effect, GetOwner()->GetActorLocation());
}

void UInteractableComponent::PlayEffect(UNiagaraSystem* effect)
{
	if (!IsValid(effect)) {return;}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), effect, GetOwner()->GetActorLocation());
}

void UInteractableComponent::UpdateCustomOutline(bool bShowOutline)
{
	if (!feedbackSettings.customOutlineMaterial)
	{
		return;
	}

	AActor* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	// 모든 MeshComponent 가져오기
	TArray<UMeshComponent*> meshComponents;
	owner->GetComponents<UMeshComponent>(meshComponents);

	if (meshComponents.Num() == 0)
	{
		return;
	}

	if (bShowOutline)
	{
		if (feedbackSettings.bUseOverlayMaterial)
		{
			// Overlay Material 방식 (기존 머티리얼 유지, 위에 오버레이)
			for (UMeshComponent* meshComp : meshComponents)
			{
				if (meshComp)
				{
					meshComp->SetOverlayMaterial(feedbackSettings.customOutlineMaterial);
				}
			}
		}
		else
		{
			// 머티리얼 교체 방식 (기존 방식)
			originalMaterials.Empty();

			for (UMeshComponent* meshComp : meshComponents)
			{
				if (!meshComp) continue;

				// 원본 머티리얼 백업
				int32 numMaterials = meshComp->GetNumMaterials();
				for (int32 i = 0; i < numMaterials; i++)
				{
					originalMaterials.Add(meshComp->GetMaterial(i));
				}

				// 커스텀 아웃라인 머티리얼로 교체
				for (int32 i = 0; i < numMaterials; i++)
				{
					meshComp->SetMaterial(i, feedbackSettings.customOutlineMaterial);
				}
			}
		}
	}
	else
	{
		if (feedbackSettings.bUseOverlayMaterial)
		{
			// Overlay Material 제거
			for (UMeshComponent* meshComp : meshComponents)
			{
				if (meshComp)
				{
					meshComp->SetOverlayMaterial(nullptr);
				}
			}
		}
		else
		{
			// 원본 머티리얼 복원
			if (originalMaterials.Num() > 0)
			{
				int32 materialIndex = 0;
				for (UMeshComponent* meshComp : meshComponents)
				{
					if (!meshComp) continue;

					int32 numMaterials = meshComp->GetNumMaterials();
					for (int32 i = 0; i < numMaterials && materialIndex < originalMaterials.Num(); i++)
					{
						meshComp->SetMaterial(i, originalMaterials[materialIndex]);
						materialIndex++;
					}
				}

				originalMaterials.Empty();
			}
		}
	}
}

