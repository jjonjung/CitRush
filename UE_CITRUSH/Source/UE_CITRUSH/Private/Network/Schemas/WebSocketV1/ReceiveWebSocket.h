// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "ReceiveWebSocket.generated.h"

/**
 * AI Agent WebSocket API v1.1 - Receive Structures
 *
 * 이 파일은 AI 서버로부터 수신할 WebSocket 메시지 구조체를 정의합니다.
 * Protocol 문서: AgentLog/Protocol.md, AgentLog/NetworkModule.md
 *
 * WebSocket 엔드포인트:
 * - ws://{server}:8000/ws/v1/overseer/stt
 *
 * 수신 데이터:
 * 1. PING 메시지 (JSON Text 프레임) - Heartbeat
 * 2. STT 결과 메시지 (JSON Text 프레임) - 선택적
 * 3. 연결 종료 메시지
 */

// ============================================================================
// Heartbeat PING Message (JSON)
// ============================================================================

/**
 * Heartbeat PING 메시지
 *
 * 서버가 30초 동안 오디오 프레임을 받지 못하면 PING 메시지를 전송합니다.
 * 클라이언트는 5초 이내에 PONG으로 응답해야 합니다.
 * 무응답 시 10초 후 서버가 연결을 종료합니다.
 *
 * JSON Text 프레임으로 수신합니다.
 */
USTRUCT(BlueprintType)
struct FWebSocketPingMessage
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): "ping" */
	UPROPERTY(BlueprintReadOnly)
	FString type;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString timestamp;

	/** PING 메시지인지 확인 */
	bool IsPing() const
	{
		return type.Equals(TEXT("ping"), ESearchCase::IgnoreCase);
	}
};

// ============================================================================
// STT Transcript Message (JSON) - 선택적
// ============================================================================

/**
 * STT 인식 결과 메시지
 *
 * AI 서버가 음성을 텍스트로 변환한 결과를 전송합니다.
 * 이 메시지는 선택적이며, 주로 디버깅/로그 용도로 사용됩니다.
 *
 * JSON Text 프레임으로 수신합니다.
 */
USTRUCT(BlueprintType)
struct FWebSocketSTTTranscript
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): "transcript" */
	UPROPERTY(BlueprintReadOnly)
	FString type;

	/** 인식된 텍스트 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString text;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString timestamp;

	/** 신뢰도 0~1 (필수) */
	UPROPERTY(BlueprintReadOnly)
	float confidence = 0.0f;

	/** STT 결과 메시지인지 확인 */
	bool IsTranscript() const
	{
		return type.Equals(TEXT("transcript"), ESearchCase::IgnoreCase);
	}

	/** 신뢰도가 임계값 이상인지 확인 */
	bool IsConfidentEnough(float Threshold = 0.7f) const
	{
		return confidence >= Threshold;
	}
};

// ============================================================================
// Partial STT Result Message (JSON) - 선택적
// ============================================================================

/**
 * STT 부분 인식 결과 메시지 (중간 결과)
 *
 * 최종 결과가 나오기 전 중간 인식 결과를 전송합니다.
 * 이 메시지는 선택적이며, 구현에 따라 제공되지 않을 수 있습니다.
 *
 * JSON Text 프레임으로 수신합니다.
 */
USTRUCT(BlueprintType)
struct FWebSocketSTTPartial
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): "partial" */
	UPROPERTY(BlueprintReadOnly)
	FString type;

	/** 부분 인식 텍스트 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString text;

	/** 타임스탬프 ISO 8601 (필수) */
	UPROPERTY(BlueprintReadOnly)
	FString timestamp;

	/** 부분 결과 메시지인지 확인 */
	bool IsPartial() const
	{
		return type.Equals(TEXT("partial"), ESearchCase::IgnoreCase);
	}
};

// ============================================================================
// WebSocket Close Message
// ============================================================================

/**
 * WebSocket 연결 종료 정보
 *
 * WebSocket 연결이 종료될 때의 상태 정보입니다.
 * OnClosed 콜백에서 전달됩니다.
 */
USTRUCT(BlueprintType)
struct FWebSocketCloseInfo
{
	GENERATED_BODY()

	/** 종료 상태 코드 */
	UPROPERTY(BlueprintReadOnly)
	int32 StatusCode = 0;

