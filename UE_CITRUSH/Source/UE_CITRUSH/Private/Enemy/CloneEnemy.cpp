// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/CloneEnemy.h"
#include "Enemy/AIEnemy/AIDirectiveComponent.h"
#include "Enemy/PixelEnemy.h"
#include "Subsystems/EnemyAISubsystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "GAS/AttributeSet/ASEnemy.h"
#include "Player/AbstractRacer.h"
#include "Net/UnrealNetwork.h"

ACloneEnemy::ACloneEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AIDirectiveComponent = CreateDefaultSubobject<UAIDirectiveComponent>(TEXT("AIDirectiveComponent"));

	// CharacterMovement 설정
	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	if (CharMovement)
	{
		CharMovement->DefaultLandMovementMode = MOVE_Flying;
		CharMovement->GravityScale = 0.0f;
		CharMovement->bConstrainToPlane = true;
		CharMovement->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Z);
		CharMovement->MaxWalkSpeed = MoveSpeed;
		CharMovement->MaxFlySpeed = MoveSpeed;
		CharMovement->bOrientRotationToMovement = false;
	}

	// Capsule 설정 (PixelEnemy와 동일)
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	}

	// AI Controller 자동 소유
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ACloneEnemy::BeginPlay()
{
	Super::BeginPlay();

	// AttributeSet 초기화
	if (attributeSet)
	{
		attributeSet->InitHealth(CloneHP);
		attributeSet->InitMaxHealth(CloneHP);
	}

	// CharacterMovement Flying 모드
	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	if (CharMovement)
	{
		CharMovement->MaxWalkSpeed = MoveSpeed;
		CharMovement->MaxFlySpeed = MoveSpeed;
		CharMovement->GravityScale = 0.0f;
		CharMovement->SetMovementMode(MOVE_Flying);
	}

	// AIController 스폰 (PixelEnemy 패턴)
	if (!GetController() && HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		AAIController* AIC = GetWorld()->SpawnActor<AAIController>(AIControllerClass, GetActorTransform(), SpawnParams);
		if (AIC)
		{
			AIC->Possess(this);
		}
	}

	// 충돌 설정
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsuleComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		CapsuleComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		CapsuleComp->OnComponentHit.AddDynamic(this, &ACloneEnemy::OnHitToRacer);
	}

	// 태그 설정
	Tags.AddUnique(TEXT("Enemy"));
	Tags.AddUnique(TEXT("Clone"));
}

void ACloneEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// AI Subsystem에서 등록 해제
	if (HasAuthority() && CachedAISubsystem)
	{
		CachedAISubsystem->UnregisterEnemy(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ACloneEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACloneEnemy, CloneIndex);
	DOREPLIFETIME(ACloneEnemy, bIsAlive);
}

// ========== IAIDecisionReceiver ==========

FString ACloneEnemy::GetEnemyID() const
{
	return FString::Printf(TEXT("clone_%d"), CloneIndex);
}

void ACloneEnemy::OnAIDecisionReceived(const FUnitCommand& Command)
{
	if (!bIsAlive || !AIDirectiveComponent) return;

	// FUnitCommand -> FDirectiveParams 변환 (PixelEnemy 패턴)
	FDirectiveParams DirectiveParams;

	DirectiveParams.TargetPosition = FVector(
		Command.params.target_position.x,
		Command.params.target_position.y,
		Command.params.target_position.z
	);
	DirectiveParams.TargetPlayerId = Command.params.target_player_id;
	DirectiveParams.SpeedFactor = Command.params.speed_factor;
	DirectiveParams.Priority = Command.params.priority;

	// RETREAT 파라미터
	FVector SafePos = FVector(
		Command.params.safe_zone_position.x,
		Command.params.safe_zone_position.y,
		Command.params.safe_zone_position.z
	);
	DirectiveParams.SafeZonePosition = SafePos;
	if (Command.directive_code == 5)
	{
		DirectiveParams.TargetPosition = SafePos;
	}

	// PATROL 파라미터
	DirectiveParams.PatrolZone = Command.params.patrol_zone;

	// CONSUME_P_POINT 파라미터
	DirectiveParams.PPointId = Command.params.p_point_id;
	DirectiveParams.EmergencyPriority = Command.params.emergency_priority;

	// CONSUME_PELLET 파라미터
	DirectiveParams.PelletId = Command.params.pellet_id;

	// GUARD / FLANK / FAKE_RETREAT 파라미터
	DirectiveParams.GuardTarget = Command.params.guard_target;
	DirectiveParams.FlankDirection = Command.params.flank_direction;
	DirectiveParams.FakeRetreatDuration = Command.params.fake_retreat_duration;
	DirectiveParams.CounterAttackPosition = FVector(
		Command.params.counter_attack_position.x,
		Command.params.counter_attack_position.y,
		Command.params.counter_attack_position.z
	);

	AIDirectiveComponent->ProcessDirective(Command.directive_code, DirectiveParams);
}

FEnemyGameState ACloneEnemy::GetCurrentGameState() const
{
	FEnemyGameState State;

	State.unit_id = GetEnemyID();
	State.position = GetActorLocation();

	if (attributeSet)
	{
		UASEnemy* EnemyAS = Cast<UASEnemy>(attributeSet);
		if (EnemyAS)
		{
			State.health = EnemyAS->GetHealth();
			State.max_health = EnemyAS->GetMaxHealth();
		}
	}

	State.speed = GetVelocity().Size();
	State.rotation_yaw_deg = GetActorRotation().Yaw;
	State.forward_vector = GetActorForwardVector();

	return State;
}

// ========== Clone Lifecycle ==========

void ACloneEnemy::InitializeClone(int32 Index, APixelEnemy* InOwner)
{
	CloneIndex = Index;
	OwnerPixelEnemy = InOwner;

	// EnemyAISubsystem에 등록
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedAISubsystem = GI->GetSubsystem<UEnemyAISubsystem>();
		if (CachedAISubsystem)
		{
			CachedAISubsystem->RegisterEnemy(this);
			UE_LOG(LogTemp, Log, TEXT("[CloneEnemy] Registered clone_%d to AI Subsystem"), CloneIndex);
		}
	}
}

void ACloneEnemy::Multicast_CloneDeath_Implementation()
{
	bIsAlive = false;

	// AI Subsystem 등록 해제
	if (CachedAISubsystem)
	{
		CachedAISubsystem->UnregisterEnemy(this);
	}

	// 부모에게 사망 알림
	OnCloneDeath.ExecuteIfBound(this);

	// 비활성화
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// 짧은 수명 후 제거
	SetLifeSpan(0.1f);

	UE_LOG(LogTemp, Log, TEXT("[CloneEnemy] clone_%d died"), CloneIndex);
}

// ========== Collision ==========

void ACloneEnemy::OnHitToRacer(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bIsAlive || !HasAuthority()) return;

	AAbstractRacer* Racer = Cast<AAbstractRacer>(OtherActor);
	if (!Racer) return;

	FVector NormDir = (Racer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	float Dot = NormDir.Dot(GetActorForwardVector());

	if (Dot >= 0.0f)
	{
		// 정면 충돌: Racer에 데미지
		Racer->NetMulticastRPC_Damaged(Hit.ImpactPoint, Hit.ImpactNormal);
	}
	else
	{
		// 후면 충돌: 분신 사망
		Multicast_CloneDeath();
	}
}
