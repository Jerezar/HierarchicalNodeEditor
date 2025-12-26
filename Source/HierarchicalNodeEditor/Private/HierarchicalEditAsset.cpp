// Copyright Gunfire Games, LLC. All Rights Reserved.


#include "HierarchicalEditAsset.h"
#include "Graph/HierarchicalRootNode.h"
#include "Graph/HierarchicalNodeGraph.h"
#include "Graph/HierarchicalStateNode.h"
#include "ActorState.h"
#include "UObject/SavePackage.h"
#include "AssetRegistryModule.h"
#include "Kismet/KismetSystemLibrary.h"

void UHierarchicalEditAsset::CompileGraphToAsset()
{
	UE_LOG(LogTemp, Log, TEXT("Attempting to compile hierarchical graph."));

	if (!ValidateInputPins(WorkingGraph)) {
		UE_LOG(LogTemp, Error, TEXT("Graph failed validation"));
		return;
	}

	if (TargetInfo.OutAssetName.IsNone()) {
		UE_LOG(LogTemp, Error, TEXT("Did not specify name for created asset."));
		return;
	}

	TArray<UHierarchicalRootNode*> RootNodes;
	WorkingGraph->GetNodesOfClass<UHierarchicalRootNode>(RootNodes);

	if (RootNodes.Num() != 1) {
		UE_LOG(LogTemp, Error, TEXT("Graph should have exactly one root node."));
		return;
	}

	UPackage* GraphOwnerPackage = this->GetPackage();

	FString OutAssetName = TargetInfo.OutAssetName.ToString();
	FString OutAssetPath = TargetInfo.OutAssetPath.ToString();

	if (TargetInfo.OutAssetPath.IsNone()) {
		OutAssetPath = GraphOwnerPackage->GetFName().ToString();
		FString Prefix = "/Game/";
		if (OutAssetPath.StartsWith(Prefix)) {
			OutAssetPath = OutAssetPath.RightChop(Prefix.Len());
		}
		if (OutAssetPath.EndsWith(this->GetName())) {
			OutAssetPath = OutAssetPath.LeftChop(this->GetName().Len());
		}
	}

	if (!OutAssetPath.EndsWith("/")) {
		OutAssetPath += "/";
	}

	FString OutPackageFolder = "/Game/" + OutAssetPath;
	FString OutFolderPath = FPaths::ProjectContentDir() + OutAssetPath;


	UPackage* OutPackage = CreatePackage(*(OutPackageFolder + OutAssetName));

	if (OutPackage == nullptr) {
		UE_LOG(LogTemp, Error, TEXT("No package to save into."));
		return;
	}

	UObject* OutAsset = FindObject<UObject>(OutPackage, *(TargetInfo.OutAssetName.ToString()), false);

	if (OutAsset != nullptr) {
		

		UE_LOG(LogTemp, Error, TEXT("Asset exists already, lets skip for now"));

		const FText DialogTitle = FText::FromString("Overwrite existing asset?");
		const FText DialogMessage = FText::FromString("An asset with the specified name already exists. Do you wish to overwrite it?");

		const EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::YesNo, DialogMessage, &DialogTitle);

		if (Response != EAppReturnType::Yes){
			return;
		}
	}

	UHierarchicalRootNode* RootNode = *RootNodes.begin();

	UObject* FinalizedAsset = RootNode->GetFinalizedAssetRecursive();

	FSavePackageArgs SaveArgs;

	// This is specified just for example
	{
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
	}


	if (OutAsset != nullptr) {

		if (OutAsset->GetClass() != RootNode->InnerClass) {

			UE_LOG(LogTemp, Error, TEXT("Existing Asset is of incompatible class."));
		}

		TArray<UObject*> SubObjectsToReparent;
		GetObjectsWithOuter(FinalizedAsset, SubObjectsToReparent);

		for (UObject* SubObject : SubObjectsToReparent) {
			SubObject->Rename(nullptr, OutAsset);
		}


	}
	else {
		FinalizedAsset->Rename(*OutAssetName, OutPackage);
		FinalizedAsset->SetFlags(EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);


		FAssetRegistryModule::AssetCreated(FinalizedAsset);
		FinalizedAsset->MarkPackageDirty();

		FString FilePath = FString::Printf(TEXT("%s%s%s"), *OutFolderPath, *OutAssetName, *FPackageName::GetAssetPackageExtension());
		bool bSuccess = UPackage::SavePackage(OutPackage, FinalizedAsset, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *FilePath);

		UE_LOG(LogTemp, Warning, TEXT("Saved Package: %s"), bSuccess ? TEXT("True") : TEXT("False"));
		UE_LOG(LogTemp, Warning, TEXT("Saved Package to: %s"), *OutFolderPath);

	}

}

