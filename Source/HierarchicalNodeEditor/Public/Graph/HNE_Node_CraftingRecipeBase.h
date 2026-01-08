#pragma once
#include "HierarchicalChildNode.h"
#include "HNE_Node_CraftingRecipeBase.generated.h"

UCLASS()
class UHNE_Node_CraftingRecipeBase : public UHierarchicalChildNode {
	GENERATED_BODY()
public:
	virtual TArray<FString> GetFieldNamesToCopy() const override;
};