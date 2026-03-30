// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Components/VoiceCaptureComponent.h"
#include "Voice.h"
#include "TimerManager.h"
#include "Private/Network/Schemas/WebSocketV1/SendWebSocket.h"

DEFINE_LOG_CATEGORY_CLASS(UVoiceCaptureComponent, CitRushVoiceCaptureLog)

// ============================================================================
// Constructor & Lifecycle
// ============================================================================

UVoiceCaptureComponent::UVoiceCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 기본 오디오 설정
	AudioConfig = FAudioCaptureConfig();
}

void UVoiceCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	// 오디오 설정 검증
	if (!AudioConfig.IsValid())
	{
		UE_LOG(CitRushVoiceCaptureLog, Warning, TEXT("[VoiceCapture] AudioConfig가 유효하지 않습니다. 기본값으로 재설정합니다."));
		AudioConfig = FAudioCaptureConfig();
	}

	UE_LOG(CitRushVoiceCaptureLog, Log, TEXT("[VoiceCapture] 초기화 완료"));
}

void UVoiceCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 캡처 중이면 중지
	if (bIsCapturing)
	{
		StopCapture();
	}

	// VoiceCapture 해제
	VoiceCapture.Reset();

	Super::EndPlay(EndPlayReason);
}

// ============================================================================
// Public API
// ============================================================================

void UVoiceCaptureComponent::StartCapture()
{
	if (bIsCapturing)
	{
		UE_LOG(CitRushVoiceCaptureLog, Warning, TEXT("[VoiceCapture] StartCapture() - 이미 캡처 중입니다"));
		return;
	}

	UE_LOG(CitRushVoiceCaptureLog, Log, TEXT("[VoiceCapture] StartCapture() - 오디오 캡처 시작"));

	// Voice Capture 초기화
	if (!VoiceCapture.IsValid())
	{
		if (!InitializeVoiceCapture())
		{
			UE_LOG(CitRushVoiceCaptureLog, Error, TEXT("[VoiceCapture] Voice Capture 초기화 실패"));
			return;
		}
	}

	// Voice Capture 시작
	if (VoiceCapture->Start())
	{
		bIsCapturing = true;
		AudioBuffer.Reset();

		// 타이머 시작 (AudioConfig.CaptureIntervalMs 간격으로 CaptureAudioTick 호출)
		GetWorld()->GetTimerManager().SetTimer(
			AudioCaptureTimerHandle,
			this,
			&UVoiceCaptureComponent::CaptureAudioTick,
			AudioConfig.CaptureIntervalMs / 1000.0f,  // 초 단위로 변환
			true  // 반복
		);

		UE_LOG(CitRushVoiceCaptureLog, Log, TEXT("[VoiceCapture] 오디오 캡처 시작됨 (간격: %.0fms)"), AudioConfig.CaptureIntervalMs);
	}
	else
	{
		UE_LOG(CitRushVoiceCaptureLog, Error, TEXT("[VoiceCapture] Voice Capture 시작 실패"));
	}
}

void UVoiceCaptureComponent::StopCapture()
{
	if (!bIsCapturing)
	{
		UE_LOG(CitRushVoiceCaptureLog, Warning, TEXT("[VoiceCapture] StopCapture() - 캡처 중이 아닙니다"));
		return;
	}

	UE_LOG(CitRushVoiceCaptureLog, Log, TEXT("[VoiceCapture] StopCapture() - 오디오 캡처 중지"));

	// 타이머 중지
	GetWorld()->GetTimerManager().ClearTimer(AudioCaptureTimerHandle);

	// Voice Capture 중지
	if (VoiceCapture.IsValid())
	{
		VoiceCapture->Stop();
	}

	bIsCapturing = false;
	AudioBuffer.Reset();

	UE_LOG(CitRushVoiceCaptureLog, Log, TEXT("[VoiceCapture] 오디오 캡처 중지됨"));
}

// ============================================================================
// Internal Methods
// ============================================================================

bool UVoiceCaptureComponent::InitializeVoiceCapture()
{
	UE_LOG(CitRushVoiceCaptureLog, Log, TEXT("[VoiceCapture] Voice Capture 초기화 시작"));

	// Voice Module 가져오기
	if (!FModuleManager::Get().IsModuleLoaded("Voice"))
	{
		FModuleManager::Get().LoadModule("Voice");
	}

	// Voice Capture 인스턴스 생성
	VoiceCapture = FVoiceModule::Get().CreateVoiceCapture(FString(), AudioConfig.SampleRate, AudioConfig.NumChannels);

	if (!VoiceCapture.IsValid())
	{
		UE_LOG(CitRushVoiceCaptureLog, Error, TEXT("[VoiceCapture] Voice Capture 인스턴스 생성 실패"));
		return false;
	}

	UE_LOG(CitRushVoiceCaptureLog, Log, TEXT("[VoiceCapture] Voice Capture 초기화 성공 (SampleRate: %d, Channels: %d)"),
		AudioConfig.SampleRate, AudioConfig.NumChannels);

	return true;
}

void UVoiceCaptureComponent::CaptureAudioTick()
{
	if (!bIsCapturing || !VoiceCapture.IsValid())
	{
		return;
	}

	// Voice Capture에서 사용 가능한 음성 데이터 가져오기
	uint32 AvailableVoiceData;
	VoiceCapture->GetCaptureState(AvailableVoiceData);

	if (AvailableVoiceData > 0)
	{
		// 임시 버퍼에 음성 데이터 가져오기
		TArray<uint8> CapturedData;
		CapturedData.SetNumUninitialized(AvailableVoiceData);

		uint32 OutVoiceDataSize = 0;
		EVoiceCaptureState::Type CaptureState = VoiceCapture->GetVoiceData(
			CapturedData.GetData(),
			AvailableVoiceData,
			OutVoiceDataSize
		);

		if (CaptureState == EVoiceCaptureState::Ok && OutVoiceDataSize > 0)
		{
			// 실제 크기로 조정
			CapturedData.SetNum(OutVoiceDataSize);

			// 캡처된 오디오 처리
			ProcessCapturedAudio(CapturedData);
		}
		else if (CaptureState == EVoiceCaptureState::NoData)
		{
			// 음성 데이터 없음 (정상)
		}
	}
}

void UVoiceCaptureComponent::ProcessCapturedAudio(const TArray<uint8>& CapturedData)
{
	// 버퍼에 추가
	AudioBuffer.Append(CapturedData);

	// 권장 크기에 도달하면 브로드캐스트
	const int32 ExpectedBufferSize = AudioConfig.GetBufferSize();

	if (AudioBuffer.Num() >= ExpectedBufferSize)
	{
		// 청크 생성 (권장 크기만큼)
		TArray<uint8> ChunkToSend;
		ChunkToSend.Append(AudioBuffer.GetData(), ExpectedBufferSize);

		// Delegate 브로드캐스트
		OnAudioCaptured.Broadcast(ChunkToSend);

		// 버퍼에서 전송한 부분 제거
		AudioBuffer.RemoveAt(0, ExpectedBufferSize, EAllowShrinking::No);
	}
}

