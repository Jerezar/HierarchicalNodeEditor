#pragma once

#include "CoreMinimal.h"
#include "HierarchicalEditAsset.generated.h"

UCLASS(BlueprintType)
class HIERARCHICALNODEEDITOR_API UHierarchicalEditAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class UEdGraph* WorkingGraph;
};
