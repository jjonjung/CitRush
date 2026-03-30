// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommanderMessageType.h"
#include "CommanderMessageWidget.generated.h"

class UTextBlock;
class UImage;
class UWidgetAnimation;

/**
 * Commander 메시지 위젯 - 개별 메시지를 표현하는 위젯
 * 동적으로 생성되어 MessageStack에 추가됨
 */
UCLASS()
class UE_CITRUSH_API UCommanderMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 메시지 설정
	UFUNCTION(BlueprintCallable, Category="Message")
	void Setup(const FText& InMessage, ECommanderMessageType InType, float InDuration = 2.0f);

	// 애니메이션 재생 (표시 후 자동으로 숨김)
	UFUNCTION(BlueprintCallable, Category="Message")
	void PlayShowHideAnimation();

protected:
	// 메시지 텍스트 (BindWidget으로 자동 연결)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Message;

	// 선택적 아이콘 (BindWidget으로 자동 연결)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Icon_Optional;

	// 표시/숨김 애니메이션 (UMG Anim)
	UPROPERTY(BlueprintReadOnly, Transient, meta=(BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> Anim_ShowHide;

	// 메시지 타입
	UPROPERTY()
	ECommanderMessageType MessageType = ECommanderMessageType::Info;

	// 메시지 지속 시간
	UPROPERTY()
	float MessageDuration = 2.0f;

	// 메시지 애니메이션 종료 콜백
	void OnMessageAnimationFinished();

	// 타입에 따른 색상 및 아이콘 설정
	void ApplyTypeSettings();
};

