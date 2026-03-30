// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Stats/CitRushLevelScript.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Network/AIDataManagerComponent.h"


ACitRushLevelScript::ACitRushLevelScript()
{
	PrimaryActorTick.bCanEverTick = false;
	httpComponent = CreateDefaultSubobject<UAIDataManagerComponent>(TEXT("Http"));

	
}

void ACitRushLevelScript::BeginPlay()
{
	Super::BeginPlay();
	ConsoleCommand("ShowDebug_AbilitySystem");
}

void ACitRushLevelScript::ConsoleCommand(const FString& cmd)
{
	if (!HasAuthority()) {return;}
	
	if (UWorld* world = GetWorld())
	{
		if (APlayerController* pc = world->GetFirstPlayerController())
		{
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), cmd, pc);
		}
	}
}
