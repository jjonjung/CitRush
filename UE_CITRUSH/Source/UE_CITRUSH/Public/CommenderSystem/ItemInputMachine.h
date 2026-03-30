// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommenderSystem/VendingItemActor.h"
#include "UI/CommanderMessageType.h"
#include "ItemInputMachine.generated.h"

// Forward declarations
class UInteractableComponent;
class ACommenderCharacter;
class UWidgetComponent;
class UTargetRacerDisplayWidget;
class UTargetRacerSelectionWidget;

// 디스크 삽입 결과
USTRUCT(BlueprintType)
struct FDiscInsertResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly)
	ECommanderItemEffect EffectType = ECommanderItemEffect::None;

	UPROPERTY(BlueprintReadOnly)
	FName ItemId;

	UPROPERTY(BlueprintReadOnly)
	FText ItemName;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTexture2D> ItemIcon = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDiscInserted, const FDiscInsertResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRacerSelected, int32, RacerIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommanderItemUseDecided, const FDiscInsertResult&, ItemInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRacerSelectionRequested);

UCLASS()
class UE_CITRUSH_API AItemInputMachine : public AActor
{
	GENERATED_BODY()
	
public:
	AItemInputMachine();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 상호작용 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<UInteractableComponent> InteractableComponent;

	/** 기계 메시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	TObjectPtr<class UStaticMeshComponent> MeshComp;

	/** 디스크 슬롯 위치 (Box Collision) - 아이템 삽입 위치 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Disc")
	TObjectPtr<class UBoxComponent> DiscSlot;

	/** 타겟 레이서 표시 위젯 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UWidgetComponent> TargetRacerWidgetComponent;

	/** 타겟 레이서 표시 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSubclassOf<UTargetRacerDisplayWidget> TargetRacerWidgetClass;

	/** 레이서 선택 위젯 클래스 (블루프린트에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSubclassOf<UTargetRacerSelectionWidget> TargetRacerSelectionWidgetClass;

public:
	/** 현재 열려있는 레이서 선택 위젯 (한 번만 열리도록 체크) */
	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<UTargetRacerSelectionWidget> CurrentSelectionWidget;

	/** 현재 타겟 레이서 인덱스 (-1이면 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_CurrentTargetRacer, Category="Racer")
	int32 TargetRacerIndex = -1;

protected:

	/** 현재 삽입된 아이템 */
	UPROPERTY(BlueprintReadOnly, Category="Disc")
	TObjectPtr<AVendingItemActor> InsertedItem;

	/** 디스크 삽입 중인지 플래그 */
	UPROPERTY(BlueprintReadOnly, Category="Disc")
	bool bIsInserting = false;

	UFUNCTION()
	void OnRep_CurrentTargetRacer();

public:
	// 디스크 삽입 완료 이벤트
	UPROPERTY(BlueprintAssignable, Category="Disc")
	FOnDiscInserted OnDiscInserted;

	// 레이서 선택 완료 이벤트
	UPROPERTY(BlueprintAssignable, Category="Disc")
	FOnRacerSelected OnRacerSelected;

	// 커맨더 아이템 사용 결정 이벤트
	UPROPERTY(BlueprintAssignable, Category="Disc")
	FOnCommanderItemUseDecided OnCommanderItemUseDecided;

	// 레이서 선택 요청 이벤트
	UPROPERTY(BlueprintAssignable, Category="Disc|Target")
	FOnRacerSelectionRequested OnRacerSelectionRequested;

	/** 디스크 삽입 (서버 RPC) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Disc")
	void Server_InsertDisc(ACommenderCharacter* Commander);
	void Server_InsertDisc_Implementation(ACommenderCharacter* Commander);

	/** 타겟 레이서 설정 (서버 RPC) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Disc|Target")
	void Server_SetTargetRacer(int32 RacerIndex);
	void Server_SetTargetRacer_Implementation(int32 RacerIndex);

	/** 타겟 레이서 변경 시 BP에서 후처리 가능 */
	UFUNCTION(BlueprintImplementableEvent, Category="Disc|Target")
	void OnTargetRacerChanged();

	/** 타겟 레이서 위젯 업데이트 */
	void UpdateTargetRacerWidget();

	/** 상호작용 공통 처리 */
	void HandleInteraction(ACommenderCharacter* Commander);

	/** 레이서 선택 (BP에서 호출) */
	UFUNCTION(BlueprintCallable, Category="Disc")
	void SelectRacer(int32 RacerIndex);

	/** 커맨더 아이템 사용 결정 (BP에서 호출) */
	UFUNCTION(BlueprintCallable, Category="Disc")
	void DecideCommanderItemUse(bool bUse);

	/** 상호작용 시작 (멀티플레이 상호작용 델리게이트 바인딩용) */
	UFUNCTION()
	void OnInteractionStarted(APawn* InteractingPawn);

	/** 클라이언트 상호작용 (싱글/클라 전용) */
	UFUNCTION()
	void OnClientInteraction(APlayerController* PlayerController);

	/** 레이서 선택 요청 이벤트 핸들러 (C++ 자동 바인딩) */
	UFUNCTION()
	void OnRacerSelectionRequestedHandler();

	/** 디스크 아이템 정보 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Disc")
	bool GetDiscItemInfo(FDiscInsertResult& OutResult) const;

	/** 아이템을 집어넣는 기준 위치 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<class USceneComponent> ItemUseOrigin;

	/** 이 반경 안에 있으면 "지정영역 내" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	float ItemUseRadius = 50.0f;

	/** 이 머신이 아이템을 받을 수 있는지 확인 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="InputMachine")
	bool CanAcceptItem(class AVendingItemActor* Item, class ACommenderCharacter* Commander) const;

	/** 레이서에게 아이템 지급 (성공 시 true, 실패 시 false 반환) */
	UFUNCTION(BlueprintCallable, Category="InputMachine")
	bool SupplyItemToRacer(class AVendingItemActor* Item, class ACommenderCharacter* Commander);

	/** TargetRacerIndex에 해당하는 레이서가 실제로 존재하는지 확인 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="InputMachine")
	bool IsTargetRacerValid() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Disc|Data")
	TObjectPtr<UDataTable> itemDataTable;

	/** 디버그 모드: 아이템 할당 과정을 상세히 로그로 출력 (싱글플레이 테스트용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug")
	bool bDebugItemAssignment = false;

private:
	/** itemDataTable을 찾는 헬퍼 함수 */
	void TryFindItemDataTable();
	
	/** Commander와 CommenderHUDWidget 유효성 확인 및 메시지 표시 헬퍼 (일반 메시지) */
	bool ShowCommanderMessage(ACommenderCharacter* Commander, ECommanderMessageID MessageID);
	
	/** Commander와 CommenderHUDWidget 유효성 확인 및 메시지 표시 헬퍼 (포맷 메시지) */
	bool ShowCommanderMessage(ACommenderCharacter* Commander, ECommanderMessageID MessageID, const TArray<FString>& FormatArgs);
};
