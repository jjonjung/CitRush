#include "UI/PingRadialMenuWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Rendering/DrawElements.h"
#include "Engine/Texture2D.h"
#include "Slate/SlateBrushAsset.h"

static void SetImageBrushSizeIfValid(UImage* Image, const FVector2D& Size)
{
	if (!Image)
	{
		return;
	}

	FSlateBrush Brush = Image->GetBrush();
	Brush.ImageSize = Size;
	Image->SetBrush(Brush);
}

// 이미지의 원본 텍스처 크기를 가져와서 ImageSize로 설정 (1.83배 확대)
static void SetImageBrushSizeFromTexture(UImage* Image)
{
	if (!Image)
	{
		return;
	}

	FSlateBrush Brush = Image->GetBrush();
	const float ScaleMultiplier = 1.83f; // Back 이미지 크기 확대 배율
	
	// 텍스처 찾기
	UTexture2D* Texture2D = nullptr;
	
	if (UTexture2D* DirectTexture = Cast<UTexture2D>(Brush.GetResourceObject()))
	{
		Texture2D = DirectTexture;
	}
	else if (USlateBrushAsset* BrushAsset = Cast<USlateBrushAsset>(Brush.GetResourceObject()))
	{
		Texture2D = Cast<UTexture2D>(BrushAsset->Brush.GetResourceObject());
	}

	// 텍스처 크기 적용
	if (Texture2D)
	{
		Brush.ImageSize = FVector2D(
			static_cast<float>(Texture2D->GetSizeX()) * ScaleMultiplier, 
			static_cast<float>(Texture2D->GetSizeY()) * ScaleMultiplier
		);
		Image->SetBrush(Brush);
	}
}

void UPingRadialMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯 트리 고정 구조 검증 (개발 단계에서 빨리 문제 찾기용)
	checkf(Button_Slice_0 && Button_Slice_1 && Button_Slice_2 && Button_Slice_3 && Button_Slice_4 && Button_Slice_5,
		TEXT("UPingRadialMenuWidget: Button_Slice_0~5 이 모두 존재해야 합니다. WBP_PingRadialMenu 구조를 변경하지 마세요."));
	checkf(Back_1 && Back_2 && Back_3 && Back_4 && Back_5 && Back_6,
		TEXT("UPingRadialMenuWidget: Back, Back_2~Back_6 이미지가 모두 존재해야 합니다. 이름/계층을 변경하지 마세요."));
	checkf(Image_6 && Image_1 && Image_2 && Image_3 && Image_4 && Image_5,
		TEXT("UPingRadialMenuWidget: 아이콘 이미지들(Image_6, Image_1~Image_5)이 모두 존재해야 합니다."));

	
	// Back 이미지들은 crop된 이미지의 원본 크기를 사용 (버튼 크기 문제 해결)
	const TArray<UImage*> BackImages = { Back_1, Back_2, Back_3, Back_4, Back_5, Back_6 };
	for (UImage* BackImage : BackImages)
		{
		SetImageBrushSizeFromTexture(BackImage);

		// 비선택(default) 상태: 반투명(0.5) 흰색
		if (BackImage)
		{
			BackImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.5f));
			}
	}

	// 아이콘 이미지들은 고정 크기 사용
	const FVector2D IconSize(100.f, 100.f);
	const TArray<UImage*> IconImages = { Image_1, Image_2, Image_3, Image_4, Image_5, Image_6 };
	for (UImage* IconImage : IconImages)
	{
		SetImageBrushSizeIfValid(IconImage, IconSize);
	}

	// 모든 슬라이스 버튼의 초기 상태 설정
	const TArray<UButton*> Buttons = { Button_Slice_0, Button_Slice_1, Button_Slice_2, Button_Slice_3, Button_Slice_4, Button_Slice_5 };
	for (UButton* Button : Buttons)
	{
		if (Button)
		{
			Button->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}
}

void UPingRadialMenuWidget::ShowAtScreenPosition(const FVector2D& InCenterScreenPosition)
{
	// 오른쪽 클릭한 지점을 중심으로 라디얼 메뉴를 배치
	// CommanderWorldMapWidget 에서 이미 InGeometry.AbsoluteToLocal()로
	// 맵 로컬 좌표로 변환된 값을 넘겨주므로, 여기서는 로컬 좌표 그대로 사용한다.
	CenterScreenPos = InCenterScreenPosition;
	
	// 시각적 표시만 하고 입력은 CommanderWorldMapWidget이 계속 받도록
	// HitTestInvisible 로 설정 (라디얼 메뉴가 마우스 이벤트를 가로채지 않게 함)
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 이 위젯은 맵 위젯의 CanvasPanel 자식으로 추가되므로,
	// Slot 을 CanvasPanelSlot 로 캐스팅하여 로컬 좌표 기준으로 위치를 설정한다.
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		// 전달받은 좌표(오른쪽 클릭 지점)를 "위젯의 중심"으로 사용하기 위해 정렬을 0.5,0.5 로 맞춰준다.
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(InCenterScreenPosition);
	}
}

