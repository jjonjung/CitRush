#include "GAS/GameplayTags/GTCommander.h"

namespace CitRushTags
{
    namespace Commander
    {
        //=================================================================
        // Commander.Ability
        //=================================================================
        namespace Ability
        {
            UE_DEFINE_GAMEPLAY_TAG(GiveItem, "Commander.Ability.GiveItem");
            UE_DEFINE_GAMEPLAY_TAG(UseItem, "Commander.Ability.UseItem");
            UE_DEFINE_GAMEPLAY_TAG(Scan, "Commander.Ability.Scan");
            UE_DEFINE_GAMEPLAY_TAG(Ping, "Commander.Ability.Ping");
        }

        //=================================================================
        // Commander.State
        //=================================================================
        namespace State
        {
            UE_DEFINE_GAMEPLAY_TAG(Commanding, "Commander.State.Commanding");
            UE_DEFINE_GAMEPLAY_TAG(CanGiveItem, "Commander.State.CanGiveItem");
        }

        //=================================================================
        // Commander.Event
        //=================================================================
        namespace Event
        {
            UE_DEFINE_GAMEPLAY_TAG(ItemGiven, "Commander.Event.ItemGiven");
            UE_DEFINE_GAMEPLAY_TAG(ItemUsed, "Commander.Event.ItemUsed");
            UE_DEFINE_GAMEPLAY_TAG(PingIssued, "Commander.Event.PingIssued");
        }

        //=================================================================
        // Commander.Cue
        //=================================================================
        namespace Cue
        {
            UE_DEFINE_GAMEPLAY_TAG(GiveItem, "Commander.Cue.GiveItem");
            UE_DEFINE_GAMEPLAY_TAG(UseItem, "Commander.Cue.UseItem");
            UE_DEFINE_GAMEPLAY_TAG(Ping, "Commander.Cue.Ping");
            UE_DEFINE_GAMEPLAY_TAG(Scan, "Commander.Cue.Scan");
        }

        //=================================================================
        // Commander.Input
        //=================================================================
        namespace Input
        {
            UE_DEFINE_GAMEPLAY_TAG(SelectRacer, "Commander.Input.SelectRacer");
            UE_DEFINE_GAMEPLAY_TAG(GiveItem, "Commander.Input.GiveItem");
            UE_DEFINE_GAMEPLAY_TAG(UseItem, "Commander.Input.UseItem");
            UE_DEFINE_GAMEPLAY_TAG(Ping, "Commander.Input.Ping");
        }
    }
}