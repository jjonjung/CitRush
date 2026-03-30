#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class UInteractableComponent;
class AMainPlayer;

/**
 * 플레이어 상호작용 처리 Component (Commander 전용). 입력 처리 및 Server RPC 전송
 */
UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 생성자 */
	UInteractionComponent();

protected:
	/** 리플리케이션 프로퍼티 등록 */
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/** 컴포넌트 초기화. InputAction/InputMappingContext 로드 */
	virtual void InitializeComponent() override;

	/** 게임 시작 시 호출. Enhanced Input 바인딩 */
	virtual void BeginPlay() override;

public:
	/** 현재 Focus 중인 Actor 반환 */
	UFUNCTION(BlueprintCallable, Category="Interaction|Focus")
	FORCEINLINE AActor* GetFocusedActor() {return focusingActor;}

	/** 상호작용 키 입력 처리. Server_Interact ServerRPC 호출 */
	UFUNCTION(BlueprintCallable, Category="Interaction|Input")
	void InteractKeyInput(const FInputActionValue& value);

	/** ViewComponent HitResult로 Focus 처리. InteractableComponent 상태 변경 */
	UFUNCTION(BlueprintCallable, Category="Interaction|Focus")
	void FocusInteractableActor(const FHitResult& hitResult);

protected:
	/** Focus ServerRPC. 서버에서 FocusingActor 설정 */
	UFUNCTION(Server, Reliable)
	void Server_Focus(AActor* focusedActor);

	/** 상호작용 ServerRPC. 서버에서 TryInteract 호출 */
	UFUNCTION(Server, Reliable)
	void Server_Interact(UInteractableComponent* interactable);

	/** 상호작용 완료 ServerRPC */
	UFUNCTION(Server, Reliable)
	void Server_FinishInteraction();

protected:
	/** 현재 Focus 중인 Actor (Replicated) */
	UPROPERTY(Replicated)
	TObjectPtr<AActor> focusingActor;

	/** Owner PlayerController */
	UPROPERTY()
	APlayerController* ownerPlayerController = nullptr;

	/** 상호작용 InputMappingContext */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Input|Context")
	TObjectPtr<UInputMappingContext> IMC_Interaction;

	/** 상호작용 InputAction */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Input|Action")
	TObjectPtr<UInputAction> IA_Interaction;
};
