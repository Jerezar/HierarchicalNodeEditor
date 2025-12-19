#pragma once 

#include "EdGraph/EdGraphNode.h"
#include "Framework/Commands/UIAction.h"
#include "HierarchicalArrayNode.generated.h"

UCLASS()
class UHierarchicalArrayNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitelType) const override { return FText(); }
	virtual FLinearColor GetNodeTitleColor() const override { return FLinearColor(0,0,0,0); }
	virtual FText GetTooltipText() { return FText::FromName(InnerClass->GetFName()); }
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;

public:
	void InitializeArrayNode();
	void CreateOutputPin();
	void DeleteOutputPin();

public:
	UPROPERTY(EditAnywhere)
	UClass* InnerClass = nullptr;

	FUIAction CreatePinUIAction;

	FUIAction DeletePinUIAction;
};