// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MainMenuGameState.generated.h"

class UMainMenuWidget;
/**
 * 메인메뉴용 GameState. MenuWidget 생성 및 관리
 */
UCLASS()
class UE_CITRUSH_API AMainMenuGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/** 생성자 */
	AMainMenuGameState();

protected:
	/** 게임 시작 시 호출. MenuWidget 생성 */
	virtual void BeginPlay() override;

protected:
	/** 메인 메뉴 Widget 클래스 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMainMenuWidget> menuWidgetClass;
};
