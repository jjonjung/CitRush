// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/GridPanel.h"
#include "GameFramework/GameStateBase.h"
#include "Data/CitRushPlayerTypes.h"
#include "Player/CitRushPlayerState.h"
#include "CCTVControlWidget.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UUserWidget;
class UCCTVFeedComponent;
class UWorld;

/**
 * CCTV 제어 위젯
 * 4분할 화면을 표시하고 포커스/확대 상태를 관리합니다.
 * 모든 로직은 C++에서 처리되며, 블루프린트는 위젯 구조만 생성합니다.
 */
UCLASS()
class UE_CITRUSH_API UCCTVControlWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** CCTV Feed Component 설정 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void SetCCTVFeedComponent(UCCTVFeedComponent* InFeedComponent);

	/** CCTV Feed Component 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	UCCTVFeedComponent* GetCCTVFeedComponent() const { return CCTVFeedComponent; }

	/** 모든 Feed의 RenderTarget 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void UpdateAllFeeds();

	/** 레이서 HP/연료 정보 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void UpdateRacerStats();

	/** 포커스 표시 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void UpdateFocusIndicator();

	/** 확대/복귀 상태 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void UpdateExpandState();

	/** 확대 상태일 때 포커스된 Feed만 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void UpdateExpandedFeed();

protected:
	/** 위젯 초기화 */
	virtual void NativeConstruct() override;
	
	/** 위젯 제거 시 호출 */
	virtual void NativeDestruct() override;
	
	/** 마우스 클릭 처리 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	/** 마우스 이동 처리 (호버 감지) */
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	/** 마우스 호버 나감 */
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
	/** 키 입력 처리 (Tab 키를 게임 입력으로 전달) */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
	/** 클릭 위치에서 Feed 인덱스 찾기 */
	int32 GetFeedIndexFromClickPosition(const FGeometry& InGeometry, const FVector2D& ScreenPosition) const;
	
	/** 마우스 위치에서 Feed 인덱스 찾기 (호버 감지용) */
	int32 GetFeedIndexFromMousePosition(const FGeometry& InGeometry, const FVector2D& ScreenPosition) const;
	
	/** 특정 Feed로 포커스 이동 및 확대 */
	void FocusAndExpandFeed(int32 FeedIndex);
	
	/** FocusIndex 변경 이벤트 핸들러 */
	UFUNCTION()
	void OnFocusIndexChanged(int32 OldIndex, int32 NewIndex);

	/** 확대 상태 변경 이벤트 핸들러 */
	UFUNCTION()
	void OnExpandedChanged(bool bExpanded);

	/** 카메라 슬롯 변경 이벤트 핸들러 */
	UFUNCTION()
	void OnCameraSlotChanged(int32 FeedIndex, int32 CameraSlot);
	
	/** 호버 애니메이션 업데이트 */
	UFUNCTION()
	void UpdateHoverAnimation();
	
	/** 클릭된 Feed의 아웃라인 업데이트 */
	void UpdateClickedOutline();
	

private:
	/** Brush에 RenderTarget을 설정 (ImageSize는 위젯 크기에 맞춰 자동 조정) */
	void SetBrushWithRenderTarget(UImage* ImageWidget, UTextureRenderTarget2D* RenderTarget);
	
	/** Brush에 Material을 설정 (ImageSize는 위젯 크기에 맞춰 자동 조정) */
	void SetBrushWithMaterial(UImage* ImageWidget, UMaterialInstanceDynamic* Material);
	
	/** Brush의 Image 속성 설정 (CCTV는 반드시 Image 타입 사용) */
	void SetBrushImageProperties(FSlateBrush& Brush) const;
	
	/** 노이즈 Material Instance 가져오기 또는 생성 */
	UMaterialInstanceDynamic* GetOrCreateNoiseMaterialInstance(int32 Index);
	
	/** RenderTarget이 없을 때 FeedImage에 노이즈 또는 배경 표시 */
	void SetFeedImageNoiseOrBackground(UImage* FeedImage, int32 FeedIndex);
	
	/** FeedIndex에 해당하는 플레이어 표시 이름 가져오기 */
	FString GetPlayerDisplayName(int32 FeedIndex) const;

	/** StatsText 위치 업데이트 (확대/축소 상태에 따라) */
	void UpdateStatsTextPosition(bool bExpanded);

