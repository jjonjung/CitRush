#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/PingTypes.h"
#include "CommanderWorldMapWidget.generated.h"

class AMapBoundsActor;
class ACitRushPlayerController;
class ACommenderCharacter;
class UPingRadialMenuWidget;
class UCommenderHUDWidget;
class APingMarkerActor;
class UMinimapIconComponent;
class URealtimeMapIcon;
class UCanvasPanel;

/**
 * Commander 전체지도 UI 위젯
 * 맵 클릭을 받아 핑을 배치
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API UCommanderWorldMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCommanderWorldMapWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 맵 클릭 처리 */
	void HandleMapClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);

	/** 클릭 좌표를 월드 좌표로 변환 */
	FVector ConvertClickToWorldLocation(const FGeometry& InGeometry, const FVector2D& ScreenPosition) const;

	/** MapImage 위젯의 실제 화면상 Rect 구하기 */
	bool GetMapImageScreenRect(const FGeometry& InGeometry, FVector2D& OutMin, FVector2D& OutMax) const;

	/** MapBounds 액터 찾기 */
	AMapBoundsActor* FindMapBounds() const;

	/** 맵 이미지의 특정 UV 위치에서 alpha 값을 확인 (건물 영역 체크용) */
	bool CheckMapAlphaAtUV(const FVector2D& UV, float AlphaThreshold = 0.5f) const;

	/** PlayerController 찾기 */
	ACitRushPlayerController* GetCitRushPlayerController() const;

	/** CommanderCharacter 찾기 */
	ACommenderCharacter* GetCommanderCharacter() const;

	/** 현재 커맨더(플레이어) 월드 위치 구하기 */
	FVector GetCommanderWorldLocation() const;

	/** CommanderStart Actor의 위치 구하기 */
	FVector GetCommanderStartLocation() const;

	/** UV(0~1)를 월드 좌표로 변환 */
	FVector ConvertUVToWorldLocation(const FVector2D& UV) const;

