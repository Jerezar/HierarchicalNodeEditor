#include "HNE_NodeSerializer.h"

#include "Graph/HNE_Node.h"
#include "Graph/HierarchicalArrayNode.h"
#include "Graph/HierarchicalNode_Base.h"
#include "Graph/HNE_RerouteNode.h"
#include "Graph/HNE_GraphUtils.h"

#include "JsonSerialization.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObject.h"

bool UHNE_NodeSerializer::SerializeNodesToString(const TArray<UEdGraphNode*> SelectedNodesOrdered, FString& OutString)
{
	UE_LOG(LogTemp, Log, TEXT("Copying %d nodes"), SelectedNodesOrdered.Num());

	TSharedPtr<FJsonObject> CopiedNodes = MakeShared<FJsonObject>();

	UEdGraphNode* LastNode = SelectedNodesOrdered.Last();

	int32 BoundsXMin = LastNode->NodePosX;
	int32 BoundsXMax = LastNode->NodePosX;
	int32 BoundsYMin = LastNode->NodePosY;
	int32 BoundsYMax = LastNode->NodePosY;

	//Find Centeroid
	for (UEdGraphNode* Node : SelectedNodesOrdered) {
		BoundsXMin = std::min(BoundsXMin, Node->NodePosX);
		BoundsXMax = std::max(BoundsXMax, Node->NodePosX);
		BoundsYMin = std::min(BoundsYMin, Node->NodePosY);
		BoundsYMax = std::max(BoundsYMax, Node->NodePosY);
	}

	int32 CenterX = BoundsXMin + 0.5 * (BoundsXMax - BoundsXMin);
	int32 CenterY = BoundsYMin + 0.5 * (BoundsYMax - BoundsYMin);

	TArray< TSharedPtr<FJsonValue>> ExportedNodes;

	int32 NodeIndex = 0;

	for (UEdGraphNode* Node : SelectedNodesOrdered) {

		TSharedPtr<FJsonObject> NodeExport = MakeShared<FJsonObject>();

		FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::EditableTitle).ToString();
		FString NodeComment = Node->NodeComment;

		int32 OffsetX = Node->NodePosX - CenterX;
		int32 OffsetY = Node->NodePosY - CenterY;

		NodeExport->SetStringField("Title", NodeTitle);
		NodeExport->SetStringField("Comment", NodeComment);

		NodeExport->SetNumberField("OffsetX", OffsetX);
		NodeExport->SetNumberField("OffsetY", OffsetY);

		TArray<UEdGraphPin*> OutputPins = Node->GetAllPins().FilterByPredicate([](UEdGraphPin* InspectPin) {return InspectPin->Direction == EGPD_Output; });

		int32 OutPinIndex = 0;
		for (UEdGraphPin* Pin : OutputPins) {
			for (UEdGraphPin* TargetPin : Pin->LinkedTo) {
				UHNE_Node* OwnerAsHNENode = Cast< UHNE_Node>(TargetPin->GetOwningNode());

				if (!OwnerAsHNENode) {
					++OutPinIndex;
					continue;
				}

				int32 TargetIndex;
				bool bTargetInSelected = SelectedNodesOrdered.Find(OwnerAsHNENode, TargetIndex);

				TArray<UEdGraphPin*> InputPins = OwnerAsHNENode->GetAllPins().FilterByPredicate([](UEdGraphPin* InspectPin) {return InspectPin->Direction == EGPD_Input; });

				++OutPinIndex;
			}
		}


		UHierarchicalNode_Base* TestObjectNode = Cast< UHierarchicalNode_Base>(Node);
		UHierarchicalArrayNode* TestArrayNode = Cast< UHierarchicalArrayNode>(Node);
		UHNE_RerouteNode* TestRerouteNode = Cast< UHNE_RerouteNode>(Node);

		if (TestObjectNode) {
			NodeExport->SetStringField(NodeType.ToString(), "Object");

			UObject* InnerObject = TestObjectNode->GetInnerObject();
			TSharedPtr<FJsonObject> InnerSerialized = FJsonSerializationModule::SerializeUObjectToJson(InnerObject, true, true);

			NodeExport->SetStringField("InnerClass", InnerObject->GetClass()->GetPathName());
			NodeExport->SetObjectField("InnerObject", InnerSerialized);
		}

		else if (TestArrayNode) {
			NodeExport->SetStringField(NodeType.ToString(), "Array");
			NodeExport->SetStringField("PinSubCategory", TestArrayNode->PinTypeTemplate.PinSubCategory.ToString());

			UObject* SubCategoryObject = TestArrayNode->PinTypeTemplate.PinSubCategoryObject.Get();

			NodeExport->SetStringField("PinSubCategoryObject", SubCategoryObject ? SubCategoryObject->GetPathName() : "");
			NodeExport->SetNumberField("NumberOfPins", OutputPins.Num());
		}

		else if (TestRerouteNode) {
			NodeExport->SetStringField(NodeType.ToString(), "Reroute");

			NodeExport->SetStringField("PinSubCategory", TestRerouteNode->PinTypeTemplate.PinSubCategory.ToString());

			UObject* SubCategoryObject = TestRerouteNode->PinTypeTemplate.PinSubCategoryObject.Get();

			NodeExport->SetStringField("PinSubCategoryObject", SubCategoryObject ? SubCategoryObject->GetPathName() : "");
		}


		TSharedPtr<FJsonValueObject> NodeExportWrapper = MakeShared<FJsonValueObject>(NodeExport);
		ExportedNodes.Add(NodeExportWrapper);
		++NodeIndex;
	}
	TSharedPtr<FJsonObject> ExportWrapper = MakeShared< FJsonObject>();
	ExportWrapper->SetArrayField(UHNE_NodeSerializer::NodeCopyMarker.ToString(), ExportedNodes);

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutString);

	return FJsonSerializer::Serialize(ExportWrapper.ToSharedRef(), Writer);
}

