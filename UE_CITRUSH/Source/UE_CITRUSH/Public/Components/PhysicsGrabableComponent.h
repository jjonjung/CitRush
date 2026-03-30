// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsGrabableComponent.generated.h"

/**
 * 이 컴포넌트를 붙인 액터는 PhysicsHandle로 grab/throw 할 수 있습니다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UPhysicsGrabableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPhysicsGrabableComponent();

protected:
	virtual void BeginPlay() override;

public:
	/** 이 액터를 grab할 수 있는지 확인 */
	UFUNCTION(BlueprintPure, Category="Grabable")
	bool CanBeGrabbed() const;

	/** PhysicsHandle로 grab하기 위해 사용할 PrimitiveComponent 반환 */
	UFUNCTION(BlueprintPure, Category="Grabable")
	UPrimitiveComponent* GetGrabableComponent() const;

	// 이벤트는 추후 필요시 추가 가능

protected:
	/** Grab 가능 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grabable")
	bool bCanBeGrabbed = true;

	/** Grab에 사용할 PrimitiveComponent (null이면 자동으로 찾음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grabable")
	TObjectPtr<UPrimitiveComponent> GrabableComponent = nullptr;

	/** 자동으로 찾을 Component 타입 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grabable")
	TSubclassOf<UPrimitiveComponent> AutoFindComponentType = UStaticMeshComponent::StaticClass();
};

