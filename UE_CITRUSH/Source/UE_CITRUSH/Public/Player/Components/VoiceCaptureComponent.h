// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoiceCaptureComponent.generated.h"

class IVoiceCapture;

// ============================================================================
// Delegates
// ============================================================================

/** 오디오 캡처 시 호출되는 Delegate (PCM 데이터 전달) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudioCaptured, const TArray<uint8>&, AudioData);

// ============================================================================
// Audio Capture Configuration
// ============================================================================

/**
 * 오디오 캡처 설정
 *
 * 마이크 캡처 및 PCM 변환에 사용되는 설정입니다.
 */
USTRUCT(BlueprintType)
struct FAudioCaptureConfig
{
	GENERATED_BODY()

	/** 샘플레이트 (Hz) - 고정: 16000 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "16000", ClampMax = "16000"))
	int32 SampleRate = 16000;

	/** 채널 수 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "1", ClampMax = "1"))
	int32 NumChannels = 1;

	/** 비트 깊이 - 고정: 16 (16-bit PCM) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "16", ClampMax = "16"))
	int32 BitDepth = 16;

	/** 캡처 주기(ms) - 권장: 100~200ms */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "100.0", ClampMax = "200.0"))
	float CaptureIntervalMs = 150.0f;

	FAudioCaptureConfig() = default;
	
	/** 버퍼 크기(bytes) - 자동 계산됨 */
	int32 GetBufferSize() const
	{
		// BufferSize = SampleRate * (CaptureIntervalMs / 1000.0) * (BitDepth / 8) * NumChannels
		return FMath::CeilToInt(SampleRate * (CaptureIntervalMs / 1000.0f) * (BitDepth / 8) * NumChannels);
	}

	/** 설정 유효성 검증 */
	bool IsValid() const
	{
		return SampleRate == 16000 && NumChannels == 1 && BitDepth == 16 &&
			   CaptureIntervalMs >= 100.0f && CaptureIntervalMs <= 200.0f;
	}
};

/**
 * 마이크 오디오 캡처 전담 Component
 *
 * 역할:
 * - IVoiceCapture를 이용한 마이크 입력 캡처
 * - 주기적으로 PCM 데이터를 수집하여 Delegate로 브로드캐스트
 * - 다른 컴포넌트들이 Delegate를 바인딩하여 캡처된 데이터 활용
 *
 * 사용 예시:
 *   VoiceCapture->OnAudioCaptured.AddUObject(this, &UMyComponent::HandleAudioData);
 *   VoiceCapture->StartCapture();
 */
UCLASS(ClassGroup=(CitRushVoice), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UVoiceCaptureComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(CitRushVoiceCaptureLog, Log, All);

public:
	UVoiceCaptureComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ========================================================================
	// Public API
	// ========================================================================

	/** Voice Capture 초기화 */
	bool InitializeVoiceCapture();
	
	/** 마이크 캡처 시작 */
	UFUNCTION(BlueprintCallable, Category = "Voice|Capture")
	void StartCapture();

	/** 마이크 캡처 중지 */
	UFUNCTION(BlueprintCallable, Category = "Voice|Capture")
	void StopCapture();

	/** 현재 캡처 중인지 확인 */
	UFUNCTION(BlueprintPure, Category = "Voice|Capture")
	FORCEINLINE bool IsCapturing() const { return bIsCapturing; }

	// ========================================================================
	// Delegate
	// ========================================================================

	/** 오디오 캡처 시 호출됨 (PCM 데이터 전달) */
	UPROPERTY(BlueprintAssignable, Category = "Voice|Events")
	FOnAudioCaptured OnAudioCaptured;

protected:
	// ========================================================================
	// Configuration
	// ========================================================================

	/** 오디오 캡처 설정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Config")
	FAudioCaptureConfig AudioConfig;

private:
	// ========================================================================
	// Internal Methods
	// ========================================================================

	/** 타이머 콜백: 주기적으로 오디오 캡처 */
	void CaptureAudioTick();

	/** 캡처된 오디오 데이터 처리 */
	void ProcessCapturedAudio(const TArray<uint8>& CapturedData);

	// ========================================================================
	// State Variables
	// ========================================================================

	/** Voice Capture 인스턴스 */
	TSharedPtr<IVoiceCapture> VoiceCapture;

	/** 오디오 캡처 타이머 핸들 */
	FTimerHandle AudioCaptureTimerHandle;

	/** 오디오 버퍼 (캡처된 PCM 데이터 누적) */
	TArray<uint8> AudioBuffer;

	/** 오디오 캡처 활성화 여부 */
	bool bIsCapturing = false;
};
