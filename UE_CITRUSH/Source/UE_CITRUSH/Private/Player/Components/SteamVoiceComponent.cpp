// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Components/SteamVoiceComponent.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include "Player/CitRushPlayerState.h"
#include "GameFlow/CitRushGameState.h"
#include "Data/CitRushPlayerTypes.h"

#include "Data/InputMappingsSettings.h"

#if WITH_STEAMWORKS
#pragma warning(push)
#pragma warning(disable: 4996)  // strncpy 경고 끄기
#include "steam/steam_api.h"
#include "steam/isteamuser.h"
#include "steam/isteamnetworking.h"
#pragma warning(pop)
#endif

DEFINE_LOG_CATEGORY_CLASS(USteamVoiceComponent, SteamVoiceLog);

USteamVoiceComponent::USteamVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	voiceBuffer.SetNum(maxVoiceBufferSize);
	voiceTarget = ETargetRacer::None;
	
	if (const FInputMappingData* data = UInputMappingsSettings::Get()->inputMappings.Find("IMC_Voice"))
	{
		IMC_Voice = data->inputMappingContext;
		IA_ChangeVoiceTarget = data->inputActions["IA_ChangeVoiceTarget"];
	}
	
	ComponentTags.Add("Voice");
}

void USteamVoiceComponent::BeginPlay()
{
	Super::BeginPlay();
	APawn* owner = GetOwner<APawn>();
	if (!IsValid(owner)) {return;}
	
	APlayerController* pc = owner->GetController<APlayerController>();
	if (!pc || !pc->IsLocalController()) {return;}
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(IMC_Voice, 1);
	}
	if (UEnhancedInputComponent* eic = Cast<UEnhancedInputComponent>(pc->InputComponent))
	{
		eic->BindAction(IA_ChangeVoiceTarget, ETriggerEvent::Started, this, &USteamVoiceComponent::ChangeTargetByKeyInput);
	}
}

void USteamVoiceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopVoiceChat();
	
	steamUserAPI = nullptr;
	steamNetworkingAPI = nullptr;
	
	Super::EndPlay(EndPlayReason);
}

void USteamVoiceComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USteamVoiceComponent, voiceTarget);
}

bool USteamVoiceComponent::RegisterLocalUser(const APlayerController* ownerPlayerController, const APlayerState* playerState)
{
	if (!IsValid(ownerPlayerController) || !IsValid(playerState)) {return false;}
	if (ownerPlayerController != GetWorld()->GetFirstPlayerController()) {return false;}

	const ACitRushPlayerState* ps = Cast<ACitRushPlayerState>(playerState);
	if (!IsValid(ps)) {UE_LOG(SteamVoiceLog, Warning, TEXT("Failed to cast CitRishPS")); return false;}
	/*IOnlineVoicePtr voiceInterface = oss->GetVoiceInterface();
	if (voiceInterface.IsValid()) {}*/
	
#if STEAM_SDK_ENABLED
	steamUserAPI = SteamUser();
	steamNetworkingAPI = SteamNetworking();
	
	if (steamUserAPI) {UE_LOG(SteamVoiceLog, Warning, TEXT("O : ISteamUser interface obtained"));}
	else {UE_LOG(SteamVoiceLog, Warning, TEXT("X : ISteamUser interface Is Null"));}
	if (steamNetworkingAPI) {UE_LOG(SteamVoiceLog, Warning, TEXT("O : ISteamNetworking interface obtained"));}
	else {UE_LOG(SteamVoiceLog, Warning, TEXT("X : ISteamNetworking interface Is Null"));}
	if (SteamAPI_IsSteamRunning())
	{
	}
	else {UE_LOG(SteamVoiceLog, Warning, TEXT("X : Steam Is not Running")); return false;}
#else
	UE_LOG(SteamVoiceLog, Warning, TEXT("X : Failed to GET SteamSDK");)
	return false;
#endif

	bIsVoiceChatNeededToCapture = ps->GetPlayerRole() == EPlayerRole::Commander;
	bIsVoiceChatEnabled = true; 
	return true;
}


void USteamVoiceComponent::StartVoiceChat()
{
#if STEAM_SDK_ENABLED
	if (!steamUserAPI) {return;}
	
	steamUserAPI->StartVoiceRecording();
	if (!bIsCapturing)
	{
		GetWorld()->GetTimerManager().SetTimer(voiceCaptureTimer,
		   this, &USteamVoiceComponent::CaptureVoice,
		   voiceCaptureInterval, true
	   );
		bIsCapturing = true;
	}
	bIsVoiceChatting = true;
#else
	CITRUSH_LOG("X : Failed to GET SteamSDK");
#endif
}

void USteamVoiceComponent::StopVoiceChat()
{
#if STEAM_SDK_ENABLED
	if (!steamUserAPI) {return;}
	
	steamUserAPI->StopVoiceRecording();
	bIsCapturing = false;
	bIsVoiceChatting = false;
#else
	CITRUSH_LOG("X : Failed to GET SteamSDK");
#endif
}