public:
	/** 핑 타입 설정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping")
	ECommanderPingType DefaultPingType = ECommanderPingType::Objective;

	/** 현재 선택된 핑 타입 (버튼으로 변경) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping", meta = (ExposeOnSpawn = "true"))
	ECommanderPingType SelectedPingType = ECommanderPingType::Objective;

	/** 핑 라디얼 메뉴 클래스 (옵션) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping")
	TSubclassOf<UPingRadialMenuWidget> PingRadialMenuClass;

	/** 생성된 라디얼 메뉴 인스턴스 (런타임) */
	UPROPERTY(BlueprintReadOnly, Category = "Ping")
	UPingRadialMenuWidget* PingRadialMenu = nullptr;

	/** 맵 위젯 크기 (위젯의 실제 크기, 자동 계산되지만 수동 설정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ToolTip = "해상도와 여유분을 기반으로 자동 계산됩니다. 수동 설정 시 자동 계산이 비활성화됩니다."))
	FVector2D MapWidgetSize = FVector2D(800.f, 600.f);

	/** 여유분(Margin) - 화면 가장자리에서 맵까지의 여유 공간 (픽셀) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ToolTip = "화면 가장자리에서 맵 이미지까지의 여유 공간. 위아래좌우 모두 동일하게 적용됩니다."))
	float MapMargin = 50.f;

	/** 자동 크기 계산 활성화 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ToolTip = "true면 해상도와 여유분을 기반으로 자동으로 위젯 크기와 오프셋을 계산합니다."))
	bool bAutoCalculateSize = true;

	/** 자동 계산된 크기/오프셋을 뷰포트에 즉시 반영할지 여부 (기본 false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ToolTip = "true면 자동 계산 결과를 SetDesiredSizeInViewport/SetPositionInViewport로 즉시 적용합니다. 디자이너 배치를 유지하려면 false로 두세요."))
	bool bApplyAutoLayout = false;

	/** 맵 이미지 영역 오프셋 (이미지가 위젯의 일부만 차지하는 경우) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ToolTip = "맵 이미지가 위젯 내에서 시작하는 위치"))
	FVector2D MapImageOffset = FVector2D::ZeroVector;

	/** 맵 이미지 실제 크기 (위젯 크기와 다를 수 있음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ToolTip = "맵 이미지의 실제 픽셀 크기"))
	FVector2D MapImageSize = FVector2D(800.f, 600.f);

	/** 맵 이미지 회전 (도, 시계방향) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ToolTip = "맵 이미지가 회전되어 있는 경우"))
	float MapImageRotation = 0.f;

	/** 알파맵 이미지 회전 (도, 시계방향, MapImageRotation과 독립적) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ToolTip = "알파맵 이미지 회전 각도. MapImageRotation과 독립적으로 설정 가능"))
	float AlphaMapRotation = 0.f;

	/** 맵 이미지 위젯 (Blueprint에서 바인딩, 실제 화면 Rect 계산용) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Map")
	class UImage* MapImageWidget = nullptr;

	/** 알파맵 텍스처 (건물 영역 체크용, 흰색=지면, 검은색=건물) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ToolTip = "건물 영역을 표시하는 알파맵 텍스처. 흰색 영역은 핑 생성 가능, 검은색 영역은 핑 생성 불가"))
	TObjectPtr<class UTexture2D> AlphaMapTexture = nullptr;

	/** 알파맵 이미지 위젯 (Blueprint에서 바인딩, MapImageWidget과 함께 회전) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Map")
	class UImage* AlphaMapImage = nullptr;

	/** 맵 위에 표시할 핑 마커 이미지 (Blueprint에서 바인딩) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Ping Marker")
	class UImage* PingMarkerImage = nullptr;

	/** 맵 위에 표시할 커맨더 위치 마커 이미지 (선택적으로 바인딩) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Commander")
	class UImage* CommanderMarkerImage = nullptr;

	/** Scale Box 위젯 (Blueprint에서 바인딩, CommanderMarkerImage의 부모) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Map")
	class UScaleBox* ScaleBox = nullptr;

	/** 핑 마커 이미지 크기 (픽셀) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Marker", meta = (ToolTip = "맵 위에 표시되는 핑 마커(빨간 점)의 크기"))
	FVector2D PingMarkerSize = FVector2D(10.f, 10.f);


	/** 커맨더 위치 마커 갱신용 타이머 */
	FTimerHandle CommanderMarkerTimerHandle;

	/** CommanderAnchor Tag를 가진 Actor 캐시 */
	TWeakObjectPtr<AActor> CachedCommanderAnchor;

	/** CommanderAnchor Actor 찾기 (재시도 로직 포함) */
	void FindCommanderAnchor();

	/** 커맨더 위치 마커를 맵 UI 위에 한 번 갱신 (맵 열릴 때 호출) */
	UFUNCTION(BlueprintCallable, Category = "Commander")
	void UpdateCommanderMarker();

	/** (디버그용) 현재 레벨에 존재하는 PingMarkerActor 들을 맵 UI 위에 마커로 표시 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugShowWorldPingActorsOnMap();

	/** 디버그 마커 제거 (레벨에서 사라진 핑의 마커 제거) */
	void ClearDebugPingMarkers();

	/** 이 맵 위젯을 생성한 HUD 위젯 설정 (F 키로 닫기용) */
	void SetOwningHUD(UCommenderHUDWidget* InHUD) { OwningHUD = InHUD; }

protected:
	/** 클릭한 위치에 핑 마커 표시 */
	void ShowPingMarkerAtPosition(const FVector2D& LocalPosition, const FVector& WorldLocation = FVector::ZeroVector);

	/** 해상도 기반으로 위젯 크기와 오프셋 자동 계산 */
	void CalculateWidgetSizeAndOffset();

	/** 자동 계산된 크기/오프셋을 실제 위젯 배치에 적용 */
	void ApplyCalculatedLayout();

	/** 맵 이미지 회전 적용 */
	void ApplyMapImageRotation();

	/** 원본 UV를 회전된 Geometry의 UV로 변환 */
	UFUNCTION(BlueprintCallable, Category = "Map", meta = (ToolTip = "원본 UV 좌표를 회전된 맵의 UV 좌표로 변환합니다. AlphaMap 회전 확인용"))
	FVector2D RotateUVToRotatedGeometry(const FVector2D& OriginalUV) const;

	/** 회전된 Geometry의 UV를 원본 UV로 변환 */
	UFUNCTION(BlueprintCallable, Category = "Map", meta = (ToolTip = "회전된 맵의 UV 좌표를 원본 UV 좌표로 변환합니다. AlphaMap 회전 확인용"))
	FVector2D RotateUVFromRotatedGeometry(const FVector2D& RotatedUV) const;

	/** UV 변환 테스트 함수 (AlphaMap 회전 확인용) */
	UFUNCTION(BlueprintCallable, Category = "Map", meta = (ToolTip = "UV 변환을 테스트하고 로그로 출력합니다. AlphaMap 회전 확인용"))
	void TestUVRotation(const FVector2D& TestUV) const;

	/** 이 맵 위젯을 생성한 Commander HUD (맵을 닫을 때 크로스헤어/입력까지 같이 복원하기 위함) */
	UPROPERTY()
	UCommenderHUDWidget* OwningHUD = nullptr;

