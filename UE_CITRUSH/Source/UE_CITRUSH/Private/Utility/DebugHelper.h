/**
 * 디버그 헬퍼 매크로
 *
 * - CITRUSH_LOG: 전역 로그 출력 (파일, 함수, 라인 포함)
 * - CITRUSH_MULTIPLAY_LOG: 멀티플레이 로그 (Local/Remote Role 포함)
 * - LOCAL_ROLE, REMOTE_ROLE: Actor의 Role 반환
 *
 * - DECLARE_CLASS_LOG :
 		Class 이름 (U/A 를 뺀 이름)을 입력 시 Log Category 생성 및 자체 Log 함수 3종 선언
 * - DEFINE_CLASS_LOG : 
		위에서 선언된 Log Category 할당 및 함수 3종 정의
 */

#pragma once
#include "GameFramework/GameStateBase.h"

namespace CitRushLogInternal
{
	inline void LogWithVerbosity(const FName& CategoryName, ELogVerbosity::Type Verbosity, const TCHAR* Fmt, va_list Args)
	{
		FMemMark Mark(FMemStack::Get());

		va_list ArgsCopy;
		va_copy(ArgsCopy, Args);
		int32 RequiredSize = FCString::GetVarArgs(nullptr, 0, Fmt, ArgsCopy) + 1;
		va_end(ArgsCopy);
		TCHAR* Buffer = new(FMemStack::Get()) TCHAR[RequiredSize];
		FCString::GetVarArgs(Buffer, RequiredSize, Fmt, Args);
		GLog->Log(CategoryName, Verbosity, Buffer);
	}
}

#define DECLARE_CLASS_LOG(ClassName) DECLARE_LOG_CATEGORY_CLASS(ClassName, Log, All)\
	void Log(const TCHAR fmt[], ...);\
	void LogWarning(const TCHAR fmt[], ...);\
	void LogError(const TCHAR fmt[], ...);

#define DEFINE_UCLASS_LOG(ClassName) \
	DEFINE_LOG_CATEGORY_CLASS(U##ClassName, ClassName); \
	\
	void U##ClassName::Log(const TCHAR* Fmt, ...) \
	{ \
		va_list Args; \
		va_start(Args, Fmt); \
		CitRushLogInternal::LogWithVerbosity(ClassName.GetCategoryName(), ELogVerbosity::Log, Fmt, Args); \
		va_end(Args); \
	} \
	void U##ClassName::LogWarning(const TCHAR* Fmt, ...) \
	{ \
		va_list Args; \
		va_start(Args, Fmt); \
		CitRushLogInternal::LogWithVerbosity(ClassName.GetCategoryName(), ELogVerbosity::Warning, Fmt, Args); \
		va_end(Args); \
	} \
	void U##ClassName::LogError(const TCHAR* Fmt, ...) \
	{ \
		va_list Args; \
		va_start(Args, Fmt); \
		CitRushLogInternal::LogWithVerbosity(ClassName.GetCategoryName(), ELogVerbosity::Error, Fmt, Args); \
		va_end(Args); \
	}

#define DEFINE_ACLASS_LOG(ClassName) \
	DEFINE_LOG_CATEGORY_CLASS(A##ClassName, ClassName); \
	\
	void A##ClassName::Log(const TCHAR* Fmt, ...) \
	{ \
		va_list Args; \
		va_start(Args, Fmt); \
		CitRushLogInternal::LogWithVerbosity(ClassName.GetCategoryName(), ELogVerbosity::Log, Fmt, Args); \
		va_end(Args); \
	} \
	void A##ClassName::LogWarning(const TCHAR* Fmt, ...) \
	{ \
		va_list Args; \
		va_start(Args, Fmt); \
		CitRushLogInternal::LogWithVerbosity(ClassName.GetCategoryName(), ELogVerbosity::Warning, Fmt, Args); \
		va_end(Args); \
	} \
	void A##ClassName::LogError(const TCHAR* Fmt, ...) \
	{ \
		va_list Args; \
		va_start(Args, Fmt); \
		CitRushLogInternal::LogWithVerbosity(ClassName.GetCategoryName(), ELogVerbosity::Error, Fmt, Args); \
		va_end(Args); \
	}

/*
 *RAII (Resource Acquisition Is Initialization)
 	* Stack에 TArray 객체, Heap에 TArray.Elements 할당
 	* 스코프 종료 → Stack 생명주기 완료 -> TArray 소멸자 자동 호출 → 힙 메모리 해제 시작
 #define DEFINE_CLASS_LOG(ClassName) DEFINE_LOG_CATEGORY_CLASS(U##ClassName, ClassName)\
 	void U##ClassName::Log(const TCHAR fmt[], ...)\
 	{\
 		va_list Args;\
 		va_start(Args, fmt);\
 		va_list ArgsCopy;\
 		va_copy(ArgsCopy, Args);\
 		int32 RequiredSize = FCString::GetVarArgs(nullptr, 0, fmt, ArgsCopy) + 1;\
 		va_end(ArgsCopy);\
 		TArray<TCHAR> Buffer;\
 		Buffer.SetNumUninitialized(RequiredSize);\
 		FCString::GetVarArgs(Buffer.GetData(), RequiredSize, fmt, Args);\
 		va_end(Args);\
 		GLog->Log(ClassName.GetCategoryName(), ELogVerbosity::Warning, Buffer.GetData());\
 	}
*/

/** VAM 로그 카테고리 선언 */
DECLARE_LOG_CATEGORY_EXTERN(VAM, Type::Warning, All)

/** Actor Local Role 반환 */
#define LOCAL_ROLE(actor) if (actor) if (actor->IsA(AActor::StaticClass())) (UEnum::GetValueAsString<ENetRole>(actor->GetLocalRole()))
/** Actor Remote Role 반환 */
#define REMOTE_ROLE(actor) if (actor) if (actor->IsA(AActor::StaticClass())) (UEnum::GetValueAsString<ENetRole>(actor->GetRemoteRole())))


#if defined(__FILEW__)
	#define __SCOPE__(str) FString::Printf(TEXT("%s : %hs, (%d)\n    %hs"), *FPaths::GetCleanFilename(__FILEW__), __FUNCTION__, __LINE__, str)
#elif defined(__FILE__)
	#define __SCOPE__(str) FString::Printf(TEXT("%s : %hs, (%d)\n    %hs"), *FPaths::GetCleanFilename(__FILE__), __FUNCTION__, __LINE__, str)
#else
	#define __SCOPE__(str) FString::Printf(TEXT("%s : %hs, (%d)\n    %hs"), *FPaths::Get, __FUNCTION__, __LINE__, str)
#endif

#define CITRUSH_LOG(status_log) UE_LOG(VAM, Warning, L"%s", *__SCOPE__(status_log))

#define CITRUSH_MULTIPLAY_LOG(RoleActor) UE_LOG(VAM, Warning, L"%s\n	Remote : %s", *__SCOPE__(LOCAL_ROLE(RoleActor)), *REMOTE_ROLE(RoleActor))

#define CITRUSH_TIME(Label) \
{\
	FName labelName = FName(Label);\
	FString netModeStr = TEXT("World Is Not Exist");\
	double time = -1.0; \
	if (GetWorld()) {\
		netModeStr = ToString(GetWorld()->GetNetMode());\
		time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();\
	} \
	UE_LOG(VAM, Warning, TEXT("\n	[%s]\n	{ NetMode: %s / Name: }\n	{ %hs(%d) : %.4f }") \
		, *labelName.ToString()\
		, *netModeStr\
		, __FUNCTION__, __LINE__, time \
	);\
}
