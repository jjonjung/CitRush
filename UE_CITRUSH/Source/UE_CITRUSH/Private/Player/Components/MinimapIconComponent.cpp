#include "Player/Components/MinimapIconComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Player/CitRushPlayerState.h"
#include "Data/CitRushPlayerTypes.h"
#include "GameFlow/CitRushGameState.h"
#include "TimerManager.h"

UMinimapIconComponent::UMinimapIconComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bShowOnMap(true)
	, IconId(ERealtimeMapIconId::Racer1)
	, TeamId(0)
	, bRotateWithActor(false)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMinimapIconComponent::BeginPlay()
{
	Super::BeginPlay();

	// 먼저 PlayerState 기반으로 시도
	TryUpdateIconFromPlayerState();
	
	// PlayerState로 설정되지 않았으면 태그 기반으로 시도
	if (IconId == ERealtimeMapIconId::Racer1) // 기본값이면 태그 확인
	{
		ApplyIconIdFromActorTag();
	}
}

void UMinimapIconComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머 취소
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTimerHandle);
	}

	// 델리게이트 구독 해제
	UnsubscribeFromPlayerStateRoleChange();

	Super::EndPlay(EndPlayReason);
}

float UMinimapIconComponent::GetActorYaw() const
{
	if (AActor* Owner = GetOwner())
	{
		return Owner->GetActorRotation().Yaw;
	}
	return 0.0f;
}

void UMinimapIconComponent::ApplyIconIdFromPlayerState()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	ACitRushPlayerState* CitRushPS = OwnerPawn->GetPlayerState<ACitRushPlayerState>();
	if (!CitRushPS)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ACitRushGameState* GameState = World->GetGameState<ACitRushGameState>();
	if (!GameState)
	{
		return;
	}

	// PlayerRole 기반으로 IconId 설정 (Racer만 처리)
	const EPlayerRole PlayerRole = CitRushPS->GetPlayerRole();
	if (PlayerRole == EPlayerRole::Racer)
	{
		// targetIndex를 사용하여 Racer1, Racer2, Racer3 결정 (더 정확함)
		const ETargetRacer TargetRacer = CitRushPS->GetPlayerInfo().targetIndex;
		
		ERealtimeMapIconId NewIconId = IconId; // 기본값 유지
		switch (TargetRacer)
		{
		case ETargetRacer::Racer1:
			NewIconId = ERealtimeMapIconId::Racer1;
			break;
		case ETargetRacer::Racer2:
			NewIconId = ERealtimeMapIconId::Racer2;
			break;
		case ETargetRacer::Racer3:
			NewIconId = ERealtimeMapIconId::Racer3;
			break;
		default:
			// targetIndex가 설정되지 않았으면 PlayerArray에서 배열 인덱스로 폴백
			if (AGameStateBase* GameStateBase = World->GetGameState())
			{
				int32 RacerIndex = -1;
				int32 CurrentRacerIndex = 0;
				
				// PlayerArray를 순회하며 Racer 역할의 PlayerState 찾기
				for (APlayerState* PS : GameStateBase->PlayerArray)
				{
					if (ACitRushPlayerState* FoundPS = Cast<ACitRushPlayerState>(PS))
					{
						if (FoundPS->GetPlayerRole() == EPlayerRole::Racer)
						{
							if (FoundPS == CitRushPS)
							{
								RacerIndex = CurrentRacerIndex;
								break;
							}
							CurrentRacerIndex++;
						}
					}
				}
				
				switch (RacerIndex)
				{
				case 0:
					NewIconId = ERealtimeMapIconId::Racer1;
					break;
				case 1:
					NewIconId = ERealtimeMapIconId::Racer2;
					break;
				case 2:
					NewIconId = ERealtimeMapIconId::Racer3;
					break;
				default:
					UE_LOG(LogTemp, Warning, TEXT("[MinimapIconComponent] Actor %s: targetIndex=%d, RacerIndex=%d (범위 초과), 기본 IconId 유지"),
						*OwnerPawn->GetName(), (int32)TargetRacer, RacerIndex);
					break;
				}
			}
			break;
		}
		
		if (NewIconId != IconId)
		{
			IconId = NewIconId;
			UE_LOG(LogTemp, Log, TEXT("[MinimapIconComponent] Actor %s: PlayerState 기반으로 IconId 설정 -> %s (targetIndex=%d)"),
				*OwnerPawn->GetName(), 
				NewIconId == ERealtimeMapIconId::Racer1 ? TEXT("Racer1") :
				NewIconId == ERealtimeMapIconId::Racer2 ? TEXT("Racer2") :
				NewIconId == ERealtimeMapIconId::Racer3 ? TEXT("Racer3") : TEXT("Unknown"),
				(int32)TargetRacer);
		}
	}
	// Commander, Spectator, None은 아이콘 ID 변경하지 않음 (기본값 유지)
}

