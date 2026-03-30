// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/CitRushPlayerTypes.h"
#include "DefaultVoiceComponent.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class IOnlineSubsystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UDefaultVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UDefaultVoiceComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	UFUNCTION()
	void ChangeVoiceTarget(const APlayerState* target);

private:
	UFUNCTION()
	void RegisterTarget(ELoadingState newState);
	UFUNCTION(Category="Voice|Input")
	void ChangeTargetByKeyInput(const FInputActionValue& value);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice|Input")
	TObjectPtr<UInputMappingContext> IMC_Voice;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice|Input")
	TObjectPtr<UInputAction> IA_ChangeVoiceTarget;
	
private:
	UPROPERTY()
	TObjectPtr<APlayerController> ownerPlayerController;

	IOnlineSubsystem* oss = nullptr;
	TSharedPtr<class IVoiceCapture> voiceCapture;

	UPROPERTY()
	TMap<FUniqueNetIdRepl, bool> voiceTargets;
	UPROPERTY()
	FUniqueNetIdRepl currentTarget;
};
