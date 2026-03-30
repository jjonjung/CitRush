// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UIPawn.generated.h"

class UCameraComponent;

/**
 * UI 표시 전용 Pawn. 카메라 컴포넌트만 가진 빈 Pawn (MainMenu, Lobby)
 */
UCLASS()
class UE_CITRUSH_API AUIPawn : public APawn
{
	GENERATED_BODY()

public:
	/** 생성자 */
	AUIPawn();

protected:
	/** 게임 시작 시 호출 */
	virtual void BeginPlay() override;

public:
	/** 입력 바인딩 설정 (빈 구현) */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	/** 카메라 컴포넌트 */
	UPROPERTY()
	TObjectPtr<UCameraComponent> cameraComponent;

	/** 루트 컴포넌트 */
	UPROPERTY()
	TObjectPtr<USceneComponent> root;
};
