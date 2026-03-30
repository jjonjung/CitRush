#include "Subsystems/BaseAISubsystem.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

DEFINE_LOG_CATEGORY_CLASS(UBaseAISubsystem, LogBaseAI)

UBaseAISubsystem::UBaseAISubsystem()
	: bIsConnected(false)
	, ReconnectAttemptCount(0)
{
	// Config 파일에서 로드되므로 기본값만 설정 (폴백용)
	/*if (ServerURL.IsEmpty())
	{
		ServerURL = TEXT("http://34.64.141.47");
	}
	if (ServerPort == 0)
	{
		ServerPort = 8000;
	}
	if (RequestTimeout == 0.0f)
	{
		RequestTimeout = 10.0f;
	}
	if (ReconnectInterval == 0.0f)
	{
		ReconnectInterval = 10.0f;
	}
	if (MaxReconnectAttempts == 0)
	{
		MaxReconnectAttempts = 5;
	}*/

	// INI 파일 경로
	const FString IniFile = FPaths::ProjectConfigDir() + TEXT("DefaultGame.ini");
	const FString Section = TEXT("/Script/UE_CITRUSH.BaseAISubsystem");

	//GConfig->GetString(*Section, TEXT("ServerURL"), ServerURL, IniFile);
	ServerURL = TEXT("http://34.64.186.153");
	GConfig->GetInt   (*Section, TEXT("ServerPort"), ServerPort, IniFile);
	GConfig->GetFloat (*Section, TEXT("RequestTimeout"), RequestTimeout, IniFile);
	GConfig->GetFloat (*Section, TEXT("ReconnectInterval"), ReconnectInterval, IniFile);
	GConfig->GetInt   (*Section, TEXT("MaxReconnectAttempts"), MaxReconnectAttempts, IniFile);

	// 값이 없거나 유효하지 않은 경우 기본값으로 fallback
	if (ServerURL.IsEmpty())
	{
		ServerURL = TEXT("http://34.64.186.153");
	}

	if (ServerPort <= 0)  // 포트는 0 이하일 수 없음
	{
		ServerPort = 8000;
	}

	if (RequestTimeout <= 0.0f)
	{
		RequestTimeout = 10.0f;  // INI에 10.0이 있으니 여기에도 맞춤
	}

	if (ReconnectInterval <= 0.0f)
	{
		ReconnectInterval = 10.0f;
	}

	if (MaxReconnectAttempts <= 0)
	{
		MaxReconnectAttempts = 5;
	}
	UE_LOG(LogTemp, Log, TEXT("Final ServerURL = %s"), *ServerURL);
	UE_LOG(LogTemp, Log, TEXT("Final ServerPort = %d"), ServerPort);
	UE_LOG(LogTemp, Log, TEXT("Final RequestTimeout = %f"), RequestTimeout);
	UE_LOG(LogTemp, Log, TEXT("Final ReconnectInterval = %f"), ReconnectInterval);
	UE_LOG(LogTemp, Log, TEXT("Final MaxReconnectAttempts = %d"), MaxReconnectAttempts);
}

void UBaseAISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] Initializing - Server: %s:%d"), *ServerURL, ServerPort);
	// 환경변수에서 서버 URL 읽기
	FString EnvServerURL = FPlatformMisc::GetEnvironmentVariable(TEXT("UE_AI_SERVER_URL"));
	if (!EnvServerURL.IsEmpty())
	{
		ServerURL = EnvServerURL;
		UE_LOG(LogBaseAI, Warning, TEXT("[BaseAISubsystem] ServerURL loaded from ENVIRONMENT: %s"), *ServerURL);
	}
	ConnectToServer();
}

void UBaseAISubsystem::Deinitialize()
{
	UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] Deinitializing"));

	// 연결 해제
	DisconnectFromServer();

	// 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReconnectTimer);
	}

	Super::Deinitialize();
}

FString UBaseAISubsystem::GetServerURL() const
{
	return ServerPort > 0
		       ? FString::Printf(TEXT("%s:%d"), *ServerURL, ServerPort)
		       : ServerURL;
}

