// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Components/SteamListenComponent.h"

#include "Data/CitRushPlayerTypes.h"

#include "Components/AudioComponent.h"
#include "GameFlow/CitRushGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Player/Components/SteamVoiceComponent.h"
#include "Sound/SoundWaveProcedural.h"

#if WITH_STEAMWORKS
#pragma warning(push)
#pragma warning(disable: 4996)  // strncpy 경고 끄기
#include "steam/steam_api.h"
#include "steam/isteamuser.h"
#include "steam/isteamnetworking.h"
#pragma warning(pop)
#endif

DEFINE_LOG_CATEGORY_CLASS(USteamListenComponent, SteamListenLog);

USteamListenComponent::USteamListenComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;
	PCMBuffer.SetNum(MaxPCMBufferSize);
}

void USteamListenComponent::BeginPlay()
{
	Super::BeginPlay();

#if STEAM_SDK_ENABLED
	
	if (SteamAPI_IsSteamRunning())
	{

	}
	else
	{
		UE_LOG(SteamListenLog, Error, TEXT("X : Steam is not running"));
	}
#else
	UE_LOG(RacerVoicePlaybackLog, Error, TEXT("X : STEAM_SDK_ENABLED not defined"));
#endif
	
	InitializeAudioComponent();

}

void USteamListenComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopListening();
	if (IsValid(audioComponent))
	{
		audioComponent->Stop();
		audioComponent->DestroyComponent();
		audioComponent = nullptr;
	}
	
	if (IsValid(soundWave))
	{
		soundWave->MarkAsGarbage();
		soundWave = nullptr;
	}

#if STEAM_SDK_ENABLED
	steamUserAPI = nullptr;
	steamNetworkingAPI = nullptr;
#endif
	
	Super::EndPlay(EndPlayReason);
}

void USteamListenComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsListening)
	{
		ReceiveVoicePacket();
	}
}

bool USteamListenComponent::RegisterLocalUser(const APlayerState* playerState)
{
	if (!IsValid(playerState)) {return false;}
	if (!playerState->GetPlayerController()) {return false;}

	const ACitRushPlayerState* ps = Cast<ACitRushPlayerState>(playerState);
	if (!IsValid(ps)) {UE_LOG(SteamListenLog, Warning, TEXT("Failed to cast CitRishPS")); return false;}
	steamUserAPI = SteamUser();
	steamNetworkingAPI = SteamNetworking();
	if (steamUserAPI)
	{
		UE_LOG(SteamListenLog, Log, TEXT("O : ISteamUser interface obtained"));
	}
	else
	{
		UE_LOG(SteamListenLog, Error, TEXT("X : Failed to get ISteamUser"));
		return false;
	}
	if (steamNetworkingAPI)
	{
		UE_LOG(SteamListenLog, Log, TEXT("O : ISteamNetworking interface obtained"));
		return true;
	}
	return false;
}

void USteamListenComponent::StartListening()
{
	if (!steamNetworkingAPI) {UE_LOG(SteamListenLog, Warning, TEXT("Start X : SteamNetworkingAPI is NULL"));return;}
	bIsListening = true;
}

void USteamListenComponent::StopListening()
{
	if (IsValid(audioComponent) && audioComponent->IsPlaying())
	{
		audioComponent->Stop();
	}
	/* // 이거 되나?
	steamVoiceComponent->OnCaptureVoiceBuffer.RemoveDynamic(this, &USteamListenComponent::ProcessOpusData);
	*/
	bIsListening = false;
}

bool USteamListenComponent::RegisterCommanderVoiceComponent(USteamVoiceComponent* voiceComponent)
{
	steamVoiceComponent = voiceComponent;
	if (!IsValid(steamVoiceComponent)) {return false;}
	steamVoiceComponent->OnVoiceTargetChanged.AddDynamic(this, &USteamListenComponent::OnCommanderVoiceTargetChanged);
	PrimaryComponentTick.TickInterval = voiceComponent->GetCaptureInterval();
	return true;
}

void USteamListenComponent::InitializeAudioComponent()
{
	soundWave = NewObject<USoundWaveProcedural>(this);
	if (!soundWave) {UE_LOG(SteamListenLog, Error, TEXT("X : Failed to create SoundWaveProcedural")); return;}

	soundWave->SetSampleRate(sampleRate);
	soundWave->NumChannels = 1;
	soundWave->Duration = INDEFINITELY_LOOPING_DURATION;
	soundWave->SoundGroup = ESoundGroup::SOUNDGROUP_Voice;
	soundWave->bLooping = false;

	audioComponent = NewObject<UAudioComponent>(GetOwner());
	if (!audioComponent) {UE_LOG(SteamListenLog, Error, TEXT("X : Failed to create AudioComponent")); return;}

	audioComponent->SetSound(soundWave);
	audioComponent->bAutoActivate = true;
	audioComponent->bAlwaysPlay = true;
	audioComponent->RegisterComponent();
}

