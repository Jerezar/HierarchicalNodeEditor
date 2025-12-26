#include "HierarchicalNodeGraph.h"
#include "UObject/UObjectIterator.h"
#include "HierarchicalEditInterface.h"
#include "HierarchicalChildNode.h"
#include "HierarchicalArrayNode.h"
#include "ActorState.h"
#include "ActorStateTransition.h"
#include "SpawnTableLink.h"
#include "Graph/HierarchicalStateNode.h"
#include "Graph/HierarchicalTransitionNode.h"
#include "Graph/HierarchicalTableLinkNode.h"

void UHierarchicalGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{

	UClass* PinObjectClass = nullptr;
	bool bIsArrayOutput = false;

	//Setup Node actions for hierarchical assets

	if (ContextMenuBuilder.FromPin != nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("Trying to limit placable nodes by FromPin.SubCategoryObject"))
		FEdGraphPinType PinType = ContextMenuBuilder.FromPin->PinType;
		UObject* SubCategoryObject = PinType.PinSubCategoryObject.Get();

		PinObjectClass = Cast<UClass>(SubCategoryObject);

		bIsArrayOutput = PinType.ContainerType == EPinContainerType::Array && (ContextMenuBuilder.FromPin->Direction == EGPD_Output);
	}

	for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
	{
		if (ClassIterator->ImplementsInterface(UHierarchicalEditInterface::StaticClass()) && (ClassIterator->HasAnyClassFlags(CLASS_Abstract) == false)) {

			if (PinObjectClass != nullptr && !ClassIterator->IsChildOf(PinObjectClass)) continue;

			TSharedPtr< FNewChildNodeAction> NewNodeAction(
				new FNewChildNodeAction(
					FText::FromString(TEXT("Nodes")),
					FText::FromName(ClassIterator->GetFName()),
					FText::FromString(TEXT("Makes a new child node")),
					0
				)
			);

			NewNodeAction->InnerClass = *ClassIterator;

			//UE_LOG(LogTemp, Warning, TEXT("Action for %s"), *(ClassIterator->GetFName().ToString()));

			ContextMenuBuilder.AddAction(NewNodeAction);
		}
	}

	if (bIsArrayOutput) {
		TSharedPtr< FNewArrayNodeAction> NewArrayAction(
			new FNewArrayNodeAction(
				FText::FromString(TEXT("Arrays")),
				FText::FromString(TEXT("Array Node")),
				FText::FromString(TEXT("Makes a new array node")),
				0
			)
		);

		ContextMenuBuilder.AddAction(NewArrayAction);
	}
}

const FPinConnectionResponse UHierarchicalGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (A == nullptr || B == nullptr) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, "Must connect 2 pins to one another");

	if (A->Direction == B->Direction) {
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Can only connect input to output."));
	}

	bool bPinAOutput = (A->Direction == EGPD_Output);

	FEdGraphPinType PinTypeA = A->PinType;
	UObject* SubCategoryObjectA = PinTypeA.PinSubCategoryObject.Get();


	FEdGraphPinType PinTypeB = B->PinType;
	UObject* SubCategoryObjectB = PinTypeB.PinSubCategoryObject.Get();

	if (PinTypeA.PinSubCategory != PinTypeB.PinSubCategory || PinTypeA.ContainerType != PinTypeB.ContainerType) {
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Incompatible sockets."));
	}

	if ((PinTypeA.PinSubCategoryObject == nullptr) != (PinTypeB.PinSubCategoryObject == nullptr)) {
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Incompatible sockets."));
	}

	if (PinTypeA.PinSubCategoryObject != nullptr) {

		UClass* PinObjectClassA = Cast<UClass>(SubCategoryObjectA);
		UClass* PinObjectClassB = Cast<UClass>(SubCategoryObjectB);

		if ((bPinAOutput && !(PinObjectClassB->IsChildOf(PinObjectClassA))) || (!bPinAOutput && !(PinObjectClassA->IsChildOf(PinObjectClassB)))) {
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Input pin must represent the same class as the connecting output pin, or a child class thereof."));
		}
	}
	
	ECanCreateConnectionResponse Break = (bPinAOutput ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B);

	return FPinConnectionResponse(Break, "");
}

FNewChildNodeAction::FNewChildNodeAction()
{
}

TMap<UClass*, UClass*> AssetClassToNodeClass{
	{UActorState::StaticClass(), UHierarchicalStateNode::StaticClass()},
	{UActorStateTransition::StaticClass(), UHierarchicalTransitionNode::StaticClass()},
	{USpawnTableLink::StaticClass(), UHierarchicalTableLinkNode::StaticClass()}
};

UEdGraphNode* FNewChildNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	bool bHasClassNodeOverride = false;
	UClass** NodeClassPtr = nullptr;


	for (UClass* InnerClassIterator = InnerClass; InnerClassIterator != nullptr; InnerClassIterator = InnerClassIterator->GetSuperClass()) {

		NodeClassPtr = AssetClassToNodeClass.Find(InnerClassIterator);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *InnerClassIterator->GetName());
		if (NodeClassPtr != nullptr) {
			bHasClassNodeOverride = true;
			UE_LOG(LogTemp, Warning, TEXT("Has Nodeclass override"));
			break;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Check NodeCLass found"))
	UClass* NodeClass = bHasClassNodeOverride ? *NodeClassPtr : UHierarchicalChildNode::StaticClass();

	UE_LOG(LogTemp, Warning, TEXT("Creating Node object"))
	UHierarchicalChildNode* Result = NewObject<UHierarchicalChildNode>(ParentGraph, NodeClass);

	Result->NodePosX = Location.X;
	Result->NodePosY = Location.Y;

	ParentGraph->Modify();
	ParentGraph->AddNode(Result, true, bSelectNewNode);

	Result->InnerClass = InnerClass;
	Result->InitializeHierarchicalNode();

	if (FromPin != nullptr && FromPin->Direction == EGPD_Output) {
		UEdGraphPin* InputPin = Result->FindPin(FName("Parent"), EGPD_Input);
		ParentGraph->GetSchema()->TryCreateConnection(FromPin, InputPin);
	}

	return Result;
}

FNewArrayNodeAction::FNewArrayNodeAction()
{
}

UEdGraphNode* FNewArrayNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UHierarchicalArrayNode* Result = NewObject< UHierarchicalArrayNode >(ParentGraph);

	Result->NodePosX = Location.X;
	Result->NodePosY = Location.Y;

	ParentGraph->Modify();
	ParentGraph->AddNode(Result, true, bSelectNewNode);

	Result->PinTypeTemplate = FEdGraphPinType(FromPin->PinType);
	Result->InitializeArrayNode();

	if (FromPin != nullptr && FromPin->Direction == EGPD_Output) {
		UEdGraphPin* InputPin = Result->FindPin(NAME_None, EGPD_Input);
		ParentGraph->GetSchema()->TryCreateConnection(FromPin, InputPin);
	}

	return Result;
}
