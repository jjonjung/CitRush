// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/AbilityTask/AT_FollowSpline.h"
#include "Enemy/SplinePathActor.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAT_FollowSpline* UAT_FollowSpline::FollowSplinePath(
	UGameplayAbility* OwningAbility,
	ASplinePathActor* SplineActor,
	float Speed,
	bool bLoop,
	bool bPingPong)
{
	UAT_FollowSpline* MyTask = NewAbilityTask<UAT_FollowSpline>(OwningAbility);

	MyTask->TargetSplineActor = SplineActor;
	MyTask->MovementSpeed = Speed;
	MyTask->bShouldLoop = bLoop;
	MyTask->bShouldPingPong = bPingPong;

	return MyTask;
}

void UAT_FollowSpline::Activate()
{
	Super::Activate();

	// Validation
	if (!IsValid(TargetSplineActor))
	{
		UE_LOG(LogTemp, Error, TEXT("AT_FollowSpline: Invalid Spline Actor"));
		OnInterrupted.Broadcast();
		EndTask();
		return;
	}

	SplineComponent = TargetSplineActor->GetSplineComponent();
	if (!IsValid(SplineComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("AT_FollowSpline: Spline Actor has no Spline Component"));
		OnInterrupted.Broadcast();
		EndTask();
		return;
	}

	// Initialize movement state
	SplineLength = SplineComponent->GetSplineLength();
	CurrentDistance = 0.0f;
	bMovingForward = true;

	// Actor 위치를 Spline 시작점으로 이동 (선택 사항)
	AActor* OwnerActor = GetAvatarActor();
	if (IsValid(OwnerActor))
	{
		const FVector StartLocation = SplineComponent->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
		// 필요하면 초기 위치 설정: OwnerActor->SetActorLocation(StartLocation);
	}

	// Enable Tick
	SetWaitingOnAvatar();
}

void UAT_FollowSpline::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!IsValid(SplineComponent))
	{
		OnInterrupted.Broadcast();
		EndTask();
		return;
	}

	AActor* OwnerActor = GetAvatarActor();
	if (!IsValid(OwnerActor))
	{
		OnInterrupted.Broadcast();
		EndTask();
		return;
	}

	UpdateMovement(DeltaTime);
}

void UAT_FollowSpline::UpdateMovement(float DeltaTime)
{
	AActor* OwnerActor = GetAvatarActor();
	if (!IsValid(OwnerActor) || !IsValid(SplineComponent))
	{
		return;
	}

	// Update distance along spline
	const float DeltaDistance = MovementSpeed * DeltaTime * (bMovingForward ? 1.0f : -1.0f);
	CurrentDistance += DeltaDistance;

	// Handle end reached
	if (HasReachedEnd())
	{
		HandleEndReached();
	}

	// Clamp distance
	CurrentDistance = FMath::Clamp(CurrentDistance, 0.0f, SplineLength);

	// Get target location and rotation from spline
	const FVector TargetLocation = SplineComponent->GetLocationAtDistanceAlongSpline(
		CurrentDistance, ESplineCoordinateSpace::World);

	const FRotator TargetRotation = SplineComponent->GetRotationAtDistanceAlongSpline(
		CurrentDistance, ESplineCoordinateSpace::World);

	// Interpolate to target location
	const FVector CurrentLocation = OwnerActor->GetActorLocation();
	const FVector NewLocation = FMath::VInterpTo(
		CurrentLocation,
		TargetLocation,
		DeltaTime,
		10.0f // Interp Speed
	);

	// Interpolate rotation
	const FRotator CurrentRotation = OwnerActor->GetActorRotation();
	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaTime,
		5.0f // Rotation Interp Speed
	);

	// Apply movement
	OwnerActor->SetActorLocation(NewLocation);
	OwnerActor->SetActorRotation(NewRotation);
}

bool UAT_FollowSpline::HasReachedEnd() const
{
	if (bMovingForward)
	{
		return CurrentDistance >= SplineLength;
	}
	else
	{
		return CurrentDistance <= 0.0f;
	}
}

void UAT_FollowSpline::HandleEndReached()
{
	if (bShouldPingPong)
	{
		// Reverse direction
		bMovingForward = !bMovingForward;
		CurrentDistance = FMath::Clamp(CurrentDistance, 0.0f, SplineLength);
	}
	else if (bShouldLoop)
	{
		// Reset to start
		CurrentDistance = 0.0f;
		bMovingForward = true;
	}
	else
	{
		// End task
		OnCompleted.Broadcast();
		EndTask();
	}
}

void UAT_FollowSpline::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
}
