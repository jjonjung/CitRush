#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Data/PingTypes.h"
#include "PingMarkerManager.generated.h"

class APingMarkerActor;
class UWorld;

/**
 * 핑 마커를 관리하는 헬퍼 클래스 (싱글톤 스타일)
 * 각 클라이언트에서 로컬로 마커를 생성/업데이트/제거
 *
 * Blueprint 서브클래스를 만들 수 있도록 Blueprintable 지정
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API UPingMarkerManager : public UObject
{
	GENERATED_BODY()

public:
	/** 싱글톤 인스턴스 가져오기 (기본 C++ 클래스 사용) */
	static UPingMarkerManager* Get(UWorld* World);

	/**
	 * 싱글톤 인스턴스 가져오기 (Blueprint 서브클래스를 사용하고 싶을 때)
	 * - ManagerClassOverride 가 지정되면 해당 클래스로 매니저를 생성
	 * - nullptr 이면 기본 C++ 클래스(UPingMarkerManager)로 생성
	 */
	static UPingMarkerManager* Get(UWorld* World, TSubclassOf<UPingMarkerManager> ManagerClassOverride);

	/** 핑 업데이트 처리 */
	UFUNCTION(BlueprintCallable, Category = "Ping Marker")
	void UpdatePingMarker(const FPingData& PingData);

	/** 활성화된 모든 핑 목록을 기반으로 마커들을 갱신 (여러 개 동시 지원) */
	UFUNCTION(BlueprintCallable, Category = "Ping Marker")
	void RefreshPingMarkers(const TArray<FPingData>& PingList);

	/** 핑 마커 제거 */
	UFUNCTION(BlueprintCallable, Category = "Ping Marker")
	void ClearPingMarker();

protected:
	/** 현재 활성 핑 ID -> 마커 액터 매핑 (여러 개 동시 지원) */
	UPROPERTY()
	TMap<FGuid, TWeakObjectPtr<APingMarkerActor>> MarkersById;

	/** 마커 클래스 (Blueprint에서 설정 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ping Marker")
	TSubclassOf<APingMarkerActor> MarkerClass;

private:
	/** 월드 참조 */
	UPROPERTY()
	TObjectPtr<UWorld> WorldRef;
};
