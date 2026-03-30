// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerInfoWidget.generated.h"

enum class EPlayerRole : uint8;
class ACitRushPlayerState;
class UTextBlock;
class UComboBoxKey;
class UImage;
/**
 * 플레이어 정보 Widget. 로비에서 플레이어 이름/역할/Ready 상태 표시
 */
UCLASS()
class UE_CITRUSH_API UPlayerInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 플레이어 정보 초기화. PlayerState 델리게이트 바인딩 */
	void InitializePlayerInfo(ACitRushPlayerState* inPlayerState, const int32& index);

	/** 플레이어 ID 비교. PlayerInfoWidget 검색용 */
	UFUNCTION()
	bool ComparePlayer(const uint32& comparedID);

	UFUNCTION()
	void SetAuthority(bool bAuth);

protected:
	/** Widget 생성 시 호출. ComboBox 델리게이트 바인딩 */
	virtual void NativeConstruct() override;

private:
	/** Ready 상태 설정 (Multicast). Ready 이미지 갱신 */
	UFUNCTION()
	void SetReady(bool bReady);

	/** 역할 선택 콜백. ComboBox에서 역할 선택 시 ServerRPC 호출 */
	UFUNCTION()
	void SelectRole(FName SelectedItem, ESelectInfo::Type SelectionType);

	/** 역할 직접 설정. RepNotify에서 호출 */
	UFUNCTION()
	void SetRole(const EPlayerRole& directlyRole);

	/** ComboBox 아이템 생성. 역할 이름을 텍스트로 표시 */
	UFUNCTION()
	UWidget* OnGenerateSelector(FName slotName);


protected:
	/** 플레이어 ID */
	uint32 playerID = -1;

	/** 플레이어 이름 텍스트 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> playerNameText;

	/** 플레이어 역할 ComboBox */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UComboBoxKey> playerRoleComboBox;

	/** Ready 상태 이미지 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> playerReadyStateImage;

	/** Ready 상태 텍스처 */
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UTexture2D> readyImage;

	/** NotReady 상태 텍스처 */
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UTexture2D> notReadyImage;

private:
	UPROPERTY()
	TObjectPtr<ACitRushPlayerState> ownerPS;
};
