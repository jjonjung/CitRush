// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/UIPawn.h"

#include "Camera/CameraComponent.h"


// Sets default values
AUIPawn::AUIPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(root);
	cameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	cameraComponent->SetupAttachment(GetRootComponent());
	cameraComponent->bUsePawnControlRotation = false;
}

// Called when the game starts or when spawned
void AUIPawn::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* pc = GetController<APlayerController>())
	{
		FInputModeUIOnly uiOnlyInputMode;
		uiOnlyInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockInFullscreen);
		pc->SetInputMode(uiOnlyInputMode);
		pc->SetShowMouseCursor(true);
	}
	
}

// Called to bind functionality to input
void AUIPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

