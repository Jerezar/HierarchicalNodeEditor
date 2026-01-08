#pragma once
#include "HierarchicalChildNode.h"
#include "HNE_Node_ChallengeObjectiveBase.generated.h"

UCLASS()
class UHNE_Node_ChallengeObjectiveBase : public UHierarchicalChildNode {
	GENERATED_BODY()
public:
	virtual TArray<FString> GetFieldNamesToCopy() const override;
};