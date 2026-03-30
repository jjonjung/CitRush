// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Components/VoiceDonorComponent.h"
#include "Player/Components/VoiceCaptureComponent.h"
#include "Player/Components/VoiceAcceptorComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_CLASS(UVoiceDonorComponent, CitRushVoiceDonorLog)

// ============================================================================
// Constructor & Lifecycle
// ============================================================================

UVoiceDonorComponent::UVoiceDonorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);  // 네트워크 복제 활성화
}

void UVoiceDonorComponent::BeginPlay()
{
	Super::BeginPlay();

	UVoiceCaptureComponent* tempVCC = GetOwner()->FindComponentByClass<UVoiceCaptureComponent>();
	RegisterVoiceCaptureComponent(tempVCC);
}

void UVoiceDonorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Delegate 언바인딩
	if (VoiceCaptureComponent)
	{
		VoiceCaptureComponent->OnAudioCaptured.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UVoiceDonorComponent::RegisterVoiceCaptureComponent(UVoiceCaptureComponent* vcc)
{
	if (IsValid(VoiceCaptureComponent) || !IsValid(vcc)) {return;}
	VoiceCaptureComponent = vcc;

	if (!VoiceCaptureComponent)
	{
		UE_LOG(CitRushVoiceDonorLog, Warning, TEXT("[VoiceDonor] VoiceCaptureComponent를 찾을 수 없습니다. 같은 Actor에 추가해야 합니다."));
		return;
	}

	// Delegate 바인딩
	VoiceCaptureComponent->OnAudioCaptured.AddDynamic(this, &UVoiceDonorComponent::OnAudioCaptured);

	UE_LOG(CitRushVoiceDonorLog, Log, TEXT("[VoiceDonor] 초기화 완료"));
}

// ============================================================================
// Public API
// ============================================================================

void UVoiceDonorComponent::SetTargetRacer(ETargetRacer Target)
{
	if (CurrentTarget == Target)
	{
		return;
	}

	CurrentTarget = Target;
	bIsTransmitting = (Target != ETargetRacer::None);

	UE_LOG(CitRushVoiceDonorLog, Log, TEXT("[VoiceDonor] SetTargetRacer: %d (Transmitting: %d)"),
		static_cast<int32>(Target), bIsTransmitting);
}

// ============================================================================
// Internal Methods
// ============================================================================

void UVoiceDonorComponent::OnAudioCaptured(const TArray<uint8>& PCMData)
{
	// 타겟이 설정되지 않았으면 무시
	if (CurrentTarget == ETargetRacer::None || !bIsTransmitting)
	{
		return;
	}

	// Client → Server RPC 호출
	ServerSendVoiceData(PCMData, CurrentTarget);
}

void UVoiceDonorComponent::ServerSendVoiceData_Implementation(const TArray<uint8>& PCMData, ETargetRacer Target)
{
	// Server에서 실행됨

	// 데이터 검증
	if (PCMData.Num() == 0)
	{
		UE_LOG(CitRushVoiceDonorLog, Warning, TEXT("[VoiceDonor] ServerSendVoiceData - PCM 데이터가 비어있습니다"));
		return;
	}

	// Target에 따라 처리
	if (Target == ETargetRacer::All)
	{
		// 전체 Racer에게 전송
		UE_LOG(CitRushVoiceDonorLog, Log, TEXT("[VoiceDonor] 전체 Racer에게 음성 전송 (크기: %d bytes)"), PCMData.Num());

		// GameState에서 모든 PlayerState 가져오기
		AGameStateBase* GameState = GetWorld()->GetGameState();
		if (!GameState)
		{
			return;
		}

		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (!PS || !PS->GetPawn())
			{
				continue;
			}

			// VoiceAcceptorComponent 찾기
			UVoiceAcceptorComponent* Acceptor = PS->GetPawn()->FindComponentByClass<UVoiceAcceptorComponent>();
			if (Acceptor)
			{
				Acceptor->ClientReceiveVoiceData(PCMData);
			}
		}
	}
	else
	{
		// 특정 Racer에게만 전송
		UE_LOG(CitRushVoiceDonorLog, Log, TEXT("[VoiceDonor] Racer %d에게 음성 전송 (크기: %d bytes)"),
			static_cast<int32>(Target), PCMData.Num());

		// TODO: Target Racer의 PlayerState 또는 Pawn 찾기
		// 현재는 GameState의 PlayerArray에서 인덱스로 찾는 방식
		// 실제로는 CitRushPlayerState에 Racer ID 같은 것이 있을 수 있음

		AGameStateBase* GameState = GetWorld()->GetGameState();
		if (!GameState)
		{
			return;
		}

		// Racer1, Racer2, Racer3 → 인덱스 1, 2, 3 (0은 Commander라고 가정)
		int32 TargetIndex = static_cast<int32>(Target);  // Racer1=1, Racer2=2, Racer3=3

		if (TargetIndex < 1 || TargetIndex > GameState->PlayerArray.Num())
		{
			UE_LOG(CitRushVoiceDonorLog, Warning, TEXT("[VoiceDonor] Target Racer 인덱스가 범위를 벗어났습니다: %d"), TargetIndex);
			return;
		}

		// PlayerArray에서 Target 찾기
		// 주의: 실제 프로젝트에서는 PlayerState에 Role 정보(Commander, Racer1, Racer2, Racer3)가 있어야 함
		APlayerState* TargetPS = GameState->PlayerArray[TargetIndex];
		if (!TargetPS || !TargetPS->GetPawn())
		{
			UE_LOG(CitRushVoiceDonorLog, Warning, TEXT("[VoiceDonor] Target Racer를 찾을 수 없습니다: %d"), TargetIndex);
			return;
		}

		// VoiceAcceptorComponent 찾기
		UVoiceAcceptorComponent* Acceptor = TargetPS->GetPawn()->FindComponentByClass<UVoiceAcceptorComponent>();
		if (Acceptor)
		{
			Acceptor->ClientReceiveVoiceData(PCMData);
		}
		else
		{
			UE_LOG(CitRushVoiceDonorLog, Warning, TEXT("[VoiceDonor] Target Racer에 VoiceAcceptorComponent가 없습니다"));
		}
	}
}

