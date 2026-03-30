#pragma once

template<IsDerivedFromUObject ElementType, typename PropertyType>
bool FPropertySortAsc<ElementType, PropertyType>::operator()(const ElementType* forward, const ElementType* backward) const
{
	FProperty* Prop = ElementType::StaticClass()->FindPropertyByName(propertyName);
	if (!Prop) return false;
	UE_LOG(LogTemp, Warning, TEXT("Prop Index : %d"), Prop->GetIndexInOwner())
	
	const PropertyType& ValA = *Prop->ContainerPtrToValuePtr<PropertyType>(&forward);
	const PropertyType& ValB = *Prop->ContainerPtrToValuePtr<PropertyType>(&backward);
    
	return ValA <= ValB;
}

template <typename ElementType, typename Predicate>
int32 ArrayHelper::SortedInsertUnique(TArray<ElementType>& array, const ElementType& newItem, Predicate predicate)
{
	int32 index = array.Find(newItem);
	int32 newIndex = Algo::UpperBound(array, newItem, predicate);
	UE_LOG(LogTemp, Warning, TEXT("Array Already Index : %d, New Index : %d"), index, newIndex)
	if (index == newIndex) {return index;}
	
	if (index != INDEX_NONE)
	{
		if (array.IsValidIndex(newIndex))
		{
			array.Swap(index, newIndex);
		}
		else
		{
			return index;
		}
	}
	else
	{
		array.Insert(newItem, newIndex);
	}
	return newIndex;
}