// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/Pellet/PelletActor.h"
#include "Enemy/AbstractEnemy.h"
#include "Enemy/PixelEnemy.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

APelletActor::APelletActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	bAlwaysRelevant = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	SetRootComponent(RootComp);

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComp);
	OverlapSphere->SetSphereRadius(OverlapRadius);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Hit 발생을 위해 Physics 필요
	OverlapSphere->SetCollisionObjectType(ECC_WorldDynamic);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // PixelEnemy만 Block (Hit 발생)
	OverlapSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore); // Racer는 통과
	OverlapSphere->SetGenerateOverlapEvents(true);

	PelletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PelletMesh"));
	PelletMesh->SetupAttachment(RootComp);
	PelletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PelletMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	if (SphereMeshAsset.Succeeded())
	{
		PelletMesh->SetStaticMesh(SphereMeshAsset.Object);
		PelletMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
	}

	static ConstructorHelpers::FObjectFinder<UMaterial> RedMaterialAsset(TEXT("/Script/Engine.Material'/Engine/VREditor/UI/ArrowMaterial.ArrowMaterial'"));
	if (RedMaterialAsset.Succeeded())
	{
		PelletMesh->SetMaterial(0, RedMaterialAsset.Object);
	}

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = RotationRate;
}

void APelletActor::BeginPlay()
{
	Super::BeginPlay();

	if (OverlapSphere)
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &APelletActor::OnOverlapBegin);
	}

	if (RotatingMovement)
	{
		RotatingMovement->RotationRate = RotationRate;
	}
}

void APelletActor::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (!HasAuthority())
	{
		return;
	}

	if (!bIsAvailable)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[PelletActor] On cooldown, cannot be picked up"));
		return;
	}

	AAbstractEnemy* Enemy = Cast<AAbstractEnemy>(OtherActor);
	if (!Enemy)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PelletActor] P-Pellet collected by Enemy: %s"), *Enemy->GetName());

	APixelEnemy* PixelEnemy = Cast<APixelEnemy>(Enemy);
	if (PixelEnemy)
	{
		PixelEnemy->OnPelletCollected(InvulnerabilityDuration);
	}

	PlayPickupEffects();

	bIsAvailable = false;

	if (PelletMesh)
	{
		PelletMesh->SetVisibility(false);
	}

	if (OverlapSphere)
	{
		OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	GetWorld()->GetTimerManager().SetTimer(
		CooldownTimerHandle,
		this,
		&APelletActor::ReactivatePellet,
		CooldownDuration,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[PelletActor] P-Pellet on cooldown for %.1f seconds"), CooldownDuration);
}

void APelletActor::PlayPickupEffects()
{
	const FVector Location = GetActorLocation();
	const FRotator Rotation = GetActorRotation();

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, Location);
	}

	if (PickupNiagaraEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			PickupNiagaraEffect,
			Location,
			Rotation
		);
	}
	else if (PickupParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			PickupParticle,
			Location,
			Rotation
		);
	}
}

void APelletActor::ReactivatePellet()
{
	bIsAvailable = true;

	if (PelletMesh)
	{
		PelletMesh->SetVisibility(true);
	}

	if (OverlapSphere)
	{
		OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	UE_LOG(LogTemp, Log, TEXT("[PelletActor] P-Pellet reactivated and ready for pickup"));
}

float APelletActor::GetRemainingCooldown() const
{
	if (bIsAvailable)
	{
		return 0.0f;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		if (TimerManager.IsTimerActive(CooldownTimerHandle))
		{
			return TimerManager.GetTimerRemaining(CooldownTimerHandle);
		}
	}

	return 0.0f;
}
