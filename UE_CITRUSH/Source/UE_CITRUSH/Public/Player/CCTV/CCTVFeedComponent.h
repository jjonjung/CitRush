// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CCTVFeedComponent.generated.h"

// CCTV 로그 카테고리 선언 (다른 파일에서도 사용 가능)
DECLARE_LOG_CATEGORY_EXTERN(LogCCTVFeed, Log, All);

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UCameraComponent;
class APawn;
class APixelEnemy;

/**
 * CCTV Feed 정보 구조체
 */
USTRUCT(BlueprintType)
struct FCCTVFeedSlot
{
	GENERATED_BODY()

	/** SceneCapture 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	/** RenderTarget */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	/** 타겟 Pawn (레이서 또는 팩맨) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CCTV")
	TObjectPtr<APawn> TargetPawn;

	/** 현재 사용 중인 카메라 슬롯 인덱스 (0~2) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CCTV")
	int32 CurrentCameraSlot = 0;

	FCCTVFeedSlot()
		: SceneCapture(nullptr)
		, RenderTarget(nullptr)
		, TargetPawn(nullptr)
		, CurrentCameraSlot(0)
	{}
};

/**
 * CCTV Feed 컴포넌트
 * 4개의 SceneCapture를 관리하고 포커스/확대 상태를 관리합니다.
 */
UCLASS(ClassGroup=(CitRush), meta=(BlueprintSpawnableComponent))
class UE_CITRUSH_API UCCTVFeedComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCTVFeedComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

public:
	/** Feed 초기화 (레이서 3명 + 팩맨) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void InitializeFeeds();

	/** Match Start 이벤트 핸들러 - GameState에서 레이서 불러오기 */
	UFUNCTION()
	void OnMatchStarted();

	/** 레이서 연결 재시도 (연결되지 않은 레이서가 있을 때 호출) */
	void RetryConnectRacers();

	/** 포커스 이동 (Q/E) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void MoveFocus(bool bNext);
	
	/** 특정 Feed 인덱스로 포커스 설정 */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void SetFocusIndex(int32 NewFocusIndex);

	/** 포커스된 Feed의 카메라 슬롯 전환 (Q/E) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void SwitchCameraSlot(bool bNext = true);

	/** 확대/복귀 토글 (T) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void ToggleExpand();

	/** CCTV 상태를 초기 상태로 리셋 (4분할 화면, 포커스 0) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void ResetToDefaultState();

	/** 현재 포커스된 Feed 인덱스 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	int32 GetFocusIndex() const { return FocusIndex; }

	/** 확대 상태인지 확인 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	bool IsExpanded() const { return bExpanded; }

	/** Feed 슬롯 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	FCCTVFeedSlot GetFeedSlot(int32 Index) const;

	/** 모든 Feed 슬롯 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	TArray<FCCTVFeedSlot> GetAllFeedSlots() const { return FeedSlots; }

	/** RenderTarget 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	UTextureRenderTarget2D* GetRenderTarget(int32 FeedIndex) const;

	/** RenderTarget Width 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	int32 GetRenderTargetWidth() const { return RenderTargetWidth; }

	/** RenderTarget Height 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CCTV")
	int32 GetRenderTargetHeight() const { return RenderTargetHeight; }

	/** CCTV UI 활성화 상태 설정 (UI 열릴 때만 SceneCapture 활성화) */
	UFUNCTION(BlueprintCallable, Category = "CCTV")
	void SetCCTVUIActive(bool bActive);

	/** FocusIndex 변경 델리게이트 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFocusIndexChanged, int32, OldIndex, int32, NewIndex);

	/** 확대 상태 변경 델리게이트 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExpandedChanged, bool, bExpanded);

	/** 카메라 슬롯 변경 델리게이트 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCameraSlotChanged, int32, FeedIndex, int32, CameraSlot);

	/** FocusIndex 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "CCTV|Events")
	FOnFocusIndexChanged OnFocusIndexChanged;

	/** 확대 상태 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "CCTV|Events")
	FOnExpandedChanged OnExpandedChanged;

	/** 카메라 슬롯 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "CCTV|Events")
	FOnCameraSlotChanged OnCameraSlotChanged;

protected:
	/** Feed 슬롯 업데이트 (카메라 Attach) */
	void UpdateFeedSlot(int32 Index);

