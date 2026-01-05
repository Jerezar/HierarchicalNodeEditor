#include "HNE_AssetImporter.h"
#include "HierarchicalEditInterface.h"
#include "HierarchicalEditAsset.h"
#include "Graph/HNE_Node.h"
#include "Graph/HierarchicalNodeGraph.h"
#include "Graph/HNE_GraphUtils.h"
#include "Graph/HierarchicalRootNode.h"
#include "Graph/HierarchicalChildNode.h"
#include "Graph/HierarchicalArrayNode.h"

#include "Importer/HNE_ConnectionBuilder.h"
#include "Importer/HNE_StateIDConnectionTracker.h"

bool FHNE_AssetImporter::ImportObjectIntoGraph(UHierarchicalEditAsset* GraphAsset, UObject* InObject, bool bSetImportedAsTarget)
{
    if (InObject == nullptr) return false;

    if ( !(InObject->GetClass()->ImplementsInterface(UHierarchicalEditInterface::StaticClass())) ) {
        return false;
    }

    //InObject = DuplicateObject(InObject, GetTransientPackage());
    //Can't do that here, messes with FGUIDs; instead done in 

    UEdGraph* GraphToOverwrite = GraphAsset->WorkingGraph;
    GraphToOverwrite->Modify();

    //Clear all existing nodes.
    TArray< UHNE_Node*> AllNodes;
    GraphToOverwrite->GetNodesOfClass<UHNE_Node>(AllNodes);
    for (UHNE_Node* Node : AllNodes) {
        GraphToOverwrite->RemoveNode(Node);
    }

    //Setup tracker for non-parent/child connections
    TSharedPtr<FHNE_ConnectionTracker> ConnectionTracker = MakeShared< FHNE_MultiConnectionBuilder>( 
        TMap<FName, TSharedPtr<FHNE_ConnectionTracker>>
        {
            {UHierarchicalGraphSchema::SC_StateTransition, MakeShared<FHNE_StateIDConnectionTracker>()}
        }
    );



    //Create new root node

    FVector2D Margins(256, 64);
    FVector2D NextNodePos(0, 0);

    UHierarchicalRootNode* RootNode = NewObject< UHierarchicalRootNode>(GraphToOverwrite, UHierarchicalRootNode::StaticClass());
    RootNode->InnerClass = InObject->GetClass();
    RootNode->NodePosX = NextNodePos.X;
    RootNode->NodePosY = NextNodePos.Y;

    GraphToOverwrite->AddNode(RootNode);

    RootNode->InitializeNode();
    RootNode->SetInnerObject(InObject);

    NextNodePos.X += RootNode->NodeWidth + Margins.X;

    for (UEdGraphPin* Pin : RootNode->Pins) {
        int BranchHeight = 0;

        FEdGraphPinType PinType = Pin->PinType;
        if (PinType.PinSubCategory != UHierarchicalGraphSchema::SC_ChildNode && PinType.ContainerType != EPinContainerType::Array) {
            ConnectionTracker->RegisterValuePin(Pin, RootNode->GetInnerObject());
        }

        if (Pin->Direction != EGPD_Output) continue;

        FProperty* OutProp = RootNode->InnerClass->FindPropertyByName(Pin->GetFName());

        if (PinType.ContainerType == EPinContainerType::Array) {
            FHNE_AssetImporter::MakeArrayNodeRecursive(Pin, OutProp, InObject, ConnectionTracker, NextNodePos, Margins, BranchHeight);
        }
        else if (PinType.PinSubCategory == UHierarchicalGraphSchema::SC_ChildNode) {
            UObject** ChildObject = OutProp->ContainerPtrToValuePtr<UObject*>(InObject);
            FHNE_AssetImporter::MakeChildNodeRecursive(Pin, *ChildObject, ConnectionTracker, NextNodePos, Margins, BranchHeight);
        }

        NextNodePos.Y += BranchHeight + Margins.Y;
    }


    UE_LOG(LogTemp, Log, TEXT("Setting up non-ownership connections"));
    ConnectionTracker->SetUpConnections();

    if (bSetImportedAsTarget) {
        GraphAsset->TargetInfo.OutAsset = InObject;
        GraphAsset->Modify();
    }

    //Clear all values governed by node connections.
    TArray< UHierarchicalNode_Base*> AllObjectNodes;
    GraphToOverwrite->GetNodesOfClass<UHierarchicalNode_Base>(AllObjectNodes);
    for (UHierarchicalNode_Base* Node : AllObjectNodes) {
        Node->ClearOutPinValues();
    }

    GraphToOverwrite->NotifyGraphChanged();

    return true;
}

