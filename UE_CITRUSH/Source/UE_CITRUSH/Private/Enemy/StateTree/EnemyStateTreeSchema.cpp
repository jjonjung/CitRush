// EnemyStateTreeSchema.cpp

#include "Enemy/StateTree/EnemyStateTreeSchema.h"
#include "Enemy/PixelEnemy.h"
#include "Enemy/AiEnemy/AIDirectiveComponent.h"

UEnemyStateTreeSchema::UEnemyStateTreeSchema()
{
}

bool UEnemyStateTreeSchema::IsStructAllowed(const UScriptStruct* InScriptStruct) const
{
	return true;
}

bool UEnemyStateTreeSchema::IsClassAllowed(const UClass* InClass) const
{
	return true;
}

bool UEnemyStateTreeSchema::IsExternalItemAllowed(const UStruct& InStruct) const
{
	return true;
}
