// Copyright Gunfire Games, LLC. All Rights Reserved.


#include "Editor/HierarchicalEditAssetApp.h"
#include "Editor/HierarchicalEditorAppMode.h"

void FHierarchicalEditAssetApp::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FWorkflowCentricApplication::RegisterTabSpawners(TabManager);
}

void FHierarchicalEditAssetApp::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UObject* InObject)
{
	TArray<UObject*> ObjectsToEdit;
	ObjectsToEdit.Add(InObject);

	InitAssetEditor(
		Mode,
		InitToolkitHost,
		CustomAppIdentifier,
		FTabManager::FLayout::NullLayout,
		true,
		true,
		ObjectsToEdit
	);

	AddApplicationMode(FHierarchicalEditorAppMode::ModeIdentifier, MakeShareable(new FHierarchicalEditorAppMode(SharedThis(this))));
	SetCurrentMode(FHierarchicalEditorAppMode::ModeIdentifier);
}
