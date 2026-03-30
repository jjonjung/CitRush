#include "GAS/GameplayTags/GTNative.h"

namespace CitRushTags
{
    //=================================================================
    // State Tags
    //=================================================================
    namespace State
    {
        UE_DEFINE_GAMEPLAY_TAG(Alive, "State.Alive");
        UE_DEFINE_GAMEPLAY_TAG(Dead, "State.Dead");
    }
    
    //=================================================================
    // Event Tags
    //=================================================================
    namespace Event
    {
        namespace Result
        {
            UE_DEFINE_GAMEPLAY_TAG(Success, "Event.Result.Success"); 
            UE_DEFINE_GAMEPLAY_TAG(Fail, "Event.Result.Fail");     
        }
    
        namespace Gameplay
        {
            UE_DEFINE_GAMEPLAY_TAG(Start, "Event.Gameplay.Start");
            UE_DEFINE_GAMEPLAY_TAG(End, "Event.Gameplay.End");  
        }
    }
    
    //=================================================================
    // GameplayCue Tags
    //=================================================================
    namespace Cue
    {
        namespace Gameplay
        {
            UE_DEFINE_GAMEPLAY_TAG(Start, "GameplayCue.Gameplay.Start");
            UE_DEFINE_GAMEPLAY_TAG(End, "GameplayCue.Gameplay.End"); 
        }
    }
}
