#include "Player/Stats/MapInteractionActor.h"
#include "Interaction/InteractableComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

AMapInteractionActor::AMapInteractionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Static Mesh 컴포넌트 생성 (Root Component로 설정)
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = StaticMeshComponent;

	// Static Mesh 에셋 로드
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshAsset(
		TEXT("/Script/Engine.StaticMesh'/Game/CITRUSH/Commender/Apartment/Meshes/SM_ComputerMonitor_A01_N1.SM_ComputerMonitor_A01_N1'")
	);
	if (StaticMeshAsset.Succeeded())
	{
		StaticMeshComponent->SetStaticMesh(StaticMeshAsset.Object);
	}

	// InteractableComponent 생성
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
}

void AMapInteractionActor::BeginPlay()
{
	Super::BeginPlay();
	

	if (InteractableComponent)
	{
	
		UE_LOG(LogTemp, Log, TEXT("[MapInteractionActor] BeginPlay: InteractionRadius=%f"), InteractionRadius);
		
	}
}
