// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "BaseAS.generated.h"

/**
 * Use Below Macros

 * in `.h`
	DECLARE_ATTRIBUTE(YourClassName, AttributeName)

 * in `.cpp`
	DEFINE_ATTRIBUTE(YourClassName, AttributeName)
 */
UCLASS()
class UE_CITRUSH_API UBaseAS : public UAttributeSet
{
	GENERATED_BODY()
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Attribute 선언 매크로. .h 파일에서 사용
 * - Replicated FGameplayAttributeData 프로퍼티 생성
 * - OnRep 함수 생성
 * - Getter/Setter/Init 함수 선언
 */
#define DECLARE_ATTRIBUTE(ClassName, PropertyName) \
public: \
	static FGameplayAttribute Get##PropertyName##Attribute() \
	{ \
		static FProperty* Prop = FindFieldChecked<FProperty>(ClassName::StaticClass(), GET_MEMBER_NAME_CHECKED(ClassName, PropertyName)); \
		return Prop; \
	} \
	float Get##PropertyName() const; \
	void Set##PropertyName(float NewVal); \
	void Init##PropertyName(float NewVal);

/**
 * Attribute 정의 매크로. .cpp 파일에서 사용
 * - OnRep 함수 구현
 * - Getter/Setter/Init 함수 구현
 */
#define DEFINE_ATTRIBUTE(ClassName, PropertyName) \
	void ClassName::OnRep_##PropertyName(const FGameplayAttributeData& old##PropertyName) const \
	{ \
		static FProperty* ThisProperty = FindFieldChecked<FProperty>(ClassName::StaticClass(), GET_MEMBER_NAME_CHECKED(ClassName, PropertyName)); \
		GetOwningAbilitySystemComponentChecked()->SetBaseAttributeValueFromReplication(FGameplayAttribute(ThisProperty), PropertyName, old##PropertyName); \
	} \
	float ClassName::Get##PropertyName() const \
	{ \
		return PropertyName.GetCurrentValue(); \
	} \
	void ClassName::Set##PropertyName(float NewVal) \
	{ \
		UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent(); \
		if (ensure(AbilityComp)) \
		{ \
		AbilityComp->SetNumericAttributeBase(Get##PropertyName##Attribute(), NewVal); \
		} \
	} \
	void ClassName::Init##PropertyName(float NewVal) \
	{ \
		PropertyName.SetBaseValue(NewVal); \
		PropertyName.SetCurrentValue(NewVal); \
	}