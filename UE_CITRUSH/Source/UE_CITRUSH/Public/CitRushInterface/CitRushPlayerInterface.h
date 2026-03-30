// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CitRushPlayerInterface.generated.h"

class UGameplayEffect;
class UAttributeSet;
class UGameplayAbility;

// This class does not need to be modified.
UINTERFACE()
class UCitRushPlayerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * CitRushPlayer 인터페이스. 순수 가상 함수로 InputMode, AbilitySystem, Voice 초기화 정의
 */
class UE_CITRUSH_API ICitRushPlayerInterface
{
	GENERATED_BODY()

public:
	/** Input Mode 초기화 */
	virtual void InitInputMode(APlayerController* playerController) = 0;
	
#pragma region AbilitySystem
public:
	/** AttributeSet 반환 */
	virtual UAttributeSet* GetAttributeSet() const = 0;
	/** 기본 Attribute 초기화 */
	virtual void InitDefaultAttributes() const = 0;

	/** Ability 부여 (TSubclassOf) */
	virtual bool ReceiveAbility(const TSubclassOf<UGameplayAbility>& ability) = 0;
	/** Ability 부여 (GameplayTag 이름) */
	virtual bool ReceiveAbility(const FName& gameplayTagName) = 0;
	/** Ability 활성화 (TSubclassOf) */
	virtual bool ActivateAbility(const TSubclassOf<UGameplayAbility>& ability) = 0;
	/** Ability 활성화 (GameplayTag 이름) */
	virtual bool ActivateAbility(const FName& gameplayTagName) = 0;

protected:
	/** AbilitySystem 초기화 */
	virtual void InitAbilitySystem() = 0;
#pragma endregion

#pragma region Voice
public:
	/** Voice Interface 초기화 */
	virtual void InitVoiceInterface() = 0;
#pragma endregion
};
