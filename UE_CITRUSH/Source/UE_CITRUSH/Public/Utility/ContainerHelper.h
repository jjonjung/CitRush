#pragma once

#include "Misc/MessageDialog.h"

#include "Algo/BinarySearch.h"
#include "Containers/Array.h"

/* Copy & Paste For Forward DECLARE
//#define FORWARD_DECLARATION_ARRAY \ // ignore this
	namespace ArrayHelper
	{
		template<typename ElementType, typename PropertyType>
		struct FPropertyPredicate;
	}
*/

template <typename T>
concept IsDerivedFromUObject = std::derived_from<T, UObject>;

template<IsDerivedFromUObject ElementType, typename PropertyType>
struct FPropertySortAsc
{
	FPropertySortAsc() = delete;
	FPropertySortAsc(FName propertyName) : propertyName(propertyName) {};
		
	FName propertyName;
	
	bool operator()(const ElementType* forward, const ElementType* backward) const;
		
};

namespace ArrayHelper
{
	// Predicate Must be Formed Binary Operator
	template<typename ElementType, typename Predicate>
	int32 SortedInsertUnique(TArray<ElementType>& array, const ElementType& newItem, Predicate predicate);
}

#include "Utility/ContainerHelper.inl"