void USteamVoiceComponent::CaptureVoice()
{
#if STEAM_SDK_ENABLED
	// SteamUser 인터페이스가 아직 설정되지 않았으면 아무 것도 하지 않는다.
	if (!steamUserAPI)
	{
		UE_LOG(SteamVoiceLog, Warning, TEXT("CaptureVoice: steamUserAPI is null. Did RegisterLocalUser/StartVoiceChat run?"));
		return;
	}

	OUT uint32 availableBytes = 0;
	EVoiceResult availableResult = steamUserAPI->GetAvailableVoice(&availableBytes);
	if (availableResult != k_EVoiceResultOK || availableBytes == 0)
	{
		if (!bIsCapturing)
		{
			GetWorld()->GetTimerManager().ClearTimer(voiceCaptureTimer);
		}
		UE_LOG(SteamVoiceLog, Warning, TEXT("X : NOT OK Available Voice"));return;
	}
	
	OUT TArray<uint8> compressedData;
	compressedData.SetNum(availableBytes);
	OUT uint32 bytesWritten = 0;
	EVoiceResult voiceResult = steamUserAPI->GetVoice(
		true,
		compressedData.GetData(),
		availableBytes,
		&bytesWritten,
		false,
		nullptr,
		0,
		nullptr,
		0
	);

	/*if (voiceResult != EVoiceResult::k_EVoiceResultOK)
	{
		UE_LOG(SteamVoiceLog, Warning, TEXT("X : Voice Result Failed __ResultState : %d"), voiceResult);
	}
	else if (bytesWritten == 0)
	{
		UE_LOG(SteamVoiceLog, Warning, TEXT("O : Nothing to Say"));
	}
	else */
	if (voiceResult == EVoiceResult::k_EVoiceResultOK && bytesWritten > 0)
	{
		
		compressedData.SetNum(bytesWritten);
		UE_LOG(SteamVoiceLog, Warning, TEXT("O : Captured %d Bytes of Compressed Data"), bytesWritten);
		//OnCaptureVoiceBuffer.Broadcast(compressedData);
		SendVoiceP2P(compressedData);
		
		
		
	}
#endif
}

void USteamVoiceComponent::FlushVoiceBuffer()
{
	CaptureVoice();
}

bool USteamVoiceComponent::GetTargetSteamID(const TArray<ACitRushPlayerState*>& cPSs, CSteamID& targetSteamID) const
{
	int32 index = static_cast<int32>(voiceTarget);
	if (!cPSs.IsValidIndex(index)) {UE_LOG(SteamVoiceLog, Warning, TEXT("X : Invalid Index : %d"), index); return false;}
		
	const FUniqueNetIdPtr& targetId = cPSs[index]->GetUniqueId().GetUniqueNetId();
	if (!targetId.IsValid()) {UE_LOG(SteamVoiceLog, Warning, TEXT("X : Invalid Net Id")); return false;}

	// UniqueNetId를 Steam ID로 변환
	uint64 steamID64 = FCString::Strtoui64(*targetId->ToString(), nullptr, 10);
	targetSteamID = CSteamID(steamID64);
	return true;
}

void USteamVoiceComponent::SendVoiceP2P(const TArray<uint8>& compressedData) const
{
#if STEAM_SDK_ENABLED
	if (!steamNetworkingAPI) {UE_LOG(SteamVoiceLog, Warning, TEXT("X : Steam Networking Failed"));return;}

	// Hard Coding 하드코딩
	if (voiceTarget == ETargetRacer::All)
	{
		// 모든 Racer에게 전송 (추후 구현: 세션의 모든 플레이어)
		UE_LOG(SteamVoiceLog, Warning, TEXT("Send to all players not yet implemented"));
		return;
	}

	if (ACitRushGameState* cGS = GetWorld()->GetGameState<ACitRushGameState>())
	{
		// Hard Coding 하드코딩
		CSteamID targetSteamID;
		if (GetTargetSteamID(cGS->GetCitRushPlayers(), targetSteamID)) return;

		bool bSuccess = steamNetworkingAPI->SendP2PPacket(
				targetSteamID,
				compressedData.GetData(),
				compressedData.Num(),
				k_EP2PSendUnreliableNoDelay,
				0  // Channel 0 = Voice
		);

		if (bSuccess)
		{
			UE_LOG(SteamVoiceLog, Verbose, TEXT("Sent voice to Racer: %d"), targetSteamID.GetAccountID());
		}
	}
#endif
}

void USteamVoiceComponent::OnRep_VoiceTargetChanged()
{
	if (bIsCapturing && GetWorld()->GetTimerManager().IsTimerPaused(voiceCaptureTimer))
	{
		GetWorld()->GetTimerManager().UnPauseTimer(voiceCaptureTimer);
	}
	OnVoiceTargetChanged.Broadcast(voiceTarget);
}

void USteamVoiceComponent::ChangeTargetByKeyInput(const FInputActionValue& value)
{
	int32 index = static_cast<int>(value.Get<float>());
	UE_LOG(SteamVoiceLog, Warning, TEXT("%d"), index);
	ETargetRacer newTarget = static_cast<ETargetRacer>(index);
	if (newTarget == voiceTarget)
	{
		ChangeVoiceTarget(ETargetRacer::None);
		StopVoiceChat();
		return;
	}
	if (bIsCapturing)
	{
		GetWorld()->GetTimerManager().PauseTimer(voiceCaptureTimer);
	}
	FlushVoiceBuffer();
	ChangeVoiceTarget(newTarget);
}

void USteamVoiceComponent::ChangeVoiceTarget(ETargetRacer newTarget)
{
	UE_LOG(SteamVoiceLog, Log, TEXT("Server_ChangeVoiceTarget_Implementation Start"));
	voiceTarget = newTarget;
	if (GetOwner()->HasAuthority())
	{
		OnRep_VoiceTargetChanged();
	}
	UE_LOG(SteamVoiceLog, Log, TEXT("Server_ChangeVoiceTarget_Implementation End"));
}