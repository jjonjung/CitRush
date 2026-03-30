// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/PixelEnemy.h"
#include "Enemy/AIEnemy/AIDirectiveComponent.h"
#include "Subsystems/EnemyAISubsystem.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Components/TimelineComponent.h"
#include "Data/DataTypeObjectManager.h"
#include "GAS/AttributeSet/ASEnemy.h"
#include "Player/AbstractRacer.h"

#include "Player/CCTV/CCTVCameraComponent.h"
#include "Player/CCTV/CCTVFeedComponent.h"
#include "Player/Components/MinimapIconComponent.h"

// CCTV 로그 매크로 (모든 CCTV 관련 로그를 [CCTVLog] 태그로 통일)
#define CCTV_LOG(Verbosity, Format, ...) UE_LOG(LogCCTVFeed, Verbosity, TEXT("[CCTVLog] " Format), ##__VA_ARGS__)
#include "AIController.h"
#include "Player/Car/VehicleDemoCejCar.h"
#include "Player/Stats/Coin/CoinActor.h"
#include "Enemy/Pellet/PelletActor.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "Network/NavSystemDataComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "GeometryCollection/GeometryCollectionActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFlow/CitRushGameState.h"

// StateTree + AIPerception
#include "Components/StateTreeComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"

// Components
#include "Enemy/Components/EnemyPelletComponent.h"
#include "Enemy/Components/EnemyCombatComponent.h"
#include "Enemy/CloneEnemy.h"

APixelEnemy::APixelEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIDirectiveComponent = CreateDefaultSubobject<UAIDirectiveComponent>(TEXT("AIDirectiveComponent"));
	CaptureGaugeComponent = CreateDefaultSubobject<UCaptureGaugeComponent>(TEXT("CaptureGaugeComponent"));

	// StateTree Component (Fallback AI)
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false); // Manual control

	// AIPerception Component
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

	// P-Pellet Component
	PelletComponent = CreateDefaultSubobject<UEnemyPelletComponent>(TEXT("PelletComponent"));

	// Combat Component
	CombatComponent = CreateDefaultSubobject<UEnemyCombatComponent>(TEXT("CombatComponent"));

	if (abilitySystemComponent)
	{
		abilitySystemComponent->SetIsReplicated(true);
		abilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

		attributeSet->InitMaxHealth(100.f);
		attributeSet->InitHealth(100.f);
	}

	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	if (CharMovement)
	{
		CharMovement->GravityScale = 0.0f;
		CharMovement->DefaultLandMovementMode = MOVE_Walking;
		CharMovement->bConstrainToPlane = true;
		CharMovement->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Z);
		CharMovement->MaxWalkSpeed = MoveSpeed;
		CharMovement->MaxFlySpeed = MoveSpeed;
		CharMovement->bOrientRotationToMovement = false;
		CharMovement->GravityScale = 0.0f;
	}

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	if (!GetCharacterMovement())
	{
		CreateDefaultSubobject<UCharacterMovementComponent>(UCharacterMovementComponent::StaticClass()->GetFName());
	}
	SetRootComponent(GetCapsuleComponent());
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bAutoActivate = true;

	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
	SceneCaptureComponent->bCaptureEveryFrame = true;
	SceneCaptureComponent->bCaptureOnMovement = true;
	SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	// HDR 비교용: SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;

	if (CCTVRenderTarget)
	{
		SceneCaptureComponent->TextureTarget = CCTVRenderTarget;
	}

	CCTVCameraComponent = CreateDefaultSubobject<UCCTVCameraComponent>(TEXT("CCTVCameraComponent"));

	MinimapIconComponent = CreateDefaultSubobject<UMinimapIconComponent>(TEXT("MinimapIconComponent"));
	if (MinimapIconComponent)
	{
		MinimapIconComponent->IconId = ERealtimeMapIconId::Enemy;
		MinimapIconComponent->bShowOnMap = true;
		MinimapIconComponent->bRotateWithActor = true;
	}

	NavSystemDataComponent = CreateDefaultSubobject<UNavSystemDataComponent>(TEXT("NavSystemDataComponent"));

	// Shield Mesh Component 생성 (에셋은 BeginPlay 또는 Blueprint에서 설정)
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	ShieldMesh->SetupAttachment(RootComponent);
	ShieldMesh->SetVisibility(false);
	ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldMesh->SetRelativeScale3D(FVector(0.4f));

	// Link ShieldMesh to PelletComponent
	if (PelletComponent)
	{
		PelletComponent->ShieldMesh = ShieldMesh;
	}

	// Note: DamagedEffect, DieEffect, ShieldMeshAsset, ShieldMaterialAsset은
	// Blueprint에서 EditDefaultsOnly로 설정됩니다.
	// 기존 ConstructorHelpers는 제거됨 - BP_PixelEnemy에서 설정 필요
	
	bAIEnabled = true;
	EnemyAISubsystem = nullptr;

	flashTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("FlashTimeLine"));
	if (const UDataTypeObjectManager* data = UDataTypeObjectManager::Get())
	{
		flashCurve = data->GetCurveFloatAsset(TEXT("FlashCurve"));
	}

}
	
