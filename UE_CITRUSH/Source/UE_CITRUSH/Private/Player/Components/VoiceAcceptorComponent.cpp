// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Components/VoiceAcceptorComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_CLASS(UVoiceAcceptorComponent, CitRushVoiceAcceptorLog)

// ============================================================================
// Constructor & Lifecycle
// ============================================================================

UVoiceAcceptorComponent::UVoiceAcceptorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UVoiceAcceptorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Client에서만 AudioComponent 초기화
	if (GetOwnerRole() != ROLE_Authority)
	{
		InitializeAudioComponent();
	}

	UE_LOG(CitRushVoiceAcceptorLog, Log, TEXT("[VoiceAcceptor] 초기화 완료 (Role: %d)"), static_cast<int32>(GetOwnerRole()));
}

void UVoiceAcceptorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// AudioComponent 정리
	if (AudioComponent)
	{
		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
		AudioComponent = nullptr;
	}

	// ProceduralSoundWave 정리
	if (ProceduralSoundWave)
	{
		ProceduralSoundWave->MarkAsGarbage();
		ProceduralSoundWave = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// ============================================================================
// Network RPC
// ============================================================================

void UVoiceAcceptorComponent::ClientReceiveVoiceData_Implementation(const TArray<uint8>& PCMData)
{
	// Client에서만 실행됨

	if (PCMData.Num() == 0)
	{
		UE_LOG(CitRushVoiceAcceptorLog, Warning, TEXT("[VoiceAcceptor] ClientReceiveVoiceData - PCM 데이터가 비어있습니다"));
		return;
	}

	UE_LOG(CitRushVoiceAcceptorLog, Log, TEXT("[VoiceAcceptor] 음성 데이터 수신 (크기: %d bytes)"), PCMData.Num());

	// 음성 재생
	PlayReceivedVoice(PCMData);
}

// ============================================================================
// Internal Methods
// ============================================================================

void UVoiceAcceptorComponent::InitializeAudioComponent()
{
	UE_LOG(CitRushVoiceAcceptorLog, Log, TEXT("[VoiceAcceptor] AudioComponent 초기화 시작"));

	// AudioComponent 생성
	AudioComponent = NewObject<UAudioComponent>(GetOwner());
	if (!AudioComponent)
	{
		UE_LOG(CitRushVoiceAcceptorLog, Error, TEXT("[VoiceAcceptor] AudioComponent 생성 실패"));
		return;
	}

	AudioComponent->RegisterComponent();
	AudioComponent->bAutoActivate = false;
	AudioComponent->bAlwaysPlay = false;

	// USoundWaveProcedural 생성
	ProceduralSoundWave = NewObject<USoundWaveProcedural>();
	if (!ProceduralSoundWave)
	{
		UE_LOG(CitRushVoiceAcceptorLog, Error, TEXT("[VoiceAcceptor] ProceduralSoundWave 생성 실패"));
		return;
	}

	// SoundWave 설정 (16kHz, Mono, 16-bit PCM)
	ProceduralSoundWave->SetSampleRate(SampleRate);
	ProceduralSoundWave->NumChannels = NumChannels;
	ProceduralSoundWave->Duration = INDEFINITELY_LOOPING_DURATION;  // 무한 스트리밍
	ProceduralSoundWave->SoundGroup = SOUNDGROUP_Voice;
	ProceduralSoundWave->bLooping = false;

	// AudioComponent에 SoundWave 설정
	AudioComponent->SetSound(ProceduralSoundWave);

	UE_LOG(CitRushVoiceAcceptorLog, Log, TEXT("[VoiceAcceptor] AudioComponent 초기화 완료 (SampleRate: %d, Channels: %d)"),
		SampleRate, NumChannels);
}

void UVoiceAcceptorComponent::PlayReceivedVoice(const TArray<uint8>& PCMData)
{
	if (!AudioComponent || !ProceduralSoundWave)
	{
		UE_LOG(CitRushVoiceAcceptorLog, Warning, TEXT("[VoiceAcceptor] PlayReceivedVoice - AudioComponent 또는 ProceduralSoundWave가 초기화되지 않았습니다"));
		return;
	}

	// PCM 데이터를 SoundWave에 추가
	QueueAudioData(PCMData);

	// AudioComponent가 재생 중이 아니면 시작
	if (!AudioComponent->IsPlaying())
	{
		AudioComponent->Play();
		UE_LOG(CitRushVoiceAcceptorLog, Log, TEXT("[VoiceAcceptor] 오디오 재생 시작"));
	}
}

void UVoiceAcceptorComponent::QueueAudioData(const TArray<uint8>& PCMData)
{
	if (!ProceduralSoundWave)
	{
		return;
	}

	// PCM 데이터를 ProceduralSoundWave의 큐에 추가
	// USoundWaveProcedural::QueueAudio는 uint8 배열을 받음
	ProceduralSoundWave->QueueAudio(PCMData.GetData(), PCMData.Num());

	UE_LOG(CitRushVoiceAcceptorLog, Log, TEXT("[VoiceAcceptor] 오디오 데이터 큐에 추가 (%d bytes)"), PCMData.Num());
}

