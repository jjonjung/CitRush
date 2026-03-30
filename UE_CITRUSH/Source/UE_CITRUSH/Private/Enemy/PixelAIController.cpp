// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/PixelAIController.h"
#include "Enemy/PixelEnemy.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AbilitySystemComponent.h"
#include "GAS/GameplayTags/GTEnemy.h"
#include "Player/Stats/Coin/CoinActor.h"
#include "Player/AbstractRacer.h"

APixelAIController::APixelAIController()
{
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*PerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.0f;

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void APixelAIController::BeginPlay()
{
	Super::BeginPlay();

	if (!PerceptionComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[PixelAI] Perception Component가 nullptr"));
	}
	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &APixelAIController::OnTargetPerceptionUpdated);
	
}

void APixelAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	APixelEnemy* PixelEnemy = Cast<APixelEnemy>(InPawn);
	if (!PixelEnemy)
	{
		UE_LOG(LogTemp, Error, TEXT("[PixelAI] PixelEnemy로 캐스팅 실패!"));
		return;
	}

	
}

void APixelAIController::OnUnPossess()
{
	Super::OnUnPossess();

	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Unpossessed"));
	}
}

void APixelAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	APixelEnemy* PixelEnemy = Cast<APixelEnemy>(GetPawn());
	if (!PixelEnemy) return;

	UAbilitySystemComponent* ASC = PixelEnemy->GetAbilitySystemComponent();
	if (!ASC) return;

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	ACoinActor* Coin = Cast<ACoinActor>(Actor);
	bool bIsCoin = (Coin != nullptr);

	AAbstractRacer* Racer = Cast<AAbstractRacer>(Actor);
	bool bIsRacer = (Racer != nullptr);

	if (Stimulus.WasSuccessfullySensed())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PixelAI] 감지: %s (Coin=%d, Racer=%d)"), *Actor->GetName(), bIsCoin, bIsRacer);

		if (bIsCoin)
		{
			BB->SetValueAsObject(TEXT("TargetCoin"), Coin);
			BB->SetValueAsObject(TEXT("TargetActor"), Coin);
			BB->SetValueAsBool(TEXT("IsTargetDetected"), true);

			UE_LOG(LogTemp, Warning, TEXT("[PixelAI] Coin 타겟 설정: %s"), *Coin->GetName());
		}
		else if (bIsRacer)
		{
			float Distance = FVector::Dist(PixelEnemy->GetActorLocation(), Racer->GetActorLocation());

			BB->SetValueAsObject(TEXT("NearbyRacer"), Racer);
			BB->SetValueAsFloat(TEXT("RacerDistance"), Distance);

			AActor* CurrentCoinTarget = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetCoin")));
			if (Distance <= RacerAttackRange && !CurrentCoinTarget)
			{
				BB->SetValueAsObject(TEXT("TargetActor"), Racer);
				BB->SetValueAsBool(TEXT("IsTargetDetected"), true);

				UE_LOG(LogTemp, Warning, TEXT("[PixelAI] Racer 공격 타겟 설정: %s (거리: %.1f)"), *Racer->GetName(), Distance);

				FGameplayEventData EventData;
				EventData.Target = Racer;
				EventData.Instigator = PixelEnemy;
				ASC->HandleGameplayEvent(CitRushTags::Enemy::Event::TargetDetected, &EventData);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[PixelAI] Racer 거리 %.1f (공격 범위 %.1f 초과 또는 Coin 타겟 우선)"), Distance, RacerAttackRange);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PixelAI] 놓침: %s"), *Actor->GetName());

		if (bIsCoin)
		{
			BB->ClearValue(TEXT("TargetCoin"));

			AActor* NearbyRacer = Cast<AActor>(BB->GetValueAsObject(TEXT("NearbyRacer")));
			if (NearbyRacer)
			{
				float Distance = BB->GetValueAsFloat(TEXT("RacerDistance"));
				if (Distance <= RacerAttackRange)
				{
					BB->SetValueAsObject(TEXT("TargetActor"), NearbyRacer);
					UE_LOG(LogTemp, Warning, TEXT("[PixelAI] Coin 소실, Racer로 타겟 전환"));
				}
				else
				{
					BB->ClearValue(TEXT("TargetActor"));
					BB->SetValueAsBool(TEXT("IsTargetDetected"), false);
				}
			}
			else
			{
				BB->ClearValue(TEXT("TargetActor"));
				BB->SetValueAsBool(TEXT("IsTargetDetected"), false);
			}
		}
		else if (bIsRacer)
		{
			BB->ClearValue(TEXT("NearbyRacer"));
			BB->ClearValue(TEXT("RacerDistance"));

			AActor* CurrentTarget = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
			if (CurrentTarget == Racer)
			{
				BB->ClearValue(TEXT("TargetActor"));
				BB->SetValueAsBool(TEXT("IsTargetDetected"), false);
			}
		}

		FGameplayEventData EventData;
		EventData.Instigator = PixelEnemy;
		ASC->HandleGameplayEvent(CitRushTags::Enemy::Event::TargetLost, &EventData);
	}
}

void APixelAIController::UpdateBlackboardTarget(AActor* Target)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(TEXT("TargetActor"), Target);
		
		if (Target)
		{
			BB->SetValueAsVector(TEXT("LastKnownLocation"), Target->GetActorLocation());
		}
	}
}

void APixelAIController::UpdateHealthPercentage()
{
	APixelEnemy* PixelEnemy = Cast<APixelEnemy>(GetPawn());
	if (!PixelEnemy) return;

}

AActor* APixelAIController::GetBlackboardTarget() const
{
	if (const UBlackboardComponent* BB = GetBlackboardComponent())
	{
		return Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
	}
	return nullptr;
}

float APixelAIController::GetBlackboardHealthPercentage() const
{
	if (const UBlackboardComponent* BB = GetBlackboardComponent())
	{
		return BB->GetValueAsFloat(TEXT("HealthPercentage"));
	}
	return 1.0f;
}