void APixelEnemy::BeginPlay()
{
	Super::BeginPlay();

	// Apply Shield assets from Blueprint settings
	if (ShieldMesh)
	{
		if (ShieldMeshAsset)
		{
			ShieldMesh->SetStaticMesh(ShieldMeshAsset);
		}
		if (ShieldMaterialAsset)
		{
			ShieldMesh->SetMaterial(0, ShieldMaterialAsset);
		}
	}

	// Setup AIPerception (Server-only for multiplayer)
	if (HasAuthority())
	{
		SetupAIPerception();

		// Setup StateTree if asset is assigned
		if (StateTreeComponent && FallbackStateTree)
		{
			StateTreeComponent->SetStateTree(FallbackStateTree);
		}
	}

	if (ACitRushGameState* cGS = GetWorld()->GetGameState<ACitRushGameState>())
	{
		cGS->OnFirstPlayerInput.AddDynamic(this, &APixelEnemy::EquipBrain);
		UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] OnFirstPlayerInput 델리게이트 바인딩 완료 - 첫 입력 시 AI 활성화"));
	}

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsuleComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		CapsuleComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		CapsuleComp->SetEnableGravity(true);
		CapsuleComp->OnComponentHit.AddDynamic(this, &APixelEnemy::OnHitToRacer);
	}
	// Setup CombatComponent references
	if (CombatComponent)
	{
		CombatComponent->OwnerMesh = GetMesh();
		CombatComponent->FlashCurve = flashCurve;
		CombatComponent->BaseDamage = CombatConfig.BaseDamage;
		CombatComponent->InvulnerabilityDuration = CombatConfig.InvulnerabilityDuration;

		// Bind to invulnerability changes to sync legacy flag
		CombatComponent->OnInvulnerabilityChanged.AddDynamic(this, &APixelEnemy::OnCombatInvulnerabilityChanged);
	}

	// Setup PelletComponent event binding
	if (PelletComponent)
	{
		PelletComponent->OnPelletStateChanged.AddDynamic(this, &APixelEnemy::OnPelletStateChanged);
	}

	// Legacy flash setup (for backward compatibility)
	flashInDamagedMaterial = GetMesh()->CreateDynamicMaterialInstance(0);
	if (flashInDamagedMaterial)
	{
		FOnTimelineFloat progressCurve;
		progressCurve.BindDynamic(this, &APixelEnemy::FlashProgress);
		if (flashCurve)
		{
			flashTimeLine->AddInterpFloat(flashCurve, progressCurve);
			flashTimeLine->SetLooping(false);
		}
	}
	
	if (HasAuthority() && abilitySystemComponent && InitialStatsEffect)
	{
		FGameplayEffectContextHandle EffectContext = abilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = abilitySystemComponent->MakeOutgoingSpec(
			InitialStatsEffect,
			1.0f,
			EffectContext
		);

		if (SpecHandle.IsValid())
		{
			abilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	if (CharMovement)
	{
		CharMovement->MaxWalkSpeed = MoveSpeed;
		CharMovement->MaxFlySpeed = MoveSpeed;
		CharMovement->bOrientRotationToMovement = false;
		CharMovement->GravityScale = 0.0f;
		CharMovement->SetMovementMode(MOVE_Flying);
		CharMovement->DefaultLandMovementMode = MOVE_Flying;

		CharMovement->bUseRVOAvoidance = false;
		CharMovement->bUseFlatBaseForFloorChecks = false;


		/*UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] CharacterMovement 설정 - MaxWalkSpeed: %.1f, MaxFlySpeed: %.1f, GravityScale: %.1f, MovementMode: Flying"),
			MoveSpeed, CharMovement->MaxFlySpeed, CharMovement->GravityScale);*/
	}
	else
	{
		//UE_LOG(LogTemp, Error, TEXT("[PixelEnemy] CharacterMovement is NULLPTR!!!"));
	}
	
	// MinimapIconComponent를 위한 태그 추가 (자동으로 IconId 설정됨)
	Tags.AddUnique(TEXT("Enemy"));

	if (!GetController() && HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		AAIController* AIC = GetWorld()->SpawnActor<AAIController>(AIControllerClass, GetActorTransform(), SpawnParams);
		if (AIC)
		{
			AIC->Possess(this);
		}
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Flying);
		Movement->GravityScale = 0.0f;

		// SM_ROAD_19로 시작하는 StaticMeshActor 찾기
		AStaticMeshActor* FoundRoad = nullptr;
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStaticMeshActor::StaticClass(), FoundActors);

		for (AActor* Actor : FoundActors)
		{
			if (Actor->GetName().StartsWith(TEXT("SM_ROAD_19")))
			{
				FoundRoad = Cast<AStaticMeshActor>(Actor);
				break;
			}
		}

		FVector CurrentLoc = GetActorLocation();

		if (FoundRoad)
		{
			// SM_ROAD_19의 Z 위치 + 1로 설정
			float RoadZ = FoundRoad->GetActorLocation().Z;
			CurrentLoc.Z = RoadZ + 1.0f;
			SetActorLocation(CurrentLoc, true);
			UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] Start position set to SM_ROAD_19 Z + 1: %.1f"), CurrentLoc.Z);
		}
		else
		{
			// 대비책: SM_ROAD_19를 찾지 못한 경우 기존 로직 사용
			UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] SM_ROAD_19 not found, using default positioning"));
			SetActorLocation(CurrentLoc + FVector(0, 0, 100.0f), true);
			UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
			if (NavSys)
			{
				FNavLocation NavLoc;
				if (NavSys->ProjectPointToNavigation(CurrentLoc + FVector(0, 0, 200), NavLoc))
				{
					SetActorLocation(NavLoc.Location + FVector(0, 0, 50), true);
				}
			}
		}
	}

	if (SpringArm)
	{
		SpringArm->TargetArmLength = CombatConfig.SpringArmLength;
		SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, CombatConfig.CameraZOffset));
	}
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	GetCharacterMovement()->MaxFlySpeed = MoveSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	if (!SceneCaptureComponent || !Camera)
	{
		if (!SceneCaptureComponent)
		{
			CCTV_LOG(Warning, "SceneCaptureComponent가 없습니다");
		}
		if (!Camera)
		{
			CCTV_LOG(Warning, "Camera가 없습니다");
		}
		return;
	}

	UTextureRenderTarget2D* TargetRenderTarget = nullptr;

	if (CCTVRenderTarget)
	{
		TargetRenderTarget = CCTVRenderTarget;
	}
	else if (SceneCaptureComponent->TextureTarget)
	{
		TargetRenderTarget = SceneCaptureComponent->TextureTarget;
		CCTVRenderTarget = TargetRenderTarget;
	}
	else
	{
		CCTV_LOG(Error, "RenderTarget이 설정되지 않았습니다! BP에서 CCTVRenderTarget을 설정해주세요. Enemy: %s",
			*GetName());
		return;
	}

	if (TargetRenderTarget)
	{
		int32 RTWidth = TargetRenderTarget->SizeX;
		int32 RTHeight = TargetRenderTarget->SizeY;
		EPixelFormat RTFormat = TargetRenderTarget->GetFormat();

		if (RTWidth <= 0 || RTHeight <= 0)
		{
			CCTV_LOG(Error, "RenderTarget 크기가 잘못되었습니다! %dx%d", RTWidth, RTHeight);
			return;
		}

		SceneCaptureComponent->TextureTarget = TargetRenderTarget;
	}

	SceneCaptureComponent->AttachToComponent(Camera, FAttachmentTransformRules::SnapToTargetIncludingScale);

	SceneCaptureComponent->FOVAngle = Camera->FieldOfView;
	SceneCaptureComponent->ProjectionType = Camera->ProjectionMode;
	SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	// HDR 비교용: SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
	SceneCaptureComponent->CompositeMode = SCCM_Overwrite;
	SceneCaptureComponent->bCaptureEveryFrame = true;
	SceneCaptureComponent->bCaptureOnMovement = true;

	FEngineShowFlags& ShowFlags = SceneCaptureComponent->ShowFlags;
	ShowFlags.SetGame(true);
	ShowFlags.SetMaterials(true);
	ShowFlags.SetLighting(true);
	ShowFlags.SetPostProcessing(true);

	ShowFlags.SetBounds(false);
	ShowFlags.SetCollision(false);
	ShowFlags.SetVisualizeBuffer(false);
	ShowFlags.SetFog(false);
	ShowFlags.SetVolumetricFog(false);

	FPostProcessSettings& PPS = SceneCaptureComponent->PostProcessSettings;

	if (Camera)
	{
		PPS = Camera->PostProcessSettings;
	}

	PPS.bOverride_AutoExposureMethod = true;
	PPS.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;

	PPS.bOverride_AutoExposureBias = true;
	PPS.AutoExposureBias = CCTVAutoExposureBias;

	PPS.bOverride_AutoExposureMinBrightness = true;
	PPS.AutoExposureMinBrightness = 1.0f;
	PPS.bOverride_AutoExposureMaxBrightness = true;
	PPS.AutoExposureMaxBrightness = 1.0f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick([this]()
			{
				CaptureCCTVScene();
			});

		World->GetTimerManager().SetTimer(CCTVSyncTimerHandle, [this]()
			{
				CaptureCCTVScene();
			}, CCTVSyncInterval, true);
	}

	UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] BeginPlay 완료"));
}

void APixelEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (CCTVSyncTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(CCTVSyncTimerHandle);
		}
	}

	if (HasAuthority() && EnemyAISubsystem)
	{
		if (AIErrorDelegateHandle.IsValid())
		{
			EnemyAISubsystem->OnAIError.Remove(AIErrorDelegateHandle);
		}

		if (ConnectionChangedHandle.IsValid())
		{
			EnemyAISubsystem->OnConnectionChanged.Remove(ConnectionChangedHandle);
		}

		EnemyAISubsystem->UnregisterEnemy(this);
		UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] Unregistered from AI Subsystem - ID: %s"), *GetEnemyID());
	}

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(DamageInvulnerabilityTimer);
		GetWorldTimerManager().ClearTimer(PPelletInvulnerabilityTimer);
		GetWorldTimerManager().ClearTimer(PPelletCooldownTimer);
	}

	// v1.5.0: EndPlay 시 잔존 분신 제거
	DespawnClones();

	Super::EndPlay(EndPlayReason);
}

void APixelEnemy::EquipBrain()
{
	if (HasAuthority() && bAIEnabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] Equip Brain - AI 활성화 및 움직임 재개"));

		// CharacterMovement 활성화 (첫 입력 후 움직임 시작)
		if (UCharacterMovementComponent* CharMovement = GetCharacterMovement())
		{
			CharMovement->SetMovementMode(MOVE_Flying);
			UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] CharacterMovement 활성화 완료"));
		}

		EnemyAISubsystem = GetGameInstance()->GetSubsystem<UEnemyAISubsystem>();

		if (EnemyAISubsystem)
		{
			EnemyAISubsystem->RegisterEnemy(this);

			AIErrorDelegateHandle = EnemyAISubsystem->OnAIError.AddUObject(
				this, &APixelEnemy::OnAIServerError
			);

			ConnectionChangedHandle = EnemyAISubsystem->OnConnectionChanged.AddUObject(
				this, &APixelEnemy::OnAIConnectionChanged
			);
		}
		else
		{
			//UE_LOG(LogTemp, Error, TEXT("[PixelEnemy] Failed to get EnemyAISubsystem"));
		}
	}
}


void APixelEnemy::OnAIDecisionReceived(const FUnitCommand& Command)
{

	// 화면에 디버그 메시지 표시
	if (GEngine)
	{
		FString DirectiveName = Command.directive_name.IsEmpty() ? TEXT("Unknown") : Command.directive_name;
		FVector TargetPos(
			Command.params.target_position.x,
			Command.params.target_position.y,
			Command.params.target_position.z
		);

		FString DebugMessage = FString::Printf(
			TEXT("[AI] Directive: %d (%s)\nTarget: (%.0f, %.0f, %.0f)\nSpeed: %.2f"),
			Command.directive_code,
			*DirectiveName,
			TargetPos.X, TargetPos.Y, TargetPos.Z,
			Command.params.speed_factor
		);

		// 화면 우측 상단에 5초간 표시 (초록색)
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, DebugMessage);
	}

	// AIDirectiveComponent로 명령 전달
	if (AIDirectiveComponent)
	{
		// FUnitCommandParams를 FDirectiveParams로 변환
		FDirectiveParams DirectiveParams;

		// 공통 파라미터
		DirectiveParams.TargetPosition = FVector(
			Command.params.target_position.x,
			Command.params.target_position.y,
			Command.params.target_position.z
		);
		DirectiveParams.TargetPlayerId = Command.params.target_player_id;
		DirectiveParams.SpeedFactor = Command.params.speed_factor;
		DirectiveParams.Priority = Command.params.priority;

		if (!Command.params.aggressiveness.IsEmpty())
		{
			DirectiveParams.Aggressiveness = GetAggressivenessFactor(Command.params.aggressiveness);
		}

		// RETREAT 파라미터
		FVector SafePos = FVector(
			Command.params.safe_zone_position.x,
			Command.params.safe_zone_position.y,
			Command.params.safe_zone_position.z
		);
		DirectiveParams.SafeZonePosition = SafePos;

		// RETREAT(5) 명령인 경우 TargetPosition에도 SafeZone 위치를 설정
		if (Command.directive_code == 5)
		{
			DirectiveParams.TargetPosition = SafePos;
		}
		// PATROL 파라미터
		DirectiveParams.PatrolZone = Command.params.patrol_zone;
		// CONSUME_P_POINT 파라미터
		DirectiveParams.PPointId = Command.params.p_point_id;
		DirectiveParams.EmergencyPriority = Command.params.emergency_priority;

		// CONSUME_PELLET 파라미터 (8)
		DirectiveParams.PelletId = Command.params.pellet_id;

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
}


FString APixelEnemy::GetEnemyID() const
{
	return TEXT("pacman_main");
}

