// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "Enemy/AbstractEnemy.h"
#include "Enemy/EnemyTypes.h"
#include "Enemy/Interface/AIDecisionReceiver.h"
#include "Enemy/EnemyConfigTypes.h"
#include "Enemy/StateTree/EnemyAIPerceptionConfig.h"
#include "Runtime/AdvancedWidgets/Public/AdvancedWidgets.h"
#include "Subsystems/CaptureGaugeComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "PixelEnemy.generated.h"

class UGeometryCollection;
class UTimelineComponent;
class UBehaviorTree;
class UBlackboardData;
class UGameplayAbility;
class UGameplayEffect;
class USpringArmComponent;
class UCameraComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UAIDirectiveComponent;
class UEnemyAISubsystem;
class UNavSystemDataComponent;
class UStateTreeComponent;
class UStateTree;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UEnemyPelletComponent;
class UEnemyCombatComponent;
class ACloneEnemy;

/*
* Enemy 캐릭터 (GAS 통합 + AI Subsystem)
* - AI Subsystem을 통해 AI 서버와 통신
* - IAIDecisionReceiver 인터페이스 구현
* - AI 서버로부터 directive_code를 받아 FSM 기반으로 행동
*
* GameInstance
  ├── UEnemyAISubsystem (전역 싱글톤)
  │   ├── HTTP 연결 (1개)
  │   ├── RegisteredEnemies: TArray<IAIDecisionReceiver*>
  │   └── DecisionTimer (1개)
  │
  └── UReportAISubsystem (전역 싱글톤)
	  ├── HTTP 연결 (1개)
	  └── ReportCache

  APixelEnemy (implements IAIDecisionReceiver)
  ├── BeginPlay(): Subsystem에 자동 등록
  ├── OnAIDecisionReceived(): AI 명령 수신
  ├── GetEnemyID(): Enemy 고유 ID 반환
  └── GetCurrentGameState(): 현재 상태 반환
 */
UCLASS()
class UE_CITRUSH_API APixelEnemy : public AAbstractEnemy, public IAIDecisionReceiver
{
	GENERATED_BODY()

public:
	APixelEnemy();
	
	// ========== Components (Public Access) ==========

	/** AI Directive Component - FSM 관리 담당 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIDirectiveComponent> AIDirectiveComponent;

	/** Capture Gauge Component - 위협도 계산 담당 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UCaptureGaugeComponent> CaptureGaugeComponent;

	/** P-Pellet Component - 파워 펠릿 시스템 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UEnemyPelletComponent> PelletComponent;

	/** Combat Component - 전투 시스템 (데미지 무적, Flash 효과) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UEnemyCombatComponent> CombatComponent;

	/** Camera SpringArm */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	/** Camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	/** CCTV Render Target (외부에서 접근 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV")
	TObjectPtr<UTextureRenderTarget2D> CCTVRenderTarget;

	/** CCTV Exposure Bias (밝기 조정, 기본값: 1.0f, 0.2 단위로 튜닝 권장) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV", meta = (ClampMin = "0.0", ClampMax = "3.0", ToolTip = "CCTV 화면 밝기 조정. 너무 어두우면 증가, 너무 밝으면 감소"))
	float CCTVAutoExposureBias = 1.0f;

	/** Shield Mesh (P-Pellet Effect) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ShieldMesh;

	/** Minimap Icon Component - 맵에 표시용 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UMinimapIconComponent> MinimapIconComponent;

	/** NavSystem Data Component - AI LLM 네비게이션 데이터 제공 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UNavSystemDataComponent> NavSystemDataComponent;

	// ========== StateTree + AIPerception (Fallback AI) ==========

	/** StateTree Component - Local AI when server disconnected */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|StateTree")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;

	/** StateTree Asset (set in Blueprint) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|StateTree")
	TObjectPtr<UStateTree> FallbackStateTree;

	/** AI Perception Component - Sight, Hearing, Damage */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	/** Perception Configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	FEnemyPerceptionConfig PerceptionConfig;

	/** Is StateTree (local AI) active */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|StateTree")
	bool bIsLocalAIActive = false;

private:
	/** CCTV용 Scene Capture Component (내부 전용, RenderTarget 렌더링용) */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent;

	/** CCTV 카메라 컴포넌트 */
	UPROPERTY(VisibleAnywhere, Category = "CCTV")
	TObjectPtr<class UCCTVCameraComponent> CCTVCameraComponent;

