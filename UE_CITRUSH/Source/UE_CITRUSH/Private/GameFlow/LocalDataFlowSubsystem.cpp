// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFlow/LocalDataFlowSubsystem.h"

#include "GameFlow/LobbyGameState.h"
#include "Player/CitRushPlayerState.h"
#include "Utility/WidgetBlueprintLoader.h"
#include "UI/End/EndWidget.h"

DEFINE_UCLASS_LOG(LocalDataFlowSubsystem)

ULocalDataFlowSubsystem::ULocalDataFlowSubsystem()
{
	const UWidgetBlueprintLoader* loader = UWidgetBlueprintLoader::Get();
	UWidgetBlueprintDataAsset* PDA_WBP = loader->PDA_WBP.LoadSynchronous();
	endWidgetClass = PDA_WBP->GetWidgetBlueprintClassByKey(TEXT("End"));
}

void ULocalDataFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
}

void ULocalDataFlowSubsystem::CollectStringData(const FName& key, const FString& value)
{
	Log(TEXT("Collect String Data %s : %s"), *key.ToString(), *value);
	if (collectedStringData.Contains(key))
	{
		collectedStringData[key] = value;
	}
	else
	{
		collectedStringData.Emplace(key, value);
	}
}

void ULocalDataFlowSubsystem::CollectFloatData(const FName& key, const float& value)
{
	if (collectedFloatData.Contains(key))
	{
		collectedFloatData[key] += value;
	}
	else
	{
		collectedFloatData.Emplace(key, value);
	}
}

void ULocalDataFlowSubsystem::CollectParticipantData(TArray<APlayerState*> inParticipants)
{
	participantNames = {"", "", "", ""};
	for (APlayerState* ps : inParticipants)
	{
		if (!IsValid(ps)) {continue;}
		if (ACitRushPlayerState* cPS = Cast<ACitRushPlayerState>(ps))
		{
			int32 index = static_cast<int32>(cPS->GetPlayerInfo().targetIndex);
			if (index > 4) {return;}
			participantNames[index] = cPS->GetPlayerName();
		}
	}
}

void ULocalDataFlowSubsystem::CollectParticipantData(TArray<TObjectPtr<APlayerState>> inParticipants)
{
	participantNames = {"-", "-", "-", "-"};
	for (TObjectPtr<APlayerState> ps : inParticipants)
	{
		if (!IsValid(ps)) {continue;}
		if (ACitRushPlayerState* cPS = Cast<ACitRushPlayerState>(ps))
		{
			int32 index = static_cast<int32>(cPS->GetPlayerInfo().targetIndex);
			if (index > 4) {return;}
			participantNames[index] = cPS->GetPlayerName();
		}
	}
}

void ULocalDataFlowSubsystem::ResetParticipantData()
{
	participantNames.Empty();
}


/*
void ULocalDataFlowSubsystem::ReadyForLevel(FName LevelPackageName, const int32& playersNum)
{
	if (!GetWorld()->GetFirstPlayerController()->IsLocalController()) {return;}
	if (collectedData.bStartLoadLevel) {return;}
	collectedData.bStartLoadLevel = true;
	loadingWidget = CreateWidget<ULoadingWidget>(GetGameInstance(), loadingWidgetClass);
	loadingWidget->AddToViewport(3);

	collectedData.playersNum = playersNum;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FName> Dependencies;
    
	AssetRegistryModule.Get().GetDependencies(LevelPackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package);
	/*TSet<FName> allowedAssetTypes = {
		FName(TEXT("StaticMesh")),
		FName(TEXT("SkeletalMesh")),
		FName(TEXT("Material")),
		FName(TEXT("Blueprint"))
	};#1#

	TArray<FSoftObjectPath> AssetsToLoad;
	for (const FName& Dependency : Dependencies)
	{
		FString DepString = Dependency.ToString();
		if (!DepString.StartsWith(TEXT("/Game/"))) { continue; }
		FSoftObjectPath objectPath = FSoftObjectPath(DepString);
		FAssetData asset = AssetRegistryModule.Get().GetAssetByObjectPath(objectPath);
		//if (!asset.IsValid()) { continue; }
		//if (!allowedAssetTypes.Contains(asset.AssetClassPath.GetAssetName())) { continue; }
		
		AssetsToLoad.Add(objectPath);
		LogWarning(TEXT("Asset : %s"), *DepString);
	}

	loadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(AssetsToLoad, FStreamableDelegate::CreateUObject(this, &ULocalDataFlowSubsystem::OnLoadCompleteCallback));
	GetGameInstance()->GetTimerManager().SetTimer(
		loadProgressTimer
		,this, &ULocalDataFlowSubsystem::ProgressLoadingTimer
		, 1.f, true
	);
}

void ULocalDataFlowSubsystem::ProgressLoadingTimer()
{
	if (loadHandle.IsValid() && loadProgressTimer.IsValid())
	{
		collectedData.levelLoadProgressPercent = loadHandle->GetLoadProgress() * 100.f;
		if (localCitRushPS)
		{
			localCitRushPS->GetDataManagerComponent()->ServerRPC_UpdateLoadingLevelPercentage(collectedData.levelLoadProgressPercent);
		}
		Log(TEXT("LoginGameLevel: %s\n	StartLoadLevel: %s\n	Progress Percentage: %.3f%%\n	EndLoadLevel: %s\n	Started: %s")
			, *FString(collectedData.bLoginGameLevel ? TEXT("TRUE") : TEXT("FALSE"))
			, *FString(collectedData.bStartLoadLevel ? TEXT("TRUE") : TEXT("FALSE"))
			, collectedData.levelLoadProgressPercent * 100.f
			, *FString(collectedData.bEndLoadLevel ? TEXT("TRUE") : TEXT("FALSE"))
			, *FString(collectedData.bMatchStarted ? TEXT("TRUE") : TEXT("FALSE"))
	);
	}
}

void ULocalDataFlowSubsystem::OnLoadCompleteCallback()
{
	if (loadProgressTimer.IsValid())
	{
		GetGameInstance()->GetTimerManager().ClearTimer(loadProgressTimer);
		collectedData.levelLoadProgressPercent = loadHandle->GetLoadProgress();
		collectedData.bEndLoadLevel = true;
		if (IsValid(localCitRushPS))
		{
			localCitRushPS->GetDataManagerComponent()->ServerRPC_EndLoadingLevel(true);
		}
		if (loadingWidget)
		{
			loadingWidget->RemoveFromParent();
			loadingWidget = nullptr;
		}
		loadHandle.Reset();
	}
}
*/
