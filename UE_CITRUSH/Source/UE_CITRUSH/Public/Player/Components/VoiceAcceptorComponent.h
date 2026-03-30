// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoiceAcceptorComponent.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

/**
 * 음성 수신 및 재생 Component (Racer 전용)
 *
 * 역할:
 * - Server로부터 Client RPC를 통해 Commander의 음성 데이터(PCM) 수신
 * - PCM 데이터를 USoundWaveProcedural로 변환하여 오디오 재생
 * - UAudioComponent를 통해 실시간 음성 출력
 *
 * 네트워크 흐름:
 *   [Server]
 *     ↓ ClientReceiveVoiceData(PCMData)
 *   [Racer Client]
 *     ↓ PlayReceivedVoice(PCMData)
 *     ↓ USoundWaveProcedural → UAudioComponent
 *   [Speaker Output]
 *
 * 사용 예시:
 *   - Racer Pawn에 자동으로 붙여놓으면 자동으로 작동
 *   - VoiceDonorComponent가 Server RPC를 통해 전달한 음성을 재생
 */
UCLASS(ClassGroup=(CitRushVoice), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UVoiceAcceptorComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(CitRushVoiceAcceptorLog, Log, All);

public:
	UVoiceAcceptorComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ========================================================================
	// Network RPC
	// ========================================================================

	/** Client RPC: Server → Client로 음성 데이터 수신 (VoiceDonorComponent에서 호출) */
	UFUNCTION(Client, Reliable)
	void ClientReceiveVoiceData(const TArray<uint8>& PCMData);

private:
	// ========================================================================
	// Internal Methods
	// ========================================================================

	/** AudioComponent 초기화 */
	void InitializeAudioComponent();

	/** 수신한 PCM 데이터를 오디오로 재생 */
	void PlayReceivedVoice(const TArray<uint8>& PCMData);

	/** PCM 데이터를 USoundWaveProcedural에 추가 */
	void QueueAudioData(const TArray<uint8>& PCMData);

	// ========================================================================
	// Audio Components
	// ========================================================================

	/** 오디오 재생 컴포넌트 */
	UPROPERTY()
	UAudioComponent* AudioComponent;

	/** 프로시저럴 사운드 웨이브 (런타임 오디오 스트리밍) */
	UPROPERTY()
	USoundWaveProcedural* ProceduralSoundWave;

	// ========================================================================
	// Configuration
	// ========================================================================

	/** 오디오 샘플레이트 (16kHz - VoiceCapture와 동일) */
	int32 SampleRate = 16000;

	/** 채널 수 (1 = Mono) */
	int32 NumChannels = 1;
};
