// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonitorActor.generated.h"

class UCCTVFeedComponent;
class UCCTVControlWidget;
class UInteractableComponent;

/**
 * CCTV 모니터 Actor
 * 월드에 배치되어 InteractableComponent와 CCTVFeedComponent를 포함합니다.
 * Commander가 범위 내에 들어와서 F키를 누르면 CCTV UI가 열립니다.
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API AMonitorActor : public AActor
{
	GENERATED_BODY()
	
public:
	AMonitorActor(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

public:
	/** CCTV 열기/닫기 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void ToggleCCTV(APlayerController* InteractingPC = nullptr);

	/** CCTV 열기 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void OpenCCTV(APlayerController* InteractingPC = nullptr);

	/** CCTV 닫기 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void CloseCCTV(APlayerController* InteractingPC = nullptr);

	/** CCTV 열려있는지 확인 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	bool IsCCTVOpen() const { return bCCTVOpen; }

	/** CCTV Feed Component 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	UCCTVFeedComponent* GetCCTVFeedComponent() const { return CCTVFeedComponent; }
	
	/** Screen CCTV Widget 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	UCCTVControlWidget* GetScreenCCTVWidget() const { return ScreenCCTVWidget; }

	/** 카메라 슬롯 전환 (Server RPC) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "CCTV")
	void ServerSwitchCameraSlot(bool bNext);

protected:
	/** Static Mesh Component (모니터 모델) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MonitorMesh;

	/** 상호작용 가능한 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractableComponent> InteractableComponent;

	/** CCTV Feed Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCCTVFeedComponent> CCTVFeedComponent;

	/** CCTV Widget 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV")
	TSubclassOf<UUserWidget> CCTVWidgetClass;

	/** Screen Space CCTV Widget (Viewport에 표시) */
	UPROPERTY()
	TObjectPtr<UCCTVControlWidget> ScreenCCTVWidget;

	/** 상호작용 반경 (InteractableComponent의 interactionRadius와 동일하게 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "50.0", ClampMax = "1000.0", ToolTip = "⚠️ 이 값을 설정한 후, Blueprint에서 InteractableComponent의 interactionRadius도 동일하게 설정해야 합니다"))
	float InteractionRadius = 300.0f;

private:
	/** CCTV 열림 상태 */
	bool bCCTVOpen = false;
};