void UBaseAISubsystem::SendPostRequest(const FString& Endpoint, const FString& JsonBody,
                                       TFunction<void(FHttpResponsePtr, bool)> OnComplete)
{
	FHttpModule& HttpModule = FHttpModule::Get();
	TSharedRef<IHttpRequest> HttpRequest = HttpModule.CreateRequest();

	// URL:SendPostRequest
	const FString URL = ServerPort > 0
		                    ? FString::Printf(TEXT("%s:%d%s"), *ServerURL, ServerPort, *Endpoint)
		                    : FString::Printf(TEXT("%s%s"), *ServerURL, *Endpoint);

	// ========== 서버 전송 데이터 로그 저장 ==========
	SaveRequestLog(Endpoint, URL, JsonBody);

	// ========== 서버 전송 JSON 데이터 Output Log 출력 ==========
	//UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] POST Request JSON Body:\n%s"), *JsonBody);

	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(JsonBody);
	HttpRequest->SetContentAsString(JsonBody);
	HttpRequest->SetTimeout(RequestTimeout);

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			OnComplete(Response, bWasSuccessful);
		}
	);

	if (HttpRequest->ProcessRequest())
	{
		UE_LOG(LogBaseAI, Verbose, TEXT("[BaseAISubsystem] POST Request Sent: %s"), *URL);
	}
	else
	{
		UE_LOG(LogBaseAI, Error, TEXT("[BaseAISubsystem] Failed to send POST request: %s"), *URL);
		OnComplete(nullptr, false);
	}
}

void UBaseAISubsystem::SendGetRequest(const FString& Endpoint,
                                      TFunction<void(FHttpResponsePtr, bool)> OnComplete)
{
	FHttpModule& HttpModule = FHttpModule::Get();
	TSharedRef<IHttpRequest> HttpRequest = HttpModule.CreateRequest();

	// URL:SendGetRequest
	const FString URL = ServerPort > 0
		                    ? FString::Printf(TEXT("%s:%d%s"), *ServerURL, ServerPort, *Endpoint)
		                    : FString::Printf(TEXT("%s%s"), *ServerURL, *Endpoint);

	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetTimeout(RequestTimeout);

	UE_LOG(LogBaseAI, Error, TEXT("[BaseAISubsystem] 1"));

	// Subsystem을 WeakPtr로 캡처 (안전성)
	TWeakObjectPtr<UBaseAISubsystem> WeakThis(this);

	HttpRequest->OnProcessRequestComplete().BindLambda(
	[WeakThis](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
			if (!bWasSuccessful || !Response.IsValid())
			{
				return;
			}

			const FString ResponseString = Response->GetContentAsString();

			TArray<TSharedPtr<FJsonValue>> JsonArray;
			TSharedRef<TJsonReader<>> Reader =
				TJsonReaderFactory<>::Create(ResponseString);

			if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
			{
				return;
			}

			TSharedPtr<FJsonObject> RootObject = JsonArray[0]->AsObject();
			if (!RootObject.IsValid())
			{
				return;
			}

			const TSharedPtr<FJsonObject> BrainCamData =
				RootObject->GetObjectField(TEXT("brain_cam_data"));

			const TSharedPtr<FJsonObject> Perception =
				BrainCamData->GetObjectField(TEXT("perception"));

			FString Summary =
				Perception->GetStringField(TEXT("summary"));

			const TSharedPtr<FJsonObject> Decision =
				BrainCamData->GetObjectField(TEXT("decision"));

			FString FinalChoice =
				Decision->GetStringField(TEXT("final_choice"));

			// Subsystem이 유효한지 확인 후 델리게이트 브로드캐스트
			if (WeakThis.IsValid())
			{
				WeakThis->OnBrainCamDataReceived.Broadcast(Summary, FinalChoice, TArray<FString>());
			}

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					5.0f,
					FColor::Green,
					FString::Printf(TEXT("[brain_cam 상황 인식]: %s"), *Summary)
				);

				GEngine->AddOnScreenDebugMessage(
					-1,
					5.0f,
					FColor(255, 105, 180),
					FString::Printf(TEXT("[전략카드 기반 규칙 선택 최종 결정]: %s"), *FinalChoice)
				);
			}
		}
	);


	if (HttpRequest->ProcessRequest())
	{
		UE_LOG(LogBaseAI, Verbose, TEXT("[BaseAISubsystem] GET Request Sent: %s"), *URL);
	}
	else
	{
		UE_LOG(LogBaseAI, Error, TEXT("[BaseAISubsystem] Failed to send GET request: %s"), *URL);
		OnComplete(nullptr, false);
	}
}

