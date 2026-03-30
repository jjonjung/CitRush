// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Stats/Coin/CoinActor.h"
#include "Player/CommenderCharacter.h"
#include "Player/AbstractRacer.h"
#include "Enemy/PixelEnemy.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "AbilitySystemComponent.h"
#include "Data/CitRushPlayerTypes.h"
#include "GameFlow/CitRushGameState.h"
#include "GAS/AttributeSet/ASCommander.h"
#include "Subsystems/EnemyAISubsystem.h"

//DEFINE_LOG_CATEGORY(CoinActor);
ACoinActor::ACoinActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(true);

    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
    SetRootComponent(RootComp);

    OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
    OverlapSphere->SetupAttachment(RootComp);
    OverlapSphere->SetSphereRadius(OverlapRadius);

    OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    OverlapSphere->SetCollisionObjectType(ECC_WorldDynamic);
    OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    OverlapSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
    OverlapSphere->SetGenerateOverlapEvents(true);

    CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
    CoinMesh->SetupAttachment(RootComp);
    CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CoinMesh->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Script/Engine.StaticMesh'/Engine/EngineMeshes/Cube.Cube'"));
    if (CubeMeshAsset.Succeeded())
    {
        CoinMesh->SetStaticMesh(CubeMeshAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInstance> CoinMaterialAsset(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/CITRUSH/Commender/Apartment/Machine/secCol.secCol'"));
    if (CoinMaterialAsset.Succeeded())
    {
        CoinMesh->SetMaterial(0, CoinMaterialAsset.Object);
    }

    RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
    RotatingMovement->RotationRate = RotationRate;

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NiagaraAsset(TEXT("/Script/Niagara.NiagaraSystem'/Game/CITRUSH/CEJ/Coin/Particle_Coin.Particle_Coin'"));
    if (NiagaraAsset.Succeeded())
    {
        PickupNiagaraEffect = NiagaraAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<USoundBase> SoundAsset(TEXT("/Script/Engine.SoundWave'/Game/CITRUSH/CEJ/Coin/pixelCoin.pixelCoin'"));
    if (SoundAsset.Succeeded())
    {
        PickupSound = SoundAsset.Object;
    }
}

void ACoinActor::BeginPlay()
{
    Super::BeginPlay();

    if (!OverlapSphere)
    {
        UE_LOG(LogTemp, Error, TEXT("[CoinActor] OverlapSphere is NULL!"));
        return;
    }

    OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ACoinActor::OnOverlapBegin);

    if (RotatingMovement)
    {
        RotatingMovement->RotationRate = RotationRate;
    }
}

void ACoinActor::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    UE_LOG(LogTemp, Warning, TEXT("[CoinActor] OnOverlapBegin - OtherActor=%s"), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));

    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    if (!HasAuthority())
    {
        return;
    }

    if (AAbstractRacer* Racer = Cast<AAbstractRacer>(OtherActor))
    {
        TArray<AActor*> FoundCommanders;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACommenderCharacter::StaticClass(), FoundCommanders);
        if (FoundCommanders.Num() > 0)
        {
            if (ACommenderCharacter* FoundCommander = Cast<ACommenderCharacter>(FoundCommanders[0]))
            {
                FoundCommander->AddCoin(CoinValue);
                UE_LOG(LogTemp, Warning, TEXT("[CoinActor] Racer collected (Overlap)! Racer=%s, Value=%d, Commander Coin=%d"),
                    *Racer->GetName(), CoinValue, FoundCommander->CurrentCoin);
            }
        }

        if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
        {
            if (UEnemyAISubsystem* AISubsystem = GameInstance->GetSubsystem<UEnemyAISubsystem>())
            {
                AISubsystem->MarkPPointAsCollected(CoinID);
            }
        }

        PlayPickupEffects();
        Destroy();
        return;
    }

    if (APixelEnemy* Enemy = Cast<APixelEnemy>(OtherActor))
    {
        UE_LOG(LogTemp, Warning, TEXT("[CoinActor] PixelEnemy collected (Overlap)! Enemy=%s, CoinValue=%d"),
            *Enemy->GetName(), CoinValue);

        if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
        {
            if (UEnemyAISubsystem* AISubsystem = GameInstance->GetSubsystem<UEnemyAISubsystem>())
            {
                AISubsystem->MarkPPointAsCollected(CoinID);
            }
        }

        Enemy->OnCoinCollected(this);
        PlayPickupEffects();
        Destroy();
    }
}

void ACoinActor::PlayPickupEffects()
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

void ACoinActor::SetCoinValue(int32 NewValue)
{
    CoinValue = FMath::Max(1, NewValue);
}
