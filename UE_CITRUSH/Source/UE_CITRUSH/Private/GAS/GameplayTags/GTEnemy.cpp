#include "GAS/GameplayTags/GTEnemy.h"

namespace CitRushTags
{
	namespace Enemy
	{
		//=================================================================
		// Enemy.Ability
		//=================================================================
		namespace Ability
		{
			UE_DEFINE_GAMEPLAY_TAG(Attack, "Enemy.Ability.Attack");
			UE_DEFINE_GAMEPLAY_TAG(Special, "Enemy.Ability.Special");
			UE_DEFINE_GAMEPLAY_TAG(Chase, "Enemy.Ability.Chase");
			UE_DEFINE_GAMEPLAY_TAG(Patrol, "Enemy.Ability.Patrol");
			UE_DEFINE_GAMEPLAY_TAG(Escape, "Enemy.Ability.Escape");
			UE_DEFINE_GAMEPLAY_TAG(FollowSpline, "Enemy.Ability.FollowSpline");
		}

		//=================================================================
		// Enemy.State
		//=================================================================
		namespace State
		{
			UE_DEFINE_GAMEPLAY_TAG(Idle, "Enemy.State.Idle");
			UE_DEFINE_GAMEPLAY_TAG(Patrolling, "Enemy.State.Patrolling");
			UE_DEFINE_GAMEPLAY_TAG(Chasing, "Enemy.State.Chasing");
			UE_DEFINE_GAMEPLAY_TAG(Attacking, "Enemy.State.Attacking");
			UE_DEFINE_GAMEPLAY_TAG(Escaping, "Enemy.State.Escaping");
			UE_DEFINE_GAMEPLAY_TAG(FollowingSpline, "Enemy.State.FollowingSpline");
		}

		//=================================================================
		// Enemy.Event
		//=================================================================
		namespace Event
		{
			UE_DEFINE_GAMEPLAY_TAG(TargetDetected, "Enemy.Event.TargetDetected");
			UE_DEFINE_GAMEPLAY_TAG(TargetLost, "Enemy.Event.TargetLost");
			
			namespace Collision
			{
				UE_DEFINE_GAMEPLAY_TAG(Front, "Enemy.Event.Collision.Front");
				UE_DEFINE_GAMEPLAY_TAG(Back, "Enemy.Event.Collision.Back"); 
			}
		}

		//=================================================================
		// Enemy.Cue
		//=================================================================
		namespace Cue
		{
			UE_DEFINE_GAMEPLAY_TAG(Attack, "Enemy.Cue.Attack");
			UE_DEFINE_GAMEPLAY_TAG(Special, "Enemy.Cue.Special");
			UE_DEFINE_GAMEPLAY_TAG(Detected, "Enemy.Cue.Detected");
			UE_DEFINE_GAMEPLAY_TAG(Damage, "Enemy.Cue.Damage");
			UE_DEFINE_GAMEPLAY_TAG(Earthquake, "Enemy.Cue.Earthquake");
		}
	}
}
