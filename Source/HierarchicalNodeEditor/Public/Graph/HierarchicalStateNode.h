#pragma once

#include "HierarchicalChildNode.h"
#include "HierarchicalStateNode.generated.h"

UCLASS()
class UHierarchicalStateNode : public UHierarchicalChildNode {
	GENERATED_BODY()

public:
	virtual void SetUpInputPins() override;
	virtual void SetUpOutputPins() override;
};