// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Blueprint/UserWidget.h"
#include "WidgetDataAsset.generated.h"

/* UUserWidget 이외의 UClass 전달 시 Runtime Error 표출 회피용
`concept` 는 C++20 표준 기능이고, `derived_from` 은 C++17 표준 기능입니다.
컴파일러 지원 : "MSVC, Clang, GCC" 모두 C++20 모드에서 Concepts를 완벽하게 지원합니다.
언리얼 환경 : UE5는 C++20 기능을 활성화하고 있습니다. Concepts 사용은 가능하지만, Unreal Build Tool (UBT)이 C++20 모드로 빌드하도록 설정되어 있는지 확인해야 합니다. (일반적으로 UE5 프로젝트는 기본적으로 지원합니다.)
안정성 고려 : Concepts는 비교적 최근에 도입되었으므로, 아주 오래된 컴파일러 버전(예: 구형 콘솔 SDK의 Clang 버전)에서는 지원되지 않을 수 있지만, UE5가 공식적으로 지원하는 환경에서는 문제가 없습니다.
*/
template <typename T>
concept IsDerivedFromUserWidget = std::derived_from<T, UUserWidget>;

/**
 * UClass 기반 타입 안전 위젯 관리 Data Asset. C++20 Concept 활용
 */
UCLASS()
class UE_CITRUSH_API UWidgetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	/** Primary Asset ID 반환 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** UClass 기반으로 타입 안전 Widget 클래스 조회 (Concept 활용) */
	template <IsDerivedFromUserWidget T>
	TSubclassOf<T> GetWidgetClass(UClass* uClass) const;

protected:
	/** Widget Blueprint 클래스 Dictionary (Key: UClass, Value: Widget 클래스) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WidgetBlueprintDataAsset")
	TMap<UClass*, TSubclassOf<UUserWidget>> wbpClasses;
};

template <IsDerivedFromUserWidget T>
TSubclassOf<T> UWidgetDataAsset::GetWidgetClass(UClass* uClass) const
{
	if (const TSubclassOf<UUserWidget>* valueParentClass = wbpClasses.Find(uClass))
	{
		UClass* childClass = valueParentClass->Get();
		if (childClass && childClass->IsChildOf(UUserWidget::StaticClass()))
		{
			return TSubclassOf<T>(childClass);
		}
	}
	return nullptr;
}

