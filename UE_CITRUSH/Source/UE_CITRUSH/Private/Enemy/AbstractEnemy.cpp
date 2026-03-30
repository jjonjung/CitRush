// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/AbstractEnemy.h"

#include "NativeGameplayTags.h"
#include "GAS/AbilitySystemComponent/BaseASC.h"
#include "GAS/AttributeSet/ASEnemy.h"

#include "Player/AbstractRacer.h"
#include "GAS/AttributeSet/ASRacer.h"

#include "GAS/GameplayAbility/BaseGA.h"


AAbstractEnemy::AAbstractEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
	if (!abilitySystemComponent)
	{
		abilitySystemComponent = CreateDefaultSubobject<UBaseASC>("EnemyASC");
	}
	if (!attributeSet)
	{
		attributeSet = CreateDefaultSubobject<UASEnemy>("AttributeSet");
	}
}

void AAbstractEnemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	/*if (IsValid(abilitySystemComponent))
	{
		abilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
		abilitySystemComponent->InitAbilityActorInfo(this, this);
		
	}*/
}

void AAbstractEnemy::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(abilitySystemComponent))
	{
		abilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
		abilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AAbstractEnemy::OnCollisionHit(const AAbstractRacer* racer, const FVector& hitLocation)
{
	if (!IsValid(racer) || !IsValid(abilitySystemComponent)) return;
	
	float currentTime = GetWorld()->GetTimeSeconds();
	if (currentTime - lastCollisionTime < collisionCooldown && lastCollisionRacer == racer) {return;}
	
	FVector hitDirection = (hitLocation - this->GetActorLocation()).GetSafeNormal();
	float dotProduct = FVector::DotProduct(GetActorForwardVector(), hitDirection);

	FGameplayEventData EventData;
	EventData.Instigator = this;
	EventData.Target = racer;
	//EventData.EventMagnitude = CalculateCollisionDamage(NormalImpulse);  // 충돌 강도 기반

	UAbilitySystemComponent* damagedASC = nullptr;
	FGameplayTag EventTag = FGameplayTag::EmptyTag;
	
	if (dotProduct < 0.0f)  // 뒤에서 충돌
	{
		EventTag = FGameplayTag::RequestGameplayTag("Event.Gameplay.Collision.Back");
		damagedASC = abilitySystemComponent;
		EventData.EventMagnitude = Cast<UASRacer>(racer->GetAttributeSet())->GetAttackPower();
	}
	else  // 앞에서 충돌
	{
		EventTag = FGameplayTag::RequestGameplayTag("Event.Gameplay.Collision.Front");
		damagedASC = racer->GetAbilitySystemComponent();
		EventData.EventMagnitude = attributeSet->GetAttackPower();
	}
	
	if (!IsValid(damagedASC) || EventTag == FGameplayTag::EmptyTag) {return;}
	damagedASC->HandleGameplayEvent(EventTag, &EventData);
}

void AAbstractEnemy::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	
}

UAbilitySystemComponent* AAbstractEnemy::GetAbilitySystemComponent() const
{
	return abilitySystemComponent;
}

UAttributeSet* AAbstractEnemy::GetAttributeSet() const
{
	return attributeSet;
}

void AAbstractEnemy::InitDefaultAttributes() const
{
	if (!abilitySystemComponent || !defaultAttributeEffect) {return;}
	
	FGameplayEffectContextHandle effectContext = abilitySystemComponent->MakeEffectContext();
	effectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle specHandle = abilitySystemComponent->MakeOutgoingSpec(defaultAttributeEffect, 1.f, effectContext);
	if (specHandle.IsValid())
	{
		abilitySystemComponent->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
	}
}

bool AAbstractEnemy::ActivateAbility(const TSubclassOf<UGameplayAbility>& ability)
{
	check(abilitySystemComponent);
	if (!HasAuthority()) {return false;}
	return false;
}

bool AAbstractEnemy::ActivateAbility(const FGameplayTag& gameplayTag)
{
	check(abilitySystemComponent);
	if (!HasAuthority()) {return false;}
	return false;
}

bool AAbstractEnemy::ReceiveAbility(const TSubclassOf<UGameplayAbility>& ability)
{
	check(abilitySystemComponent);
	if (!HasAuthority()) {return false;}
	FGameplayAbilitySpec abilitySpec(ability);  // Level도 정할 수 있음.
	abilitySystemComponent->GiveAbility(abilitySpec);
	return false;
}

bool AAbstractEnemy::ReceiveAbility(const FGameplayTag& gameplayTag)
{
	check(abilitySystemComponent);
	if (!HasAuthority()) {return false;}
	return false;
}