int32 UPingRadialMenuWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 RetLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (bShowDebugLine)
	{
		// 위젯 로컬 좌표에서 중심은 (Width/2, Height/2)
		const FVector2D WidgetSize        = AllottedGeometry.GetLocalSize();
		const FVector2D WidgetCenterLocal = WidgetSize * 0.5f;

		// 중심(클릭 지점)을 기준으로 커서까지의 방향 벡터를 계산
		// CenterScreenPos / LastCursorScreenPos 는 둘 다 맵 로컬 좌표이므로 그대로 사용 가능
		const FVector2D Delta = LastCursorScreenPos - CenterScreenPos;

			TArray<FVector2D> LinePoints;
		const FVector2D Start = WidgetCenterLocal;        // 라디얼 중심
		const FVector2D End   = WidgetCenterLocal + Delta; // 중심에서 커서 방향으로 선을 그린다

		LinePoints.Add(Start);
		LinePoints.Add(End);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				RetLayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				ESlateDrawEffect::None,
				FLinearColor::Red,
			false,
			3.0f);

		return RetLayerId + 1;
	}

	return RetLayerId;
}

void UPingRadialMenuWidget::HideMenu()
{
	SetVisibility(ESlateVisibility::Collapsed);
	HighlightIndex = INDEX_NONE;

	// 모든 버튼 초기화
	const TArray<UButton*> Buttons = { Button_Slice_0, Button_Slice_1, Button_Slice_2, Button_Slice_3, Button_Slice_4, Button_Slice_5 };
	for (UButton* Button : Buttons)
	{
		if (Button)
		{
			Button->SetRenderScale(FVector2D(1.0f, 1.0f));
		}
	}

	// 모든 Back 이미지도 비선택 상태로 되돌림 (반투명 흰색)
	const TArray<UImage*> BackImages = { Back_1, Back_2, Back_3, Back_4, Back_5, Back_6 };
	for (UImage* BackImage : BackImages)
	{
		if (BackImage)
		{
			BackImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.5f));
		}
	}
}

/**
 * 드래그 벡터의 각도를 기반으로 섹션 인덱스 계산
 *
 * 기준:
 * - 입력 좌표는 모두 \"맵 위젯 로컬 좌표\" (CommanderWorldMapWidget 의 InGeometry.AbsoluteToLocal 결과)
 * - 라디얼의 중심(CenterScreenPos)을 기준으로 커서 위치까지의 벡터를 구한 뒤,
 *   그 벡터의 각도를 이용해 6분할(또는 PingTypes.Num() 분할) 섹터 인덱스를 구한다.
 * - 시계 방향 기준:
 *     0: 12시, 1: 2시, 2: 4시, 3: 6시, 4: 8시, 5: 10시
 *
 * @param CursorPos 현재 커서의 맵 로컬 좌표
 * @return 선택된 섹션 인덱스 [0, PingTypes.Num-1], 또는 INDEX_NONE (데드존 내부)
 */
int32 UPingRadialMenuWidget::GetIndexFromCursor(const FVector2D& CursorPos) const
{
	if (PingTypes.Num() == 0)
	{
		return INDEX_NONE;
	}

	// 드래그 벡터 계산: CurrentMousePos - CenterPos
	FVector2D Dir = CursorPos - CenterScreenPos;
	const float DistanceSquared = Dir.SizeSquared();
	
	// 중심부 데드존: 너무 가운데에 있으면 어떤 버튼도 선택하지 않음
	const float CenterDeadZoneRadius = RadialRadius * 0.35f;
	const float DeadZoneRadiusSquared = CenterDeadZoneRadius * CenterDeadZoneRadius;
	if (DistanceSquared < DeadZoneRadiusSquared)
	{
		return INDEX_NONE;
	}

	// 각도 계산: atan2(Dir.Y, Dir.X)
	//   - 화면 좌표계: +X = 오른쪽(3시), +Y = 아래(6시)
	//   - atan2 결과(도 단위):
	//       0도   : 오른쪽(3시)
	//       90도  : 아래(6시)
	//       180도 : 왼쪽(9시)
	//       270도 : 위(12시)
	const float AngleRad = FMath::Atan2(Dir.Y, Dir.X);
	float AngleDeg = FMath::RadiansToDegrees(AngleRad); // [-180, 180]

	// [0, 360) 으로 정규화
	if (AngleDeg < 0.f)
	{
		AngleDeg += 360.f;
	}

	// 위쪽(12시)을 0도로 맞추기 위해 +90도 회전 (3시 기준 → 12시 기준)
	float AngleFromTop = AngleDeg + 90.f;
	if (AngleFromTop >= 360.f)
	{
		AngleFromTop -= 360.f;
	}

	// 섹터 크기 (예: 6개 → 60도)
	const float SectorAngle = 360.f / static_cast<float>(PingTypes.Num());

	// 각 섹터의 중앙을 기준으로 하도록 절반(=SectorAngle/2) 만큼 더한 뒤 floor
	//  - AngleFromTop = 0도 부근 → 인덱스 0 (12시)
	//  - 그 다음 60도 단위로 1,2,... 증가 (시계 방향)
	const float Adjusted = AngleFromTop + (SectorAngle * 0.5f);
	const int32 Index = (static_cast<int32>(FMath::FloorToFloat(Adjusted / SectorAngle)) % PingTypes.Num());

	return Index;
}

