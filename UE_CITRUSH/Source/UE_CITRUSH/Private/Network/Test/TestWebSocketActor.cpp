// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/Test/TestWebSocketActor.h"

#include "Network/VoiceWebSocketComponent.h"


// Sets default values
ATestWebSocketActor::ATestWebSocketActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	voiceWebSocketComponent = CreateDefaultSubobject<UVoiceWebSocketComponent>("TestWebSocket");
}

// Called when the game starts or when spawned
void ATestWebSocketActor::BeginPlay()
{
	Super::BeginPlay();

	voiceWebSocketComponent->OnConnected.AddDynamic(this, &ATestWebSocketActor::OnWSConnected);
	voiceWebSocketComponent->Connect();
	
	// 음성 전송 시작
	//voiceWebSocketComponent->StartTransmissionTo(ETargetRacer::Racer1);

}

void ATestWebSocketActor::OnTranscript(FString Text, float Confidence)
{
	UE_LOG(LogTemp, Warning, TEXT("Transcript : %s"), *Text);
}

void ATestWebSocketActor::OnWSConnected()
{
	UE_LOG(LogTemp, Warning, TEXT("Connected"));
	voiceWebSocketComponent->StartTransmissionTo(ETargetRacer::Racer1);
	
}
