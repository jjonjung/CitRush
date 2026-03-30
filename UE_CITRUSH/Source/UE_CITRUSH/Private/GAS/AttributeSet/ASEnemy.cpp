// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AttributeSet/ASEnemy.h"

#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/AbstractRacer.h"
#include "GameFlow/CitRushGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Player/Car/VehicleDemoUITest.h"
#include "VehicleTemplate/UE_CITRUSHPlayerController.h"
#include "Player/Controller/CitRushPlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UASEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UASEnemy, Health, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASEnemy, MaxHealth, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASEnemy, DetectionRange, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASEnemy, DefaultDetectionRange, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASEnemy, Speed, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASEnemy, DefaultSpeed, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASEnemy, AttackPower, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UASEnemy, DefaultAttackPower, COND_None, ELifetimeRepNotifyCondition::REPNOTIFY_Always);
}

void UASEnemy::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		// 이전 HP 저장 (HP 감소 감지용)
		PreviousHealth = GetHealth();
		NewValue = FMath::Clamp<float>(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetDetectionRangeAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.f, GetDefaultDetectionRange());
	}
	else if (Attribute == GetSpeedAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.f, GetDefaultSpeed());
	}
	else if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.f, GetDefaultAttackPower());
	}
}

void UASEnemy::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		float OldHealth = PreviousHealth;
		float NewHealth = GetHealth();
		SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));
		
		// HP가 감소했는지 확인 (공격 성공)
		// 서버에서만 실행되도록 확인 (AttributeSet은 GetOwningAbilitySystemComponent를 통해 소유자 확인)
		bool bHasAuthority = false;
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			if (AActor* OwnerActor = ASC->GetAvatarActor())
			{
				bHasAuthority = OwnerActor->HasAuthority();
			}
		}
		
		if (OldHealth > 0.0f && NewHealth < OldHealth && bHasAuthority)
		{
			// 공격한 레이서 찾기
			AAbstractRacer* AttackingRacer = nullptr;
			
			// GameplayEffect의 Instigator 찾기
			if (Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent())
			{
				if (AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent()->GetAvatarActor())
				{
					AttackingRacer = Cast<AAbstractRacer>(InstigatorActor);
				}
			}
			
			// 공격한 레이서가 있으면 전체 레이서들에게 알림 전송
			if (AttackingRacer)
			{
				UWorld* World = GetWorld();
				if (World)
				{
					if (ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>())
					{
						// 모든 레이서 PlayerState 가져오기
						TArray<ACitRushPlayerState*> RacerPlayerStates = GameState->GetPlayerStatesByRole(EPlayerRole::Racer);
						
						// 공격한 레이서의 이름 가져오기
						FString AttackerName = AttackingRacer->GetPlayerState<ACitRushPlayerState>() 
							? AttackingRacer->GetPlayerState<ACitRushPlayerState>()->GetPlayerName() 
							: TEXT("레이서");
						
						FString NotificationText = FString::Printf(TEXT("%s가 팩맨 공격에 성공했습니다!"), *AttackerName);
						
						// 각 레이서에게 알림 전송 (Client RPC 사용)
						for (ACitRushPlayerState* RacerPS : RacerPlayerStates)
						{
							if (!RacerPS)
							{
								continue;
							}
							
							// PlayerController 찾기 및 메시지 전송
							if (APlayerController* PC = RacerPS->GetPlayerController())
							{
								// AUE_CITRUSHPlayerController로 캐스팅 시도
								if (AUE_CITRUSHPlayerController* VehiclePC = Cast<AUE_CITRUSHPlayerController>(PC))
								{
									VehiclePC->ClientShowStateMessage(NotificationText);
								}
								// CitRushPlayerController로 캐스팅 시도 (멀티플레이 지원)
								else if (ACitRushPlayerController* CitRushPC = Cast<ACitRushPlayerController>(PC))
								{
									CitRushPC->ClientShowStateMessage(NotificationText);
								}
							}
						}
						
						// 커맨더에게도 알림 전송 (Client RPC 사용)
						TArray<ACitRushPlayerState*> CommanderPlayerStates = GameState->GetPlayerStatesByRole(EPlayerRole::Commander);
						for (ACitRushPlayerState* CommanderPS : CommanderPlayerStates)
						{
							if (!CommanderPS)
							{
								continue;
							}
							
							// PlayerController 찾기 및 메시지 전송
							if (APlayerController* PC = CommanderPS->GetPlayerController())
							{
								// CitRushPlayerController로 캐스팅 시도
								if (ACitRushPlayerController* CitRushPC = Cast<ACitRushPlayerController>(PC))
								{
									CitRushPC->ClientShowStateMessage(NotificationText);
									UE_LOG(LogTemp, Log, TEXT("[ASEnemy] 커맨더에게 팩맨 공격 성공 알림 전송: %s"), *NotificationText);
								}
							}
						}
						
						UE_LOG(LogTemp, Log, TEXT("[ASEnemy] 팩맨 공격 성공 알림 전송: %s"), *NotificationText);
					}
				}
			}
		}
		
		// 이전 HP 초기화
		PreviousHealth = -1.0f;
	}
	else if (Data.EvaluatedData.Attribute == GetDetectionRangeAttribute())
	{
		SetDetectionRange(FMath::Clamp(GetDetectionRange(), 0.f, GetDefaultDetectionRange()));
	}
	else if (Data.EvaluatedData.Attribute == GetSpeedAttribute())
	{
		SetSpeed(FMath::Clamp(GetSpeed(), 0.f, GetDefaultSpeed()));
	}
	else if (Data.EvaluatedData.Attribute == GetAttackPowerAttribute())
	{
		SetAttackPower(FMath::Clamp(GetAttackPower(), 0.f, GetDefaultAttackPower()));
	}
}

DEFINE_ATTRIBUTE(UASEnemy, Health)
DEFINE_ATTRIBUTE(UASEnemy, MaxHealth)
DEFINE_ATTRIBUTE(UASEnemy, DetectionRange)
DEFINE_ATTRIBUTE(UASEnemy, DefaultDetectionRange)
DEFINE_ATTRIBUTE(UASEnemy, Speed)
DEFINE_ATTRIBUTE(UASEnemy, DefaultSpeed)
DEFINE_ATTRIBUTE(UASEnemy, AttackPower)
DEFINE_ATTRIBUTE(UASEnemy, DefaultAttackPower)
