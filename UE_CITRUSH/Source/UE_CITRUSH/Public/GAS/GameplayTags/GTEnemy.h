#pragma once
#include "NativeGameplayTags.h"

/** Enemy 전용 GameplayTag 네임스페이스 */
namespace CitRushTags::Enemy
{
    //=================================================================
    // Enemy.Ability - 적 능력
    //=================================================================
    namespace Ability
    {
        /** 공격 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);         // Enemy.Ability.Attack
        /** 특수 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Special);        // Enemy.Ability.Special
        /** 추격 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chase);          // Enemy.Ability.Chase
        /** 순찰 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Patrol);         // Enemy.Ability.Patrol
        /** 도망 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Escape);         // Enemy.Ability.Escape
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(FollowSpline);   // Enemy.Ability.FollowSpline
    }

    //=================================================================
    // Enemy.State - 적 상태
    //=================================================================
    namespace State
    {
        /** 대기 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Idle);           // Enemy.State.Idle
        /** 순찰 중 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Patrolling);     // Enemy.State.Patrolling
        /** 추격 중 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chasing);        // Enemy.State.Chasing
        /** 공격 중 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacking);      // Enemy.State.Attacking
        /** 도망 중 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Escaping);       // Enemy.State.Escaping
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(FollowingSpline);// Enemy.State.FollowingSpline
    }

    //=================================================================
    // Enemy.Event - 적 이벤트
    //=================================================================
    namespace Event
    {
        /** 타겟 감지 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetDetected); // Enemy.Event.TargetDetected
        /** 타겟 놓침 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(TargetLost);     // Enemy.Event.TargetLost

        namespace Collision
        {
            /** 전방 충돌 이벤트 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Front);     // Enemy.Event.Collision.Front
            /** 후방 충돌 이벤트 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Back);      // Enemy.Event.Collision.Back
        }
    }

    //=================================================================
    // Enemy.Cue - 적 Cue
    //=================================================================
    namespace Cue
    {
        /** 공격 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);         // Enemy.Cue.Attack
        /** 특수 능력 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Special);        // Enemy.Cue.Special
        /** 감지 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Detected);       // Enemy.Cue.Detected
        /** 데미지 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage);         // Enemy.Cue.Damage
        /** 데미지 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Earthquake);         // Enemy.Cue.Earthquake
    }
}
