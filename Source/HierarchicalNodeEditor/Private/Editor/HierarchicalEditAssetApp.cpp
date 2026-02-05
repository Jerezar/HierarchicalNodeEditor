// Copyright Gunfire Games, LLC. All Rights Reserved.


#include "Editor/HierarchicalEditAssetApp.h"
#include "Editor/HierarchicalEditorAppMode.h"
#include "Editor/HNE_NodeSerializer.h"
#include "Graph/HierarchicalNode_Base.h"
#include "Graph/HierarchicalArrayNode.h"
#include "Graph/HNE_RerouteNode.h"

#include "HAL/PlatformApplicationMisc.h"
#include "GenericCommands.h"


void FHierarchicalEditAssetApp::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FWorkflowCentricApplication::RegisterTabSpawners(TabManager);
}

void FHierarchicalEditAssetApp::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UObject* InObject)
{
	TArray<UObject*> ObjectsToEdit;
	ObjectsToEdit.Add(InObject);

	WorkingAsset = Cast< UHierarchicalEditAsset >(InObject);

	InitAssetEditor(
		Mode,
		InitToolkitHost,
		CustomAppIdentifier,
		FTabManager::FLayout::NullLayout,
		true,
		true,
		ObjectsToEdit
	);

	TSharedPtr< FHierarchicalEditorAppMode> AppMode = MakeShareable(new FHierarchicalEditorAppMode(SharedThis(this)));
	AppMode->ExtendToolbar();


	AddApplicationMode(FHierarchicalEditorAppMode::ModeIdentifier, AppMode.ToSharedRef());
	SetCurrentMode(FHierarchicalEditorAppMode::ModeIdentifier);

	ToolkitCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FHierarchicalEditAssetApp::DeleteGraphAction),
		FCanExecuteAction::CreateSP(this, &FHierarchicalEditAssetApp::CanDeleteGraphAction)
	);

	ToolkitCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &FHierarchicalEditAssetApp::CopyGraphAction)
	);

	ToolkitCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &FHierarchicalEditAssetApp::PasteGraphAction)
	);
}

void FHierarchicalEditAssetApp::SetNodeDetailsView(TSharedPtr<class IDetailsView> NodeDetailsView)
{
	_NodeDetailsView = NodeDetailsView;
}

void FHierarchicalEditAssetApp::OnGraphSelectionChanged(const FGraphPanelSelectionSet& Selection)
{
	TArray<UHierarchicalNode_Base*> SelectedNodes;

	for (UObject* SelectedObject : Selection) {
		UHierarchicalNode_Base* SelectionAsNode = Cast< UHierarchicalNode_Base>(SelectedObject);
		if (SelectionAsNode != nullptr) {
			SelectedNodes.Add(SelectionAsNode);
		}
	}

	if (SelectedNodes.Num() == 1) {
		_NodeDetailsView->SetObject((SelectedNodes[0])->GetInnerObject());
	}
	else {
		_NodeDetailsView->SetObject(nullptr);
	}
}

void FHierarchicalEditAssetApp::OnNodeTitleComitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (CommitInfo == ETextCommit::OnEnter && NodeBeingChanged != nullptr) {
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

/*
bool FHierarchicalEditAssetApp::OnVerifyNodeTitleComitted(const FText& NewText, UEdGraphNode* NodeBeingChanged, FText& OutErrorMessage)
{
	return true;
}
*/

void FHierarchicalEditAssetApp::DeleteGraphAction()
{
	if (!_WorkingGraphUI.IsValid()) return;

	FGraphPanelSelectionSet SelectedNodes = _WorkingGraphUI->GetSelectedNodes();

	for (UObject* SelectedObject : SelectedNodes) {
		if (UHNE_Node* SelectionAsNode = Cast< UHNE_Node>(SelectedObject)) {
			SelectionAsNode->OnDeleteNodeAction();
		}
		else if (UEdGraphNode_Comment* SelectionAsComment = Cast< UEdGraphNode_Comment>(SelectedObject)) {
			GetWorkingGraph()->RemoveNode(SelectionAsComment);
		}
	}
	
}

bool FHierarchicalEditAssetApp::CanDeleteGraphAction()
{
	TSharedPtr<SDockTab> Tab = GetTabManager()->GetOwnerTab();
	return Tab->IsForeground();
}

void FHierarchicalEditAssetApp::CopyGraphAction()
{
	UE_LOG(LogTemp, Log, TEXT("Tried to copy"));
	if (!_WorkingGraphUI.IsValid()) return;

	FGraphPanelSelectionSet SelectedNodes = _WorkingGraphUI->GetSelectedNodes();
	TArray<UEdGraphNode*> SelectedNodesOrdered;

	for (UObject* SelectedObject : SelectedNodes) {
		UEdGraphNode* Node = Cast< UEdGraphNode>(SelectedObject);
		if (!(Node != nullptr && UHNE_NodeSerializer::CanSerializeNode(Node))) continue;

		SelectedNodesOrdered.Add(Node);
	}

	if (!SelectedNodesOrdered.Num()) return;

	FString Text;
	
	if (UHNE_NodeSerializer::SerializeNodesToString(SelectedNodesOrdered, Text)){
		FPlatformApplicationMisc::ClipboardCopy(*Text);
	}
	
}

void FHierarchicalEditAssetApp::PasteGraphAction()
{
	UE_LOG(LogTemp, Log, TEXT("Tried to paste"));
	if (!_WorkingGraphUI.IsValid()) return;

	FString Text;
	FPlatformApplicationMisc::ClipboardPaste(Text);
	
	FVector2D PasteLocation = _WorkingGraphUI->GetPasteLocation();

	UHNE_NodeSerializer::DeserializeNodesFromString(Text, GetWorkingGraph(), PasteLocation);
}
