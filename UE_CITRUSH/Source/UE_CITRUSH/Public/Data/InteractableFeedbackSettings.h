#pragma once

#include "CoreMinimal.h"
#include "InteractableFeedbackSettings.generated.h"

class UPrimitiveComponent;
class UUserWidget;
class USoundBase;
class UNiagaraSystem;
class UParticleSystem;

/**
 * 상호작용 이펙트 타입 비트플래그 (Outline, Widget, Sound, Niagara, Particle, Network)
 */
UENUM(BlueprintType, meta=(BitFlags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EEffectType : uint8
{
    None = 0 << 0 UMETA(Hidden),
    Outline = 1 << 0, 
    Widget = 1 << 1, 
    Sound = 1 << 2, 
    Niagara = 1 << 3, 
    Particle = 1 << 4,
    Network = 1 << 7
};

/*
 * https://dev.epicgames.com/documentation/ko-kr/unreal-engine/unreal-engine-uproperties
 * https://dev.epicgames.com/documentation/ko-kr/unreal-engine/edit-conditions-for-properties-in-the-details-panel-in-unreal-engine
 * https://forums.unrealengine.com/t/bitmask-enum-value-in-editcondition-freezes-the-editor/1311664
*/
/**
 * 상호작용 피드백 설정 (Outline, Widget, Sound, VFX 등 비트마스크 기반)
 */
USTRUCT(BlueprintType)
struct UE_CITRUSH_API FInteractableFeedbackSettings
{
    GENERATED_BODY()

protected:
    /** 비트마스크에서 특정 이펙트 타입이 활성화되어 있는지 확인 */
    static bool IsEnableEffect(const uint8& bitmask, const EEffectType& bitflag)
    {return (bitmask & static_cast<uint8>(bitflag)) > 0;}

public:
    /** 생성자. 기본 피드백 설정 초기화 */
    FInteractableFeedbackSettings();

    /** 활성화할 이펙트 타입 비트마스크 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/UE_CITRUSH.EEffectType"))
    uint8 effectType = 0;
    
    /** Outline을 적용할 Mesh Component (약참조) */
    UPROPERTY(/*VisibleAnywhere, BlueprintReadOnly, Category = "Outline", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Outline", EditConditionHides)*/)
    TWeakObjectPtr<UMeshComponent> ownerMeshComponent;

    /** CustomDepth Stencil 값 (사용 안 함 - 커스텀 머티리얼 방식으로 대체) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Outline", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Outline", EditConditionHides))
    int32 outlineStencilValue = 252;

    /** Outline 색상 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Outline", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Outline", EditConditionHides))
    FLinearColor outlineColor = FLinearColor::Green;

    /** Overlay 머티리얼 사용 여부 (true: Overlay, false: Replace) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Outline", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Outline", EditConditionHides))
    bool bUseOverlayMaterial = true;

    /** 커스텀 Outline 머티리얼 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Outline", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Outline", EditConditionHides))
    TObjectPtr<UMaterialInterface> customOutlineMaterial;

    /** 상호작용 가이드 Widget 클래스 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Widget", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Widget", EditConditionHides))
    TSubclassOf<UUserWidget> interactionGuideWidgetClass;

    /** Widget을 부착할 소켓 이름 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Widget", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Widget", EditConditionHides))
    FName widgetSocketName = NAME_None;

    /** Widget 회전 오프셋 (카메라 방향 기준 추가 회전) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Widget", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Widget", EditConditionHides))
    FRotator widgetRotationOffset = FRotator::ZeroRotator;

    /** 상호작용 가이드 텍스트 (위젯에 표시할 텍스트) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Widget", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Widget", EditConditionHides))
    FText interactionText = FText::FromString(TEXT("Press E to Interact"));

    /** 상호작용 시 재생할 사운드 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sound", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Sound", EditConditionHides))
    TObjectPtr<USoundBase> interactedSound;

    /** 상호작용 시 재생할 Niagara VFX */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Niagara", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Niagara", EditConditionHides))
    TObjectPtr<UNiagaraSystem> interactedNiagaraVFX;

    /** 상호작용 시 재생할 Particle VFX */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Particle", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Particle", EditConditionHides))
    TObjectPtr<UParticleSystem> interactedParticleVFX;

    /** 네트워크 동기화 여부 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Network", meta=(EditCondition="effectType & \"/Script/UE_CITRUSH.EEffectType::Network", EditConditionHides))
    bool bInteractionEffectsInNetwork = false;

    /** Outline 이펙트 활성화 여부 */
    bool IsOutlineOn() const {return IsEnableEffect(effectType, EEffectType::Outline);}

    /** Widget 이펙트 활성화 여부 */
    bool IsWidgetOn() const {return IsEnableEffect(effectType , EEffectType::Widget);}

    /** Sound 이펙트 활성화 여부 */
    bool IsSoundOn() const {return IsEnableEffect(effectType, EEffectType::Sound);}

    /** Niagara 이펙트 활성화 여부 */
    bool IsNiagaraOn() const {return IsEnableEffect(effectType, EEffectType::Niagara);}

    /** Particle 이펙트 활성화 여부 */
    bool IsParticleOn() const {return IsEnableEffect(effectType, EEffectType::Particle);}

    /** Network 동기화 활성화 여부 */
    bool IsNetworkOn() const {return IsEnableEffect(effectType, EEffectType::Network);}

    /** Outline 이펙트 설정 */
    void EnableOutline(const bool& bEnabled, UMeshComponent* inOutlineComponent = nullptr, FLinearColor inOutlineColor = FLinearColor::Green);

    /** Widget 이펙트 설정 */
    void EnableWidget(const bool& bEnabled, const TSubclassOf<UUserWidget>& inInteractionGuideWidgetClass, FName inWidgetSocketName = NAME_None);

    /** Sound 이펙트 설정 */
    void EnableSound(const bool& bEnabled, USoundBase* inInteractedSound);

    /** Niagara 이펙트 설정 */
    void EnableNiagara(const bool& bEnabled, UNiagaraSystem* inInteractedNiagaraVFX);

    /** Particle 이펙트 설정 */
    void EnableParticle(const bool& bEnabled, UParticleSystem* inInteractedParticleVFX);

    /** Network 동기화 설정 */
    void EnableNetwork(const bool& bEnabled);    
};