void USteamListenComponent::ReceiveVoicePacket()
{
#if STEAM_SDK_ENABLED
	if (!steamNetworkingAPI) {UE_LOG(SteamListenLog, Warning, TEXT("Receive X : SteamNetworkingAPI is NULL"));return;}

	uint32 PacketSize = 0;
	while (steamNetworkingAPI->IsP2PPacketAvailable(&PacketSize, voiceChannel))
	{
		if (PacketSize == 0) {break;}

		TArray<uint8> OpusData;
		OpusData.SetNum(PacketSize);

		OUT CSteamID SenderSteamID;
		OUT uint32 BytesRead = 0;

		bool bSuccess = steamNetworkingAPI->ReadP2PPacket(
				OpusData.GetData(),
				PacketSize,
				&BytesRead,
				&SenderSteamID,
				voiceChannel
		);

		if (bSuccess && BytesRead > 0)
		{
			OpusData.SetNum(BytesRead);

			UE_LOG(SteamListenLog, Verbose,
				TEXT("Received %d bytes from %llu"),
				BytesRead, SenderSteamID.ConvertToUint64()
			);

			// Opus 디코딩 및 재생
			ProcessOpusData(OpusData);
		}
	}
#endif

}

void USteamListenComponent::ProcessOpusData(const TArray<uint8>& opusData)
{
#if STEAM_SDK_ENABLED
	if (!steamUserAPI) {UE_LOG(SteamListenLog, Warning, TEXT("Opus Decode X : SteamUserAPI is NULL")); return;}

	uint32 PCMSize = 0;
	EVoiceResult Result = steamUserAPI->DecompressVoice(
		opusData.GetData(),      // Opus 입력
		opusData.Num(),
		PCMBuffer.GetData(),       // PCM 출력
		PCMBuffer.Num(),
		&PCMSize,                // 실제 PCM 크기
		sampleRate                    // Sample Rate
	);

	if (Result == k_EVoiceResultOK && PCMSize > 0)
	{
		TArray<uint8> PCMData;
		PCMData.Append(PCMBuffer.GetData(), PCMSize);
		PlayPCMData(PCMData);
	}
#endif

}

void USteamListenComponent::PlayPCMData(const TArray<uint8>& PCMData)
{
	if (!IsValid(soundWave)) {UE_LOG(SteamListenLog, Warning, TEXT("X : SoundWave Object is Not Created"));return;}

	soundWave->QueueAudio(PCMData.GetData(), PCMData.Num());

	if (IsValid(audioComponent) && !audioComponent->IsPlaying())
	{
		audioComponent->Play();
	}
}

void USteamListenComponent::OnCommanderVoiceTargetChanged(ETargetRacer newTarget)
{
	UE_LOG(SteamListenLog, Warning, TEXT("Voice target changed to: %d"), (int32)newTarget);

	// 자신이 타겟인지 확인
	if (IsTargetedAtMe(newTarget))
	{
		UE_LOG(SteamListenLog, Warning, TEXT("I am the target! Starting listening..."));
		StartListening();
	}
	else
	{
		UE_LOG(SteamListenLog, Warning, TEXT("Not my target. Stopping listening..."));
		StopListening();
	}

}

bool USteamListenComponent::IsTargetedAtMe(ETargetRacer newTarget)
{
	if (newTarget == ETargetRacer::All) {return true;}

	// 자신의 Racer 번호와 비교
	ETargetRacer myNumber = GetMyRacerNumber();
	return myNumber == newTarget;
}

ETargetRacer USteamListenComponent::GetMyRacerNumber()
{
	APawn* owner = GetOwner<APawn>();
	if (!IsValid(owner)) {return ETargetRacer::None;}
	ACitRushPlayerState* ps = owner->GetPlayerState<ACitRushPlayerState>();
	if (!IsValid(ps) || ps->GetPlayerRole() != EPlayerRole::Racer) {return ETargetRacer::None;}
	
	if (ACitRushGameState* gs = GetWorld()->GetGameState<ACitRushGameState>())
	{
		TArray<ACitRushPlayerState*> racers = gs->GetPlayerStatesByRole(EPlayerRole::Racer);
		int32 index = racers.IndexOfByKey(ps);
		if (index == INDEX_NONE) {return ETargetRacer::None;}
		
		UE_LOG(SteamListenLog, Warning, TEXT("Owner Racer Index : %d"), index + 1);
		return static_cast<ETargetRacer>(index + 1);
	}
	
	return ETargetRacer::None;
}
