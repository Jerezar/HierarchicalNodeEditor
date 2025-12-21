#include "HierarchicalTransitionNode.h"
#include "Graph/HierarchicalNodeGraph.h"

void UHierarchicalTransitionNode::SetUpOutputPins()
{
	UHierarchicalChildNode::SetUpOutputPins();

	FEdGraphPinType PinType;
	PinType.PinSubCategory = UHierarchicalGraphSchema::SC_StateTransition;
	PinType.ContainerType = EPinContainerType::None;

	this->CreatePin(
		EGPD_Output,
		PinType,
		FName("StateID")
	);
}
