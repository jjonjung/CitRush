// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/AbilitySystemComponent/BaseASC.h"

UBaseASC::UBaseASC()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicated(true);
}

void UBaseASC::BeginPlay()
{
	Super::BeginPlay();
}
