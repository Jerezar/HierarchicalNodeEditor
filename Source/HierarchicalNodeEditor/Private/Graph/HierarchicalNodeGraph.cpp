#include "HierarchicalNodeGraph.h"
#include "UObject/UObjectIterator.h"
#include "HierarchicalEditInterface.h"
#include "HierarchicalChildNode.h"
#include "HierarchicalArrayNode.h"
#include "Graph/HNE_RerouteNode.h"
#include "Graph/HNE_GraphUtils.h"
#include "SGraphNodeKnot.h"
#include "EdGraphNode_Comment.h"

const TMap<FName, FLinearColor> PinTypeColorMap{
	{UHierarchicalGraphSchema::SC_ChildNode, FLinearColor(FColor::Cyan)},
	{UHierarchicalGraphSchema::SC_StateTransition, FLinearColor(FColor::Orange)}
};

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

	if (ContextMenuBuilder.FromPin != nullptr) {
		TSharedPtr< FNewRerouteNodeAction> NewArrayAction(
			new FNewRerouteNodeAction(
				FText::FromString(TEXT("Reroute")),
				FText::FromString(TEXT("Reroute Node")),
				FText::FromString(TEXT("Makes a new reroute node")),
				0
			)
		);

		ContextMenuBuilder.AddAction(NewArrayAction);
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

	TSharedPtr< FHNENewCommentAction> NewCommentAction(
		new FHNENewCommentAction(
			FText::FromString(TEXT("Comment")),
			FText::FromString(TEXT("Add comment")),
			FText::FromString(TEXT("Makes a new comment box")),
			0
		)
	);

	ContextMenuBuilder.AddAction(NewCommentAction);
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

FLinearColor UHierarchicalGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	const FLinearColor* PinTypeColor = PinTypeColorMap.Find(PinType.PinSubCategory);

	if (PinTypeColor != nullptr) return *PinTypeColor;

	return FLinearColor();
}

FConnectionDrawingPolicy* UHierarchicalGraphSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
	return new FHNE_ConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

void UHierarchicalGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	if (PinA == nullptr && PinB == nullptr) return;

	UEdGraphPin* InspectPin = (PinA == nullptr) ? PinA : PinB;

	UEdGraph* ParentGraph = InspectPin->GetOwningNode()->GetGraph();

	UHNE_RerouteNode* Result = NewObject< UHNE_RerouteNode >(ParentGraph);

	Result->NodePosX = GraphPosition.X - (Result->NodeHeight * 0.5);
	Result->NodePosY = GraphPosition.Y - (Result->NodeWidth * 0.5);

	ParentGraph->Modify();
	ParentGraph->AddNode(Result, true, true);

	Result->PinTypeTemplate = FEdGraphPinType(InspectPin->PinType);
	Result->InitializeNode();

	if (PinA != nullptr) {
		UEdGraphPin* TargetNode = Result->FindPin(NAME_None, (PinA->Direction == EGPD_Output) ? EGPD_Input : EGPD_Output);
		TryCreateConnection(PinA, TargetNode);
	}

	if (PinB != nullptr) {
		UEdGraphPin* TargetNode = Result->FindPin(NAME_None, (PinB->Direction == EGPD_Output) ? EGPD_Input : EGPD_Output);
		TryCreateConnection(PinB, TargetNode);
	}
}

UEdGraphPin* UHierarchicalGraphSchema::DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const
{
	UHierarchicalArrayNode* TargetAsArray = Cast< UHierarchicalArrayNode>(InTargetNode);

	if (TargetAsArray != nullptr) {
		return TargetAsArray->DropPin(InSourcePinName, InSourcePinType, InSourcePinDirection);
	}
	return nullptr;
}

bool UHierarchicalGraphSchema::SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const
{
	UHierarchicalArrayNode* TargetAsArray = Cast< UHierarchicalArrayNode>(InTargetNode);
	return (TargetAsArray != nullptr);
}

FNewChildNodeAction::FNewChildNodeAction()
{
}