/**
 * HighlightIndex 변경 시 즉시 시각적 피드백 처리
 * 
 * 가이드: "각도로 섹터 선택 (hover 의존 X)" - 순수 각도 계산만 사용
 * ✅ Hover 애니메이션/타이머 없이 즉시 시각적 피드백만 적용
 * 
 * @param OldIndex 이전 선택 인덱스
 * @param NewIndex 새로운 선택 인덱스
 */
void UPingRadialMenuWidget::HandleHighlightIndexChanged(int32 OldIndex, int32 NewIndex)
{
	auto GetBackImageForIndex = [this](int32 Index) -> UImage*
	{
		switch (Index)
		{
		case 0: return Back_1;
		case 1: return Back_2;
		case 2: return Back_3;
		case 3: return Back_4;
		case 4: return Back_5;
		case 5: return Back_6;
		default: return nullptr;
		}
	};

	// 이전 버튼 초기화 (즉시 시각적 피드백만, hover 애니메이션 없음)
	if (OldIndex != INDEX_NONE && OldIndex != NewIndex)
	{
		if (UButton* OldButton = GetButtonForIndex(OldIndex))
		{
			OldButton->SetRenderScale(FVector2D(1.0f, 1.0f));
		}

		// 이전 Back 이미지는 다시 비선택 상태(반투명 흰색)로
		if (UImage* OldBack = GetBackImageForIndex(OldIndex))
		{
			OldBack->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.5f));
		}
	}

	// 새 버튼에 즉시 시각적 피드백 적용 (각도 계산 기반, hover 없음)
	if (NewIndex != INDEX_NONE)
	{
		if (UButton* NewButton = GetButtonForIndex(NewIndex))
		{
			NewButton->SetRenderScale(FVector2D(1.03f, 1.03f));
		}

		// 선택된 Back 이미지는 노란색 틴트 + 완전 불투명
		if (UImage* NewBack = GetBackImageForIndex(NewIndex))
		{
			// 약간 부드러운 노랑 (R=1, G=0.9, B=0.3)
			NewBack->SetColorAndOpacity(FLinearColor(1.f, 0.9f, 0.3f, 1.f));
		}
	}
}

UImage* UPingRadialMenuWidget::GetImageWidgetForPingIndex(int32 Index) const
{
	switch (Index)
	{
	case 0: return Image_1;
	case 1: return Image_2;
	case 2: return Image_3;
	case 3: return Image_4;
	case 4: return Image_5;
	case 5: return Image_6;
	default: return nullptr;
	}
}

FSlateBrush UPingRadialMenuWidget::GetIconBrushForType(ECommanderPingType InType) const
{
	const int32 Index = PingTypes.IndexOfByKey(InType);
	if (!PingTypes.IsValidIndex(Index))
	{
		return FSlateBrush();
	}

	if (UImage* ImageWidget = GetImageWidgetForPingIndex(Index))
	{
		return ImageWidget->GetBrush();
	}

	return FSlateBrush();
}

UButton* UPingRadialMenuWidget::GetButtonForIndex(int32 Index) const
{
	// 순서대로 사용: 0=12시, 1=2시, 2=4시, 3=6시, 4=8시, 5=10시 (시계방향)
	switch (Index)
	{
	case 0: return Button_Slice_0; // 12시
	case 1: return Button_Slice_1; // 2시
	case 2: return Button_Slice_2; // 4시
	case 3: return Button_Slice_3; // 6시
	case 4: return Button_Slice_4; // 8시
	case 5: return Button_Slice_5; // 10시
	default: return nullptr;
	}
}

void UPingRadialMenuWidget::UpdateSelectionByScreenPosition(const FVector2D& InCursorScreenPosition)
{
	// 디버그용으로 마지막 커서 위치 저장
	LastCursorScreenPos = InCursorScreenPosition;

	const int32 NewIndex = GetIndexFromCursor(InCursorScreenPosition);

	// 인덱스가 변경되지 않았으면 아무 것도 하지 않음
	if (NewIndex == HighlightIndex)
	{
		return;
	}

	const int32 OldIndex = HighlightIndex;

	// 유효한 인덱스인 경우
	if (PingTypes.IsValidIndex(NewIndex))
	{
	HighlightIndex = NewIndex;
	CurrentSelectedType = PingTypes[NewIndex];
	HandleHighlightIndexChanged(OldIndex, NewIndex);
}
	else
	{
		// 유효하지 않은 인덱스면 하이라이트 해제
		HighlightIndex = INDEX_NONE;
		HandleHighlightIndexChanged(OldIndex, INDEX_NONE);
	}
}

