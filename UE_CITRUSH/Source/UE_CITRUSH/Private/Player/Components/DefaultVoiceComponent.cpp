// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Components/DefaultVoiceComponent.h"

#include "OnlineSubsystemUtils.h"
#include "Data/InputMappingsSettings.h"
#include "GameFramework/GameStateBase.h"
#include "Player/CitRushPlayerState.h"
#include "Utility/DebugHelper.h"


UDefaultVoiceComponent::UDefaultVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	if (const FInputMappingData* data = UInputMappingsSettings::Get()->inputMappings.Find("IMC_Voice"))
	{
		IMC_Voice = data->inputMappingContext;
		IA_ChangeVoiceTarget = data->inputActions["IA_ChangeVoiceTarget"];
	}
	
	ComponentTags.Add("Voice");
}

void UDefaultVoiceComponent::BeginPlay()
{
	Super::BeginPlay();
	oss = Online::GetSubsystem(GetWorld());
	if (APawn* owner = GetOwner<APawn>())
	{
		if (ACitRushPlayerState* cPS = owner->GetPlayerState<ACitRushPlayerState>())
		{
			cPS->OnLoadingStateChanged.AddUObject(this, &UDefaultVoiceComponent::RegisterTarget);
		}
	}
}

void UDefaultVoiceComponent::ChangeVoiceTarget(const APlayerState* target)
{
	if (!oss) {return;}
	if (voiceTargets.Num() <= 0) {return;}
	IOnlineVoicePtr voiceInterface = oss->GetVoiceInterface();
	if (!voiceInterface.IsValid())
	{
		CITRUSH_LOG("Voice Interface Is Not Valid");
		return;
	}
	if (!IsValid(target)) {return;}
	FUniqueNetIdRepl uniqueId = target->GetUniqueId();
	if (!uniqueId.IsValid()) {return;}
	if (voiceTargets.Contains(uniqueId))
	{
		voiceInterface->MuteRemoteTalker(0, *currentTarget, false);
		voiceTargets[uniqueId] = !voiceTargets[uniqueId];
		voiceTargets[uniqueId] ?
			voiceInterface->UnmuteRemoteTalker(0, *uniqueId, false)
			: voiceInterface->MuteRemoteTalker(0, *uniqueId, false);
		
	}
	if (AGameStateBase* gs = GetWorld()->GetGameState())
	{
	}
}

void UDefaultVoiceComponent::RegisterTarget(ELoadingState newState)
{
}

void UDefaultVoiceComponent::ChangeTargetByKeyInput(const FInputActionValue& value)
{
}