	UPROPERTY()
	UCharacterMovementComponent* CharacterMovementComp;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void EquipBrain();

public:
	// ========== IAIDecisionReceiver Interface ==========

	virtual void OnAIDecisionReceived(const FUnitCommand& Command) override;
	virtual FString GetEnemyID() const override;
	virtual FEnemyGameState GetCurrentGameState() const override;
	virtual bool IsAIEnabled() const override { return bAIEnabled; }

	// ========== GAS Settings ==========

	/** 초기 스탯 설정 GameplayEffect */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> InitialStatsEffect;

	/** 초기 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Stats")
	float InitialHealth = 100.0f;

	/** 초기 공격력 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Stats")
	float InitialAttackPower = 10.0f;

	/** 이동 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Stats")
	float MoveSpeed = 3000.f;

	// ========== Config Data ==========
	/** 위협도 계산 설정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Config")
	FEnemyThreatConfig ThreatConfig;

	/** 전투 설정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Config")
	FEnemyCombatConfig CombatConfig;

	/** Aggressiveness 매핑 테이블 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Config")
	TObjectPtr<UDataTable> AggressivenessMappingTable;

	/** 위협도 설정 프리셋 이름 (DataTable Row Name) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	FName ThreatConfigPreset = TEXT("Default");

	/** 전투 설정 프리셋 이름 (DataTable Row Name) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	FName CombatConfigPreset = TEXT("Default");


	// ========== AI Subsystem ==========

	/** Enemy AI Subsystem 참조 (캐시) */
	UPROPERTY()
	UEnemyAISubsystem* EnemyAISubsystem;

	/** AI 활성화 여부 */
	UPROPERTY(EditAnywhere, Category = "AI")
	bool bAIEnabled = true;

	/** AI 서버 에러 델리게이트 핸들 */
	FDelegateHandle AIErrorDelegateHandle;

	/** CCTV Transform 동기화 타이머 핸들 */
	FTimerHandle CCTVSyncTimerHandle;

	/** AI 서버 에러 발생 시 호출 */
	UFUNCTION()
	void OnAIServerError(const FString& EnemyID, const FString& ErrorMessage);

	/** AI 서버 연결 상태 변경 시 호출 */
	void OnAIConnectionChanged(bool bConnected);

	/** 연결 변경 델리게이트 핸들 */
	FDelegateHandle ConnectionChangedHandle;

	/** 코인 획득 시 호출 */
	void OnCoinCollected(class ACoinActor* Coin);

protected:
	/** 획득한 코인 개수 */
	int32 CollectedCoinCount = 0;

public:
	/** Fallback 동작 실행 (서버 끊김 시) */
	void ExecuteFallbackBehavior();

	/** 가장 가까운 Coin 찾기 */
	class ACoinActor* FindNearestCoin() const;

	/** 가장 가까운 Racer 찾기 */
	class AAbstractRacer* FindNearestRacer() const;

	// ========== StateTree Control ==========

	/** Activate local AI (StateTree) */
	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void ActivateLocalAI();

	/** Deactivate local AI and use server commands */
	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void DeactivateLocalAI();

	/** Check if local AI is active */
	UFUNCTION(BlueprintPure, Category = "AI|StateTree")
	bool IsLocalAIActive() const { return bIsLocalAIActive; }

	/** Called when perception is updated */
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** Get all currently perceived actors */
	UFUNCTION(BlueprintCallable, Category = "AI|Perception")
	TArray<FPerceivedTargetInfo> GetPerceivedTargets() const;

	/** Get nearest perceived racer */
	UFUNCTION(BlueprintCallable, Category = "AI|Perception")
	AActor* GetNearestPerceivedRacer() const;

protected:
	/** Setup AIPerception senses */
	void SetupAIPerception();

	/** Cached perceived targets */
	UPROPERTY()
	TArray<FPerceivedTargetInfo> PerceivedTargets;

	// ========== Helper ==========
	/** 포획 게이지 계산 (v1.4.0) */
	float CalculateCaptureGauge() const;

	/** 설정 로드 (DataTable에서) */
	void LoadConfigFromDataTable();

	/** Aggressiveness 문자열을 계수로 변환 */
	float GetAggressivenessFactor(const FString& AggressivenessLevel) const;

	// ========== Damage ==========

protected:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_TriggerDestruction();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Destruction")
	TObjectPtr<UGeometryCollection> GeometryCollectionAsset;

