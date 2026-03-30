#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PingTypes.h"
#include "RacerPingHUDWidget.generated.h"

class ACitRushGameState;
class APawn;
class UTextBlock;
class UImage;
class UCanvasPanel;

/**
 * Racer용 핑 방위계/미니맵 HUD 위젯
 * GameState의 OnPingUpdated 이벤트를 구독하여 핑 표시
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API URacerPingHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URacerPingHUDWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 핑 업데이트 이벤트 핸들러 */
	UFUNCTION()
	void OnPingUpdated(const FPingData& NewPing);

	/** 방위계 업데이트 */
	void UpdateCompass();

	/** 미니맵 업데이트 (선택적) */
	void UpdateMinimap();

public:
	/** 방위계 반 FOV (예: 90도면 좌우 90도씩 표시) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
	float CompassHalfFOV = 90.f;

	/** 방위계 너비 (픽셀) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
	float CompassWidth = 400.f;

	/** 미니맵 반경 (월드 단위) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float MiniMapRadiusWorld = 2000.f;

protected:
	/** 현재 활성 핑 데이터 */
	UPROPERTY(BlueprintReadOnly, Category = "Ping")
	FPingData CurrentPing;

	/** GameState 참조 */
	UPROPERTY()
	ACitRushGameState* GameState;

	/** 플레이어 Pawn 참조 */
	UPROPERTY()
	APawn* PlayerPawn;

	/** 방위계 아이콘 위젯 (Blueprint에서 바인딩) */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "UI")
	TObjectPtr<UImage> CompassIcon;

	/** 거리 텍스트 (Blueprint에서 바인딩) */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "UI")
	TObjectPtr<UTextBlock> DistanceText;

	/** 방위계 패널 (Blueprint에서 바인딩) */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "UI")
	TObjectPtr<UCanvasPanel> CompassPanel;

	/** 미니맵 패널 (선택적, Blueprint에서 바인딩) */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "UI")
	TObjectPtr<UCanvasPanel> MinimapPanel;

	/** 업데이트 주기 (초, 0이면 매 프레임) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float UpdateInterval = 0.05f;

	/** 마지막 업데이트 시간 */
	float LastUpdateTime = 0.f;
};
