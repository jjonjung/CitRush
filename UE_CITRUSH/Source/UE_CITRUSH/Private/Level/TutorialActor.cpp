// Fill out your copyright notice in the Description page of Project Settings.

#include "Level/TutorialActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/Interface.h"
#include "UI/TutorialWidget.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

ATutorialActor::ATutorialActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// Skeletal Mesh Component 생성
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	RootComponent = SkeletalMeshComponent;

	// 충돌 비활성화 (튜토리얼 액터는 통과 가능)
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);

	// Widget Component 생성 (머리 위)
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(SkeletalMeshComponent);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetDrawSize(FVector2D(512.0f, 256.0f));
	WidgetComponent->SetRelativeLocation(WidgetOffset);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 기본 설정
	bLoopAnimation = true;
	PlayRate = 1.0f;
	WidgetOffset = FVector(0.0f, 0.0f, 150.0f);
	WidgetSize = 2.0f;
	bWidgetFacePlayer = true;
	bWidgetVisible = true;
	MessageIndex = -1; // 기본값을 -1로 설정하여 레벨에서 명시적으로 설정하지 않으면 기본 메시지 표시 안 함
	MessageName = NAME_None;
}

void ATutorialActor::BeginPlay()
{
	Super::BeginPlay();

	// 위젯 초기화
	if (WidgetComponent)
	{
		if (TutorialWidgetClass)
		{
			WidgetComponent->SetWidgetClass(TutorialWidgetClass);
		}
		
		// 위젯 크기 설정 (기본 크기: 512x256, WidgetSize로 배율 조절)
		// WidgetSize = 0.1 → 51.2x25.6, WidgetSize = 1.0 → 512x256, WidgetSize = 2.0 → 1024x512
		WidgetComponent->SetDrawSize(FVector2D(512.0f * WidgetSize, 256.0f * WidgetSize));
		WidgetComponent->SetRelativeLocation(WidgetOffset);
		WidgetComponent->SetVisibility(bWidgetVisible);
		
		// 위젯 텍스트 업데이트
		UpdateWidget();
	}

	// 애니메이션 재생
	if (CurrentAnimationSequence)
	{
		PlayAnimation();
	}

	// 위젯 회전 업데이트 시작 (플레이어를 바라보도록)
	if (bWidgetFacePlayer)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().SetTimer(
				WidgetRotationTimerHandle,
				this,
				&ATutorialActor::UpdateWidgetRotation,
				0.1f, // 0.1초마다 업데이트
				true  // 루프
			);
		}
	}
}

void ATutorialActor::SetAnimationSequence(UAnimSequence* NewAnimationSequence)
{
	if (!NewAnimationSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialActor] 애니메이션 시퀀스가 null입니다."));
		return;
	}

	CurrentAnimationSequence = NewAnimationSequence;
	
	// 즉시 재생
	if (SkeletalMeshComponent)
	{
		PlayAnimation();
	}

	UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 애니메이션 시퀀스 변경 - %s"), *NewAnimationSequence->GetName());
}

void ATutorialActor::SetTutorialText(const FText& NewText)
{
	TutorialText = NewText;
	MessageIndex = -1; // 직접 텍스트 설정 시 인덱스 무효화
	UpdateWidget();
	UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 튜토리얼 텍스트 변경 - %s"), *NewText.ToString());
}

void ATutorialActor::SetMessageIndex(int32 NewMessageIndex)
{
	MessageIndex = NewMessageIndex;
	MessageName = NAME_None; // 인덱스 설정 시 이름 초기화
	
	// 위젯 즉시 업데이트
	if (WidgetComponent && WidgetComponent->GetWidget())
	{
		UpdateWidget();
	}
	
	UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 멘트 인덱스 변경 - Index: %d"), MessageIndex);
}

void ATutorialActor::SetMessageName(const FName& NewMessageName)
{
	MessageName = NewMessageName;
	MessageIndex = -1; // 이름 설정 시 인덱스 무효화
	
	// 위젯 즉시 업데이트
	if (WidgetComponent && WidgetComponent->GetWidget())
	{
		UpdateWidget();
	}
	
	UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 멘트 이름 변경 - Name: %s"), *MessageName.ToString());
}