void UMinimapIconComponent::ApplyIconIdFromActorTag()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// Actor의 태그를 확인하여 IconId 설정
	// 태그 이름과 IconId Enum 이름을 매칭
	const TArray<FName>& ActorTags = OwnerActor->Tags;
	
	for (const FName& Tag : ActorTags)
	{
		FString TagString = Tag.ToString();
		
		// 태그 이름을 IconId로 매핑
		if (TagString.Equals(TEXT("Racer1"), ESearchCase::IgnoreCase))
		{
			IconId = ERealtimeMapIconId::Racer1;
			UE_LOG(LogTemp, Log, TEXT("[MinimapIconComponent] Actor %s: 태그 'Racer1'로 IconId 설정"), *OwnerActor->GetName());
			return;
		}
		else if (TagString.Equals(TEXT("Racer2"), ESearchCase::IgnoreCase))
		{
			IconId = ERealtimeMapIconId::Racer2;
			UE_LOG(LogTemp, Log, TEXT("[MinimapIconComponent] Actor %s: 태그 'Racer2'로 IconId 설정"), *OwnerActor->GetName());
			return;
		}
		else if (TagString.Equals(TEXT("Racer3"), ESearchCase::IgnoreCase))
		{
			IconId = ERealtimeMapIconId::Racer3;
			UE_LOG(LogTemp, Log, TEXT("[MinimapIconComponent] Actor %s: 태그 'Racer3'로 IconId 설정"), *OwnerActor->GetName());
			return;
		}
		else if (TagString.Equals(TEXT("Enemy"), ESearchCase::IgnoreCase))
		{
			IconId = ERealtimeMapIconId::Enemy;
			UE_LOG(LogTemp, Log, TEXT("[MinimapIconComponent] Actor %s: 태그 'Enemy'로 IconId 설정"), *OwnerActor->GetName());
			return;
		}
	}
	
	// 매칭되는 태그가 없으면 기본값 유지
	UE_LOG(LogTemp, Log, TEXT("[MinimapIconComponent] Actor %s: 매칭되는 태그 없음, 기본 IconId 유지"), *OwnerActor->GetName());
}

void UMinimapIconComponent::TryUpdateIconFromPlayerState()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		// Pawn이 아니면 태그 기반으로만 동작
		return;
	}

	ACitRushPlayerState* CitRushPS = OwnerPawn->GetPlayerState<ACitRushPlayerState>();
	if (!CitRushPS)
	{
		// PlayerState가 아직 없으면 짧은 시간 후 재시도
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				RetryTimerHandle,
				this,
				&UMinimapIconComponent::TryUpdateIconFromPlayerState,
				0.1f, // 0.1초 후 재시도
				false // 반복하지 않음
			);
		}
		return;
	}

	// PlayerState를 찾았으면 타이머 취소
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTimerHandle);
	}

	// 아이콘 업데이트
	ApplyIconIdFromPlayerState();

	// PlayerState 역할 변경 델리게이트 구독
	SubscribeToPlayerStateRoleChange();
}

void UMinimapIconComponent::SubscribeToPlayerStateRoleChange()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	ACitRushPlayerState* CitRushPS = OwnerPawn->GetPlayerState<ACitRushPlayerState>();
	if (!CitRushPS)
	{
		return;
	}

	// 기존 구독 해제 후 새로 구독
	UnsubscribeFromPlayerStateRoleChange();
	CitRushPS->OnPlayerRoleChangedDelegate.AddUObject(this, &UMinimapIconComponent::OnPlayerRoleChanged);
}

void UMinimapIconComponent::UnsubscribeFromPlayerStateRoleChange()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	ACitRushPlayerState* CitRushPS = OwnerPawn->GetPlayerState<ACitRushPlayerState>();
	if (!CitRushPS)
	{
		return;
	}

	CitRushPS->OnPlayerRoleChangedDelegate.RemoveAll(this);
}

void UMinimapIconComponent::OnPlayerRoleChanged(const EPlayerRole& NewRole)
{
	// 역할이 변경되면 아이콘 다시 업데이트
	ApplyIconIdFromPlayerState();
}

