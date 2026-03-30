#include "Player/Stats/PingMarkerActor.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Player/AbstractRacer.h"
#include "GameFlow/CitRushGameState.h"
#include "Player/CommenderCharacter.h"
#include "Player/CitRushPlayerState.h"

APingMarkerActor::APingMarkerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false; // 클라이언트 전용, 비복제

	// Root Component
	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	// Decal Component (바닥 표시용, 선택)
	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
	DecalComponent->SetupAttachment(RootComponent);
	DecalComponent->DecalSize = FVector(200.f, 200.f, 200.f);
	DecalComponent->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));

	// Widget Component (선택적, 필요시 Blueprint에서 설정)
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(RootComponent);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetDrawSize(FVector2D(100.f, 100.f));
	WidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 200.f));

	// Emissive 기둥 메쉬 (머티리얼에서 Emissive 세게 세팅)
	PillarMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarMesh"));
	PillarMeshComponent->SetupAttachment(RootComponent);
	PillarMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, 0.f));

	// 충돌: Pawn과 Vehicle과만 Overlap, 나머지는 무시
	PillarMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PillarMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	PillarMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	PillarMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PillarMeshComponent->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	PillarMeshComponent->SetGenerateOverlapEvents(true);

	// 오버랩 이벤트 바인딩 (PillarMeshComponent용)
	PillarMeshComponent->OnComponentBeginOverlap.AddDynamic(this, &APingMarkerActor::OnPillarOverlap);

	// 충돌 감지용 Box Component (레이서가 들어오면 감지)
	CollisionBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBoxComponent->SetupAttachment(RootComponent);
	CollisionBoxComponent->SetBoxExtent(FVector(100.f, 100.f, 200.f)); // 충돌 박스 크기 (X, Y, Z)
	CollisionBoxComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f)); // 중앙 높이에 배치
	
	// Box Component 충돌 설정: Pawn과 Vehicle과만 Overlap
	CollisionBoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBoxComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBoxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBoxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionBoxComponent->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	CollisionBoxComponent->SetGenerateOverlapEvents(true);
	
	// Box Component 오버랩 이벤트 바인딩 (메쉬가 없어도 충돌 감지 가능)
	CollisionBoxComponent->OnComponentBeginOverlap.AddDynamic(this, &APingMarkerActor::OnPillarOverlap);

	// (필요하면) Niagara 이펙트도 함께 사용 가능 - BP 에서 시스템만 지정
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(RootComponent);
	NiagaraComponent->SetAutoActivate(true);
	NiagaraComponent->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
}

void APingMarkerActor::BeginPlay()
{
	Super::BeginPlay();
	
	// BeginPlay에서 충돌 설정 재확인 (블루프린트에서 덮어쓸 수 있으므로)
	if (PillarMeshComponent)
	{
		PillarMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PillarMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
		PillarMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		PillarMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		PillarMeshComponent->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
		PillarMeshComponent->SetGenerateOverlapEvents(true);
	}
	
	// Box Component 충돌 설정 재확인 (메쉬가 없어도 충돌 감지 가능)
	if (CollisionBoxComponent)
	{
		CollisionBoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionBoxComponent->SetCollisionObjectType(ECC_WorldDynamic);
		CollisionBoxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionBoxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		CollisionBoxComponent->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
		CollisionBoxComponent->SetGenerateOverlapEvents(true);
		
		UE_LOG(LogTemp, Log, TEXT("[PingMarkerActor] BeginPlay: Box 충돌 설정 완료 (HasAuthority: %d, Location: %s, BoxExtent: %s)"), 
			HasAuthority() ? 1 : 0, 
			*GetActorLocation().ToString(),
			*CollisionBoxComponent->GetScaledBoxExtent().ToString());
	}
}

void APingMarkerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PillarMeshComponent && RotationAxis != EPingMarkerRotationAxis::None && RotationsPerSecond != 0.f)
	{
		// 초당 RotationsPerSecond 회전 → 초당 각도 = 360 * RotationsPerSecond
		const float DegreesPerSecond = 360.f * RotationsPerSecond;
		const float DeltaDegrees = DegreesPerSecond * DeltaSeconds;

		FRotator DeltaRot = FRotator::ZeroRotator;

		switch (RotationAxis)
		{
		case EPingMarkerRotationAxis::X: // Pitch
			DeltaRot.Pitch = DeltaDegrees;
			break;
		case EPingMarkerRotationAxis::Y: // Roll
			DeltaRot.Roll = DeltaDegrees;
			break;
		case EPingMarkerRotationAxis::Z: // Yaw
			DeltaRot.Yaw = DeltaDegrees;
			break;
		default:
			break;
		}

		if (!DeltaRot.IsZero())
		{
			PillarMeshComponent->AddLocalRotation(DeltaRot);
		}
	}
}

