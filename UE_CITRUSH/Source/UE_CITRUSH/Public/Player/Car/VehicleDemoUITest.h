
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "VehicleDemoUITest.generated.h"

class UChaosWheeledVehicleMovementComponent;
class AWheeledVehiclePawn;
class UProgressBar;
class AAbstractRacer;
struct FOnAttributeChangeData;
class AVehicleDemoCejCar;

UCLASS()
class UE_CITRUSH_API UVehicleDemoUITest : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	/** Controls the display of speed in Km/h or MPH */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Vehicle)
	bool bIsMPH = false;

	/** 속도계 바늘 최소 각도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speedometer")
	float MinAngle = -135.0f;

	/** 속도계 바늘 최대 각도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speedometer")
	float MaxAngle = 90.0f;

	/** 최대 속도 (Km/h) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speedometer")
	float MaxSpeed = 200.0f;

	/*
	/** 연료계 바늘 최소 각도 #1#
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FuelGauge")
	float FuelMinAngle = -135.0f;

	/** 연료계 바늘 최대 각도 #1#
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FuelGauge")
	float FuelMaxAngle = 90.0f;*/

public:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Called to update the speed display */
	void UpdateSpeed(float NewSpeed);

	/** Called to update the gear display */
	void UpdateGear(int32 NewGear);

	/** 상태 텍스트 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UTextBlock* StateText;
	
	/** 경고 메시지 숨김 타이머 핸들 */
	FTimerHandle WarningHideTimerHandle;

	/** 연료 부족 경고 메시지 표시 (3초간) */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowFuelWarning();

	/** 부스트 소진 경고 메시지 표시 (2초간) */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowBoostEmptyWarning();

	// ========== 나침반 UI ==========
	/** 나침반 컨테이너 (CanvasPanel 등) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	class UWidget* CP_Compass;

	/** 나침반 이미지 (회전 또는 Material 적용 대상) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* IMG_Compass;

	/** 나침반 테두리 이미지 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* IMG_CompassBorder;

	/** 나침반 업데이트 함수 */
	void UpdateCompass();

	UPROPERTY()
	UMaterialInstanceDynamic* CompassMID;
	
	/** 경고 메시지 숨김 */
	void HideFuelWarning();

	/** 아이템 슬롯 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateItemSlots();
	
	/** 플레이어 이름 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdatePlayerName();

	/** 아이템 획득 알림 표시 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowItemNotification(const FString& ItemName);

	/** 상태 메시지 표시 (2초간) */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowStateMessage(const FString& Message);

	// ========== 속도계 UI 위젯 ==========
	/** 속도계 바늘 이미지 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* ImgPin;

	/** 속도 텍스트 레이블 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UTextBlock* SpeedLabel;

	/** HP 텍스트 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UTextBlock* HealthText;

	/** 플레이어 이름 텍스트 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UTextBlock* PlayerNameText;
	
	/*//========== 아이템 슬롯 UI ==========
	// 전방 아이템 아이콘 
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* FrontItemImage;

	//후방 아이템 아이콘 
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UImage* BackItemImage;*/

	// ========== 아이템 슬롯 UI ==========
	/** 아이템 슬롯 위젯 클래스 (동적 생성용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Slot")
	TSubclassOf<class UItemSlotWidget> ItemSlotWidgetClass;
	
	/** 아이템 슬롯 위젯 (Blueprint에서 직접 참조) */
	UPROPERTY(BlueprintReadWrite, Category = "Item Slot")
	class UItemSlotWidget* ItemSlotWidget;

	UPROPERTY(meta = (BindWidget))
	UPanelWidget* ItemSlotContainer;

	// ========== 타이머 UI ==========
	/** 타이머 텍스트 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UTextBlock* TimerText;

	// ========== 아이템 알림 UI ==========
	/** 아이템 획득 알림 텍스트 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UTextBlock* ItemNotificationText;

	// ========== 타이머 관련 함수 ==========
	/** 타이머 시작 */
	UFUNCTION(BlueprintCallable, Category = "Timer")
	void StartTimer();

	/** 타이머 업데이트 */
	UFUNCTION()
	void UpdateTimer();

	/** 타이머 종료 처리 */
	UFUNCTION()
	void OnTimerEnd();

protected:

	/** Implemented in Blueprint to display the new speed */
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnSpeedUpdate(float NewSpeed);

	/** Implemented in Blueprint to display the new gear */
	UFUNCTION(BlueprintImplementableEvent, Category = Vehicle)
	void OnGearUpdate(int32 NewGear);

	UPROPERTY(BlueprintReadWrite, Category = Vehicle)
	AWheeledVehiclePawn* CarInfo;

	UPROPERTY(BlueprintReadWrite, Category = Vehicle)
	UChaosWheeledVehicleMovementComponent* VehicleMovementComponent;

private:
	void InitializeVehicleReferences();
	int32 GetCurrentGear() const;

	/** 현재 선택된 플레이어 인덱스 */
	UPROPERTY()
	int32 CurrentSelectedPlayerIndex = 0;

	/** 현재 뷰 타겟 Racer */
	UPROPERTY()
	TWeakObjectPtr<AAbstractRacer> CurrentViewTargetRacer;

	// ========== 이벤트 기반 업데이트 ==========

	/** 현재 남은 부스트 횟수 */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle", meta = (AllowPrivateAccess = "true"))
	int32 CurrentBoostCount = 2;

	/** 속도/기어 업데이트 타이머 핸들 */
	FTimerHandle VehicleUpdateTimerHandle;

	/** 속도/기어 업데이트 함수 (타이머 콜백) */
	UFUNCTION()
	void UpdateVehicleInfo();

	/** 아이템 슬롯 아이콘 업데이트 */
	//void UpdateItemSlots();

	/** GAS Attribute 변경 델리게이트 핸들 */
	FDelegateHandle FuelAttributeChangedHandle;
	
	// 델리게이트 핸들 저장 (언바인드용)
	FDelegateHandle FuelChangeHandle;

	/** GAS 델리게이트 바인딩 */
	void BindAttributeDelegates();

	/** GAS 델리게이트 언바인딩 */
	void UnbindAttributeDelegates();

	/** 아이템 슬롯 델리게이트 바인딩 */
	void BindItemSlotDelegates();

	/** 아이템 슬롯 델리게이트 언바인딩 */
	void UnbindItemSlotDelegates();

	/** 아이템 슬롯 변경 콜백 */
	UFUNCTION()
	void OnItemSlotChanged(UItemData* FrontItem, UItemData* BackItem);

	/** 상태 메시지 숨김 타이머 핸들 */
	FTimerHandle StateHideTimerHandle;

	/** 상태 메시지 숨김 */
	void HideStateMessage();

protected:
	//타이머 핸들
	FTimerHandle TimerHandle;

	//남은 시간 (초) 
	UPROPERTY(BlueprintReadOnly, Category = "Timer")
	float RemainingTime = 0.0f;

	// 타이머 진행 여부
	UPROPERTY(BlueprintReadOnly, Category = "Timer")
	bool bIsTimerRunning = false;

	UPROPERTY()
	AVehicleDemoCejCar* Vehicle;
	
};
