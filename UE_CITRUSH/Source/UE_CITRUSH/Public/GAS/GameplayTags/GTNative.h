#pragma once
#include "NativeGameplayTags.h"

/** CitRush 공통 GameplayTag 네임스페이스 */
namespace CitRushTags
{
    //=================================================================
    // State Tags - 공통 상태 (모든 역할이 사용)
    //=================================================================
    namespace State
    {
        /** 생존 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Alive);      // State.Alive
        /** 사망 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead);       // State.Dead
    }

    //=================================================================
    // Event Tags - 공통 이벤트
    //=================================================================
    namespace Event
    {
        namespace Result
        {
            /** 성공 이벤트 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Success);   // Event.Result.Success
            /** 실패 이벤트 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fail);      // Event.Result.Fail
        }

        namespace Gameplay
        {
            /** 게임 시작 이벤트 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Start);  // Event.Gameplay.Start
            /** 게임 종료 이벤트 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(End);    // Event.Gameplay.End
        }
    }

    //=================================================================
    // GameplayCue Tags - 공통 Cue
    //=================================================================
    namespace Cue
    {
        namespace Gameplay
        {
            /** 게임 시작 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Start);     // GameplayCue.Gameplay.Start
            /** 게임 종료 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(End);       // GameplayCue.Gameplay.End
        }
    }
}