bool FHNE_AssetImporter::MakeArrayNodeRecursive(UEdGraphPin* FromPin, FProperty* InProperty, UObject* InObject, TSharedPtr<FHNE_ConnectionTracker> ConnectionTracker, FVector2D NodePos, FVector2D Margins, int& OutBranchHeight)
{
    UE_LOG(LogTemp, Log, TEXT("%s"), *(InProperty->GetFName().ToString()));

    FArrayProperty* PropAsArray = CastField< FArrayProperty>(InProperty);

    if (PropAsArray == nullptr) return false; //Wasn't actually an array

    FScriptArrayHelper ArrayHelper(PropAsArray, PropAsArray->ContainerPtrToValuePtr<void>(InObject));

    if (!ArrayHelper.Num()) return true;

    UEdGraph* ParentGraph = FromPin->GetOwningNode()->GetGraph();
    FEdGraphPinType PinType = FromPin->PinType;

    UHierarchicalArrayNode* ArrayNode = NewObject< UHierarchicalArrayNode>(ParentGraph, UHierarchicalArrayNode::StaticClass());
    ArrayNode->PinTypeTemplate = FromPin->PinType;
    ArrayNode->InitializeNode();
    ArrayNode->SetNumberOfOutPins(ArrayHelper.Num());

    ArrayNode->NodePosX = NodePos.X;
    ArrayNode->NodePosY = NodePos.Y;

    ParentGraph->Modify();
    ParentGraph->AddNode(ArrayNode, true);

    UEdGraphPin* ArrayParentPin = ArrayNode->FindPin(FName("Input"));

    ParentGraph->GetSchema()->TryCreateConnection(FromPin, ArrayParentPin);

    int BranchHeightTotal = 0;
    FVector2D NextNodePos(NodePos);
    NextNodePos.X += ArrayNode->NodeWidth + Margins.X;

    if (PinType.PinSubCategory == UHierarchicalGraphSchema::SC_ChildNode) {
        TArray<UObject*>* ChildObjects = InProperty->ContainerPtrToValuePtr<TArray<UObject*>>(InObject);

        for (int i = 0; i < ChildObjects->Num(); ++i) {
            int BranchHeight = 0;
            FHNE_AssetImporter::MakeChildNodeRecursive(ArrayNode->GetPinAt(i + 1), (*ChildObjects)[i], ConnectionTracker, NextNodePos, Margins, BranchHeight);
            NextNodePos.Y += BranchHeight + Margins.Y;
            BranchHeightTotal += BranchHeight + Margins.Y;
        }

    }
    else {
        // get connection tracker for subcategory
        ConnectionTracker->RegisterArrayPins(ArrayNode->Pins, PropAsArray, InObject);
    }


    OutBranchHeight = std::max(ArrayNode->NodeHeight, BranchHeightTotal);


    UE_LOG(LogTemp, Log, TEXT("Finished branch"));

    return true;
}


bool FHNE_AssetImporter::MakeChildNodeRecursive(UEdGraphPin* FromPin, UObject* InObject, TSharedPtr<FHNE_ConnectionTracker> ConnectionTracker, FVector2D NodePos, FVector2D Margins, int& OutBranchHeight)
{
    if (InObject == nullptr) return true;
    if (!(InObject->GetClass())->ImplementsInterface(UHierarchicalEditInterface::StaticClass())) {
        return false;
    }


    UEdGraph* ParentGraph = FromPin->GetOwningNode()->GetGraph();

    UClass* NodeClass = FHNE_GraphUtils::GetNodeClassForObjectClass(InObject->GetClass());

    UE_LOG(LogTemp, Warning, TEXT("Creating Node object"))
    UHierarchicalChildNode* Result = NewObject<UHierarchicalChildNode>(ParentGraph, NodeClass);

    Result->NodePosX = NodePos.X;
    Result->NodePosY = NodePos.Y;

    ParentGraph->Modify();
    ParentGraph->AddNode(Result, true);

    Result->InnerClass = InObject->GetClass();
    Result->InitializeNode();
    Result->SetInnerObject(InObject);

    UEdGraphPin* ParentPin = Result->FindPin(FName("Parent"));

    ParentGraph->GetSchema()->TryCreateConnection(FromPin, ParentPin);

    int BranchHeightTotal = 0;
    FVector2D NextNodePos(NodePos);
    NextNodePos.X += Result->NodeWidth + Margins.X;

    for (UEdGraphPin* Pin : Result->Pins) {

        FEdGraphPinType PinType = Pin->PinType;
        if (PinType.PinSubCategory != UHierarchicalGraphSchema::SC_ChildNode && PinType.ContainerType != EPinContainerType::Array) {
            ConnectionTracker->RegisterValuePin(Pin, Result->GetInnerObject());
        }

        if (Pin->Direction != EGPD_Output) continue;


        FProperty* OutProp = Result->InnerClass->FindPropertyByName(Pin->GetFName());

        int BranchHeight = 0;

        if (PinType.ContainerType == EPinContainerType::Array) {
            FHNE_AssetImporter::MakeArrayNodeRecursive(Pin, OutProp, InObject, ConnectionTracker, NextNodePos, Margins, BranchHeight);
        }
        else if (PinType.PinSubCategory == UHierarchicalGraphSchema::SC_ChildNode) {
            UObject** ChildObject = OutProp->ContainerPtrToValuePtr<UObject*>(InObject);
            FHNE_AssetImporter::MakeChildNodeRecursive(Pin, *ChildObject, ConnectionTracker, NextNodePos, Margins, BranchHeight);
        }

        NextNodePos.Y += BranchHeight + Margins.Y;
        BranchHeightTotal += BranchHeight + Margins.Y;
    }

    OutBranchHeight = std::max(Result->NodeHeight, BranchHeightTotal);


    UE_LOG(LogTemp, Log, TEXT("Finished branch"));

    return true;
}