void UBaseAISubsystem::HandleResponse(FHttpResponsePtr Response, bool bWasSuccessful,
                                      TFunction<void(const FString&, int32)> OnSuccess)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogBaseAI, Error, TEXT("[BaseAISubsystem] HTTP Request Failed - No response"));

		// 재연결 시도
		if (!bIsConnected)
		{
			AttemptReconnect();
		}

		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString Content = Response->GetContentAsString();

	if (StatusCode == 200)// 성공 응답 (200)
	{
		if (!bIsConnected)
		{
			SetConnected(true);
			ReconnectAttemptCount = 0;
			UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] Connected to AI Server 연결 성공"));
		}

		OnSuccess(Content, StatusCode);
		return;
	}

	UE_LOG(LogBaseAI, Error, TEXT("[BaseAISubsystem] HTTP Error %d: %s"), StatusCode, *Content);
}

void UBaseAISubsystem::SaveRequestLog(const FString& Endpoint, const FString& URL, const FString& JsonBody)
{
	// testlog 디렉토리 경로 (프로젝트 디렉토리 기준 상대경로)
	const FString LogDirectory = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("../testlog")));

	// 디렉토리 생성 (없으면)
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*LogDirectory))
	{
		if (!PlatformFile.CreateDirectoryTree(*LogDirectory))
		{
			UE_LOG(LogBaseAI, Error, TEXT("[BaseAISubsystem] Failed to create log directory: %s"), *LogDirectory);
			return;
		}
	}

	// 타임스탬프 생성 (YYYYMMDD_HHMMSS)
	const FDateTime Now = FDateTime::Now();
	const FString Timestamp = FString::Printf(TEXT("%04d%02d%02d_%02d%02d%02d"),
		Now.GetYear(), Now.GetMonth(), Now.GetDay(),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond());

	// 파일명 생성
	const FString FileName = FString::Printf(TEXT("request_%s.json"), *Timestamp);
	const FString FilePath = FPaths::Combine(LogDirectory, FileName);

	// JSON 객체 생성
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("timestamp"), Now.ToString(TEXT("%Y-%m-%d %H:%M:%S")));
	JsonObject->SetStringField(TEXT("endpoint"), Endpoint);
	JsonObject->SetStringField(TEXT("url"), URL);

	// JsonBody를 파싱해서 객체로 저장 (가독성 향상)
	TSharedPtr<FJsonObject> BodyObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonBody);
	if (FJsonSerializer::Deserialize(Reader, BodyObject) && BodyObject.IsValid())
	{
		JsonObject->SetObjectField(TEXT("request_body"), BodyObject);
	}
	else
	{
		// 파싱 실패 시 문자열로 저장
		JsonObject->SetStringField(TEXT("request_body_raw"), JsonBody);
	}

	// JSON을 문자열로 변환 (Pretty Print)
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		// 파일 저장
		if (FFileHelper::SaveStringToFile(OutputString, *FilePath))
		{
			UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] Request log saved: %s"), *FilePath);

			// 최신 5개만 유지하도록 오래된 로그 파일 삭제
			CleanupOldLogFiles(LogDirectory, 5);
		}
		else
		{
			UE_LOG(LogBaseAI, Error, TEXT("[BaseAISubsystem] Failed to save request log: %s"), *FilePath);
		}
	}
}