bool UHierarchicalEditAsset::ValidateInputPins(UEdGraph* Graph) {
	UE_LOG(LogTemp, Log, TEXT("Validating hierarchical graph"));

	TMap<FString, TArray< UHierarchicalStateNode*>> StateNodesByNameID;
	TArray<FString> DuplicateStateNameIds;

	TArray< UEdGraphNode*> MultiParentStates;

	for (UEdGraphNode* Node : Graph->Nodes) {
		//Clear existing error messages
		Node->ErrorMsg = "";
		Node->bHasCompilerMessage = false;

		//UniqueStateNameIDValidator
		UHierarchicalStateNode* StateNode = Cast< UHierarchicalStateNode>(Node);

		if (StateNode != nullptr) {
			UActorState* StateObject = Cast< UActorState>(StateNode->GetInnerObject());

			if (StateObject != nullptr) {
				FString NameID = StateObject->NameID.ToString();

				TArray< UHierarchicalStateNode*>* NodesOfNameID = StateNodesByNameID.Find(NameID);

				if (NodesOfNameID == nullptr) {
					StateNodesByNameID.Add(NameID, TArray< UHierarchicalStateNode*>());
					NodesOfNameID = StateNodesByNameID.Find(NameID);
				}

				NodesOfNameID->Add(StateNode);
			}
		}

		// UniqueStateParents
		for (UEdGraphPin* Pin : Node->GetAllPins()) {

			if (Pin->Direction != EGPD_Input) continue;

			FEdGraphPinType PinType = Pin->PinType;
			if (PinType.PinSubCategory != UHierarchicalGraphSchema::SC_ChildNode) continue;

			if (PinType.PinSubCategoryObject == nullptr) continue;

			UClass* NodeClass = Cast<UClass>(PinType.PinSubCategoryObject);

			if (NodeClass == nullptr) continue;
			if (!NodeClass->IsChildOf(UActorState::StaticClass())) continue;

			if (Pin->LinkedTo.Num() > 1) {
				MultiParentStates.Add(Node);
			}
		}
	}


	UE_LOG(LogTemp, Log, TEXT("Unique IDs: %d"), StateNodesByNameID.Num());
	for (TPair<FString, TArray< UHierarchicalStateNode*>> Entry : StateNodesByNameID) {
		if (Entry.Value.Num() > 1) {
			UE_LOG(LogTemp, Log, TEXT("Yuuup"));
			DuplicateStateNameIds.Add(Entry.Key);
		}
	}

	//Apply new error messages
	for (FString DuplicateID : DuplicateStateNameIds) {
		TArray< UHierarchicalStateNode*>* NodesOfNameID = StateNodesByNameID.Find(DuplicateID);

		for (UEdGraphNode* InvalidNode : *NodesOfNameID) {
			InvalidNode->ErrorMsg += "NameID must be unique.\n";
			InvalidNode->bHasCompilerMessage = true;
		}
	}

	for (UEdGraphNode* InvalidNode : MultiParentStates) {
		InvalidNode->ErrorMsg += "States cannot have multiple parents.\n";
		InvalidNode->bHasCompilerMessage = true;
	}

	Graph->NotifyGraphChanged();

	return !bool(MultiParentStates.Num() || DuplicateStateNameIds.Num());
}