void APingMarkerActor::SetPingData(const FPingData& InPingData)
{
	CurrentPingData = InPingData;
	UpdateLocation(InPingData.WorldLocation);

	// 타입별 메쉬가 설정되어 있으면 교체 (없으면 기존 메쉬 유지)
	if (PillarMeshComponent)
	{
		if (TObjectPtr<UStaticMesh>* FoundMeshPtr = MeshByPingType.Find(InPingData.Type))
		{
			// TObjectPtr<UStaticMesh> 는 UStaticMesh* 로 암시적 변환 가능
			if (*FoundMeshPtr)
			{
				PillarMeshComponent->SetStaticMesh(*FoundMeshPtr);
				
				// 메쉬 설정 후 충돌 설정 재확인 (메쉬 변경 시 충돌이 리셋될 수 있음)
				PillarMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				PillarMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
				PillarMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
				PillarMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
				PillarMeshComponent->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
				PillarMeshComponent->SetGenerateOverlapEvents(true);
			}
		}

		// 타입별 머티리얼이 설정되어 있으면 교체 (없으면 기존 머티리얼 유지)
		if (TObjectPtr<UMaterialInterface>* FoundMatPtr = MaterialByPingType.Find(InPingData.Type))
		{
			if (*FoundMatPtr)
			{
				// 기본 슬롯 0 기준으로 세팅 (필요하면 슬롯 인덱스 노출 가능)
				PillarMeshComponent->SetMaterial(0, *FoundMatPtr);
			}
		}
	}

	// 머티리얼/색 변경 등 추가적인 효과는 Blueprint에서 CurrentPingData.Type 기반으로 구현 가능
}

void APingMarkerActor::UpdateLocation(const FVector& NewLocation)
{
	SetActorLocation(NewLocation + FVector(0.f, 0.f, MarkerHeightOffset));
}

void APingMarkerActor::RemoveMarker()
{
	Destroy();
}

void APingMarkerActor::OnPillarOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                       bool bFromSweep, const FHitResult& SweepResult)
{
	// 디버그: 모든 오버랩 로그 (서버/클라이언트 모두)
	UE_LOG(LogTemp, Warning, TEXT("[PingMarkerActor] OnPillarOverlap 호출 [%s] - OtherActor: %s, HasAuthority: %d"), 
		HasAuthority() ? TEXT("Server") : TEXT("Client"), 
		OtherActor ? *OtherActor->GetName() : TEXT("NULL"),
		HasAuthority() ? 1 : 0);
	
	// 서버에서만 처리 (Destroy는 서버에서만 가능)
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("[PingMarkerActor] 클라이언트에서 호출됨 - 무시"));
		return;
	}

	// 자기 자신 또는 유효하지 않은 액터는 무시
	if (!OtherActor || OtherActor == this)
	{
		UE_LOG(LogTemp, Log, TEXT("[PingMarkerActor] [Server] 유효하지 않은 액터 무시"));
		return;
	}

	// Racer Player만 체크
	AAbstractRacer* Racer = Cast<AAbstractRacer>(OtherActor);
	if (!Racer)
	{
		UE_LOG(LogTemp, Log, TEXT("[PingMarkerActor] [Server] Racer가 아님: %s"), *OtherActor->GetName());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PingMarkerActor] [Server] ✓ Racer와 충돌 감지: %s, 핑 제거"), *Racer->GetName());
	
	// 핑을 생성한 Commander의 쿨타임 초기화
	if (CurrentPingData.OwnerPS.IsValid())
	{
		if (ACitRushPlayerState* OwnerPlayerState = Cast<ACitRushPlayerState>(CurrentPingData.OwnerPS.Get()))
		{
			if (ACommenderCharacter* Commander = OwnerPlayerState->GetPawn<ACommenderCharacter>())
			{
				// 글로벌 쿨타임 초기화
				Commander->ResetPingCooldown();
				
				UE_LOG(LogTemp, Log, TEXT("[PingMarkerActor] [Server] Commander 쿨타임 초기화: %s"), *Commander->GetName());
			}
		}
	}
	
	// 핑 위치 계산 (MarkerHeightOffset을 빼서 원래 위치로 복원)
	FVector PingLocation = GetActorLocation();
	PingLocation.Z -= MarkerHeightOffset;
	
	// GameState에서 핑 제거
	// RemovePingByLocation이 ActivePings에서 핑을 제거하고 OnRep_ActivePings를 호출하여:
	// 1. 모든 클라이언트의 PingMarkerManager가 RefreshPingMarkers를 호출하여 레벨의 PingActor 삭제
	// 2. OnRep_ActivePing이 OnPingUpdated 이벤트를 브로드캐스트하여 Map UI의 마커도 즉시 삭제
	if (UWorld* World = GetWorld())
	{
		if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
		{
			GameState->RemovePingByLocation(PingLocation);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[PingMarkerActor] [Server] GameState를 찾을 수 없습니다!"));
		}
	}
	
	// 서버의 PingActor도 즉시 Destroy (클라이언트의 PingActor는 UpdatePingMarkerManager에서 제거됨)
	Destroy();
}
