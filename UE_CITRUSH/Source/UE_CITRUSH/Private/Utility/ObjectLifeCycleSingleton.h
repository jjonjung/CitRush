#pragma once

// https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/FCoreDelegates
#include "Misc/CoreDelegates.h"

/**
 * 엔진 생명주기에 맞춰 자동 생성/소멸되는 싱글톤 템플릿 클래스
 */
template<class DerivedClass>
class TObjectLifeCycleSingleton
{
	/** 싱글톤 인스턴스 */
	static DerivedClass* Instance;

	/** 소멸 예정 플래그 */
	static bool MarkedPendingKill;

public:
	/** 싱글톤 인스턴스 반환. 엔진 종료 전까지 유효 */
	static DerivedClass* Get()
	{
		if (!GEngine || !IsInGameThread())
		{
			return nullptr;
		}
		if (MarkedPendingKill)
		{
			return nullptr;
		}
		
		if (!Instance)
		{
			Instance = new DerivedClass();
			
			FCoreDelegates::OnEnginePreExit.AddStatic(&TObjectLifeCycleSingleton::Destroy);
		}
		if (Instance && !Instance->PreventGetter())
		{
			return Instance;
		}
		
		return nullptr;
	}

	/** 인스턴스 소멸. OnEnginePreExit 델리게이트에서 호출 */
	static void Destroy()
	{
		MarkedPendingKill = true;
		if (Instance != nullptr)
		{
			delete Instance;
			Instance = nullptr;
		}
	}

	/** 복사 생성자 삭제 */
	explicit TObjectLifeCycleSingleton(const TObjectLifeCycleSingleton&) = delete;

	/** 이동 생성자 삭제 */
	explicit TObjectLifeCycleSingleton(TObjectLifeCycleSingleton&&) = delete;

	/** 파생 클래스 이동 생성자 삭제 */
	explicit TObjectLifeCycleSingleton<DerivedClass>(DerivedClass&&) = delete;

	/** 파생 클래스 복사 생성자 삭제 */
	explicit TObjectLifeCycleSingleton<DerivedClass>(const DerivedClass&) = delete;

	/** 복사 대입 연산자 삭제 */
	TObjectLifeCycleSingleton& operator=(const TObjectLifeCycleSingleton&) const = delete;

	/** 이동 대입 연산자 삭제 */
	TObjectLifeCycleSingleton& operator=(TObjectLifeCycleSingleton&&) const = delete;

	/** 파생 클래스 복사 대입 연산자 삭제 */
	TObjectLifeCycleSingleton& operator=(const DerivedClass&) const = delete;

	/** 파생 클래스 이동 대입 연산자 삭제 */
	TObjectLifeCycleSingleton& operator=(DerivedClass&&) const = delete;

protected:
	/** 기본 생성자 (protected) */
	TObjectLifeCycleSingleton() = default;

	/** 가상 소멸자 (protected) */
	virtual ~TObjectLifeCycleSingleton() = default;

	/** Get() 호출 방지 여부. true 반환 시 Get()이 nullptr 반환 */
	virtual bool PreventGetter() const = 0;
};

template<class DerivedClass>
DerivedClass* TObjectLifeCycleSingleton<DerivedClass>::Instance = nullptr;
template<class DerivedClass>
bool TObjectLifeCycleSingleton<DerivedClass>::MarkedPendingKill = false;