	/** 종료 사유 */
	UPROPERTY(BlueprintReadOnly)
	FString Reason;

	/** 정상 종료 여부 */
	UPROPERTY(BlueprintReadOnly)
	bool bWasClean = false;

	/** 정상 종료인지 확인 */
	bool IsNormalClosure() const
	{
		return bWasClean && StatusCode == 1000;  // 1000 = Normal Closure
	}

	/** 재연결이 필요한지 확인 */
	bool ShouldReconnect() const
	{
		// 정상 종료가 아니거나, Going Away(1001) 또는 Protocol Error(1002)인 경우
		return !bWasClean || StatusCode == 1001 || StatusCode == 1002;
	}
};

// ============================================================================
// WebSocket Error Message
// ============================================================================

/**
 * WebSocket 연결 오류 정보
 *
 * WebSocket 연결 중 발생한 오류 정보입니다.
 * OnConnectionError 콜백에서 전달됩니다.
 */
USTRUCT(BlueprintType)
struct FWebSocketErrorInfo
{
	GENERATED_BODY()

	/** 오류 메시지 */
	UPROPERTY(BlueprintReadOnly)
	FString Error;

	/** 오류 발생 시각 */
	UPROPERTY(BlueprintReadOnly)
	FString Timestamp;

	FWebSocketErrorInfo()
	{
		Timestamp = FDateTime::UtcNow().ToIso8601();
	}

	/** 타임아웃 오류인지 확인 */
	bool IsTimeout() const
	{
		return Error.Contains(TEXT("timeout"), ESearchCase::IgnoreCase);
	}

	/** 네트워크 오류인지 확인 */
	bool IsNetworkError() const
	{
		return Error.Contains(TEXT("network"), ESearchCase::IgnoreCase) ||
		       Error.Contains(TEXT("connection"), ESearchCase::IgnoreCase);
	}
};

// ============================================================================
// Generic WebSocket Message (메시지 타입 판별용)
// ============================================================================

/**
 * 범용 WebSocket 메시지
 *
 * 수신한 JSON 메시지의 타입을 먼저 판별하기 위한 구조체입니다.
 * type 필드를 확인한 후 적절한 구조체로 파싱합니다.
 */
USTRUCT(BlueprintType)
struct FWebSocketGenericMessage
{
	GENERATED_BODY()

	/** 메시지 타입 (필수): ping, pong, transcript, partial 등 */
	UPROPERTY(BlueprintReadOnly)
	FString type;

	/** 메시지 타입 열거형 */
	enum class EMessageType : uint8
	{
		Unknown,
		Ping,
		Pong,
		Transcript,
		Partial,
		Error
	};

	/** 메시지 타입 반환 */
	EMessageType GetMessageType() const
	{
		if (type.Equals(TEXT("ping"), ESearchCase::IgnoreCase))
			return EMessageType::Ping;
		else if (type.Equals(TEXT("pong"), ESearchCase::IgnoreCase))
			return EMessageType::Pong;
		else if (type.Equals(TEXT("transcript"), ESearchCase::IgnoreCase))
			return EMessageType::Transcript;
		else if (type.Equals(TEXT("partial"), ESearchCase::IgnoreCase))
			return EMessageType::Partial;
		else if (type.Equals(TEXT("error"), ESearchCase::IgnoreCase))
			return EMessageType::Error;
		else
			return EMessageType::Unknown;
	}
};

// ============================================================================
// Utility Functions (유틸리티)
// ============================================================================

/**
 * WebSocket Receive 유틸리티 클래스
 *
 * WebSocket 수신 관련 헬퍼 함수들입니다.
 */