	/** SceneCapture를 Pawn의 CCTV 카메라에 Attach */
	void AttachSceneCaptureToCamera(USceneCaptureComponent2D* SceneCapture, APawn* TargetPawn, int32 CameraSlot);

	/** Pawn에서 CCTV 카메라 가져오기 */
	UCameraComponent* GetCCTVCamera(APawn* Pawn, int32 SlotIndex) const;

	/** 성능 최적화: 포커스된 Feed만 실시간 업데이트 */
	void UpdateCaptureSettings();

	/** SceneCapture 공통 초기화 (ShowFlags, CaptureSource 등 설정) */
	void InitializeSceneCapture(USceneCaptureComponent2D* SceneCapture);

	/** SceneCapture 기본 설정 (ShowFlags, CaptureSource만 설정, PostProcessSettings는 제외) */
	void SetupSceneCaptureBasicSettings(USceneCaptureComponent2D* SceneCapture);

	/** RenderTarget 공통 생성 (포맷 고정: RTF_RGBA8, 알파 1) */
	UTextureRenderTarget2D* CreateCCTVRenderTarget(int32 Width, int32 Height);

	/** 특정 슬롯 강제 캡처 (Transform 동기화 + 즉시 캡처) */
	void ForceCaptureSlot(int32 SlotIndex);

protected:
	/** RenderTarget 해상도 (Width) - VRAM 절약을 위해 기본값 축소 (1024 권장) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Render", meta = (ClampMin = "256", ClampMax = "4096"))
	int32 RenderTargetWidth = 1024;

	/** RenderTarget 해상도 (Height) - VRAM 절약을 위해 기본값 축소 (576 권장, 16:9 비율) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Render", meta = (ClampMin = "256", ClampMax = "4096"))
	int32 RenderTargetHeight = 576;

	/** PixelEnemy BP 클래스 필터 (null이면 모든 APixelEnemy 검색) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Enemy")
	TSubclassOf<APixelEnemy> PixelEnemyBPClass;

	/** 성능 최적화 모드 활성화 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Performance")
	bool bUsePerformanceOptimization = false;

	/** 저주기 업데이트 간격 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CCTV|Performance", meta = (EditCondition = "bUsePerformanceOptimization"))
	float LowFrequencyUpdateInterval = 0.1f;

private:
	/** Feed 슬롯 배열 (최대 4개: 레이서 3명 + 팩맨) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CCTV", meta = (AllowPrivateAccess = "true"))
	TArray<FCCTVFeedSlot> FeedSlots;

	/** 현재 포커스된 Feed 인덱스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CCTV", meta = (AllowPrivateAccess = "true"))
	int32 FocusIndex = 0;

	/** 확대 상태 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CCTV", meta = (AllowPrivateAccess = "true"))
	bool bExpanded = false;

	/** 저주기 업데이트 타이머 핸들 */
	FTimerHandle LowFrequencyUpdateTimerHandle;

	/** 카메라 attach 재시도 타이머 (레이서 카메라가 준비될 때까지) */
	TMap<int32, FTimerHandle> RetryAttachTimerHandles;
	
	/** 카메라 attach 재시도 횟수 (무한 재시도 방지) */
	TMap<int32, int32> RetryAttachCounts;

	/** CCTV UI 활성화 상태 (UI가 열려있을 때만 true) */
	bool bCCTVUIActive = false;

	/** Feed 초기화 완료 여부 (중복 초기화 방지) */
	bool bFeedsInitialized = false;

	/** 레이서 연결 재시도 타이머 핸들 */
	FTimerHandle RetryConnectRacersTimerHandle;

	/** 레이서 연결 재시도 횟수 (최대 5회) */
	int32 RetryConnectRacersCount = 0;
};

