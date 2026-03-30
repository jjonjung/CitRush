// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialWidget.generated.h"

// 전방 선언
class UTextBlock;

/**
 * 튜토리얼 멘트 구조체 (이름과 텍스트)
 */
USTRUCT(BlueprintType)
struct FTutorialMessage
{
	GENERATED_BODY()

	/** 멘트 이름/태그 (예: "Message Index 0", "Jump Tutorial" 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName MessageName;

	/** 멘트 텍스트 (한국어 및 줄바꿈 지원) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial", meta = (MultiLine = "true", MultiLineAllowEdit = "true"))
	FText MessageText;

	FTutorialMessage()
		: MessageName(NAME_None)
	{
	}

	FTutorialMessage(const FName& InName, const FText& InText)
		: MessageName(InName)
		, MessageText(InText)
	{
	}
};

/**
 * 튜토리얼 위젯
 * 여러 멘트를 배열로 저장하고 이름 또는 인덱스로 선택하여 표시
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API UTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 생성자 */
	UTutorialWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	/** 이름으로 멘트 표시 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ShowMessageByName(const FName& MessageName);

	/** 인덱스로 멘트 표시 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ShowMessage(int32 MessageIndex);

	/** 직접 텍스트 설정 (기존 호환성) */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetTutorialText(const FText& InText);

	/** 이름으로 인덱스 찾기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tutorial")
	int32 FindMessageIndexByName(const FName& MessageName) const;

	/** 현재 표시 중인 멘트 인덱스 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tutorial")
	int32 GetCurrentMessageIndex() const { return CurrentMessageIndex; }

	/** 현재 표시 중인 멘트 이름 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tutorial")
	FName GetCurrentMessageName() const;

	/** 멘트 배열 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tutorial")
	TArray<FTutorialMessage> GetMessages() const { return Messages; }

	/** 멘트 개수 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tutorial")
	int32 GetMessageCount() const { return Messages.Num(); }


	/** 위젯 크기에 비례하여 폰트 크기 설정 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetFontSizeByWidgetScale(float WidgetScale);

protected:
	/** 튜토리얼 멘트 배열 (이름과 텍스트) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	TArray<FTutorialMessage> Messages;

	/** 현재 표시 중인 멘트 인덱스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial")
	int32 CurrentMessageIndex = -1;

	/** 현재 표시 중인 텍스트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FText CurrentText;

	/** 배경 이미지 (Blueprint에서 할당) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Tutorial")
	TObjectPtr<class UImage> BackgroundImage;

	/** 튜토리얼 텍스트 블록 (Blueprint에서 할당) */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Tutorial")
	TObjectPtr<class UTextBlock> TutorialTextBlock;

	/** 현재 위젯 크기 배율 (폰트 크기 계산용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial")
	float CurrentWidgetScale = 1.0f;

	/** 기본 배경 이미지 크기 (WidgetScale = 1.0일 때) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FVector2D BaseBackgroundImageSize = FVector2D(400.0f, 450.0f);

	/** TextBlock과 배경 이미지 사이의 패딩 (좌우, 상하) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FVector2D TextPadding = FVector2D(20.0f, 20.0f);

	/** 배경 이미지 크기 업데이트 주기 (초, 0이면 타이머 사용 안 함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	float BackgroundUpdateInterval = 0.1f;

	/** 배경 이미지 크기 업데이트 타이머 핸들 */
	FTimerHandle BackgroundUpdateTimerHandle;

protected:
	/** TextBlock 스타일 설정 */
	void SetupTextBlockStyle();

	/** WidgetSize에 비례하여 TextBlock 크기 조절 */
	void UpdateTextBlockSizeForWidgetScale();

	/** 현재 표시 중인 TextBlock 크기에 맞춰 배경 이미지 크기 업데이트 */
	void UpdateBackgroundImageSizeToFitText();

	/** 배경 이미지 크기 업데이트 타이머 콜백 */
	void OnBackgroundUpdateTimer();

	/** 직접 텍스트 입력 여부 확인 */
	bool HasDirectTextInput() const;

	/** 내부 메시지 변경 처리 */
	void OnMessageChangedInternal(const FText& NewText, int32 MessageIndex, const FName& MessageName);

	/** 위젯 업데이트 (Blueprint에서 오버라이드 가능) */
	UFUNCTION(BlueprintNativeEvent, Category = "Tutorial")
	void OnMessageChanged(const FText& NewText, int32 MessageIndex, const FName& MessageName);
	virtual void OnMessageChanged_Implementation(const FText& NewText, int32 MessageIndex, const FName& MessageName);
};
