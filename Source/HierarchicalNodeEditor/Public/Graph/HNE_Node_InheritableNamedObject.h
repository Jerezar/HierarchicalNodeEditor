#pragma once
#include "HierarchicalChildNode.h"
#include "HNE_Node_InheritableNamedObject.generated.h"

UCLASS()
class UHNE_Node_InheritableNamedObject : public UHierarchicalChildNode {
	GENERATED_BODY()
public:
	virtual TArray<FString> GetFieldNamesToCopy() const override;
};