class FWebSocketReceiveUtils
{
public:
	/**
	 * JSON 문자열에서 메시지 타입 추출
	 * @param JsonString - JSON 문자열
	 * @return 메시지 타입 (Unknown이면 파싱 실패)
	 */
	static FWebSocketGenericMessage::EMessageType GetMessageType(const FString& JsonString)
	{
		FWebSocketGenericMessage GenericMsg;
		if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &GenericMsg, 0, 0))
		{
			return GenericMsg.GetMessageType();
		}
		return FWebSocketGenericMessage::EMessageType::Unknown;
	}

	/**
	 * PING 메시지 파싱
	 * @param JsonString - JSON 문자열
	 * @param OutMessage - 파싱된 메시지
	 * @return 성공 여부
	 */
	static bool ParsePingMessage(const FString& JsonString, FWebSocketPingMessage& OutMessage)
	{
		return FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &OutMessage, 0, 0);
	}

	/**
	 * STT Transcript 메시지 파싱
	 * @param JsonString - JSON 문자열
	 * @param OutMessage - 파싱된 메시지
	 * @return 성공 여부
	 */
	static bool ParseTranscriptMessage(const FString& JsonString, FWebSocketSTTTranscript& OutMessage)
	{
		return FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &OutMessage, 0, 0);
	}

	/**
	 * STT Partial 메시지 파싱
	 * @param JsonString - JSON 문자열
	 * @param OutMessage - 파싱된 메시지
	 * @return 성공 여부
	 */
	static bool ParsePartialMessage(const FString& JsonString, FWebSocketSTTPartial& OutMessage)
	{
		return FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &OutMessage, 0, 0);
	}

	/**
	 * WebSocket 메시지 자동 파싱 및 처리
	 *
	 * @param JsonString - 수신한 JSON 문자열
	 * @param OnPingReceived - PING 수신 콜백
	 * @param OnTranscriptReceived - Transcript 수신 콜백
	 * @param OnPartialReceived - Partial 수신 콜백 (선택)
	 * @return 파싱 성공 여부
	 */
	static bool AutoParseAndHandle(
		const FString& JsonString,
		TFunction<void(const FWebSocketPingMessage&)> OnPingReceived,
		TFunction<void(const FWebSocketSTTTranscript&)> OnTranscriptReceived,
		TFunction<void(const FWebSocketSTTPartial&)> OnPartialReceived = nullptr
	)
	{
		FWebSocketGenericMessage::EMessageType MsgType = GetMessageType(JsonString);

		switch (MsgType)
		{
		case FWebSocketGenericMessage::EMessageType::Ping:
			{
				FWebSocketPingMessage PingMsg;
				if (ParsePingMessage(JsonString, PingMsg))
				{
					if (OnPingReceived)
						OnPingReceived(PingMsg);
					return true;
				}
				break;
			}

		case FWebSocketGenericMessage::EMessageType::Transcript:
			{
				FWebSocketSTTTranscript TranscriptMsg;
				if (ParseTranscriptMessage(JsonString, TranscriptMsg))
				{
					if (OnTranscriptReceived)
						OnTranscriptReceived(TranscriptMsg);
					return true;
				}
				break;
			}

		case FWebSocketGenericMessage::EMessageType::Partial:
			{
				FWebSocketSTTPartial PartialMsg;
				if (OnPartialReceived && ParsePartialMessage(JsonString, PartialMsg))
				{
					OnPartialReceived(PartialMsg);
					return true;
				}
				break;
			}

		default:
			break;
		}

		return false;
	}
};

// ============================================================================
// WebSocket Status Codes (참고용)
// ============================================================================

/**
 * WebSocket 표준 종료 코드
 *
 * RFC 6455 표준 종료 상태 코드입니다.
 * 참고: https://tools.ietf.org/html/rfc6455#section-7.4.1
 */
namespace EWebSocketCloseCode
{
	enum Type
	{
		/** 1000: Normal Closure - 정상 종료 */
		NormalClosure = 1000,

		/** 1001: Going Away - 서버/클라이언트 종료 */
		GoingAway = 1001,

		/** 1002: Protocol Error - 프로토콜 오류 */
		ProtocolError = 1002,

		/** 1003: Unsupported Data - 지원하지 않는 데이터 */
		UnsupportedData = 1003,

		/** 1006: Abnormal Closure - 비정상 종료 (코드 없음) */
		AbnormalClosure = 1006,

		/** 1007: Invalid Frame Payload Data - 잘못된 페이로드 */
		InvalidPayload = 1007,

		/** 1008: Policy Violation - 정책 위반 */
		PolicyViolation = 1008,

		/** 1009: Message Too Big - 메시지 크기 초과 */
		MessageTooBig = 1009,

		/** 1011: Internal Server Error - 서버 내부 오류 */
		InternalError = 1011,

		/** 1015: TLS Handshake Failure - TLS 핸드셰이크 실패 */
		TLSHandshakeFailed = 1015
	};
}