	/** 데미지 이펙트 (Blueprint에서 설정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> DamagedEffect;

	/** 사망 이펙트 (Blueprint에서 설정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> DieEffect;

	// ========== Shield Visual Assets (Blueprint 설정) ==========

	/** Shield 메시 에셋 (Blueprint에서 설정, 기본: Engine Sphere) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|Shield")
	TObjectPtr<UStaticMesh> ShieldMeshAsset;

	/** Shield 머티리얼 에셋 (Blueprint에서 설정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX|Shield")
	TObjectPtr<UMaterialInterface> ShieldMaterialAsset;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void TryAttack();
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Damage")
	void NetMulticast_Damaged(FVector hitLocation);
	
private:
	UFUNCTION(Category = "Damage")
	void OnHitToRacer(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION(Category = "Damage")
	void OnOverlapToRacer(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	bool bPowerPellet = false;
	bool bUntouchable = false;

	// ========== P-Pellet 관련 변수 (v1.4.0) ==========

	/** P-Pellet 쿨타임 (초) */
	UPROPERTY(VisibleAnywhere, Category = "AI|P-Pellet")
	float PPelletCooldown = 0.0f;

	/** 마지막 P-Pellet 섭취 시각 (ISO 8601 형식) */
	UPROPERTY(VisibleAnywhere, Category = "AI|P-Pellet")
	FString LastPelletConsumedAt;

	/** P-Pellet 무적 타이머 핸들 */
	FTimerHandle PPelletInvulnerabilityTimer;

	/** P-Pellet 쿨타임 타이머 핸들 */
	FTimerHandle PPelletCooldownTimer;

	/** Damage 무적 타이머 핸들 */
	FTimerHandle DamageInvulnerabilityTimer;

public:
	/**
	 * P-Pellet 획득 처리
	 * @param Duration - 무적 지속 시간 (초)
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|P-Pellet")
	void OnPelletCollected(float Duration);

	/** P-Pellet 무적 종료 */
	UFUNCTION()
	void OnPPelletInvulnerabilityEnd();

	/** P-Pellet 쿨타임 감소 (1초마다) */
	UFUNCTION()
	void DecreasePPelletCooldown();

	// ========== Clone Management (v1.5.0) ==========

	/** 최대 분신 개수 */
	UPROPERTY(EditDefaultsOnly, Category = "Clone", meta = (ClampMin = "0", ClampMax = "4"))
	int32 MaxCloneCount = 4;

	/** 분신 액터 클래스 (Blueprint에서 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "Clone")
	TSubclassOf<ACloneEnemy> CloneClass;

	/** 활성 분신 목록 */
	UPROPERTY()
	TArray<TWeakObjectPtr<ACloneEnemy>> ActiveClones;

	/** 분신 스폰 (P-Pellet 획득 시) */
	void SpawnClones();

	/** 분신 제거 (P-Pellet 만료 시) */
	void DespawnClones();

	/** 분신 사망 콜백 */
	void OnCloneDied(ACloneEnemy* DeadClone);

	// ========== Component Event Callbacks ==========

	/** CombatComponent 무적 상태 변경 콜백 */
	UFUNCTION()
	void OnCombatInvulnerabilityChanged(bool bIsInvulnerable);

	/** PelletComponent 상태 변경 콜백 */
	UFUNCTION()
	void OnPelletStateChanged(bool bIsActive);

	// ========== CCTV ==========
	/** CCTV 전용 카메라 가져오기 (Slot: 0~2, 현재는 Camera만 사용) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	UCameraComponent* GetCCTVCamera(int32 SlotIndex);

	/** RenderTarget 강제 캡처 (CCTV 화면 업데이트용) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void CaptureCCTVScene();

	/** CCTV SceneCapture 활성화/비활성화 (UI 열림/닫힘에 따라 호출) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void SetCCTVCaptureEnabled(bool bEnabled);

	/** CCTV SceneCaptureComponent 가져오기 (CCTVFeedComponent에서 사용) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	USceneCaptureComponent2D* GetCCTVSceneCaptureComponent() const { return SceneCaptureComponent; }

	/** CCTV Transform 동기화 타이머 간격 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float CCTVSyncInterval = 0.033f; // 약 30fps (0.033초)

private:
	UFUNCTION()
	void FlashProgress(float output);
	UPROPERTY(EditAnywhere, Category = "Damage")
	TObjectPtr<UCurveFloat> flashCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TObjectPtr<UTimelineComponent> flashTimeLine;
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TObjectPtr<UMaterialInstanceDynamic> flashInDamagedMaterial;
};