#pragma once
#include "CoreMinimal.h"
#include "HNE_NodeSerializer.generated.h"

UCLASS()
class UHNE_NodeSerializer : public UObject {
	GENERATED_BODY()
public:
	static bool CanSerializeNode(const UEdGraphNode* Node);
	static bool SerializeNodesToString(const TArray<UEdGraphNode*> SelectedNodesOrdered, FString& OutString);
	static void DeserializeNodesFromString(const FString InString, UEdGraph* ParentGraph, const FVector2D PasteLocation);

	static const FName NodeCopyMarker;
	static const FName NodeType;
};

const FName UHNE_NodeSerializer::NodeCopyMarker = FName("CopiedNodes");
const FName UHNE_NodeSerializer::NodeType = FName("NodeType");