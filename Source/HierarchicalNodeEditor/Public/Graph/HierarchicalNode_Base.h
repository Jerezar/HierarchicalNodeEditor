#pragma once 

#include "EdGraph/EdGraphNode.h"
#include "HierarchicalNode_Base.generated.h"

UCLASS()
class UHierarchicalNode_Base : public UEdGraphNode
{
	GENERATED_BODY()

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitelType) const override { return FText::FromName(InnerClass->GetFName()); }
	virtual FLinearColor GetNodeTitleColor() const override { return FLinearColor(FColor::Cyan); }
	virtual bool CanUserDeleteNode() const override { return true; }

public:
	void InitializeHierarchicalNode();
	UObject* GetInnerObject() const { return InnerObject; }
	virtual UObject* GetFinalizedAssetRecursive() const;

protected:
	virtual void SetUpInputPins() {};
	virtual void SetUpOutputPins();

public:
	UPROPERTY(EditAnywhere)
	UClass* InnerClass = nullptr;

private:
	UPROPERTY()
	UObject* InnerObject = nullptr;
};