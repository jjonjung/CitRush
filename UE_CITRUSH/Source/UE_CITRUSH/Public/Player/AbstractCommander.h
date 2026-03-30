// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CitRushInterface/CitRushPlayerInterface.h"
#include "AbstractCommander.generated.h"

class UVoiceCaptureComponent;
class UVoiceDonorComponent;
class USteamVoiceComponent;
class UVoiceWebSocketComponent;

class UGameplayAbility;
class UGameplayEffect;
class UAttributeSet;
class UASCommander;

class UViewComponent;
class UInteractionComponent;

/**
 * Commander 추상 클래스. Character + GAS + Voice + Interaction(View) 시스템 통합
 */
UCLASS(Abstract)
class UE_CITRUSH_API AAbstractCommander : public ACharacter, public IAbilitySystemInterface, public ICitRushPlayerInterface
{
	GENERATED_BODY()

protected:
	DECLARE_LOG_CATEGORY_CLASS(CommanderLog, Warning, All);

public:
	/** 생성자 */
	AAbstractCommander();

	/** 서버에서 Controller에 빙의될 때 호출. GAS 초기화 (Server) */
	virtual void PossessedBy(AController* NewController) override;

	/** 클라이언트에서 PlayerState 리플리케이션 시 호출. GAS 초기화 (Client) */
	virtual void OnRep_PlayerState() override;

	/** 입력 바인딩 설정 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 게임 시작 시 호출 */
	virtual void BeginPlay() override;

	/** 게임 종료 시 호출 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** InputMode 초기화 (GameAndUI) */
	virtual void InitInputMode(APlayerController* playerController) override;

protected:
	FInputModeGameOnly commanderInputMode;

#pragma region Interaction
public:
	/** 현재 Focus 중인 Actor 반환 */
	UFUNCTION(BlueprintCallable)
	AActor* GetFocusedActor();

protected:
	/** Interaction 시스템 초기화 (View + Interaction Component) */
	virtual void InitInteractionSystem();

protected:
	/** 카메라 LineTrace 컴포넌트 (Focus 감지) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UViewComponent> viewComponent;

	/** 상호작용 처리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInteractionComponent> interactionComponent;
#pragma endregion

#pragma region Voice
public:
	/** Voice 인터페이스 초기화 (Steam P2P + WebSocket STT) */
	virtual void InitVoiceInterface() override;

	/** Steam Voice Component 반환 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE USteamVoiceComponent* GetSteamVoiceComponent() const {return steamVoiceComponent;}

	/** Raw Voice Donor Component 반환 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE UVoiceDonorComponent* GetVoiceDonorComponent() const {return rawVoiceDonorComponent;}

protected:
	/** WebSocket STT 음성 전송 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice")
	TObjectPtr<UVoiceWebSocketComponent> voiceWebSocketComponent;

	/** Steam P2P 음성 전송 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice")
	TObjectPtr<USteamVoiceComponent> steamVoiceComponent = nullptr;

	/** Raw 음성 데이터 송신 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice")
	TObjectPtr<UVoiceDonorComponent> rawVoiceDonorComponent = nullptr;

	/** Raw 음성 캡처 컴포넌트 (마이크) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Voice")
	TObjectPtr<UVoiceCaptureComponent> rawVoiceCaptureComponent = nullptr;
#pragma endregion
	
	
#pragma region AbilitySystem
public:
	/** AbilitySystemComponent 반환 */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** AttributeSet 반환 */
	virtual UAttributeSet* GetAttributeSet() const override;

	/** 기본 Attribute 값 적용 (GameplayEffect) */
	virtual void InitDefaultAttributes() const override;

	/** Ability 활성화 (클래스 기반) */
	virtual bool ActivateAbility(const TSubclassOf<UGameplayAbility>& ability) override;

	/** Ability 활성화 (Tag 이름 기반) */
	virtual bool ActivateAbility(const FName& gameplayTagName) override;

	/** Ability 부여 (클래스 기반) */
	virtual bool ReceiveAbility(const TSubclassOf<UGameplayAbility>& ability) override;

	/** Ability 부여 (Tag 이름 기반) */
	virtual bool ReceiveAbility(const FName& gameplayTagName) override;

protected:
	/** GAS 시스템 초기화 */
	virtual void InitAbilitySystem() override;

protected:
	/** AbilitySystemComponent */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> abilitySystemComponent;

	/** Commander AttributeSet */
	UPROPERTY()
	TObjectPtr<UASCommander> attributeSet;

	/** 기본 Attribute 초기화 Effect */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Effect")
	TSubclassOf<UGameplayEffect> defaultAttributeEffect;

#pragma endregion

#pragma region CCTV
public:
	/** CCTV 카메라 슬롯 전환 - Tab 키 (확대 상태에서 레이서의 다른 시점 카메라 전환) */
	void HandleCCTVSwitchCameraTab();

public:
	/** MonitorActor 찾기 */
	void FindMonitorActor();

private:
	/** 로컬 컨트롤러인지 확인 */
	bool IsLocalController() const;

	/** 현재 활성화된 MonitorActor */
	UPROPERTY()
	TObjectPtr<class AMonitorActor> ActiveMonitorActor;
#pragma endregion
};