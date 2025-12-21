#pragma once 

#include "EdGraph/EdGraphNode.h"
#include "Framework/Commands/UIAction.h"
#include "HierarchicalArrayNode.generated.h"

UCLASS()
class UHierarchicalArrayNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitelType) const override { return FText::FromString("Array"); }
	virtual FLinearColor GetNodeTitleColor() const override { return FLinearColor(0,0,0,0); }
	virtual FText GetTooltipText() { return FText::FromString("Array Node"); }
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;

public:
	void InitializeArrayNode();
	void CreateOutputPin();
	void DeleteOutputPin();

public:

	FEdGraphPinType PinTypeTemplate;

	FUIAction CreatePinUIAction;

	FUIAction DeletePinUIAction;
};