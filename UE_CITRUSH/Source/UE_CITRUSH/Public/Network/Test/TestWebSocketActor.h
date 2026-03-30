// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestWebSocketActor.generated.h"

class UVoiceWebSocketComponent;

/**
 * WebSocket STT 테스트 Actor. VoiceWebSocketComponent 기능 검증용
 */
UCLASS()
class UE_CITRUSH_API ATestWebSocketActor : public AActor
{
	GENERATED_BODY()

public:
	/** 생성자 */
	ATestWebSocketActor();

protected:
	/** 게임 시작 시 호출 */
	virtual void BeginPlay() override;

	/** STT Transcript 수신 콜백 */
	UFUNCTION()
	void OnTranscript(FString Text, float Confidence);

	/** WebSocket 연결 성공 콜백 */
	UFUNCTION()
	void OnWSConnected();

public:
	/** 테스트용 VoiceWebSocket Component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	TObjectPtr<UVoiceWebSocketComponent> voiceWebSocketComponent;	
};