bool UHNE_NodeSerializer::CanSerializeNode(const UEdGraphNode* Node)
{
	return Node->IsA(UHNE_Node::StaticClass());
}

void UHNE_NodeSerializer::DeserializeNodesFromString(const FString InString, UEdGraph* ParentGraph, const FVector2D PasteLocation)
{
	if (ParentGraph == nullptr) return;

	UE_LOG(LogTemp, Log, TEXT("Tried to paste to (%d | %d)"), int(PasteLocation.X), int(PasteLocation.Y));
	TSharedPtr<FJsonObject> JsonImport = MakeShared< FJsonObject>();

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InString);

	if (!FJsonSerializer::Deserialize(Reader, JsonImport)) return;
	UE_LOG(LogTemp, Log, TEXT("Deserialized properly"));
	if (!JsonImport->HasTypedField<EJson::Array>(NodeCopyMarker.ToString())) return;

	UE_LOG(LogTemp, Log, TEXT("Has CopiedNode field"));
	//CREATE NODES

	TArray<UEdGraphNode*> Nodes;

	for (TSharedPtr<FJsonValue> JsonNodeValue : JsonImport->GetArrayField(NodeCopyMarker.ToString())) {
		//PREADD NULLPTR FOR CONNECTION RESTORE LATER
		Nodes.Add(nullptr);

		if(JsonNodeValue->Type != EJson::Object) continue;

		UE_LOG(LogTemp, Log, TEXT("Node value is object"));

		TSharedPtr<FJsonObject> JsonNode = JsonNodeValue->AsObject();

		if (!JsonNode->HasTypedField<EJson::String>(NodeType.ToString())) continue;


		UE_LOG(LogTemp, Log, TEXT("Node has type"));

		FString ImportNodeType = JsonNode->GetStringField(NodeType.ToString());
		UClass * NodeClass = nullptr;
		if (ImportNodeType == "Object") {
			FString InnerClassValue = JsonNode->GetStringField("InnerClass");
			UClass* InnerClass = StaticLoadClass(UObject::StaticClass(), nullptr, *InnerClassValue);

			if (InnerClass == nullptr) continue;

			NodeClass = FHNE_GraphUtils::GetNodeClassForObjectClass(InnerClass);
		}
		else if (ImportNodeType == "Array") {
			NodeClass = UHierarchicalArrayNode::StaticClass();
		}
		else if (ImportNodeType == "Reroute") {
			NodeClass = UHNE_RerouteNode::StaticClass();
		}

		if (NodeClass == nullptr) continue;
		
		UEdGraphNode* CreatedNode = NewObject<UEdGraphNode>(ParentGraph, NodeClass, NAME_None);
		int32 OffsetX = JsonNode->GetNumberField("OffsetX");
		int32 OffsetY = JsonNode->GetNumberField("OffsetY");

		CreatedNode->NodePosX = PasteLocation.X + OffsetX;
		CreatedNode->NodePosY = PasteLocation.Y + OffsetY;

		UHierarchicalNode_Base* TestObjectNode = Cast< UHierarchicalNode_Base>(CreatedNode);
		UHierarchicalArrayNode* TestArrayNode = Cast< UHierarchicalArrayNode>(CreatedNode);
		UHNE_RerouteNode* TestRerouteNode = Cast< UHNE_RerouteNode>(CreatedNode);

		if (TestObjectNode) {
			FString InnerClassValue = JsonNode->GetStringField("InnerClass");
			UClass* InnerClass = StaticLoadClass(UObject::StaticClass(), nullptr, *InnerClassValue);
			TestObjectNode->InnerClass = InnerClass;
			TestObjectNode->InitializeNode();

			TSharedPtr<FJsonObject> InnerObjectJson = JsonNode->GetObjectField("InnerObject");

			UObject* InnerObject = nullptr;
			FJsonSerializationModule::DeserializeJsonToUObject(InnerObject, InnerObjectJson, true);

			if (InnerObject == nullptr) continue;

			TestObjectNode->SetInnerObject(InnerObject);
		}
		else if (TestArrayNode) {
			FString PinSubCategory = JsonNode->GetStringField("PinSubCategory");
			FString PinSubCategoryObjectPath = JsonNode->GetStringField("PinSubCategoryObject");
			int32 NumberOfPins = JsonNode->GetNumberField("NumberOfPins");

			UObject* PinSubCategoryObject = PinSubCategoryObjectPath.Len() ? StaticLoadObject(UObject::StaticClass(), nullptr, *PinSubCategoryObjectPath) : nullptr;

			FEdGraphPinType PinType;{
				PinType.PinSubCategory = FName(PinSubCategory);
				PinType.PinSubCategoryObject = PinSubCategoryObject;
			}
			TestArrayNode->PinTypeTemplate = PinType;

			TestArrayNode->InitializeNode();

			TestArrayNode->SetNumberOfOutPins(NumberOfPins);
		}
		else if (TestRerouteNode) {
			FString PinSubCategory = JsonNode->GetStringField("PinSubCategory");
			FString PinSubCategoryObjectPath = JsonNode->GetStringField("PinSubCategoryObject");

			UObject* PinSubCategoryObject = PinSubCategoryObjectPath.Len() ? StaticLoadObject(UObject::StaticClass(), nullptr, *PinSubCategoryObjectPath) : nullptr;

			FEdGraphPinType PinType; {
				PinType.PinSubCategory = FName(PinSubCategory);
				PinType.PinSubCategoryObject = PinSubCategoryObject;
			}
			TestRerouteNode->PinTypeTemplate = PinType;

			TestRerouteNode->InitializeNode();
		}

		ParentGraph->AddNode(CreatedNode);
		Nodes.Last() = CreatedNode;
	}

	//RESTORE CONNECTIONS
}
