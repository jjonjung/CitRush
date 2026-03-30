#pragma once
#include "NativeGameplayTags.h"

/** Commander 전용 GameplayTag 네임스페이스 */
namespace CitRushTags::Commander
{
    //=================================================================
    // Commander.Ability - 지휘관 능력
    //=================================================================
    namespace Ability
    {
        /** 아이템 제공 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(GiveItem);       // Commander.Ability.GiveItem
        /** 아이템 사용 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseItem);       // Commander.Ability.UseItem
        /** 스캔 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Scan);           // Commander.Ability.Scan
        /** 핑 발령 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ping);           // Commander.Ability.Ping
        /** 전역 버프 능력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(GlobalBuff);     // Commander.Ability.GlobalBuff
    }

    //=================================================================
    // Commander.State - 지휘관 상태
    //=================================================================
    namespace State
    {
        /** 지휘 중 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Commanding);     // Commander.State.Commanding
        /** 아이템 제공 가능 상태 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(CanGiveItem);    // Commander.State.CanGiveItem
    }

    //=================================================================
    // Commander.Event - 지휘관 이벤트
    //=================================================================
    namespace Event
    {
        /** 아이템 제공 완료 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemGiven);      // Commander.Event.ItemGiven
        /** 아이템 사용 완료 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemUsed);    // Commander.Event.ItemUsed
        /** 핑 발령 이벤트 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(PingIssued);     // Commander.Event.PingIssued
    }

    //=================================================================
    // Commander.Cue - 지휘관 Cue
    //=================================================================
    namespace Cue
    {
        /** 아이템 제공 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(GiveItem);       // Commander.Cue.GiveItem
        /** 아이템 사용 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseItem);        // Commander.Cue.UseItem
        /** 핑 발령 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ping);           // Commander.Cue.Ping
        /** 스캔 Cue */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Scan);           // Commander.Cue.Scan
    }

    //=================================================================
    // Commander.Input - 지휘관 입력
    //=================================================================
    namespace Input
    {
        /** Racer 선택 입력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SelectRacer);    // Commander.Input.SelectRacer
        /** 아이템 제공 입력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(GiveItem);       // Commander.Input.GiveItem
        /** 아이템 사용 입력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseItem);        // Commander.Input.UseItem
        /** 핑 입력 */
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ping);           // Commander.Input.Ping
    }
}
