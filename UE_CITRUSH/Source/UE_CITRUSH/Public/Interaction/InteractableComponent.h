#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/InteractableFeedbackSettings.h"
#include "InteractableComponent.generated.h"

class USphereComponent;
class UWidgetComponent;

/**
 * 상호작용 가능한 Actor 상태 (OutOfBound, InRange, Focused, UnFocused)
 */
UENUM(BlueprintType, meta=(BitFlags))
enum class EInteractableState : uint8
{
	Default UMETA(Hidden),
	OutOfBound = 1 << 1, // 범위에서 벗어났을때
	InRange = 1 << 2, // 범위에 들어왔을때
	Focused = 1 << 3, // 범위에 있으면서 바라보는 상태
	UnFocused = 1 << 4
};
ENUM_CLASS_FLAGS(EInteractableState)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClientInteraction, APlayerController*, playerController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiInteraction, APawn*, player);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChanged, APlayerController*, playerController, EInteractableState, newState);

/**
 * 상호작용 가능한 Actor Component. 범위/Focus 감지, Feedback(Outline/Widget/Sound/VFX) 표시
 */
UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 생성자 */
	UInteractableComponent();

protected:
	/** 리플리케이션 프로퍼티 등록 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 컴포넌트 초기화. InteractionSphere 생성 및 Overlap 델리게이트 바인딩 */
	virtual void InitializeComponent() override;

	/** 게임 시작 시 호출. WidgetComponent 생성 */
	virtual void BeginPlay() override;

	/** 게임 종료 시 호출. 타이머 정리 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	/** 에디터에서 프로퍼티 변경 시 호출. InteractionSphere 반경 갱신 */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR

public:
	// TickComponent 제거됨 - UpdateVisuals에서 이벤트 기반으로 처리

	/** 현재 상태 반환 */
	UFUNCTION(BlueprintPure, Category="Interactable|Each Client")
	FORCEINLINE EInteractableState GetState() const {return clientState;}

	/** 상호작용 가능 여부 (Focused 상태) */
	UFUNCTION(BlueprintPure, Category="Interactable|Each Client")
	FORCEINLINE bool IsInteractable() const {return clientState == EInteractableState::Focused;}

	/** 멀티플레이 가능 여부 (NetworkOn) */
	UFUNCTION(BlueprintPure, Category="Interactable|All Client")
	FORCEINLINE bool IsMultiPlayable() const {return feedbackSettings.IsNetworkOn();}

	/** 상태 변경 시도. 상태에 따라 Feedback 갱신 */
	UFUNCTION(BlueprintCallable, Category="Interactable|Each Client")
	void TryChangeState(APlayerController* playerController, EInteractableState newState);

	/** 상호작용 시도 (Client). Server RPC 또는 로컬 처리 */
	UFUNCTION(BlueprintCallable, Category="Interactable|Each Client")
	void TryInteract(APlayerController* playerController);

	/** 상호작용 멀티캐스트. 모든 클라이언트에 상호작용 이벤트 전파 */
	UFUNCTION(NetMulticast, Reliable, Category="Interactable|Multicast")
	void Multicast_TryInteract(APawn* player);


protected:
	/** 상호작용 완료 처리. OnClientInteraction 델리게이트 브로드캐스트 */
	UFUNCTION(BlueprintCallable, Category="Interactable|Each Client")
	void FinishInteracting(APlayerController* Player, bool bSuccess);

	/** 시각적 Feedback 갱신. Outline/Widget 표시 */
	UFUNCTION(BlueprintCallable, Category="Interactable|Visual")
	void UpdateVisuals(APlayerController* playerController);

	/** 효과 재생. Sound/VFX 재생 */
	UFUNCTION(BlueprintCallable, Category="Interactable|Effect")
	void PlayEffects(bool bSuccess);

	#pragma region Overlap Event
	/** InteractionSphere Overlap 시작. InRange 상태로 변경 */
	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	/** InteractionSphere Overlap 종료. OutOfBound 상태로 변경 */
	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	#pragma endregion Overlap Event

	#pragma region Effects
	/** PrimitiveComponent 목록 갱신. Outline용 MeshComponent 찾기 */
	bool UpdateAvailablePrimitiveComponents();

	/** 사운드 재생 */
	void PlaySound(USoundBase* sound);

	/** Cascade 파티클 이펙트 재생 */
	void PlayEffect(UParticleSystem* effect);

	/** Niagara 이펙트 재생 */
	void PlayEffect(UNiagaraSystem* effect);
	#pragma endregion Effects

	#pragma region Custom Outline
	/** 커스텀 Outline 갱신. PostProcess Material 적용 */
	void UpdateCustomOutline(bool bShowOutline);
	#pragma endregion Custom Outline

	/** 서버 여부 확인 */
	UFUNCTION(BlueprintPure)
	bool IsServer() const {AActor* owner = GetOwner();
		return IsValid(owner) && owner->HasAuthority();}
	
public:
	/** 클라이언트별 상호작용 이벤트 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category="Interactable|Event")
	FOnClientInteraction OnClientInteraction;

	/** 멀티캐스트 상호작용 이벤트 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category="Interactable|Event")
	FOnMultiInteraction OnMultiInteraction;

	/** 상태 변경 이벤트 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category="Interactable|Event")
	FOnStateChanged OnStateChanged;

	/** Feedback 설정 (Outline/Widget/Sound/VFX) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InteractableFeedback")
	FInteractableFeedbackSettings feedbackSettings;

protected:
	/** 상호작용 안내 Widget Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DynamicComponents|UI")
	TObjectPtr<UWidgetComponent> interactionGuideComponent;

	/** 상호작용 범위 Sphere Component */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DynamicComponents|Range")
	TObjectPtr<USphereComponent> interactionSphere;

	/** 상호작용 반경 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DynamicComponents|Range")
	float interactionRadius = 300.f;

	/** 현재 클라이언트 상태 (OutOfBound/InRange/Focused/UnFocused) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "InteractableFeedback")
	EInteractableState clientState = EInteractableState::Default;

private:
	/** 범위 내에 있는 플레이어 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Client", meta=(AllowPrivateAccess=true))
	APawn* playerInRange = nullptr;

	/** 커스텀 Outline용 원본 머티리얼 백업 */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> originalMaterials;

};
