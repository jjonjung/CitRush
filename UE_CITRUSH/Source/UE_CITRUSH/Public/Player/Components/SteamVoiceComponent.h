// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SteamVoiceComponent.generated.h"

class ACitRushPlayerState;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class ACitRushGameState;
enum class ETargetRacer : uint8;
class IOnlineVoice;
class ISteamUser;
class ISteamNetworking;
class CSteamID;

/**
 * Steam P2P Voice 송신 Component (Commander 전용)
 * Steam Voice API로 음성 캡처 후 Opus 인코딩하여 P2P 전송
 */
UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API USteamVoiceComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(SteamVoiceLog, Warning, All);

public:
	USteamVoiceComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	// Must Register in PlayerController
	UFUNCTION(BlueprintCallable)
	bool RegisterLocalUser(const APlayerController* ownerPlayerController, const APlayerState* playerState);
	
	UFUNCTION(BlueprintCallable, Category="Voice")
	void StartVoiceChat();
	UFUNCTION(BlueprintCallable, Category="Voice")
	void StopVoiceChat();
	UFUNCTION(BlueprintCallable, Category="Voice")
	FORCEINLINE bool IsCapturing() const {return bIsCapturing;}
	
	// Hard Coding 하드코딩
	UFUNCTION(Category="Voice")
	void ChangeVoiceTarget(ETargetRacer newTarget);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptureVoiceBuffer, const TArray<uint8>&, OpusData);
	FOnCaptureVoiceBuffer OnCaptureVoiceBuffer;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVoiceTargetChanged, ETargetRacer, TargetRacer);
	FOnVoiceTargetChanged OnVoiceTargetChanged;

	UFUNCTION(BlueprintCallable, Category="Voice")
	FORCEINLINE float GetCaptureInterval() {return voiceCaptureInterval;}

private:
	void CaptureVoice();
	void FlushVoiceBuffer();
	bool GetTargetSteamID(const TArray<ACitRushPlayerState*>& cPSs, CSteamID& targetSteamID) const;
	void SendVoiceP2P(const TArray<uint8>& opusData) const;
	// Hard Coding 하드코딩
	UFUNCTION(Category="Voice")
	void OnRep_VoiceTargetChanged();
	UFUNCTION(Category="Voice|Input")
	void ChangeTargetByKeyInput(const FInputActionValue& value);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice|Status")
	bool bIsVoiceChatEnabled = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Voice|Status")
	bool bIsVoiceChatNeededToCapture = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice|Status")
	bool bIsVoiceChatting = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice|Status")
	bool bIsCapturing = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Voice|Status")
	float voiceCaptureInterval = 0.3f;
	// Hard Coding 하드코딩
	UPROPERTY(ReplicatedUsing=OnRep_VoiceTargetChanged, EditDefaultsOnly, BlueprintReadWrite, Category="Voice|Status")
	ETargetRacer voiceTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice|Input")
	TObjectPtr<UInputMappingContext> IMC_Voice;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice|Input")
	TObjectPtr<UInputAction> IA_ChangeVoiceTarget;

private:
	FTimerHandle voiceCaptureTimer;
	ISteamUser* steamUserAPI = nullptr;
	ISteamNetworking* steamNetworkingAPI = nullptr;

	static constexpr uint32 maxVoiceBufferSize = 8 * 1024;  // 8 KB
	TArray<uint8> voiceBuffer;
};
