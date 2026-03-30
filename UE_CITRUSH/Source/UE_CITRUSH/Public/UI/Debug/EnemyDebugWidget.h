// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Enemy/AiEnemy/AIDirectiveComponent.h"    // EAIDirectiveState, FOnAIStateChanged
#include "Enemy/Components/EnemyCombatComponent.h"  // FOnDamageReceived, FOnInvulnerabilityChanged
#include "EnemyDebugWidget.generated.h"

class APixelEnemy;
class UTextBlock;
class UProgressBar;
class UEnemyAISubsystem;

/**
 * Enemy AI/Combat 실시간 디버그 위젯
 *
 * [AI 패널]
 *   - AI 모드: AI SERVER (AIDirectiveComponent FSM) vs STATE TREE (서버 연결 끊김 시 로컬 폴백)
 *   - 현재 EAIDirectiveState (서버 모드) 또는 "(StateTree Active)" 표시
 *   - 서버 연결 상태 힌트
 *
 * [서버 접속 정보 패널] ← 추가
 *   - ServerURL + Port (UBaseAISubsystem::GetServerSettings)
 *   - 연결 상태: CONNECTED / DISCONNECTED
 *   - Auto Decision Request 활성화 여부
 *   - 등록된 Enemy 수
 *   - OnConnectionChanged 델리게이트로 연결 상태 변경 즉시 반영
 *
 * [전투 패널]
 *   - HP 바 (GAS AttributeSet, 체력 비율에 따라 초록→빨강 보간)
 *   - Speed / AttackPower / DetectionRange
 *   - 무적 상태: Normal / INVULNERABLE [Damage] / INVULNERABLE [P-Pellet]
 *
 * [이벤트 로그]
 *   - State 전환, 데미지 수신, 무적 상태 변경, 사망 실시간 기록
 *
 * [사용법]
 *   1. Blueprint에서 이 클래스를 부모로 위젯 생성 후 BindWidget 슬롯 배치
 *   2. BeginPlay 등에서 InitWithEnemy(PixelEnemy) 호출
 *   3. NativeDestruct 시 델리게이트 자동 해제
 */
UCLASS()
class UE_CITRUSH_API UEnemyDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 추적할 Enemy 설정 및 델리게이트 바인딩.
	 * 이전에 추적 중인 Enemy가 있으면 자동으로 해제 후 교체.
	 * @param Enemy 추적할 APixelEnemy (nullptr 전달 시 추적 해제)
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void InitWithEnemy(APixelEnemy* Enemy);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** PollInterval마다 AI/Combat 패널 갱신 */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// ========== Polling ==========

	void RefreshAIPanel();
	void RefreshServerPanel();
	void RefreshCombatPanel();

	// ========== Delegate Callbacks ==========

	/** AIDirectiveComponent::OnAIStateChanged → 로그 + AI 패널 즉시 갱신 */
	UFUNCTION()
	void OnAIStateChanged(EAIDirectiveState NewState, EAIDirectiveState OldState);

	/** EnemyCombatComponent::OnDamageReceived → 로그 + 전투 패널 즉시 갱신 */
	UFUNCTION()
	void OnDamageReceived(float DamageAmount, FVector HitLocation);

	/** EnemyCombatComponent::OnInvulnerabilityChanged → 로그 + 전투 패널 즉시 갱신 */
	UFUNCTION()
	void OnInvulnerabilityChanged(bool bIsInvulnerable);

	/** EnemyCombatComponent::OnEnemyDeath → 로그 기록 */
	UFUNCTION()
	void OnEnemyDeath();

	/**
	 * UBaseAISubsystem::OnConnectionChanged → 서버 연결 상태 변경 즉시 반영
	 * (non-dynamic multicast delegate, UFUNCTION 불필요)
	 */
	void OnAIConnectionChanged(bool bConnected);

	// ========== Helpers ==========

	/** 이벤트 로그 버퍼에 타임스탬프 포함 메시지 추가 (최신 항목이 맨 위) */
	void PushEventLog(const FString& Message);

	/** 버퍼 내용을 Text_EventLog에 반영 */
	void FlushEventLog();

	/** EAIDirectiveState → DisplayName 문자열 변환 */
	static FString GetDirectiveStateName(EAIDirectiveState State);

	// ========== State ==========

	/** 현재 추적 중인 Enemy */
	UPROPERTY()
	TObjectPtr<APixelEnemy> TrackedEnemy;

	/** OnConnectionChanged 델리게이트 핸들 (non-dynamic, RemoveAll 대신 핸들로 정확히 해제) */
	FDelegateHandle ConnectionChangedHandle;

	/** 이벤트 로그 최대 줄 수 */
	static constexpr int32 MaxLogEntries = 6;

	/** NativeTick 폴링 주기 (초). 이벤트성 갱신은 별도로 즉시 처리 */
	static constexpr float PollInterval = 0.1f;

	TArray<FString> EventLogBuffer;
	float           TickAccumulator = 0.f;

	// ========== Widget Bindings ==========

	// [AI 패널]

	/** AI 모드 레이블 - "AI SERVER" (초록) / "STATE TREE (Local)" (주황) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_AIMode;

	/** 현재 Directive State 이름 또는 "(StateTree Active)" */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CurrentState;

	/** 서버 연결 상태 힌트 - "Server: CONNECTED" / "Server: DISCONNECTED" (옵션) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ServerStatus;

	// [전투 패널]

	/** HP 프로그레스 바 (체력 비율에 따라 초록→빨강 색상 보간) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Health;

	/** HP 수치 텍스트 - "75 / 100" */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_HPValue;

	/** 무적 상태 - "Normal" / "INVULNERABLE [Damage]" / "INVULNERABLE [P-Pellet]" */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_InvulStatus;

	/** 이동 속도 (옵션) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Speed;

	/** 공격력 (옵션) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AttackPower;

	/** 감지 범위 (옵션) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DetectionRange;

	// [서버 접속 정보 패널] - UBaseAISubsystem / UEnemyAISubsystem에서 읽음

	/** 서버 URL + 포트 - "http://127.0.0.1:8000" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ServerURL;

	/** 서버 연결 상태 - "CONNECTED" (초록) / "DISCONNECTED" (빨강) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ConnectionStatus;

	/** Auto Decision Request 활성화 - "Auto Request: ON/OFF" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AutoRequest;

	/** 등록된 Enemy 수 - "Enemies: 3" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RegisteredEnemies;

	// [이벤트 로그]

	/** State 전환 / 데미지 / 무적 변경 / 연결 변경 / 사망 실시간 로그 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_EventLog;
};
