// Fill out your copyright notice in the Description page of Project Settings.


#include "CommenderSystem/VendingItemActor.h"
#include "CommenderSystem/AVendingMachineBase.h"
#include "SkeletonTreeBuilder.h"


AVendingItemActor::AVendingItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;
	
	MeshRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRoot"));
	MeshRoot->SetupAttachment(RootComp);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(MeshRoot);

	MeshComp->SetSimulatePhysics(false);
	
	// 충돌 프로필 설정 (PhysicsActor는 기본적으로 WorldStatic과 Block)
	MeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	// 캐릭터(Pawn)와 충돌 무시
	MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	
	// 바닥(WorldStatic)과 충돌 명시적 설정
	MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	
	MeshComp->OnComponentHit.AddDynamic(this, &AVendingItemActor::OnHit);
}

void AVendingItemActor::InitItem(FName InItemId, int32 InCoin, int32 InSlotIndex, int32 InStackIndex)
{
	ItemId      = InItemId;
	Coin        = InCoin;
	SlotIndex   = InSlotIndex;
	StackIndex  = InStackIndex;
}

void AVendingItemActor::StartDispense(const FVector& DropPointWorld)
{
	MeshComp->SetSimulatePhysics(true);
	const FVector Dir = (DropPointWorld - GetActorLocation()).GetSafeNormal();
	MeshComp->AddImpulse(Dir * 50.f, NAME_None, true);
}

void AVendingItemActor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 바닥 충돌 감지
	if (!bHasHitGround && Hit.ImpactNormal.Z > 0.7f)
	{
		bHasHitGround = true;
		if (VendingMachine)
			VendingMachine->OnItemHitGround(this);
	}
}
