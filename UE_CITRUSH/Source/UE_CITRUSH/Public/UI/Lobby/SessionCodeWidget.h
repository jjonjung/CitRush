// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SessionCodeWidget.generated.h"

class UButton;
class UTextBlock;
/**
 * 세션 코드 Widget. 세션 코드 표시 및 클립보드 복사 기능
 */
UCLASS()
class UE_CITRUSH_API USessionCodeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Widget 생성 시 호출. 세션 코드 텍스트 설정 및 버튼 바인딩 */
	virtual void NativeConstruct() override;

private:
	/** 복사 버튼 클릭 처리. 세션 코드를 클립보드에 복사 */
	UFUNCTION()
	void OnClickCopyButton();

protected:
	/** 세션 코드 텍스트 블록 */
	UPROPERTY(VisibleDefaultsOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> sessionCodeText;

	/** 세션 코드 복사 버튼 */
	UPROPERTY(EditDefaultsOnly, meta=(BindWidget))
	TObjectPtr<UButton> copySessionCodeButton;
};