void ATutorialActor::PlayAnimation()
{
	if (!SkeletalMeshComponent || !CurrentAnimationSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialActor] SkeletalMeshComponent 또는 AnimationSequence가 설정되지 않았습니다."));
		return;
	}

	// 애니메이션 재생
	// 재생 속도는 PlayAnimation의 세 번째 파라미터로 전달할 수 없으므로,
	// Montage를 사용하거나 기본 재생 속도로 재생
	SkeletalMeshComponent->PlayAnimation(CurrentAnimationSequence, bLoopAnimation);
	
	// 재생 속도 설정은 Montage를 사용해야 하므로, 여기서는 기본 재생 속도로 재생
	// PlayRate가 1.0이 아니면 경고 메시지 출력
	if (PlayRate != 1.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialActor] PlayRate는 현재 지원되지 않습니다. Montage를 사용하거나 기본 재생 속도(1.0)를 사용하세요."));
	}

	UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 애니메이션 재생 시작 - Sequence: %s, Loop: %d, PlayRate: %.2f"),
		*CurrentAnimationSequence->GetName(), bLoopAnimation ? 1 : 0, PlayRate);
}

void ATutorialActor::StopAnimation()
{
	if (!SkeletalMeshComponent)
	{
		return;
	}

	SkeletalMeshComponent->Stop();
	UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 애니메이션 정지"));
}

void ATutorialActor::SetWidgetVisibility(bool bVisible)
{
	bWidgetVisible = bVisible;
	
	if (WidgetComponent)
	{
		WidgetComponent->SetVisibility(bVisible);
	}

	UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 위젯 표시 상태 변경 - %d"), bVisible ? 1 : 0);
}

void ATutorialActor::UpdateWidget()
{
	if (!WidgetComponent)
	{
		return;
	}

	UUserWidget* Widget = WidgetComponent->GetWidget();
	if (!Widget)
	{
		return;
	}

	// TutorialWidget인 경우 멘트 이름 또는 인덱스로 표시
	if (UTutorialWidget* TutorialWidget = Cast<UTutorialWidget>(Widget))
	{
		// 위젯 크기에 비례하여 폰트 크기 설정
		TutorialWidget->SetFontSizeByWidgetScale(WidgetSize);
		
		// 우선순위: TutorialText > MessageName > MessageIndex >= 0
		// Detail 창에서 TutorialText를 직접 입력하는 방식이 가장 우선
		if (!TutorialText.IsEmpty())
		{
			// 직접 텍스트 설정 (Detail 창에서 입력한 텍스트 우선 사용)
			TutorialWidget->SetTutorialText(TutorialText);
			UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 위젯에 텍스트 직접 설정 완료 - Actor: %s, Text: %s"), 
				*GetName(), *TutorialText.ToString());
		}
		else if (MessageName != NAME_None)
		{
			// 멘트 이름으로 표시
			TutorialWidget->ShowMessageByName(MessageName);
			UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 위젯에 멘트 이름 설정 완료 - Actor: %s, Name: %s"), 
				*GetName(), *MessageName.ToString());
		}
		else if (MessageIndex >= 0)
		{
			// 멘트 인덱스로 표시
			TutorialWidget->ShowMessage(MessageIndex);
			UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 위젯에 멘트 인덱스 설정 완료 - Actor: %s, Index: %d"), 
				*GetName(), MessageIndex);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TutorialActor] 표시할 멘트가 없습니다 - Actor: %s, MessageIndex: %d, MessageName: %s"), 
				*GetName(), MessageIndex, *MessageName.ToString());
		}
		return;
	}

	// 일반 위젯인 경우 기존 방식 사용
	UFunction* SetTextFunction = Widget->FindFunction(FName("SetTutorialText"));
	if (SetTextFunction)
	{
		Widget->ProcessEvent(SetTextFunction, &TutorialText);
		UE_LOG(LogTemp, Verbose, TEXT("[TutorialActor] 위젯에 텍스트 설정 완료 - Text: %s"), *TutorialText.ToString());
	}
	else
	{
		// 함수가 없는 경우, Blueprint에서 위젯 클래스에 SetTutorialText 함수를 구현해야 함
		UE_LOG(LogTemp, Warning, TEXT("[TutorialActor] 위젯에 SetTutorialText 함수가 없습니다. Blueprint 위젯 클래스에 SetTutorialText(FText InText) 함수를 구현하거나 TutorialWidget을 사용하세요."));
	}
}

