#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapInteractionActor.generated.h"

class UInteractableComponent;
class UStaticMeshComponent;

/**
 * 맵 UI를 열 수 있는 상호작용 Actor
 * Commander가 이 Actor를 보고 F키를 누르면 맵 UI가 열림
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API AMapInteractionActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AMapInteractionActor();

protected:
	virtual void BeginPlay() override;

public:
	/** 시각적 표현을 위한 Static Mesh 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	/** 상호작용 가능한 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractableComponent> InteractableComponent;

	/** 상호작용 반경 (InteractableComponent의 interactionRadius와 동일하게 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "50.0", ClampMax = "1000.0", ToolTip = "⚠️ 이 값을 설정한 후, Blueprint에서 InteractableComponent의 interactionRadius도 동일하게 설정해야 합니다"))
	float InteractionRadius = 300.0f;
};
