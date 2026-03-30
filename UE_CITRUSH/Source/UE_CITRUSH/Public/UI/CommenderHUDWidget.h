// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommanderMessageType.h"
#include "CommenderHUDWidget.generated.h"

class UTextBlock;
class UCrosshairWidget;
class UCoinWidget;
class UVerticalBox;
class UOverlay;
class UCanvasPanel;
class UCommanderMessageWidget;
class UCommanderWorldMapWidget;
class ACommenderCharacter;

/**
 * Commander 전용 메인 HUD 위젯
 * Crosshair, Coin, MessageStack을 포함하고 관리
 */
UCLASS()
class UE_CITRUSH_API UCommenderHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 코인 업데이트
	UFUNCTION(BlueprintCallable, Category="HUD")
	void UpdateCoin(int32 NewCoin);

	// 메시지 표시 (직접 텍스트 전달)
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowMessage(const FText& InMessage, ECommanderMessageType Type, float Duration = 2.0f);

	// 메시지 표시 (메시지 ID 사용 - 일괄 관리)
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowMessageByID(ECommanderMessageID MessageID);

	// 동적 메시지 표시 (메시지 ID + 포맷 인자)
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowMessageByIDWithFormat(ECommanderMessageID MessageID, const TArray<FString>& FormatArgs);

	// 코인 부족 메시지 표시 (편의 함수 - 내부적으로 ShowMessageByID 사용)
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowInsufficientCoinMessage();

	// 메시지 맵에 메시지 추가/수정 (블루프린트에서 사용 가능)
	UFUNCTION(BlueprintCallable, Category="HUD|Messages")
	void AddOrUpdateMessage(ECommanderMessageID MessageID, const FText& MessageText, ECommanderMessageType MessageType = ECommanderMessageType::Info, float Duration = 2.0f);

	// 메시지 맵에서 메시지 제거 (블루프린트에서 사용 가능)
	UFUNCTION(BlueprintCallable, Category="HUD|Messages")
	void RemoveMessage(ECommanderMessageID MessageID);

	// 메시지 맵에 메시지가 존재하는지 확인 (블루프린트에서 사용 가능)
	UFUNCTION(BlueprintCallable, Category="HUD|Messages")
	bool HasMessage(ECommanderMessageID MessageID) const;

	// 크로스헤어 색상 설정
	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetCrosshairColor(const FLinearColor& Color);

	// 크로스헤어 색상을 기본 색상으로 복원
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ResetCrosshairToDefault();

	// 맵 UI 열기
	UFUNCTION(BlueprintCallable, Category="HUD|Map")
	void OpenMapUI();

	// 맵 UI 닫기
	UFUNCTION(BlueprintCallable, Category="HUD|Map")
	void CloseMapUI();

	// 맵 UI 토글 (열려있으면 닫고, 닫혀있으면 엶) - F 키에 바인딩용
	UFUNCTION(BlueprintCallable, Category="HUD|Map")
	void ToggleMapUI();

	// 맵 UI가 열려있는지 확인
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="HUD|Map")
	bool IsMapUIOpen() const;

	// 크로스헤어 위젯 (RootOverlay의 직접 자식) - 외부 접근용
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="HUD")
	TObjectPtr<UCrosshairWidget> CrosshairWidget;

	void UpdateTimer();

protected:
	// 루트 오버레이 (모든 위젯의 컨테이너)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UOverlay> RootOverlay;

	// 코인 위젯 (RootOverlay의 직접 자식)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCoinWidget> CoinWidget;

	// 메시지 스택 (RootOverlay의 직접 자식)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> MessageStack;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UTextBlock* TimerText;
	FTimerHandle GameTimer;

	// 메시지 위젯 클래스 (동적 생성용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD")
	TSubclassOf<UCommanderMessageWidget> MessageWidgetClass;

	// 캐릭터 참조
	UPROPERTY()
	TObjectPtr<ACommenderCharacter> OwnerCharacter;

	// 이전 코인 값 (0이 되었는지 확인용)
	UPROPERTY()
	int32 PreviousCoinValue = -1;

	// 메시지 맵 (메시지 ID별 메시지 정보 저장)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Messages")
	TMap<ECommanderMessageID, FCommanderMessageInfo> MessageMap;

	// 맵 UI 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HUD|Map")
	TSubclassOf<UCommanderWorldMapWidget> MapWidgetClass;

	// 맵 UI 위젯 인스턴스
	UPROPERTY()
	TObjectPtr<UCommanderWorldMapWidget> MapWidgetInstance;

	// 메시지 맵 초기화 (기본 메시지들 설정)
	void InitializeMessageMap();

private:
	// 메시지 맵이 초기화되었는지 확인하고 필요시 초기화
	void EnsureMessageMapInitialized();

	// 캐릭터 찾기 및 초기화 (중복 제거)
	void FindAndInitializeCharacter();
};