#if WITH_EDITOR
void ATutorialActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// MemberProperty를 사용하여 정확한 프로퍼티 확인
	const FProperty* ChangedProperty = PropertyChangedEvent.MemberProperty;
	if (!ChangedProperty)
	{
		return;
	}

	const FName PropertyName = ChangedProperty->GetFName();
	
	if (!WidgetComponent)
	{
		return;
	}

	// WidgetSize 또는 WidgetOffset 변경 시 위젯 크기와 위치 즉시 업데이트
	// 이 함수는 현재 인스턴스(this)에 대해서만 호출되므로 각 인스턴스가 독립적으로 업데이트됨
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ATutorialActor, WidgetSize) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ATutorialActor, WidgetOffset))
	{
		// 현재 인스턴스의 WidgetSize와 WidgetOffset 값 사용
		// 위젯 크기 설정 (기본 크기: 512x256, WidgetSize로 배율 조절)
		WidgetComponent->SetDrawSize(FVector2D(512.0f * WidgetSize, 256.0f * WidgetSize));
		
		// 위젯 위치 설정 (현재 인스턴스의 WidgetOffset 사용)
		WidgetComponent->SetRelativeLocation(WidgetOffset);
		
		// 위젯이 생성되어 있으면 폰트 크기도 업데이트
		if (UUserWidget* Widget = WidgetComponent->GetWidget())
		{
			if (UTutorialWidget* TutorialWidget = Cast<UTutorialWidget>(Widget))
			{
				TutorialWidget->SetFontSizeByWidgetScale(WidgetSize);
			}
		}
		
		UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 에디터에서 위젯 크기/위치 변경 - Actor: %s (Instance: %p), WidgetSize: %.2f, WidgetOffset: (%.1f, %.1f, %.1f)"), 
			*GetName(), this, WidgetSize, WidgetOffset.X, WidgetOffset.Y, WidgetOffset.Z);
	}
	
	// MessageIndex, MessageName, TutorialText 변경 시 위젯 텍스트 업데이트
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ATutorialActor, MessageIndex) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ATutorialActor, MessageName) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ATutorialActor, TutorialText))
	{
		// 위젯 클래스가 설정되어 있으면 위젯 생성 시도
		if (TutorialWidgetClass && !WidgetComponent->GetWidget())
		{
			WidgetComponent->SetWidgetClass(TutorialWidgetClass);
			WidgetComponent->InitWidget();
		}

		// 위젯이 생성되어 있으면 즉시 업데이트
		if (UUserWidget* Widget = WidgetComponent->GetWidget())
		{
			UpdateWidget();
			UE_LOG(LogTemp, Log, TEXT("[TutorialActor] 에디터에서 프로퍼티 변경 감지 - Actor: %s, MessageIndex: %d, MessageName: %s"), 
				*GetName(), MessageIndex, *MessageName.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TutorialActor] 위젯이 생성되지 않았습니다. TutorialWidgetClass가 설정되어 있는지 확인하세요."));
		}
	}
}
#endif

void ATutorialActor::UpdateWidgetRotation()
{
	if (!WidgetComponent || !bWidgetFacePlayer)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 플레이어 컨트롤러 가져오기
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		return;
	}

	APawn* PlayerPawn = PlayerController->GetPawn();
	if (!PlayerPawn)
	{
		return;
	}

	// 위젯 위치
	FVector WidgetLocation = WidgetComponent->GetComponentLocation();
	
	// 플레이어 위치
	FVector PlayerLocation = PlayerPawn->GetActorLocation();
	
	// 플레이어를 바라보는 방향 계산
	FVector DirectionToPlayer = (PlayerLocation - WidgetLocation).GetSafeNormal();
	
	// 위젯이 플레이어를 바라보도록 회전 설정
	FRotator NewRotation = DirectionToPlayer.Rotation();
	
	// 위젯은 일반적으로 Y축을 중심으로 회전 (Billboard 효과)
	// Z축 회전만 사용하여 플레이어를 바라보도록
	FRotator CurrentRotation = WidgetComponent->GetComponentRotation();
	NewRotation.Pitch = CurrentRotation.Pitch;
	NewRotation.Roll = CurrentRotation.Roll;
	
	WidgetComponent->SetWorldRotation(NewRotation);
}

