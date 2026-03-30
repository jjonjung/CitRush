#include "GAS/GameplayTags/GTRacer.h"

namespace CitRushTags
{
    namespace Racer
    {
        //=================================================================
        // Racer.Ability
        //=================================================================
        namespace Ability
        {
            UE_DEFINE_GAMEPLAY_TAG(UseItem, "Racer.Ability.UseItem");
            UE_DEFINE_GAMEPLAY_TAG(SwapItem, "Racer.Ability.SwapItem");
            UE_DEFINE_GAMEPLAY_TAG(Move, "Racer.Ability.Move");
            UE_DEFINE_GAMEPLAY_TAG(Brake, "Racer.Ability.Brake");
            UE_DEFINE_GAMEPLAY_TAG(Boost, "Racer.Ability.Boost");
            UE_DEFINE_GAMEPLAY_TAG(SideBrake, "Racer.Ability.SideBrake");
        }
        
        namespace Type
        {
            UE_DEFINE_GAMEPLAY_TAG(Offensive, "Racer.Type.Offensive");
            UE_DEFINE_GAMEPLAY_TAG(Defensive, "Racer.Type.Defensive");
            UE_DEFINE_GAMEPLAY_TAG(Utility, "Racer.Type.Utility");
        }
        //=================================================================
        // Racer.Item
        //=================================================================
        namespace Item
        {
            namespace Native
            {
                UE_DEFINE_GAMEPLAY_TAG(Heal, "Racer.Item.Native.Heal");
                UE_DEFINE_GAMEPLAY_TAG(Refuel, "Racer.Item.Native.Refuel");
            }

            namespace Offensive
            {
                UE_DEFINE_GAMEPLAY_TAG(SpeedBoost, "Racer.Item.Offensive.SpeedBoost");
                UE_DEFINE_GAMEPLAY_TAG(Mine, "Racer.Item.Offensive.Mine");
            }

            namespace Defensive
            {
                UE_DEFINE_GAMEPLAY_TAG(Shield, "Racer.Item.Defensive.Shield");
            }

            namespace Utility
            {
                UE_DEFINE_GAMEPLAY_TAG(Dash, "Racer.Item.Utility.Dash");
                UE_DEFINE_GAMEPLAY_TAG(Decoy, "Racer.Item.Utility.Decoy");
                UE_DEFINE_GAMEPLAY_TAG(Scan, "Racer.Item.Utility.Scan");
            }

            namespace Slot
            {
                UE_DEFINE_GAMEPLAY_TAG(Front, "Racer.Item.Slot.Front");
                UE_DEFINE_GAMEPLAY_TAG(Back, "Racer.Item.Slot.Back");
            }
        }

        //=================================================================
        // Racer.State
        //=================================================================
        namespace State
        {
            UE_DEFINE_GAMEPLAY_TAG(UsingItem, "Racer.State.UsingItem");
            UE_DEFINE_GAMEPLAY_TAG(Swapping, "Racer.State.Swapping");
            UE_DEFINE_GAMEPLAY_TAG(CanUseItem, "Racer.State.CanUseItem");
            UE_DEFINE_GAMEPLAY_TAG(CanSwapItem, "Racer.State.CanSwapItem");
            UE_DEFINE_GAMEPLAY_TAG(Boosting, "Racer.State.Boosting");
            UE_DEFINE_GAMEPLAY_TAG(SideBraking, "Racer.State.SideBraking");
        }

        //=================================================================
        // Racer.Event
        //=================================================================
        namespace Event
        {
            UE_DEFINE_GAMEPLAY_TAG(ItemReceived, "Racer.Event.ItemReceived");
            UE_DEFINE_GAMEPLAY_TAG(ItemUsed, "Racer.Event.ItemUsed");
            UE_DEFINE_GAMEPLAY_TAG(ItemSwapped, "Racer.Event.ItemSwapped");
            UE_DEFINE_GAMEPLAY_TAG(ItemDestroyed, "Racer.Event.ItemDestroyed");
            UE_DEFINE_GAMEPLAY_TAG(Attacked, "Racer.Event.Attacked");
        }

        //=================================================================
        // Racer.Cue
        //=================================================================
        namespace Cue
        {
            UE_DEFINE_GAMEPLAY_TAG(ItemReceived, "Racer.Cue.ItemReceived");
            UE_DEFINE_GAMEPLAY_TAG(ItemUsed, "Racer.Cue.ItemUsed");
            UE_DEFINE_GAMEPLAY_TAG(ItemSwapped, "Racer.Cue.ItemSwapped");

            namespace Effect
            {
                UE_DEFINE_GAMEPLAY_TAG(SpeedBoost, "Racer.Cue.Effect.SpeedBoost");
                UE_DEFINE_GAMEPLAY_TAG(Shield, "Racer.Cue.Effect.Shield");
                UE_DEFINE_GAMEPLAY_TAG(Heal, "Racer.Cue.Effect.Heal");
                UE_DEFINE_GAMEPLAY_TAG(Dash, "Racer.Cue.Effect.Dash");
                UE_DEFINE_GAMEPLAY_TAG(Scan, "Racer.Cue.Effect.Scan");
                UE_DEFINE_GAMEPLAY_TAG(Decoy, "Racer.Cue.Effect.Decoy");
                UE_DEFINE_GAMEPLAY_TAG(Brake, "Racer.Cue.Effect.Brake");
                UE_DEFINE_GAMEPLAY_TAG(Drift, "Racer.Cue.Effect.Drift");
                UE_DEFINE_GAMEPLAY_TAG(Damage, "Racer.Cue.Effect.Damage");
            }
        }

        //=================================================================
        // Racer.Input
        //=================================================================
        namespace Input
        {
            UE_DEFINE_GAMEPLAY_TAG(Move, "Racer.Input.Move");
            UE_DEFINE_GAMEPLAY_TAG(Look, "Racer.Input.Look");
            UE_DEFINE_GAMEPLAY_TAG(Brake, "Racer.Input.Brake");
            UE_DEFINE_GAMEPLAY_TAG(UseItem, "Racer.Input.UseItem");
            UE_DEFINE_GAMEPLAY_TAG(SwapItem, "Racer.Input.SwapItem");
            UE_DEFINE_GAMEPLAY_TAG(Boost, "Racer.Input.Boost");
            UE_DEFINE_GAMEPLAY_TAG(SideBrake, "Racer.Input.SideBrake");
        }
    }
}