protected:
	/** CCTV Feed Component 참조 */
	UPROPERTY(BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UCCTVFeedComponent> CCTVFeedComponent;

	/** 클릭 아웃라인 색상 (기본값: 빨간색) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Visual")
	FLinearColor OutlineColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
	
	/** 클릭 아웃라인 두께 (픽셀) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Visual", meta = (ClampMin = "1", ClampMax = "10"))
	float OutlineThickness = 3.0f;
	
	/** 호버 시 최대 Scale 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Animation", meta = (ClampMin = "1.0", ClampMax = "1.5"))
	float HoverScaleMax = 1.05f;
	
	/** 호버 애니메이션 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Animation", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float HoverAnimationSpeed = 5.0f;

	// ========== BindWidget: 블루프린트에서 위젯 이름이 일치하면 자동 바인딩 ==========
	
	/** 4분할 화면 컨테이너 (Grid Panel 또는 Canvas Panel) */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UPanelWidget> CCTVContainer;

	/** 확대 화면 컨테이너 (Canvas Panel) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UPanelWidget> ExpandedContainer;

	/** 레이서 1 화면 Image */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UImage> FeedImage_0;

	/** 레이서 2 화면 Image */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UImage> FeedImage_1;

	/** 레이서 3 화면 Image */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UImage> FeedImage_2;

	/** 팩맨 화면 Image */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UImage> FeedImage_3;

	/** 확대 상태일 때 표시할 Image (ExpandedContainer 내부) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UImage> ExpandedFeedImage;
	
	/** 클릭 시 표시할 빨간색 아웃라인 Border (동적으로 재사용) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<class UBorder> OutlineBorder;


	/** 확대 상태 표시 Text */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UTextBlock> ExpandText;

	/** 레이서 1 없음 표시 Text */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UTextBlock> Racer1Text;

	/** 레이서 2 없음 표시 Text */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UTextBlock> Racer2Text;

	/** 레이서 3 없음 표시 Text */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UTextBlock> Racer3Text;

	/** 팩맨 없음 표시 Text */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UTextBlock> PixelEnemyText;

	/** 레이서 1 HP/연료 표시 Text (오른쪽 상단) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UTextBlock> Racer1StatsText;

	/** 레이서 2 HP/연료 표시 Text (오른쪽 상단) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UTextBlock> Racer2StatsText;

	/** 레이서 3 HP/연료 표시 Text (오른쪽 상단) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UTextBlock> Racer3StatsText;

	/** BrainCam 위젯 (Enemy 확대 시에만 표시) */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<class UUserWidget> BrainCamWidget;

	/** 노이즈 효과 Material (지지직거리는 효과) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Visual")
	TObjectPtr<UMaterialInterface> NoiseMaterial;

	/** 노이즈 효과 Material Instance (런타임 생성) */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> NoiseMaterialInstances;

private:
	/** 이전 포커스 인덱스 (변경 감지용) */
	int32 PreviousFocusIndex = -1;

	/** 이전 확대 상태 (변경 감지용) */
	bool bPreviousExpanded = false;
	
	/** 현재 호버 중인 Feed 인덱스 (-1이면 호버 없음) */
	int32 HoveredFeedIndex = -1;
	
	/** 이전 호버 인덱스 (애니메이션 방향 결정용) */
	int32 PreviousHoveredFeedIndex = -1;
	
	/** 현재 클릭된 Feed 인덱스 (-1이면 클릭 없음) */
	int32 ClickedFeedIndex = -1;
	
	/** 호버 애니메이션 타이머 핸들 */
	FTimerHandle HoverAnimationTimerHandle;

	/** 레이서 HP/연료 업데이트 타이머 핸들 */
	FTimerHandle RacerStatsUpdateTimerHandle;
	
	/** 호버 애니메이션 진행도 (0.0 ~ 1.0) */
	float HoverAnimationProgress = 0.0f;
	
	/** 호버 애니메이션 방향 (true: 확대, false: 축소) */
	bool bHoverExpanding = true;
	
	/** FeedImage 위젯 배열 (인덱스 접근용) */
	TArray<TObjectPtr<UImage>> FeedImages;
	
	/** 특정 Feed 인덱스의 FeedImage 가져오기 */
	UImage* GetFeedImageByIndex(int32 Index) const;
	
	/** FeedIndex를 ETargetRacer로 변환 */
	static ETargetRacer FeedIndexToTargetRacer(int32 FeedIndex);
	
	/** PlayerArray에서 targetIndex로 레이서 PlayerState 찾기 */
	static ACitRushPlayerState* FindRacerPlayerStateByTargetIndex(UWorld* World, ETargetRacer TargetRacer);
};
