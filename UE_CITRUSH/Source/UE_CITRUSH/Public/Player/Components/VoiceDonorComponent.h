// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/CitRushPlayerTypes.h"  // ETargetRacer
#include "VoiceDonorComponent.generated.h"

class UVoiceCaptureComponent;

/**
 * 음성 전송 Component (Commander 전용)
 *
 * 역할:
 * - VoiceCaptureComponent의 Delegate를 바인딩하여 캡처된 PCM 데이터 수신
 * - Client → Server RPC로 음성 데이터 전송
 * - Server에서 Target Racer의 VoiceAcceptorComponent로 전달
 *
 * 네트워크 흐름:
 *   [Commander Client]
 *     ↓ ServerSendVoiceData(PCMData, Target)
 *   [Server]
 *     ↓ Find Target Racer Actor
 *     ↓ VoiceAcceptorComponent::ClientReceiveVoiceData(PCMData)
 *   [Target Racer Client]
 *
 * 사용 예시:
 *   VoiceDonor->SetTargetRacer(ETargetRacer::Racer1);
 *   VoiceCapture->StartCapture();  // VoiceDonor가 자동으로 전송
 */
UCLASS(ClassGroup=(CitRushVoice), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UVoiceDonorComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(CitRushVoiceDonorLog, Log, All);

public:
	UVoiceDonorComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ========================================================================
	// Public API
	// ========================================================================

	/** VoiceCapture 등록 */
	UFUNCTION(BlueprintCallable)
	void RegisterVoiceCaptureComponent(UVoiceCaptureComponent* vcc);
	
	/** 타겟 Racer 설정 */
	UFUNCTION(BlueprintCallable, Category = "Voice|Donor")
	void SetTargetRacer(ETargetRacer Target);

	/** 현재 타겟 Racer 가져오기 */
	UFUNCTION(BlueprintPure, Category = "Voice|Donor")
	FORCEINLINE ETargetRacer GetTargetRacer() const { return CurrentTarget; }

	/** 음성 전송 활성화 여부 */
	UFUNCTION(BlueprintPure, Category = "Voice|Donor")
	FORCEINLINE bool IsTransmitting() const { return bIsTransmitting; }

private:
	// ========================================================================
	// Internal Methods
	// ========================================================================

	/** VoiceCaptureComponent의 Delegate 콜백 */
	UFUNCTION()
	void OnAudioCaptured(const TArray<uint8>& PCMData);

	/** Server RPC: Client → Server로 음성 데이터 전송 */
	UFUNCTION(Server, Reliable)
	void ServerSendVoiceData(const TArray<uint8>& PCMData, ETargetRacer Target);

	// ========================================================================
	// State Variables
	// ========================================================================

	/** VoiceCapture Component 참조 (자동으로 찾음) */
	UPROPERTY()
	UVoiceCaptureComponent* VoiceCaptureComponent;

	/** 현재 타겟 Racer */
	UPROPERTY(BlueprintReadOnly, Category = "Voice|State", meta = (AllowPrivateAccess = "true"))
	ETargetRacer CurrentTarget = ETargetRacer::None;

	/** 음성 전송 중 여부 */
	bool bIsTransmitting = false;
};
