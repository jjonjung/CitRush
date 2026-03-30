// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CrosshairWidget.generated.h"

class UImage;
class ACommenderCharacter;

/**
 * 크로스헤어 위젯 - 아이템 조준 시 색상 변경
 */
UCLASS()
class UE_CITRUSH_API UCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 크로스헤어 색상 직접 설정
	UFUNCTION(BlueprintCallable, Category="Crosshair")
	void SetCrosshairColor(const FLinearColor& Color);

	// 크로스헤어 색상을 기본값으로 복원
	UFUNCTION(BlueprintCallable, Category="Crosshair")
	void ResetCrosshairToDefault();

	// 아이템 사용 모드 설정 (초록색 + 안내 메시지)
	UFUNCTION(BlueprintCallable, Category="Crosshair")
	void SetUseMode(bool bUseMode);

	// 크로스헤어 이미지 컴포넌트 가져오기
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crosshair")
	UImage* GetCrosshairImage() const { return Image_Crosshair; }

	// 크로스헤어 색상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crosshair")
	FLinearColor DefaultColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crosshair")
	FLinearColor AimingItemColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crosshair")
	FLinearColor AimingUsingMachineColor = FLinearColor(0.0f, 1.0f, 0.4f, 1.0f); // 초록색 (#00FF66)

protected:
	// 크로스헤어 이미지 (BindWidget으로 자동 연결)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Image_Crosshair;

	// 캐릭터 캐싱
	UPROPERTY()
	TObjectPtr<ACommenderCharacter> OwnerCharacter;

	// 커스텀 색상 사용 여부 (SetCrosshairColor로 설정된 경우 true)
	UPROPERTY()
	bool bUseCustomColor = false;

	// 현재 설정된 커스텀 색상
	UPROPERTY()
	FLinearColor CustomColor = FLinearColor::White;
};