FEnemyGameState APixelEnemy::GetCurrentGameState() const
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
	State.capture_gauge = CalculateCaptureGauge();

	// Check invulnerability via components or legacy flags
	bool bHasPowerPellet = bPowerPellet || (PelletComponent && PelletComponent->IsPowerPelletActive());
	bool bIsUntouchable = bUntouchable || (CombatComponent && CombatComponent->IsUntouchable());
	State.is_invulnerable = bHasPowerPellet || bIsUntouchable;

	State.rotation_yaw_deg = GetActorRotation().Yaw;
	State.forward_vector = GetActorForwardVector();

	// Get P-Pellet state from component or legacy fields
	if (PelletComponent)
	{
		State.p_pellet_cooldown = PelletComponent->GetCooldownRemaining();
		State.last_pellet_consumed_at = PelletComponent->GetLastConsumedTime();
	}
	else
	{
		State.p_pellet_cooldown = PPelletCooldown;
		State.last_pellet_consumed_at = LastPelletConsumedAt;
	}

	return State;
}

float APixelEnemy::CalculateCaptureGauge() const
{
	if (!CaptureGaugeComponent)
	{
		//UE_LOG(LogTemp, Error, TEXT("[PixelEnemy] CalculateCaptureGauge - CaptureGaugeComponent is NULL! Returning 0.0"));
		return 0.0f;
	}

	float Gauge = CaptureGaugeComponent->GetCaptureGauge();
	//UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] CalculateCaptureGauge - Returning capture_gauge: %.1f"), Gauge);

	if (Gauge == 0.0f)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] CalculateCaptureGauge - Gauge is 0! Check if CaptureGaugeComponent has calculated yet."));
	}

	return Gauge;
}

void APixelEnemy::Multicast_TriggerDestruction_Implementation()
{
	if (!GeometryCollectionAsset)
	{
		if (HasAuthority())
		{
			// POST /api/v1/match/end
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UEnemyAISubsystem* AISubsystem = GI->GetSubsystem<UEnemyAISubsystem>())
				{
					AISubsystem->SendMatchEndToServer(TEXT("PLAYERS_WIN"), TEXT("PACMAN_DEAD"));
				}
			}

			if (ACitRushGameState* cGS = GetWorld()->GetGameState<ACitRushGameState>())
			{
				cGS->Victory();
			}
			if (DieEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					DieEffect,
					GetActorLocation(),
					GetActorRotation()
				);
			}
			Destroy();
		}
		return;
	}

	FTransform CurrentTransform = GetActorTransform();
	FVector CurrentVelocity = GetActorUpVector() * 100.f;;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGeometryCollectionActor* GCActor = GetWorld()->SpawnActor<AGeometryCollectionActor>(
		AGeometryCollectionActor::StaticClass(),
		CurrentTransform,
		SpawnParams
	);

	if (GCActor)
	{
		UGeometryCollectionComponent* GCComponent = GCActor->GetGeometryCollectionComponent();
		GCComponent->SetRestCollection(GeometryCollectionAsset);

		GCComponent->SetSimulatePhysics(true);

		if (!CurrentVelocity.IsNearlyZero())
		{
			GCComponent->SetPhysicsLinearVelocity(CurrentVelocity);
		}

		GCComponent->ApplyBreakingLinearVelocity(
			0,
			FVector(1000.f) // 충분히 큰 값으로 즉시 파괴
		);

		GCActor->SetLifeSpan(10.0f);
	}
	if (HasAuthority())
	{
		// POST /api/v1/match/end
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UEnemyAISubsystem* AISubsystem = GI->GetSubsystem<UEnemyAISubsystem>())
			{
				AISubsystem->SendMatchEndToServer(TEXT("PLAYERS_WIN"), TEXT("PACMAN_DEAD"));
			}
		}

		if (ACitRushGameState* cGS = GetWorld()->GetGameState<ACitRushGameState>())
		{
			cGS->Victory();
		}
		Destroy();
	}
}

void APixelEnemy::TryAttack()
{
	// Effect
}

void APixelEnemy::NetMulticast_Damaged_Implementation(FVector hitLocation)
{
	// Check P-Pellet invulnerability
	if (bPowerPellet || (PelletComponent && PelletComponent->IsPowerPelletActive()))
	{
		return;
	}

	// Check CombatComponent invulnerability
	if (CombatComponent && CombatComponent->IsUntouchable())
	{
		return;
	}

	// Damage processing (Server only)
	if (HasAuthority())
	{
		float DamageAmount = CombatComponent ? CombatComponent->BaseDamage : CombatConfig.BaseDamage;
		attributeSet->SetHealth(attributeSet->GetHealth() - DamageAmount);
		UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : %f"), attributeSet->GetHealth());

		if (attributeSet->GetHealth() <= 0)
		{
			if (attributeSet->GetMaxHealth() < 0)
			{
				attributeSet->SetHealth(0);
			}
			// 죽음 / Die / Died / Destroy
			Multicast_TriggerDestruction();

			if (ACitRushGameState* lGS = GetWorld()->GetGameState<ACitRushGameState>())
			{
				lGS->Victory();
			}

			// Broadcast death event
			if (CombatComponent)
			{
				CombatComponent->OnEnemyDeath.Broadcast();
			}
			return;
		}

		// Broadcast damage event
		if (CombatComponent)
		{
			CombatComponent->OnDamageReceived.Broadcast(DamageAmount, hitLocation);
		}
	}

	// Spawn damage effect
	if (DamagedEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			DamagedEffect,
			hitLocation,
			GetActorRotation(),
			FVector(50.f, 50.f, 50.f)
		);
	}

	// Start damage invulnerability via CombatComponent
	if (CombatComponent)
	{
		CombatComponent->StartDamageInvulnerability(CombatConfig.InvulnerabilityDuration);
		bUntouchable = true; // Sync legacy flag
	}
	else
	{
		// Legacy fallback (if CombatComponent not available)
		bUntouchable = true;
		flashTimeLine->PlayFromStart();

		GetWorldTimerManager().ClearTimer(DamageInvulnerabilityTimer);

		TWeakObjectPtr<APixelEnemy> WeakThis(this);
		GetWorldTimerManager().SetTimer(DamageInvulnerabilityTimer, [WeakThis]()
			{
				if (!WeakThis.IsValid())
				{
					return;
				}

				APixelEnemy* StrongThis = WeakThis.Get();
				UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : Flash End"));
				StrongThis->bUntouchable = false;

				if (IsValid(StrongThis->flashTimeLine))
				{
					StrongThis->flashTimeLine->Stop();
				}

				if (IsValid(StrongThis->flashInDamagedMaterial))
				{
					StrongThis->flashInDamagedMaterial->SetScalarParameterValue(TEXT("FlashIntensity"), 1.f);
				}
			}, CombatConfig.InvulnerabilityDuration, false);
	}
}

