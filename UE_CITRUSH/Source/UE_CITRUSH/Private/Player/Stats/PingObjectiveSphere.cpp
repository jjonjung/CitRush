#include "Player/Stats/PingObjectiveSphere.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerState.h"
#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"
#include "Engine/World.h"

APingObjectiveSphere::APingObjectiveSphere()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 서버에서 복제

	// Root Component
	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	// Sphere Component
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetupAttachment(RootComponent);
	SphereComponent->SetSphereRadius(SphereRadius);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionObjectType(ECC_WorldDynamic);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComponent->SetGenerateOverlapEvents(true);
}

void APingObjectiveSphere::BeginPlay()
{
	Super::BeginPlay();

	// 오버랩 이벤트 바인딩 (서버에서만)
	if (HasAuthority())
	{
		SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &APingObjectiveSphere::OnBeginOverlap);
	}
}

void APingObjectiveSphere::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	// Racer만 처리
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	APlayerState* PS = Pawn->GetPlayerState();
	if (!PS)
	{
		return;
	}

	// Racer 역할 확인
	ACitRushPlayerState* CitRushPS = Cast<ACitRushPlayerState>(PS);
	if (!CitRushPS || CitRushPS->GetPlayerRole() != EPlayerRole::Racer)
	{
		return;
	}

	// 이미 점수를 받은 플레이어인지 확인
	if (ScoredPlayers.Contains(PS))
	{
		return;
	}

	// 점수 부여
	AwardScore(PS);
}

void APingObjectiveSphere::ResetScoredPlayers()
{
	if (!HasAuthority())
	{
		return;
	}

	ScoredPlayers.Empty();
	UE_LOG(LogTemp, Log, TEXT("[PingObjectiveSphere] ScoredPlayers reset"));
}

void APingObjectiveSphere::AwardScore(APlayerState* PlayerState)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(PlayerState))
	{
		return;
	}

	// 이미 점수를 받은 플레이어인지 확인
	if (ScoredPlayers.Contains(PlayerState))
	{
		return;
	}

	// 점수 부여 (PlayerState의 SetScore 사용)
	PlayerState->SetScore(PlayerState->GetScore() + ScoreValue);
	ScoredPlayers.Add(PlayerState);

	UE_LOG(LogTemp, Log, TEXT("[PingObjectiveSphere] Score awarded to %s: +%d (Total: %.0f)"), 
		*PlayerState->GetPlayerName(), ScoreValue, PlayerState->GetScore());

	// Blueprint 이벤트 호출 가능
	// OnScoreAwarded.Broadcast(PlayerState, ScoreValue);
}
