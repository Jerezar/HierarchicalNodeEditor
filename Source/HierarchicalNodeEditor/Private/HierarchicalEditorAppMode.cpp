#include "HierarchicalEditorAppMode.h"
#include "HierarchicalEditAssetApp.h"
#include "HierachicalGraphTabFactory.h"

FHierarchicalEditorAppMode::FHierarchicalEditorAppMode(TSharedPtr<class FHierarchicalEditAssetApp> App): FApplicationMode(ModeIdentifier)
{
	_App = App;
	_Tabs.RegisterFactory( MakeShareable<FHierachicalGraphTabFactory>(new FHierachicalGraphTabFactory(App)) );

	TabLayout = FTabManager::NewLayout("FHierarchicalEditorAppMode_Layout_v1")
	->AddArea(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
		->Split(
			FTabManager::NewStack()
			->AddTab(FHierachicalGraphTabFactory::CustomTabIdentifier, ETabState::OpenedTab)
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
