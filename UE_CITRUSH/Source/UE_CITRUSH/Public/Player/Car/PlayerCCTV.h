// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerCCTV.generated.h"

class UWidgetComponent;
class UUserWidget;

UCLASS()
class UE_CITRUSH_API APlayerCCTV : public AActor
{
	GENERATED_BODY()
	DECLARE_LOG_CATEGORY_CLASS(playerCCTV, Log, All);
	
public:
	APlayerCCTV();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// ========== Components ==========
	/** 3D World Widget Component */
	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* CCTVWidgetComponent;*/

	// ========== CCTV Settings ==========
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "CCTV Settings")
	int32 TargetIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV Settings")
	TSubclassOf<AActor> TargetClass;

	// 이 PlayerCCTV가 사용할 단일 RenderTarget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV Settings")
	UTextureRenderTarget2D* RenderTarget;

	// CCTV 화면을 표시할 BP_CCTV 액터 (Plane이 있는 액터)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "CCTV Settings")
	AActor* CCTVDisplayActor;

	// CCTV 화면 Plane의 Material Parameter 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV Settings")
	FName TextureParameterName = "CCTVTexture";

	// ========== CCTV Switching ==========
	/** 전체 CCTV RenderTarget 배열 (Player0, Player1, Player2, PixelEnemy) */
	UPROPERTY(BlueprintReadOnly, Category = "CCTV")
	TArray<UTextureRenderTarget2D*> CCTVRenderTargets;

	/** 현재 보고 있는 CCTV 인덱스 */
	UPROPERTY(BlueprintReadOnly, Category = "CCTV")
	int32 CurrentCCTVIndex = 0;

	/** 다음 CCTV로 전환 (E 키) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void SwitchToNextCCTV();

	/** 이전 CCTV로 전환 (Q 키) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void SwitchToPreviousCCTV();

	/** 현재 CCTV RenderTarget 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	UTextureRenderTarget2D* GetCurrentCCTVRenderTarget() const;

private:
	// 실제로 찾은 타겟 액터를 저장할 변수
	AActor* TargetActor;

	/** CCTV RenderTarget 업데이트 */
	void UpdateCCTVDisplay();
};