void APixelEnemy::OnHitToRacer(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// [추가] 코인 충돌 처리 (Hit) - Collision이 Block일 경우 대비
	if (ACoinActor* Coin = Cast<ACoinActor>(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] Hit Coin (Block)! %s"), *Coin->GetName());

		// AI Subsystem에 코인 획득 알림
		if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
		{
			if (UEnemyAISubsystem* AISubsystem = GameInstance->GetSubsystem<UEnemyAISubsystem>())
			{
				// GetCoinID()가 있다고 가정 (Fallback 코드 참조)
				AISubsystem->MarkPPointAsCollected(Coin->GetCoinID());
			}
		}

		OnCoinCollected(Coin);
		Coin->PlayPickupEffects();
		Coin->Destroy();
		return;
	}

	// [추가] Pellet 충돌 처리 (Hit)
	if (APelletActor* Pellet = Cast<APelletActor>(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] Hit Pellet! %s (ID: %s)"), *Pellet->GetName(), *Pellet->GetPelletID());
		
		// 강제 획득 처리
		// OnOverlapBegin 내부에서 bIsAvailable = false로 변경되고 Mesh가 숨겨짐 -> AI Subsystem이 다음 주기 전송 시 반영함
		Pellet->OnOverlapBegin(nullptr, this, HitComponent, 0, false, Hit);
		return;
	}

	if (AAbstractRacer* racer = Cast<AAbstractRacer>(OtherActor))
	{
		FVector normDir = (racer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		float dot = normDir.Dot(GetActorForwardVector());
		UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy Begin Hit : %s"), *OtherActor->GetActorNameOrLabel());

		// Check P-Pellet invulnerability (via component or legacy flag)
		bool bHasPowerPellet = bPowerPellet || (PelletComponent && PelletComponent->IsPowerPelletActive());
		if (bHasPowerPellet)
		{
			TryAttack();
			racer->NetMulticastRPC_Damaged(Hit.ImpactPoint, Hit.ImpactNormal);
			return;
		}
		if (dot >= 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : Attack"));
			TryAttack();
			racer->NetMulticastRPC_Damaged(Hit.ImpactPoint, Hit.ImpactNormal);
			return;
		}

		// Check damage invulnerability (via CombatComponent or legacy flag)
		bool bIsUntouchable = bUntouchable || (CombatComponent && CombatComponent->IsUntouchable());
		if (bIsUntouchable)
		{
			UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : Untouchable"));
			racer->NetMulticastRPC_TryAttack(false, Hit.ImpactPoint, Hit.ImpactNormal);
			return;
		}

		if (ShieldMesh && ShieldMesh->IsVisible())
		{
			UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : Shield Block"));
			ShieldMesh->SetVisibility(false);
			racer->NetMulticastRPC_TryAttack(false, Hit.ImpactPoint, Hit.ImpactNormal);
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : NetMulticastRPC_Damaged"));
		this->NetMulticast_Damaged(Hit.ImpactPoint);
		racer->NetMulticastRPC_TryAttack(true, Hit.ImpactPoint, Hit.ImpactNormal);
		return;
	}
}

void APixelEnemy::OnOverlapToRacer(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAbstractRacer* racer = Cast<AAbstractRacer>(OtherActor);
		HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy Begin Overlap : %s"), *OtherActor->GetActorNameOrLabel());

		// Check P-Pellet invulnerability (via component or legacy flag)
		bool bHasPowerPellet = bPowerPellet || (PelletComponent && PelletComponent->IsPowerPelletActive());
		if (bHasPowerPellet)
		{
			NetMulticast_Damaged(SweepResult.ImpactPoint);
			racer->NetMulticastRPC_Damaged(SweepResult.ImpactPoint, SweepResult.ImpactNormal);
			return;
		}

		FVector normDir = (racer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		float dot = normDir.Dot(GetActorForwardVector());
		if (dot >= 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : Attack"));
			TryAttack();
			racer->NetMulticastRPC_Damaged(SweepResult.ImpactPoint, SweepResult.ImpactNormal);
			return;
		}

		// Check damage invulnerability (via CombatComponent or legacy flag)
		bool bIsUntouchable = bUntouchable || (CombatComponent && CombatComponent->IsUntouchable());
		if (bIsUntouchable)
		{
			UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : Untouchable"));
			racer->NetMulticastRPC_TryAttack(false, SweepResult.ImpactPoint, SweepResult.ImpactNormal);
			return;
		}

		if (ShieldMesh && ShieldMesh->IsVisible())
		{
			UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : Shield Block"));
			ShieldMesh->SetVisibility(false);
			racer->NetMulticastRPC_TryAttack(false, SweepResult.ImpactPoint, SweepResult.ImpactNormal);
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("==================== APixelEnemy : NetMulticastRPC_Damaged"));
		NetMulticast_Damaged(SweepResult.ImpactPoint);
		racer->NetMulticastRPC_TryAttack(true, SweepResult.ImpactPoint, SweepResult.ImpactNormal);
		return;
	}
}

void APixelEnemy::FlashProgress(float output)
{
	if (flashInDamagedMaterial)
	{
		flashInDamagedMaterial->SetScalarParameterValue(TEXT("FlashIntensity"), output);
	}
}

void APixelEnemy::OnPelletCollected(float Duration)
{
	// Delegate to PelletComponent
	if (PelletComponent)
	{
		// Set cooldown from config
		PelletComponent->CooldownDuration = CombatConfig.PPelletCooldownDuration;
		PelletComponent->OnPelletCollected(Duration);

		// Sync legacy flag for compatibility
		bPowerPellet = true;

		// Sync to CombatComponent for invulnerability check
		if (CombatComponent)
		{
			CombatComponent->bPowerPelletActive = true;
		}
	}

	// v1.5.0: P-Pellet 획득 시 분신 스폰
	if (HasAuthority())
	{
		SpawnClones();
	}
}

void APixelEnemy::OnPPelletInvulnerabilityEnd()
{
	// Sync legacy flag
	bPowerPellet = false;

	// Sync to CombatComponent
	if (CombatComponent)
	{
		CombatComponent->bPowerPelletActive = false;
	}

	// Note: Visual update is handled by PelletComponent

	// v1.5.0: P-Pellet 만료 시 분신 제거
	if (HasAuthority())
	{
		DespawnClones();
	}
}

void APixelEnemy::DecreasePPelletCooldown()
{
	// Legacy function - cooldown is now handled by PelletComponent
	// Kept for backward compatibility
	if (PelletComponent)
	{
		PPelletCooldown = PelletComponent->GetCooldownRemaining();
	}
}

// ========== Clone Management (v1.5.0) ==========

void APixelEnemy::SpawnClones()
{
	if (!HasAuthority() || !CloneClass) return;

	// 중복 방지: 기존 분신 먼저 제거
	DespawnClones();

	UWorld* World = GetWorld();
	if (!World) return;

	const FVector OwnerLocation = GetActorLocation();
	const float SpawnRadius = 500.0f;

	for (int32 i = 0; i < MaxCloneCount; ++i)
	{
		// 원형 배치: 360/N * i 각도
		const float AngleDeg = (360.0f / MaxCloneCount) * i;
		const float AngleRad = FMath::DegreesToRadians(AngleDeg);
		const FVector Offset(FMath::Cos(AngleRad) * SpawnRadius, FMath::Sin(AngleRad) * SpawnRadius, 0.0f);
		const FVector SpawnLocation = OwnerLocation + Offset;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Owner = this;

		ACloneEnemy* Clone = World->SpawnActor<ACloneEnemy>(CloneClass, SpawnLocation, GetActorRotation(), SpawnParams);
		if (Clone)
		{
			Clone->InitializeClone(i + 1, this);
			Clone->OnCloneDeath.BindUObject(this, &APixelEnemy::OnCloneDied);
			ActiveClones.Add(Clone);

			UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] Spawned clone_%d at (%.0f, %.0f, %.0f)"),
				i + 1, SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] Spawned %d clones"), ActiveClones.Num());
}

void APixelEnemy::DespawnClones()
{
	if (!HasAuthority()) return;

	for (TWeakObjectPtr<ACloneEnemy>& ClonePtr : ActiveClones)
	{
		if (ClonePtr.IsValid() && ClonePtr->bIsAlive)
		{
			ClonePtr->Multicast_CloneDeath();
		}
	}

	ActiveClones.Empty();
}

void APixelEnemy::OnCloneDied(ACloneEnemy* DeadClone)
{
	if (!DeadClone) return;

	ActiveClones.RemoveAll([DeadClone](const TWeakObjectPtr<ACloneEnemy>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == DeadClone;
	});

	UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] Clone %s died. Remaining clones: %d"),
		*DeadClone->GetEnemyID(), ActiveClones.Num());
}