public:
	/** 버튼으로 핑 타입 변경 (Blueprint 바인딩용) */
	UFUNCTION(BlueprintCallable, Category = "Ping")
	void SetSelectedPingType(ECommanderPingType NewType);

protected:
	/** 라디얼 메뉴 표시/갱신/숨김 */
	void OpenRadialMenu(const FVector2D& ScreenPos);
	void UpdateRadialMenu(const FVector2D& ScreenPos);
	void CloseRadialMenu();

	bool bRadialActive = false;
	/** 우클릭 위치 (스크린 좌표) - 핑 생성 시 사용 */
	FVector2D RadialMenuClickPosition = FVector2D::ZeroVector;

	/** 디버그용 핑 마커 추적 (Actor 포인터 -> UI 마커 위젯) */
	UPROPERTY()
	TMap<APingMarkerActor*, TWeakObjectPtr<class UImage>> DebugPingMarkers;

	/** 맵에 표시된 핑 마커 추적 (월드 위치 -> UI 마커 위젯) */
	UPROPERTY()
	TMap<FVector, TWeakObjectPtr<class UImage>> MapPingMarkers;

	/** GameState 핑 업데이트 이벤트 핸들러 */
	UFUNCTION()
	void OnPingUpdated(const FPingData& NewPing);

	/** 맵 핑 마커 제거 */
	void RemoveMapPingMarkers();

	// ==================== 실시간 아이콘 시스템 ====================

	/** 실시간 아이콘 레이어 (CanvasPanel, Blueprint에서 바인딩) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Realtime Icons")
	class UCanvasPanel* RealtimeIconsLayer = nullptr;

	/** 실시간 아이콘 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Realtime Icons")
	TSubclassOf<URealtimeMapIcon> RealtimeMapIconClass;

	/** 실시간 아이콘 업데이트 타이머 */
	FTimerHandle RealtimeIconsTimerHandle;

	/** 실시간 아이콘 업데이트 주기 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Realtime Icons", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float RealtimeIconsUpdateInterval = 0.1f;

	/** 캐싱된 MinimapIconComponent 목록 (맵 열릴 때 1회 수집, 이후 주기적으로 갱신) */
	UPROPERTY()
	TArray<TWeakObjectPtr<UMinimapIconComponent>> CachedIconComponents;

	/** 마지막 컴포넌트 수집 시간 (새로운 컴포넌트를 찾기 위한 주기 체크용) */
	float LastComponentCollectionTime = 0.f;

	/** 컴포넌트 수집 주기 (초, 기본 1초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Realtime Icons", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float ComponentCollectionInterval = 1.0f;

	/** Actor -> 아이콘 위젯 맵 (재사용을 위한 풀링) */
	UPROPERTY()
	TMap<UMinimapIconComponent*, TWeakObjectPtr<URealtimeMapIcon>> IconWidgetMap;

	/** 실시간 아이콘 업데이트 (타이머에서 호출) */
	UFUNCTION()
	void UpdateRealtimeIcons();

	/** 월드에서 MinimapIconComponent를 가진 Actor들을 수집 (맵 열릴 때 1회 호출) */
	void CollectIconComponents();

	/** 실시간 아이콘 정리 (맵 닫힐 때 호출) */
	void CleanupRealtimeIcons();
};
