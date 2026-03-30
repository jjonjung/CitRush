#pragma once
#include "NativeGameplayTags.h"

/** Racer 전용 GameplayTag 네임스페이스 */
namespace CitRushTags::Racer
{
    //=================================================================
    // Racer.Ability - 레이서 기본 능력
    //=================================================================
    namespace Ability
    {
        /** 아이템 사용 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseItem);    // Racer.Ability.UseItem
        /** 아이템 교체 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwapItem);   // Racer.Ability.SwapItem
        /** 이동 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move);       // Racer.Ability.Move
        /** 브레이크 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brake);      // Racer.Ability.Brake
        /** 드리프트 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Drift);      // Racer.Ability.Drift
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Boost);      // Racer.Ability.Boost
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SideBrake);  // Racer.Ability.SideBrake
    }

    namespace Type
    {
        /** 공격형 타입 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Offensive);  // Racer.Type.Offensive
        /** 방어형 타입 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Defensive);  // Racer.Type.Defensive
        /** 유틸리티형 타입 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Utility);    // Racer.Type.Utility
    }
    
    //=================================================================
    // Racer.Item - 레이서 아이템
    //=================================================================
    namespace Item
    {
        // Native Items
        namespace Native
        {
            /** 회복 아이템 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Heal);       // Racer.Item.Native.Heal
            /** 연료 충전 아이템 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Refuel);     // Racer.Item.Native.Refuel
        }

        // Offensive Items
        namespace Offensive
        {
            /** 속도 증가 아이템 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpeedBoost); // Racer.Item.Offensive.SpeedBoost
            /** 지뢰 아이템 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mine);       // Racer.Item.Offensive.Mine
        }

        // Defensive Items
        namespace Defensive
        {
            /** 보호막 아이템 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shield);     // Racer.Item.Defensive.Shield
        }

        // Utility Items
        namespace Utility
        {
            /** 대시 아이템 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);       // Racer.Item.Utility.Dash
            /** 미끼 아이템 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Decoy);      // Racer.Item.Utility.Decoy
            /** 스캔 아이템 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Scan);       // Racer.Item.Utility.Scan
        }

        // Slot Tags
        namespace Slot
        {
            /** 전방 슬롯 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Front);      // Racer.Item.Slot.Front
            /** 후방 슬롯 */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Back);       // Racer.Item.Slot.Back
        }
    }

    //=================================================================
    // Racer.State - 레이서 전용 상태
    //=================================================================
    namespace State
    {
        /** 아이템 사용 중 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UsingItem);      // Racer.State.UsingItem
        /** 아이템 교체 중 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Swapping);       // Racer.State.Swapping
        /** 아이템 사용 가능 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(CanUseItem);     // Racer.State.CanUseItem
        /** 아이템 교체 가능 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(CanSwapItem);    // Racer.State.CanSwapItem
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Boosting);       // Racer.State.Boosting
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SideBraking);    // Racer.State.SideBraking
    }

    //=================================================================
    // Racer.Event - 레이서 이벤트
    //=================================================================
    namespace Event
    {
        /** 아이템 수신 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemReceived);   // Racer.Event.ItemReceived
        /** 아이템 사용 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUsed);       // Racer.Event.ItemUsed
        /** 아이템 교체 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemSwapped);    // Racer.Event.ItemSwapped
        /** 아이템 파괴 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemDestroyed);  // Racer.Event.ItemDestroyed
        /** 공격당함 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacked);       // Racer.Event.Attacked
    }

    //=================================================================
    // Racer.Cue - 레이서 Cue
    //=================================================================
    namespace Cue
    {
        /** 아이템 수신 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemReceived);   // Racer.Cue.ItemReceived
        /** 아이템 사용 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUsed);       // Racer.Cue.ItemUsed
        /** 아이템 교체 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemSwapped);    // Racer.Cue.ItemSwapped

        namespace Effect
        {
            /** 속도 증가 효과 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpeedBoost); // Racer.Cue.Effect.SpeedBoost
            /** 보호막 효과 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shield);     // Racer.Cue.Effect.Shield
            /** 회복 효과 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Heal);       // Racer.Cue.Effect.Heal
            /** 대시 효과 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dash);       // Racer.Cue.Effect.Dash
            /** 스캔 효과 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Scan);       // Racer.Cue.Effect.Scan
            /** 미끼 효과 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Decoy);      // Racer.Cue.Effect.Decoy
            /** 브레이크 효과 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brake);      // Racer.Cue.Effect.Brake
            /** 드리프트 효과 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Drift);      // Racer.Cue.Effect.Drift
            /** 데미지 효과 Cue */
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage);     // Racer.Cue.Effect.Damage
        }
    }

    //=================================================================
    // Racer.Input - 레이서 입력
    //=================================================================
    namespace Input
    {
        /** 이동 입력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move);           // Racer.Input.Move
        /** 시점 입력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Look);           // Racer.Input.Look
        /** 브레이크 입력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brake);          // Racer.Input.Brake
        /** 아이템 사용 입력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseItem);        // Racer.Input.UseItem
        /** 아이템 교체 입력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwapItem);       // Racer.Input.SwapItem
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Boost);          // Racer.Input.Boost
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SideBrake);      // Racer.Input.SideBrake
    }
}