void APixelEnemy::OnCombatInvulnerabilityChanged(bool bIsInvulnerable)
{
	// Sync legacy flag with CombatComponent state
	bUntouchable = bIsInvulnerable;
}

void APixelEnemy::OnPelletStateChanged(bool bIsActive)
{
	// Sync legacy flag with PelletComponent state
	bPowerPellet = bIsActive;

	// Also sync to CombatComponent for combined invulnerability check
	if (CombatComponent)
	{
		CombatComponent->bPowerPelletActive = bIsActive;
	}
}

float APixelEnemy::GetAggressivenessFactor(const FString& AggressivenessLevel) const
{
	if (!AggressivenessMappingTable)
	{
		// 기본 매핑
		if (AggressivenessLevel == TEXT("VERY_HIGH")) return 1.3f;
		if (AggressivenessLevel == TEXT("HIGH")) return 1.0f;
		if (AggressivenessLevel == TEXT("MEDIUM")) return 0.7f;
		if (AggressivenessLevel == TEXT("LOW")) return 0.4f;
		if (AggressivenessLevel == TEXT("VERY_LOW")) return 0.2f;

		//UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] Unknown aggressiveness level: %s, using default 0.7f"), *AggressivenessLevel);
		return 0.7f;
	}

	// DataTable에서 매핑 찾기
	TArray<FName> RowNames = AggressivenessMappingTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FAggressivenessMapping* Row = AggressivenessMappingTable->FindRow<FAggressivenessMapping>(RowName, TEXT(""));
		if (Row && Row->AggressivenessLevel == AggressivenessLevel)
		{
			//UE_LOG(LogTemp, Verbose, TEXT("[PixelEnemy] Aggressiveness '%s' mapped to %.2f"), *AggressivenessLevel, Row->AggressivenessFactor);
			return Row->AggressivenessFactor;
		}
	}

	// 매핑을 찾지 못한 경우 기본값
	//UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] Aggressiveness '%s' not found in DataTable, using default 0.7f"), *AggressivenessLevel);
	return 0.7f;
}

// AI Server Fallback Logic
void APixelEnemy::OnAIServerError(const FString& EnemyID, const FString& ErrorMessage)
{
	// 이 Enemy에 대한 에러인지 확인
	if (EnemyID != GetEnemyID())
	{
		return;
	}

	/*UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] AI Server Error - ID: %s, Error: %s"), *EnemyID, *ErrorMessage);
	UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] Executing fallback behavior..."));*/
	
	ExecuteFallbackBehavior();
}

void APixelEnemy::OnAIConnectionChanged(bool bConnected)
{
	if (!HasAuthority()) return;

	if (bConnected)
	{
		DeactivateLocalAI();
		UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] Server reconnected - StateTree deactivated"));
	}
	else
	{
		ActivateLocalAI();
		UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] Server disconnected - StateTree activated"));
	}
}

// Fallback - Activate StateTree for local AI
void APixelEnemy::ExecuteFallbackBehavior()
{
	if (!HasAuthority())
	{
		return;
	}

	// Activate StateTree-based local AI
	ActivateLocalAI();
}

// ============================================================================
// StateTree Control
// ============================================================================

void APixelEnemy::ActivateLocalAI()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsLocalAIActive)
	{
		return; // Already active
	}

	if (StateTreeComponent && FallbackStateTree)
	{
		StateTreeComponent->SetStateTree(FallbackStateTree);
		StateTreeComponent->StartLogic();
		bIsLocalAIActive = true;
		UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] Local AI (StateTree) activated - %s"), *GetName());
	}
	else
	{
		// Fallback to simple directive if no StateTree
		if (AIDirectiveComponent)
		{
			FDirectiveParams Params;
			Params.SpeedFactor = 1.2f;
			AIDirectiveComponent->ProcessDirective(1, Params);
		}
		UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] No StateTree assigned, using simple fallback"));
	}
}

