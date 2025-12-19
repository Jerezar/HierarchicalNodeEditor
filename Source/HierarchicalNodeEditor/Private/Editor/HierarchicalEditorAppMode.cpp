#include "Editor/HierarchicalEditorAppMode.h"
#include "Editor/HierarchicalEditAssetApp.h"
#include "Editor/HierachicalGraphTabFactory.h"
#include "Editor/HierachicalPropertiesTabFactory.h"

FHierarchicalEditorAppMode::FHierarchicalEditorAppMode(TSharedPtr<class FHierarchicalEditAssetApp> App): FApplicationMode(ModeIdentifier)
{
	_App = App;

	_Tabs.RegisterFactory( MakeShareable<FHierachicalGraphTabFactory>(new FHierachicalGraphTabFactory(App)) );
	_Tabs.RegisterFactory(MakeShareable<FHierachicalPropertiesTabFactory>(new FHierachicalPropertiesTabFactory(App)));

	TabLayout = FTabManager::NewLayout("FHierarchicalEditorAppMode_Layout_v1")
	->AddArea(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split(
			FTabManager::NewSplitter()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.75)
				->AddTab(FHierachicalGraphTabFactory::CustomTabIdentifier, ETabState::OpenedTab)
			)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.25)
				->AddTab(FHierachicalPropertiesTabFactory::CustomTabIdentifier, ETabState::OpenedTab)
			)
		)
	);
}

void FHierarchicalEditorAppMode::RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager)
{
	TSharedPtr< FHierarchicalEditAssetApp> App = _App.Pin();
	App->PushTabFactories(_Tabs);
	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FHierarchicalEditorAppMode::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();
}

void FHierarchicalEditorAppMode::PostActivateMode()
{
	FApplicationMode::PostActivateMode();
}
