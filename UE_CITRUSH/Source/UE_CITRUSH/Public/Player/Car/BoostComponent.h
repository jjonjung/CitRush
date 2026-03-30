// Chaos 차량 물리 기반 부스터 시각 효과 컴포넌트
// FOV, 모션 블러, 포스트 프로세스 머티리얼을 Timeline으로 부드럽게 전환
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
 #include "Materials/MaterialInterface.h"
 #include "Materials/MaterialInstanceDynamic.h"
 #include "Components/TimelineComponent.h"
 #include "Engine/EngineTypes.h"
 #include "Curves/CurveFloat.h"
#include "BoostComponent.generated.h"

class UChaosVehicleMovementComponent;

UENUM(BlueprintType)
enum class EBoostState : uint8
{
	Ready UMETA(DisplayName = "Ready"),
	Active UMETA(DisplayName = "Active"),
	Cooldown UMETA(DisplayName = "Cooldown"),
	Depleted UMETA(DisplayName = "Depleted")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UBoostComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(Boost, Log, All);

public:
	UBoostComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ========== 부스터 제어 ==========
	UFUNCTION(BlueprintCallable, Category = "Boost")
	void ActivateBoost();

	UFUNCTION(BlueprintCallable, Category = "Boost")
	void DeactivateBoost();

	UFUNCTION(BlueprintCallable, Category = "Boost")
	void RefillBoost(float Amount);

	// ========== 네트워크 RPC ==========
	/** 서버에서 부스터 활성화 (클라이언트 → 서버) */
	UFUNCTION(Server, Reliable)
	void ServerRPC_ActivateBoost();

	/** 서버에서 부스터 비활성화 (클라이언트 → 서버) */
	UFUNCTION(Server, Reliable)
	void ServerRPC_DeactivateBoost();

	// ========== Getters ==========
	UFUNCTION(BlueprintPure, Category = "Boost")
	EBoostState GetBoostState() const { return BoostState; }

	UFUNCTION(BlueprintPure, Category = "Boost")
	float GetBoostFuel() const { return BoostFuel; }

	UFUNCTION(BlueprintPure, Category = "Boost")
	float GetBoostFuelPercent() const { return BoostFuel / MaxBoostFuel; }

protected:
	void ApplyBoostForce();
	void UpdateBoostState(float DeltaTime);

	UPROPERTY()
	UChaosVehicleMovementComponent* VehicleMovement;

	// ========== 튜닝 파라미터 ==========
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost|Tuning")
	float MaxBoostFuel = 100.f; // NFS 스타일 니트로 게이지

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost|Tuning")
	float BoostForce = 4000.f; // 전진 방향 힘 (Newtons)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost|Tuning")
	float BoostConsumptionRate = 33.f; // 초당 소모량 (3초 지속)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost|Tuning")
	float BoostCooldown = 2.f; // 재사용 대기 시간

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost|Tuning")
	float MinSpeedForBoost = 10.f; // 최소 속도 (km/h)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost|Tuning")
	bool bAllowInAir = false; // 공중에서 사용 가능 여부

	// ========== 런타임 상태 ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Boost|State")
	EBoostState BoostState = EBoostState::Ready;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Boost|State")
	float BoostFuel = 100.f;

	float CooldownTimer = 0.f;

	/** 부스터 활성화 상태 (네트워크 복제) */
	UPROPERTY(Replicated)
	bool bIsBoostActive = false;

	// ========== FOV 설정 ==========
	/** 일반 상태의 카메라 FOV (기본값: 90도) */
	UPROPERTY(EditAnywhere, Category = "Boost|FOV", meta = (ClampMin = "60.0", ClampMax = "210.0"))
	float NormalFOV = 90.0f;

	/** 부스터 활성화 시 카메라 FOV (기본값: 120도 - 극적인 효과) */
	UPROPERTY(EditAnywhere, Category = "Boost|FOV", meta = (ClampMin = "60.0", ClampMax = "210.0"))
	float BoostFOV = 120.0f;

	// ========== Motion Blur 설정 ==========
    /** 부스터 활성화 시 모션 블러 강도 (기본값: 1.0 - 극적인 효과) */
    UPROPERTY(EditAnywhere, Category = "Boost|MotionBlur", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BoostMotionBlurAmount = 1.0f;

    /** 원본 모션 블러 값 (복원용, Transient) */
    UPROPERTY(Transient)
    float OriginalMotionBlurAmount = 0.0f;

	UPROPERTY(Transient)
	float OriginalDrag = 0.f;

    // ========== Post-Process Material 설정 ==========
    /** 부스터 효과용 포스트 프로세스 머티리얼 (에디터에서 할당) */
    UPROPERTY(EditAnywhere, Category = "Boost|PostProcess")
    TObjectPtr<UMaterialInterface> BoostPostProcessMaterial;

    /** 런타임에 생성된 머티리얼 인스턴스 다이나믹 (Private, Transient) */
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> PostProcessMID;

    /**
     * WeightedBlendables 배열에서 PostProcessMID의 인덱스 (캐시)
     * BeginPlay에서 한 번 찾아서 저장하면 매 프레임 배열 탐색을 피할 수 있음
     * INDEX_NONE이면 아직 캐시되지 않은 상태
     */
    int32 CachedWBIndex = INDEX_NONE;

    /**
     * PostProcess Material Weight의 최대값 (기본값: 1.0 - 극적인 라인 효과)
     * Timeline의 Value(0~1)가 1.0일 때 이 값으로 스케일링됨
     */
    UPROPERTY(EditAnywhere, Category = "Boost|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxPostProcessWeight = 1.0f;

    // ========== Timeline 설정 ==========
    /**
     * 부스터 효과 전환 커브 (에디터에서 할당)
     * 0.0 → 1.0 범위의 Float Curve를 사용하여 Ease-in/out 제어
     */
    UPROPERTY(EditAnywhere, Category = "Boost|Timeline")
    TObjectPtr<UCurveFloat> BoostCurve;

    /**
     * Timeline 보간 속도 (초 단위)
     * BoostCurve의 재생 길이를 제어 (기본값: 0.3초 - 빠르고 극적인 전환)
     */
    UPROPERTY(EditAnywhere, Category = "Boost|Timeline", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float TimelineLength = 0.3f;

    /** Timeline 컴포넌트 (멤버로 직접 선언) */
    FTimeline BoostTimeline;

    /** 현재 부스터 가중치 (0.0 ~ 1.0, Timeline에서 업데이트) */
    float CurrentBoostWeight = 0.0f;

    /** 부스터 활성화 상태 플래그 */
    bool bIsBoosting = false;

    // ========== 캐시된 컴포넌트 참조 ==========
    /** Owner Actor의 카메라 컴포넌트 참조 (Transient) */
    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> Camera;

    // ========== Timeline 콜백 함수 ==========
    /** Timeline 업데이트 시 호출 - FOV, 모션 블러, PostProcess Weight 조정 */
    UFUNCTION()
    void OnTimelineUpdate(float Value);

    /** Timeline 완료 시 호출 - 정리 작업 수행 */
    UFUNCTION()
    void OnTimelineFinished();

    /** WeightedBlendables 배열에서 PostProcessMID의 Weight 설정 (캐시된 인덱스 활용) */
    void UpdatePostProcessWeight(float Weight);

    /** 현재 활성화된 카메라를 찾아서 반환 (동적 검색) */
    UCameraComponent* GetActiveCamera();
};
