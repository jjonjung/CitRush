// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "CitRushLevelScript.generated.h"

class UAIDataManagerComponent;

/** CitRush LevelScript. AIDataManagerComponent 및 ConsoleCommand 실행 */
UCLASS()
class UE_CITRUSH_API ACitRushLevelScript : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	/** httpComponent 생성 */
	ACitRushLevelScript();

protected:
	/** "ShowDebug_AbilitySystem" 콘솔 명령 실행 */
	virtual void BeginPlay() override;

	/** 콘솔 명령 실행 (Server Only) */
	void ConsoleCommand(const FString& cmd);

protected:
	/** AI 데이터 관리 컴포넌트 (HTTP) */
	UPROPERTY(BlueprintReadOnly, Category="Network")
	TObjectPtr<UAIDataManagerComponent> httpComponent;
	
};