UEdGraphNode* FNewChildNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	
	UClass* NodeClass = FHNE_GraphUtils::GetNodeClassForObjectClass(InnerClass);

	if (NodeClass == nullptr) return nullptr;

	UE_LOG(LogTemp, Warning, TEXT("Creating Node object"))
	UHierarchicalChildNode* Result = NewObject<UHierarchicalChildNode>(ParentGraph, NodeClass);

	Result->NodePosX = Location.X;
	Result->NodePosY = Location.Y;

	ParentGraph->Modify();
	ParentGraph->AddNode(Result, true, bSelectNewNode);

	Result->InnerClass = InnerClass;
	Result->InitializeNode();

	if (FromPin != nullptr && FromPin->Direction == EGPD_Output) {
		UEdGraphPin* ConnectionPin = FromPin;

		if (FromPin->PinType.ContainerType == EPinContainerType::Array) {
			UHierarchicalArrayNode* NewArray = NewObject< UHierarchicalArrayNode >(ParentGraph);

			NewArray->NodePosX = Location.X;
			NewArray->NodePosY = Location.Y;

			Result->NodePosX += 128;

			ParentGraph->Modify();
			ParentGraph->AddNode(NewArray, true, bSelectNewNode);

			NewArray->PinTypeTemplate = FEdGraphPinType(FromPin->PinType);
			NewArray->InitializeNode();
			NewArray->SetNumberOfOutPins(1);

			ConnectionPin = NewArray->Pins.Last();

			UEdGraphPin* ArrayInputPin = NewArray->FindPin(FName("Input"), EGPD_Input);
			ParentGraph->GetSchema()->TryCreateConnection(FromPin, ArrayInputPin);
		}

		UEdGraphPin* InputPin = Result->FindPin(FName("Parent"), EGPD_Input);
		ParentGraph->GetSchema()->TryCreateConnection(ConnectionPin, InputPin);
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
	Result->InitializeNode();

	if (FromPin != nullptr && FromPin->Direction == EGPD_Output) {
		UEdGraphPin* InputPin = Result->FindPin(FName("Input"), EGPD_Input);
		ParentGraph->GetSchema()->TryCreateConnection(FromPin, InputPin);
	}

	return Result;
}

FNewRerouteNodeAction::FNewRerouteNodeAction()
{
}

UEdGraphNode* FNewRerouteNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	
	UHNE_RerouteNode* Result = NewObject< UHNE_RerouteNode >(ParentGraph);

	Result->NodePosX = Location.X;
	Result->NodePosY = Location.Y;

	ParentGraph->Modify();
	ParentGraph->AddNode(Result, true, bSelectNewNode);

	Result->PinTypeTemplate = FEdGraphPinType(FromPin->PinType);
	Result->InitializeNode();

	if (FromPin != nullptr && FromPin->Direction == EGPD_Output) {
		UEdGraphPin* InputPin = Result->FindPin(NAME_None, EGPD_Input);
		ParentGraph->GetSchema()->TryCreateConnection(FromPin, InputPin);
	}

	return Result;
}

FHNE_ConnectionDrawingPolicy::FHNE_ConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	:FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements),
	Graph(InGraphObj)
{
}

void FHNE_ConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params)
{
	if (OutputPin == nullptr || InputPin == nullptr) return;

	FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

	UEdGraphPin* InspectPin = (OutputPin == nullptr) ? OutputPin : InputPin;
	
	FEdGraphPinType PinType = InspectPin->PinType;

	const FLinearColor* PinTypeColor = PinTypeColorMap.Find(PinType.PinSubCategory);

	if (PinTypeColor != nullptr) Params.WireColor = *PinTypeColor;

	if (PinType.PinSubCategory == UHierarchicalGraphSchema::SC_ChildNode) Params.WireThickness *= 1.1;
}

TSharedPtr<class SGraphNode> FHNE_NodeFactory::CreateNode(UEdGraphNode* InNode) const
{
	if (UHNE_RerouteNode* AsReroute = Cast< UHNE_RerouteNode>(InNode)) {
		return SNew(SGraphNodeKnot, AsReroute);
	}
	return nullptr;
}

FHNENewCommentAction::FHNENewCommentAction()
{

}

UEdGraphNode* FHNENewCommentAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	FGraphNodeCreator<UEdGraphNode_Comment> NodeCreator(*ParentGraph);
	UEdGraphNode_Comment* CommentNode = NodeCreator.CreateNode();

	CommentNode->NodePosX = Location.X;
	CommentNode->NodePosY = Location.Y;

	NodeCreator.Finalize();

	return CommentNode;
}
