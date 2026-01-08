#pragma once
#include "HierarchicalChildNode.h"
#include "HNE_Node_Challenge.generated.h"

UCLASS()
class UHNE_Node_Challenge : public UHierarchicalChildNode {
	GENERATED_BODY()
public:
	virtual TArray<FString> GetFieldNamesToCopy() const override;
};