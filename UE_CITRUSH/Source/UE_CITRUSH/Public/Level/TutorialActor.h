// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialActor.generated.h"

class USkeletalMeshComponent;
class UWidgetComponent;
class UAnimSequence;
class UUserWidget;

/**
 * 튜토리얼 전용 액터
 * Skeletal Mesh를 사용하여 애니메이션을 재생하고 머리 위에 사용방법 위젯을 표시
 */
UCLASS(BlueprintType, Blueprintable)
class UE_CITRUSH_API ATutorialActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATutorialActor(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	/** 에디터에서 프로퍼티 변경 시 위젯 업데이트 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	/** 애니메이션 시퀀스 변경 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetAnimationSequence(UAnimSequence* NewAnimationSequence);

	/** 위젯 텍스트 변경 (기존 호환성) */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetTutorialText(const FText& NewText);

	/** 표시할 멘트 인덱스 변경 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetMessageIndex(int32 NewMessageIndex);

	/** 표시할 멘트 이름 변경 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetMessageName(const FName& NewMessageName);

	/** 애니메이션 재생 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void PlayAnimation();

	/** 애니메이션 정지 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StopAnimation();

	/** 위젯 표시/숨김 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetWidgetVisibility(bool bVisible);

	/** 현재 애니메이션 시퀀스 가져오기 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tutorial")
	UAnimSequence* GetCurrentAnimationSequence() const { return CurrentAnimationSequence; }

protected:
	/** Skeletal Mesh Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	/** 머리 위 위젯 Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> WidgetComponent;

	/** 현재 재생 중인 애니메이션 시퀀스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Animation")
	TObjectPtr<UAnimSequence> CurrentAnimationSequence;

	/** 루프 재생 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Animation")
	bool bLoopAnimation = true;

	/** 재생 속도 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Animation", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float PlayRate = 1.0f;

	/** 위젯에 표시할 튜토리얼 텍스트 (기존 호환성용, MessageIndex 사용 권장) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Widget")
	FText TutorialText = FText::FromString(TEXT("사용 방법을 표시합니다"));

	/** 표시할 멘트 인덱스 (TutorialWidget의 Messages 배열 인덱스, -1이면 기본 메시지 표시 안 함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Widget", meta = (ClampMin = "-1"))
	int32 MessageIndex = -1;

	/** 표시할 멘트 이름 (MessageIndex보다 우선순위 높음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Widget")
	FName MessageName = NAME_None;

	/** 위젯 클래스 (머리 위에 표시될 위젯) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Widget")
	TSubclassOf<UUserWidget> TutorialWidgetClass;

	/** 위젯 오프셋 (머리 위 위치) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Widget")
	FVector WidgetOffset = FVector(0.0f, 0.0f, 150.0f);

	/** 위젯 크기 (기본 단위: 0.1 = 10%, 0.5 = 50%, 1.0 = 100%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Widget", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float WidgetSize = 1.0f;

	/** 위젯이 항상 플레이어를 바라보도록 설정 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Widget")
	bool bWidgetFacePlayer = true;

	/** 위젯 표시 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Widget")
	bool bWidgetVisible = true;

private:
	/** 위젯 업데이트 (텍스트 설정) */
	void UpdateWidget();

	/** 위젯 회전 업데이트 (플레이어를 바라보도록) */
	void UpdateWidgetRotation();

	/** Tick에서 위젯 회전 업데이트를 위한 타이머 핸들 */
	FTimerHandle WidgetRotationTimerHandle;
};

