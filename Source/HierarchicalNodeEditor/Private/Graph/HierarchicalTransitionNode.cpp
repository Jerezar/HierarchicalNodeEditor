#include "HierarchicalTransitionNode.h"
#include "Graph/HierarchicalNodeGraph.h"
#include "Graph/HierarchicalStateNode.h"
#include "ActorStateID.h"
#include "ActorState.h"

void UHierarchicalTransitionNode::SetUpOutputPins()
{
	UHierarchicalChildNode::SetUpOutputPins();

	FEdGraphPinType PinType;
	PinType.PinSubCategory = UHierarchicalGraphSchema::SC_StateTransition;
	PinType.ContainerType = EPinContainerType::None;

	this->CreatePin(
		EGPD_Output,
		PinType,
		FName("DestinationStateID")
	);
}

UObject* UHierarchicalTransitionNode::GetFinalizedAssetRecursive() const
{

	//No caching, 
	UObject* OutObject = UHierarchicalChildNode::GetFinalizedAssetRecursive();

	for (UEdGraphPin* Pin : Pins) {
		if (Pin->Direction != EGPD_Output) continue; // only consider output pins
		if (!Pin->LinkedTo.Num()) continue; //only consider pins with actual connections

		FEdGraphPinType PinType = Pin->PinType;

		if (PinType.PinSubCategory != UHierarchicalGraphSchema::SC_StateTransition) continue;

		FProperty* Property = InnerClass->FindPropertyByName(Pin->PinName);

		if (Property == nullptr) {

			UE_LOG(LogTemp, Warning, TEXT("Property %s does not exist"), *(Pin->PinName.ToString()))
			return nullptr;
		}

		if (PinType.ContainerType == EPinContainerType::Array) {
			TArray<FActorStateID> StateIDs;

			UEdGraphNode* ArrayNode = nullptr;

			//Get connected array node from pin
			if (Pin->LinkedTo.Num()) {
				UEdGraphPin* ConnectedPin = *Pin->LinkedTo.begin();
				ArrayNode = ConnectedPin->GetOwningNode();
			}

			if (ArrayNode == nullptr) continue;

			//get connected child nodes from array node and add respective finalized assets
			for (UEdGraphPin* ArrayPin : ArrayNode->Pins) {
				if (ArrayPin->Direction != EGPD_Output) continue;
				if (!ArrayPin->LinkedTo.Num()) continue;

				UEdGraphPin* ConnectedPin = *ArrayPin->LinkedTo.begin();
				UEdGraphNode* ChildNode = ConnectedPin->GetOwningNode();

				UHierarchicalStateNode* ChildAsState = Cast< UHierarchicalStateNode>(ChildNode);

				if (ChildAsState == nullptr) continue;

				UActorState* StateObject = Cast< UActorState>(ChildAsState->GetInnerObject());
				StateIDs.Add(StateObject->UniqueId);
			}

			TArray<FActorStateID>* ArrayPointer = Property->ContainerPtrToValuePtr<TArray<FActorStateID>>(OutObject);
			*ArrayPointer = StateIDs;
		}
		else {

			UEdGraphNode* ChildNode = nullptr;

			if (Pin->LinkedTo.Num()) {
				UEdGraphPin* ConnectedPin = *Pin->LinkedTo.begin();
				ChildNode = ConnectedPin->GetOwningNode();
			}

			if (ChildNode == nullptr) continue;

			UHierarchicalStateNode* ChildAsState = Cast< UHierarchicalStateNode>(ChildNode);

			if (ChildAsState == nullptr) continue;

			FActorStateID* StateIDPointer = Property->ContainerPtrToValuePtr<FActorStateID>(OutObject);

			UActorState* StateObject = Cast< UActorState>(ChildAsState->GetInnerObject());

			*StateIDPointer = StateObject->UniqueId;
		}
	}

	return OutObject;
}
