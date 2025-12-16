// Copyright Gunfire Games, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

/**
 * 
 */
class FHierarchicalEditAssetApp : public FWorkflowCentricApplication, public FEditorUndoClient, public FNotifyHook
{
public:
	virtual void RegisterTabSpawners(const TSharedRef< class FTabManager >& TabManager) override;
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UObject* InObject);

public: //FAssetEditorToolkit
	virtual FName GetToolkitFName() const override { return FName(TEXT("HierarchicalEditAssetEditorApp")); }
	virtual FText GetBaseToolkitName() const override { return FText::FromString(TEXT("HierarchicalEditAssetEditorApp")); }
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("HierarchicalEditAssetEditorApp"); }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0,1,0,0.5); }
	virtual void OnToolkitHostingStarted(const TSharedRef< IToolkit >& Toolkit) override {}
	virtual void OnToolkitHostingFinished(const TSharedRef< IToolkit >& Toolkit) override {}

public:
	static const FName CustomAppIdentifier;
};

const FName FHierarchicalEditAssetApp::CustomAppIdentifier = FName(TEXT("HierarchicalEditAssetEditor"));