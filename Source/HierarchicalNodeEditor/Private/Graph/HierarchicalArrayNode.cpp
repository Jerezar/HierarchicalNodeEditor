
#include "HierarchicalArrayNode.h"
#include "Graph/HierarchicalNodeGraph.h"
#include "HierarchicalEditInterface.h"
#include "ToolMenu.h"
#include "HNE_Node.h"

UHierarchicalArrayNode::UHierarchicalArrayNode() : UHNE_Node()
{
	CreatePinUIAction = FUIAction(
		FExecuteAction::CreateLambda([this]() {this->CreateOutputPin(); })
	);

	DeletePinUIAction = FUIAction(
		FExecuteAction::CreateLambda([this]() {this->DeleteOutputPin(); })
	);
}

void UHierarchicalArrayNode::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	UHNE_Node::GetNodeContextMenuActions(Menu, Context);

	FToolMenuSection& Section = Menu->AddSection(TEXT("ArrayNodeActions"), FText::FromString(TEXT("Array Node Actions")));

	Section.AddMenuEntry(
		TEXT("AddPinEntry"),
		FText::FromString(TEXT("Add Pin")),
		FText::FromString(TEXT("Adds a new pin to the array.")),
		FSlateIcon(TEXT(""), TEXT("")),
		CreatePinUIAction
	);

	Section.AddMenuEntry(
		TEXT("DeletePinEntry"),
		FText::FromString(TEXT("Remove Pin")),
		FText::FromString(TEXT("Removes the last pin.")),
		FSlateIcon(),
		DeletePinUIAction
	);
}

void UHierarchicalArrayNode::InitializeNode()
{
	UHNE_Node::InitializeNode();

	FEdGraphPinType InputType(PinTypeTemplate);
	InputType.ContainerType = EPinContainerType::Array;

	this->CreatePin(
		EGPD_Input,
		InputType,
		NAME_None
	);

	CreateOutputPin();
	CreateOutputPin();
}


void UHierarchicalArrayNode::CreateOutputPin()
{
	FEdGraphPinType OutputType(PinTypeTemplate);
	OutputType.ContainerType = EPinContainerType::None;

	this->CreatePin(
		EGPD_Output,
		OutputType,
		NAME_None
	);

	this->GetGraph()->NotifyGraphChanged();
	this->GetGraph()->Modify();
}

void UHierarchicalArrayNode::DeleteOutputPin()
{
	UEdGraphPin* LastPin = this->GetPinAt(this->Pins.Num() - 1);
	if (LastPin->Direction == EGPD_Output) {
		this->RemovePin(LastPin);
	}

	this->GetGraph()->NotifyGraphChanged();
	this->GetGraph()->Modify();
}