void APixelEnemy::DeactivateLocalAI()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!bIsLocalAIActive)
	{
		return; // Already inactive
	}

	if (StateTreeComponent)
	{
		StateTreeComponent->StopLogic(TEXT("Server Reconnected"));
		bIsLocalAIActive = false;
		UE_LOG(LogTemp, Log, TEXT("[PixelEnemy] Local AI (StateTree) deactivated - %s"), *GetName());
	}
}

// ============================================================================
// AIPerception Setup
// ============================================================================

void APixelEnemy::SetupAIPerception()
{
	if (!AIPerceptionComponent)
	{
		return;
	}

	// Sight Configuration
	if (PerceptionConfig.bEnableSight)
	{
		UAISenseConfig_Sight* SightConfig = NewObject<UAISenseConfig_Sight>(this);
		SightConfig->SightRadius = PerceptionConfig.SightRadius;
		SightConfig->LoseSightRadius = PerceptionConfig.LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = PerceptionConfig.PeripheralVisionAngle;
		SightConfig->DetectionByAffiliation = PerceptionConfig.SightAffiliationFilter;
		SightConfig->AutoSuccessRangeFromLastSeenLocation = PerceptionConfig.AutoSuccessRangeFromLastSeen;
		SightConfig->PointOfViewBackwardOffset = PerceptionConfig.PointOfViewBackwardOffset;
		SightConfig->NearClippingRadius = PerceptionConfig.NearClippingRadius;
		SightConfig->SetMaxAge(PerceptionConfig.MaxStimuliAge);

		AIPerceptionComponent->ConfigureSense(*SightConfig);
	}

	// Hearing Configuration
	if (PerceptionConfig.bEnableHearing)
	{
		UAISenseConfig_Hearing* HearingConfig = NewObject<UAISenseConfig_Hearing>(this);
		HearingConfig->HearingRange = PerceptionConfig.HearingRange;
		HearingConfig->DetectionByAffiliation = PerceptionConfig.HearingAffiliationFilter;
		HearingConfig->SetMaxAge(PerceptionConfig.MaxStimuliAge);

		AIPerceptionComponent->ConfigureSense(*HearingConfig);
	}

	// Damage Configuration
	if (PerceptionConfig.bEnableDamage)
	{
		UAISenseConfig_Damage* DamageConfig = NewObject<UAISenseConfig_Damage>(this);
		DamageConfig->SetMaxAge(PerceptionConfig.MaxStimuliAge);

		AIPerceptionComponent->ConfigureSense(*DamageConfig);
	}

	// Set dominant sense
	if (PerceptionConfig.DominantSense)
	{
		AIPerceptionComponent->SetDominantSense(PerceptionConfig.DominantSense);
	}
	else if (PerceptionConfig.bEnableSight)
	{
		AIPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
	}

	// Bind perception update delegate
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &APixelEnemy::OnPerceptionUpdated);
}

void APixelEnemy::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !HasAuthority())
	{
		return;
	}

	// Update perceived targets list
	FPerceivedTargetInfo* ExistingInfo = PerceivedTargets.FindByPredicate([Actor](const FPerceivedTargetInfo& Info)
	{
		return Info.Actor == Actor;
	});

	if (Stimulus.WasSuccessfullySensed())
	{
		if (ExistingInfo)
		{
			ExistingInfo->LastKnownLocation = Stimulus.StimulusLocation;
			ExistingInfo->bIsCurrentlySensed = true;
			ExistingInfo->TimeSinceLastSensed = 0.0f;
			ExistingInfo->StimulusStrength = Stimulus.Strength;

			// Update sense type flags
			if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
			{
				ExistingInfo->bWasSensedBySight = true;
			}
			else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
			{
				ExistingInfo->bWasSensedByHearing = true;
			}
			else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
			{
				ExistingInfo->bWasSensedByDamage = true;
			}
		}
		else
		{
			FPerceivedTargetInfo NewInfo;
			NewInfo.Actor = Actor;
			NewInfo.LastKnownLocation = Stimulus.StimulusLocation;
			NewInfo.bIsCurrentlySensed = true;
			NewInfo.TimeSinceLastSensed = 0.0f;
			NewInfo.StimulusStrength = Stimulus.Strength;

			if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
			{
				NewInfo.bWasSensedBySight = true;
			}
			else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
			{
				NewInfo.bWasSensedByHearing = true;
			}
			else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
			{
				NewInfo.bWasSensedByDamage = true;
			}

			PerceivedTargets.Add(NewInfo);
		}
	}
	else
	{
		// Lost perception
		if (ExistingInfo)
		{
			ExistingInfo->bIsCurrentlySensed = false;
		}
	}
}

TArray<FPerceivedTargetInfo> APixelEnemy::GetPerceivedTargets() const
{
	return PerceivedTargets;
}

AActor* APixelEnemy::GetNearestPerceivedRacer() const
{
	AActor* NearestRacer = nullptr;
	float MinDistance = FLT_MAX;
	FVector MyLocation = GetActorLocation();

	for (const FPerceivedTargetInfo& Info : PerceivedTargets)
	{
		if (!Info.bIsCurrentlySensed || !Info.Actor)
		{
			continue;
		}

		// Check if it's a racer
		if (Info.Actor->IsA(AAbstractRacer::StaticClass()))
		{
			float Distance = FVector::Dist(MyLocation, Info.LastKnownLocation);
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				NearestRacer = Info.Actor;
			}
		}
	}

	return NearestRacer;
}

