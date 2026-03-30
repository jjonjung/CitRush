#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/Components/MinimapIconComponent.h"
#include "RealtimeMapIcon.generated.h"

class UImage;
class UMinimapIconComponent;

/**
 * 실시간 맵 아이콘 위젯
 * 전체지도에 표시되는 개별 Actor 아이콘
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API URealtimeMapIcon : public UUserWidget
{
	GENERATED_BODY()

public:
	URealtimeMapIcon(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	/** 아이콘 설정 (컴포넌트 기반) */
	UFUNCTION(BlueprintCallable, Category = "Realtime Map Icon")
	void SetupIcon(UMinimapIconComponent* InIconComponent);

	/** 아이콘 위치 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "Realtime Map Icon")
	void UpdatePosition(const FVector2D& InPosition);

	/** 아이콘 회전 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "Realtime Map Icon")
	void UpdateRotation(float InYaw);

	/** 아이콘 ID에 따라 아이콘 이미지 변경 */
	UFUNCTION(BlueprintCallable, Category = "Realtime Map Icon")
	void UpdateIconId(ERealtimeMapIconId InIconId);

	/** 팀 ID에 따라 색상 변경 */
	UFUNCTION(BlueprintCallable, Category = "Realtime Map Icon")
	void UpdateTeamColor(int32 InTeamId);

	/** 아이콘 이미지 컴포넌트 가져오기 */
	UFUNCTION(BlueprintPure, Category = "Realtime Map Icon")
	UImage* GetIconImage() const { return IconImage; }

protected:
	/** 아이콘 이미지 (BindWidget으로 자동 연결) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	/** 현재 설정된 아이콘 컴포넌트 */
	UPROPERTY()
	TObjectPtr<UMinimapIconComponent> IconComponent;

	/** 아이콘 ID별 텍스처 맵 (Blueprint에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Realtime Map Icon")
	TMap<ERealtimeMapIconId, TObjectPtr<UTexture2D>> IconIdTextures;

	/** 팀 ID별 색상 맵 (Blueprint에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Realtime Map Icon")
	TMap<int32, FLinearColor> TeamColors;

	/** 기본 아이콘 색상 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Realtime Map Icon")
	FLinearColor DefaultColor = FLinearColor::White;

	/** 아이콘 크기 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Realtime Map Icon")
	FVector2D IconSize = FVector2D(32.0f, 32.0f);
};

