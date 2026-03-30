// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Utility/DebugHelper.h"
#include "LocalDataFlowSubsystem.generated.h"

class ACitRushPlayerState;
struct FStreamableHandle;
class UEndWidget;

/**
 * 
 */
UCLASS()
class UE_CITRUSH_API ULocalDataFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	DECLARE_CLASS_LOG(LocalDataFlowSubsystem)

public:
	ULocalDataFlowSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void CollectStringData(const FName& key, const FString& value);
	void CollectFloatData(const FName& key, const float& value);
	void CollectParticipantData(TArray<APlayerState*> inParticipants);
	void CollectParticipantData(TArray<TObjectPtr<APlayerState>> inParticipants);
	void ResetParticipantData();

protected:
	UPROPERTY()
	TSubclassOf<UEndWidget> endWidgetClass;
	UPROPERTY()
	TObjectPtr<UEndWidget> endWidget;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FString> collectedStringData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, float> collectedFloatData;
	UPROPERTY()
	TArray<FString> participantNames;
};