/*
// Legacy fallback code (replaced by StateTree)
void APixelEnemy::LegacyFallbackBehavior()
{
	TArray<AActor*> FoundCars;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVehicleDemoCejCar::StaticClass(), FoundCars);

	AActor* TestCar = nullptr;
	float MinDist = FLT_MAX;
	FVector MyLocation = GetActorLocation();

	for (AActor* Car : FoundCars)
	{
		if (IsValid(Car))
		{
			float Dist = FVector::Dist(MyLocation, Car->GetActorLocation());
			if (Dist < MinDist)
			{
				MinDist = Dist;
				TestCar = Car;
			}
		}
	}

	if (TestCar)
	{
		FDirectiveParams Params;
		Params.TargetPosition = TestCar->GetActorLocation();
		Params.SpeedFactor = 1.2f;
		AIDirectiveComponent->ProcessDirective(2, Params);
		return;
	}

	AAbstractRacer* NearestRacer = FindNearestRacer();
	if (NearestRacer)
	{
		FDirectiveParams Params;
		Params.TargetPosition = NearestRacer->GetActorLocation();
		Params.SpeedFactor = 1.2f;
		Params.Aggressiveness = 1.0f;
		AIDirectiveComponent->ProcessDirective(4, Params);
		return;
	}

	ACoinActor* NearestCoin = FindNearestCoin();
	if (NearestCoin)
	{
		FDirectiveParams Params;
		Params.TargetPosition = NearestCoin->GetActorLocation();
		Params.PPointId = NearestCoin->GetCoinID();
		Params.SpeedFactor = 1.5f;
		AIDirectiveComponent->ProcessDirective(9, Params);
		return;
	}
}
*/

ACoinActor* APixelEnemy::FindNearestCoin() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoundCoins;
	UGameplayStatics::GetAllActorsOfClass(World, ACoinActor::StaticClass(), FoundCoins);

	ACoinActor* NearestCoin = nullptr;
	float MinDistance = FLT_MAX;
	FVector MyLocation = GetActorLocation();

	for (AActor* Actor : FoundCoins)
	{
		ACoinActor* Coin = Cast<ACoinActor>(Actor);
		if (Coin)
		{
			float Distance = FVector::Dist(MyLocation, Coin->GetActorLocation());
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				NearestCoin = Coin;
			}
		}
	}

	return NearestCoin;
}

AAbstractRacer* APixelEnemy::FindNearestRacer() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoundRacers;
	UGameplayStatics::GetAllActorsOfClass(World, AAbstractRacer::StaticClass(), FoundRacers);

	AAbstractRacer* NearestRacer = nullptr;
	float MinDistance = FLT_MAX;
	FVector MyLocation = GetActorLocation();

	for (AActor* Actor : FoundRacers)
	{
		AAbstractRacer* Racer = Cast<AAbstractRacer>(Actor);
		if (Racer)
		{
			float Distance = FVector::Dist(MyLocation, Racer->GetActorLocation());
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				NearestRacer = Racer;
			}
		}
	}

	return NearestRacer;
}

UCameraComponent* APixelEnemy::GetCCTVCamera(int32 SlotIndex)
{
	if (CCTVCameraComponent)
	{
		UCameraComponent* CCTVCamera = CCTVCameraComponent->GetCCTVCamera(SlotIndex);
		if (CCTVCamera)
		{
			return CCTVCamera;
		}
	}

	if (Camera)
	{
		return Camera;
	}

	TArray<UCameraComponent*> AllCameras;
	GetComponents<UCameraComponent>(AllCameras);

	if (AllCameras.Num() > 0)
	{
		return AllCameras[0];
	}

	TArray<USpringArmComponent*> SpringArms;
	GetComponents<USpringArmComponent>(SpringArms);

	for (USpringArmComponent* Arm : SpringArms)
	{
		if (!Arm) continue;

		TArray<USceneComponent*> ChildComponents;
		Arm->GetChildrenComponents(true, ChildComponents);

		for (USceneComponent* Child : ChildComponents)
		{
			if (UCameraComponent* Cam = Cast<UCameraComponent>(Child))
			{
				return Cam;
			}
		}
	}

	CCTV_LOG(Warning, "GetCCTVCamera: Camera 컴포넌트를 찾을 수 없습니다. SlotIndex: %d, this=%s Class=%s",
		SlotIndex, *GetNameSafe(this), *GetNameSafe(GetClass()));
	return nullptr;
}

void APixelEnemy::CaptureCCTVScene()
{
	if (!SceneCaptureComponent || !SceneCaptureComponent->TextureTarget)
	{
		return;
	}

	if (Camera)
	{
		if (SceneCaptureComponent->GetAttachParent() != Camera)
		{
			SceneCaptureComponent->AttachToComponent(Camera, FAttachmentTransformRules::SnapToTargetIncludingScale);
			CCTV_LOG(Warning, "SceneCaptureComponent Attach 관계 복구 - Camera: %s", *Camera->GetName());
		}

		SceneCaptureComponent->FOVAngle = Camera->FieldOfView;
		SceneCaptureComponent->ProjectionType = Camera->ProjectionMode;

		FPostProcessSettings& PPS = SceneCaptureComponent->PostProcessSettings;
		
		// Camera의 PostProcessSettings를 복사하지 않고 노출 설정만 유지 (노출이 계속 밝아지는 문제 방지)
		// WeightedBlendables는 복사하지 않음 (Camera의 PostProcessSettings 변경이 SceneCapture에 영향을 주지 않도록)
		
		PPS.bOverride_AutoExposureMethod = true;
		PPS.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		PPS.bOverride_AutoExposureBias = true;
		PPS.AutoExposureBias = CCTVAutoExposureBias; // 고정값 사용 (계속 밝아지지 않도록)
		PPS.bOverride_AutoExposureMinBrightness = true;
		PPS.AutoExposureMinBrightness = 1.0f;
		PPS.bOverride_AutoExposureMaxBrightness = true;
		PPS.AutoExposureMaxBrightness = 1.0f;
	}

	SceneCaptureComponent->CaptureScene();
}

void APixelEnemy::SetCCTVCaptureEnabled(bool bEnabled)
{
	if (SceneCaptureComponent)
	{
		SceneCaptureComponent->bCaptureEveryFrame = bEnabled;
	}
}

void APixelEnemy::OnCoinCollected(ACoinActor* Coin)
{
	if (!Coin) return;

	CollectedCoinCount++;
	UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] Coin Collected! Count: %d"), CollectedCoinCount);

	if (CollectedCoinCount >= 1)
	{
		CollectedCoinCount = 0; // 카운트 초기화 (필요 시)

		// 모든 레이서에게 시야 감소 효과 적용
		TArray<AActor*> FoundRacers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVehicleDemoCejCar::StaticClass(), FoundRacers);

		for (AActor* Actor : FoundRacers)
		{
			if (AVehicleDemoCejCar* Racer = Cast<AVehicleDemoCejCar>(Actor))
			{
				Racer->ApplyReducedVision();
				UE_LOG(LogTemp, Warning, TEXT("[PixelEnemy] Apply Reduced Vision to Racer: %s"), *Racer->GetName());
			}
		}
	}
}