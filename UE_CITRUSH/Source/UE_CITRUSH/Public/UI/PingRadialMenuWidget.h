#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PingTypes.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "PingRadialMenuWidget.generated.h"

/**
 * 핑 타입 라디얼 메뉴
 * - 중심 기준 드래그 각도로 타입 선택
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API UPingRadialMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 위젯 트리 고정 확인 및 기본 사이즈 세팅 */
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	/** 표시/숨김 시 위치 설정용 */
	UFUNCTION(BlueprintCallable, Category = "PingRadial")
	void ShowAtScreenPosition(const FVector2D& InCenterScreenPosition);

	/** 숨기기 */
	UFUNCTION(BlueprintCallable, Category = "PingRadial")
	void HideMenu();

	/** 커서 위치로 현재 선택 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "PingRadial")
	void UpdateSelectionByScreenPosition(const FVector2D& InCursorScreenPosition);

	/** 라디얼 메뉴에 사용 중인 아이콘과 동일한 브러시를 타입 기준으로 반환 */
	UFUNCTION(BlueprintCallable, Category = "PingRadial")
	FSlateBrush GetIconBrushForType(ECommanderPingType InType) const;

	/** 현재 선택된 타입 반환 */
	UFUNCTION(BlueprintCallable, Category = "PingRadial")
	ECommanderPingType GetCurrentSelectedType() const { return CurrentSelectedType; }

	/** 타입 목록 설정 (BP에서 아이콘 배치 등 활용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PingRadial")
	TArray<ECommanderPingType> PingTypes = {
		ECommanderPingType::Objective,
		ECommanderPingType::Danger,
		ECommanderPingType::Collect,
		ECommanderPingType::Custom,
		ECommanderPingType::Extra1,
		ECommanderPingType::Extra2  // 기본 6개 구성 (BP에서 자유롭게 교체 가능)
	};

	/** 슬라이스 하이라이트용 인덱스 (BP에서 바인딩) */
	UPROPERTY(BlueprintReadOnly, Category = "PingRadial")
	int32 HighlightIndex = INDEX_NONE;

	/** 반경 설정 (픽셀, BP에서 조정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PingRadial")
	float RadialRadius = 120.f;

	/** 디버그: 중심-커서 선 그리기 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PingRadial|Debug")
	bool bShowDebugLine = false;

protected:
	/** 중심 스크린 좌표 */
	UPROPERTY(BlueprintReadOnly, Category = "PingRadial")
	FVector2D CenterScreenPos = FVector2D::ZeroVector;

	/** 마지막 커서 스크린 좌표 (디버그용 선 그리기) */
	UPROPERTY(Transient)
	FVector2D LastCursorScreenPos = FVector2D::ZeroVector;

	/** 현재 선택 타입 */
	UPROPERTY(BlueprintReadOnly, Category = "PingRadial")
	ECommanderPingType CurrentSelectedType = ECommanderPingType::Objective;

	/** HighlightIndex 변경 시 즉시 시각적 피드백 처리 (각도 계산 기반, hover 애니메이션 없음) */
	void HandleHighlightIndexChanged(int32 OldIndex, int32 NewIndex);

	/** 고정 구조: 슬라이스 버튼들 (WBP에서 이름/계층 변경 금지) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Slice_0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Slice_1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Slice_2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Slice_3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Slice_4;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Slice_5;

	/** 고정 구조: 각 슬라이스의 배경(파이 조각) 이미지들 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Back_1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Back_2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Back_3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Back_4;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Back_5;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Back_6;

	/** 고정 구조: 각 슬라이스의 아이콘 이미지들 (예시 이름, BP에서 동일하게 유지) */

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_4;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_5;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_6;

	/** 각도 기반 인덱스 계산 */
	int32 GetIndexFromCursor(const FVector2D& Cursor) const;

	/** PingTypes 인덱스에 대응하는 아이콘 이미지 위젯 반환 */
	UImage* GetImageWidgetForPingIndex(int32 Index) const;

	/** 인덱스(0~5)에 해당하는 버튼을 반환 (순서대로: 0=12시, 1=2시, 2=4시, 3=6시, 4=8시, 5=10시) */
	UButton* GetButtonForIndex(int32 Index) const;
};
