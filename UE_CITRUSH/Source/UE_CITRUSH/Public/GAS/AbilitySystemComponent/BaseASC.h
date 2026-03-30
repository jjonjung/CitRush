// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "BaseASC.generated.h"

/**
 * 1. Include Library `#include "AbilitySystemInterface.h"`
 * 2. Inherit `, public IAbilitySystemInterface`
 * 3. X ( Forward Declaration `class UAbilitySystemComponent;` )
 	=> "AbilitySystemInterface.h"에 전방선언 되어있고, Public Dependency에 등록해서 이용 가능
 * 4. Add Below Scripts 
``` script
#pragma region IAbilitySystemInterface Interface
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> abilitySystemComponent;
#pragma endregion
```

 */
UCLASS()
class UE_CITRUSH_API UBaseASC : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** 생성자 */
	UBaseASC();

protected:
	/** 컴포넌트 시작 시 호출 */
	virtual void BeginPlay() override;
};
