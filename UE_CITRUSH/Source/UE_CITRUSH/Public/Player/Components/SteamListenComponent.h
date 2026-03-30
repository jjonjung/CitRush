// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SteamListenComponent.generated.h"


enum class ETargetRacer : uint8;
class USteamVoiceComponent;
class USoundWaveProcedural;
class IOnlineVoice;
class ISteamUser;
class ISteamNetworking;

/**
 * Steam P2P Voice 수신 Component (Racer 전용)
 * Steam Networking API로 Opus 음성 패킷 수신 후 PCM 변환하여 재생
 */
UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API USteamListenComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(SteamListenLog, Warning, All);

public:
	USteamListenComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool RegisterLocalUser(const APlayerState* playerState);
	
	UFUNCTION(BlueprintCallable, Category="Listen")
	FORCEINLINE void SetVoiceChannel(int32 newChannel) {voiceChannel = newChannel;}
	UFUNCTION(BlueprintCallable, Category="Listen")
	void StartListening();
	UFUNCTION(BlueprintCallable, Category="Listen")
	void StopListening();
	UFUNCTION(BlueprintPure, Category="Listen")
	FORCEINLINE bool IsListening() const {return bIsListening;}
	UFUNCTION(BlueprintCallable, Category = "Listen|Voice")
	bool RegisterCommanderVoiceComponent(USteamVoiceComponent* voiceComponent);

private:
	void InitializeAudioComponent();

	void ReceiveVoicePacket();
	UFUNCTION()
	void ProcessOpusData(const TArray<uint8>& opusData);
	UFUNCTION()
	void PlayPCMData(const TArray<uint8>& PCMData);
	UFUNCTION()
	void OnCommanderVoiceTargetChanged(ETargetRacer newTarget);
	bool IsTargetedAtMe(ETargetRacer newTarget);
	ETargetRacer GetMyRacerNumber();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Listen")
	TObjectPtr<USoundWaveProcedural> soundWave = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Listen")
	TObjectPtr<UAudioComponent> audioComponent = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Listen")
	bool bIsListening = false;

	UPROPERTY(EditDefaultsOnly, Category = "Listen|Config")
	int32 sampleRate = 24000;
	/** P2P Voice Channel (Commander와 동일해야 함) */
	UPROPERTY(EditDefaultsOnly, Category = "Listen|Config")
	int32 voiceChannel = 0;
	
	static constexpr uint32 MaxPCMBufferSize = 11025 * 2 * 2;  // 2초 분량..
	TArray<uint8> PCMBuffer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Listen|Voice")
	TObjectPtr<USteamVoiceComponent> steamVoiceComponent = nullptr;

private:
	ISteamUser* steamUserAPI = nullptr;
	ISteamNetworking* steamNetworkingAPI = nullptr;
};
