// Copyright Gunfire Games, LLC. All Rights Reserved.


#include "HierachicalGraphTabFactory.h"
#include "HierarchicalEditAssetApp.h"

FHierachicalGraphTabFactory::FHierachicalGraphTabFactory(TSharedPtr<class FHierarchicalEditAssetApp> App) : FWorkflowTabFactory(CustomTabIdentifier, App)
{
	_App = App;
	TabLabel = FText::FromString(TEXT("Graph"));
	ViewMenuDescription = FText::FromString(TEXT("Graph view of the hierarchical asset."));
	ViewMenuTooltip = FText::FromString(TEXT("Show graph view."));
}

TSharedRef<SWidget> FHierachicalGraphTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(STextBlock).Text(FText::FromString(TEXT("A placeholder for the graph view")));
}

FText FHierachicalGraphTabFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return FText::FromString(TEXT("Graph view for modifying hierarchical assets."));
}