void UBaseAISubsystem::CleanupOldLogFiles(const FString& LogDirectory, int32 MaxLogCount)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 디렉토리의 모든 request_*.json 파일 찾기
	TArray<FString> LogFiles;
	PlatformFile.FindFiles(LogFiles, *LogDirectory, TEXT(".json"));

	// request_로 시작하는 파일만 필터링
	TArray<FString> RequestLogFiles;
	for (const FString& FileName : LogFiles)
	{
		FString FullPath = FPaths::Combine(LogDirectory, FileName);
		if (FileName.StartsWith(TEXT("request_")))
		{
			RequestLogFiles.Add(FullPath);
		}
	}

	// 파일 개수가 MaxLogCount 이하면 삭제 불필요
	if (RequestLogFiles.Num() <= MaxLogCount)
	{
		return;
	}

	// 파일을 생성 시간 기준으로 정렬 (최신 파일이 앞에 오도록)
	RequestLogFiles.Sort([&PlatformFile](const FString& A, const FString& B)
	{
		FDateTime TimeA = PlatformFile.GetTimeStamp(*A);
		FDateTime TimeB = PlatformFile.GetTimeStamp(*B);
		return TimeA > TimeB; // 내림차순 (최신이 앞)
	});

	// 최신 MaxLogCount개를 제외한 나머지 삭제
	for (int32 i = MaxLogCount; i < RequestLogFiles.Num(); ++i)
	{
		if (PlatformFile.DeleteFile(*RequestLogFiles[i]))
		{
			UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] Deleted old log file: %s"), *RequestLogFiles[i]);
		}
		else
		{
			UE_LOG(LogBaseAI, Warning, TEXT("[BaseAISubsystem] Failed to delete old log file: %s"), *RequestLogFiles[i]);
		}
	}
}

// 서버 연결 시도
// 실제 연결은 첫 요청 시 자동으로 이루어짐
// 여기서는 준비 상태만 설정
void UBaseAISubsystem::ConnectToServer()
{
	UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] Attempting to connect to AI Server: %s"),
	       *GetServerURL());

	// 연결 상태 초기화
	SetConnected(false);
	ReconnectAttemptCount = 0;
}

// 기존 연결 해제
void UBaseAISubsystem::DisconnectFromServer()
{
	if (bIsConnected)
	{
		UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] Disconnecting from AI Server"));
		SetConnected(false);
	}
}

void UBaseAISubsystem::AttemptReconnect()
{
	if (ReconnectAttemptCount >= MaxReconnectAttempts)
	{
		UE_LOG(LogBaseAI, Error,
		       TEXT("[BaseAISubsystem] Max reconnect attempts reached (%d). Giving up."),
		       MaxReconnectAttempts);
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	ReconnectAttemptCount++;

	UE_LOG(LogBaseAI, Warning,
	       TEXT("[BaseAISubsystem] Attempting reconnect (%d/%d) in %.1f seconds..."),
	       ReconnectAttemptCount, MaxReconnectAttempts, ReconnectInterval);

	// 재연결 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		ReconnectTimer,
		[this]()
		{
			UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] Reconnecting..."));
			ConnectToServer();
		},
		ReconnectInterval,
		false
	);
}

void UBaseAISubsystem::SetConnected(bool bNewConnected)
{
	if (bIsConnected == bNewConnected) return;
	bIsConnected = bNewConnected;
	OnConnectionChanged.Broadcast(bNewConnected);
}

void UBaseAISubsystem::SetServerSettings(const FString& NewServerURL, int32 NewServerPort)
{
	UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] SetServerSettings - Old: %s:%d, New: %s:%d"),
		*ServerURL, ServerPort, *NewServerURL, NewServerPort);

	// 유효성 검증
	if (NewServerURL.IsEmpty())
	{
		UE_LOG(LogBaseAI, Error, TEXT("[BaseAISubsystem] SetServerSettings failed - Empty URL"));
		return;
	}

	if (NewServerPort <= 0 || NewServerPort > 65535)
	{
		UE_LOG(LogBaseAI, Error, TEXT("[BaseAISubsystem] SetServerSettings failed - Invalid port: %d"), NewServerPort);
		return;
	}

	DisconnectFromServer();

	ServerURL = NewServerURL;
	ServerPort = NewServerPort;

	UE_LOG(LogBaseAI, Warning, TEXT("[BaseAISubsystem] Server settings changed - New: %s:%d"), *ServerURL, ServerPort);
	SaveConfig();
	ConnectToServer(); 
}

void UBaseAISubsystem::ResetServerSettings()
{
	UE_LOG(LogBaseAI, Log, TEXT("[BaseAISubsystem] ResetServerSettings called"));

	DisconnectFromServer(); 
	LoadConfig(); // Config 파일에서 다시 로드

	UE_LOG(LogBaseAI, Warning, TEXT("[BaseAISubsystem] Reset from config - URL: %s:%d"), *ServerURL, ServerPort);

	ConnectToServer(); 
}
