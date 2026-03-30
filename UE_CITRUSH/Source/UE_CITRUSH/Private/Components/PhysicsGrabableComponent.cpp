// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/PhysicsGrabableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"

UPhysicsGrabableComponent::UPhysicsGrabableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPhysicsGrabableComponent::BeginPlay()
{
	Super::BeginPlay();

	// GrabableComponent가 설정되지 않았으면 자동으로 찾기
	if (!GrabableComponent)
	{
		AActor* Owner = GetOwner();
		if (IsValid(Owner))
		{
			// 먼저 StaticMeshComponent 찾기
			if (UStaticMeshComponent* StaticMesh = Owner->FindComponentByClass<UStaticMeshComponent>())
			{
				GrabableComponent = StaticMesh;
			}
			// 없으면 SkeletalMeshComponent 찾기
			else if (USkeletalMeshComponent* SkeletalMesh = Owner->FindComponentByClass<USkeletalMeshComponent>())
			{
				GrabableComponent = SkeletalMesh;
			}
			// 없으면 AutoFindComponentType으로 찾기
			else if (AutoFindComponentType)
			{
				TArray<UActorComponent*> Components;
				Owner->GetComponents(AutoFindComponentType, Components);
				if (Components.Num() > 0)
				{
					GrabableComponent = Cast<UPrimitiveComponent>(Components[0]);
				}
			}
		}
	}
}

bool UPhysicsGrabableComponent::CanBeGrabbed() const
{
	if (!bCanBeGrabbed) {return false;}
	if (!IsValid(GetOwner())) {return false;}
	
	UPrimitiveComponent* Comp = GetGrabableComponent();
	if (!IsValid(Comp)) {return false;}
	
	// 물리 시뮬레이션이 활성화되어 있어야 함
	return Comp->IsSimulatingPhysics();
}

UPrimitiveComponent* UPhysicsGrabableComponent::GetGrabableComponent() const
{
	return GrabableComponent;
}

