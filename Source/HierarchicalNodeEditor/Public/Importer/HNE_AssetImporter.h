#pragma once
#include "CoreMinimal.h"
#include "HNE_ConnectionBuilder.h"

class FHNE_AssetImporter {
public:
	static bool ImportObjectIntoGraph(UHierarchicalEditAsset* GraphAsset, UObject* InObject, bool bSetImportedAsTarget);
	static bool MakeArrayNodeRecursive(UEdGraphPin* FromPin, FProperty* InProperty, UObject* InObject, TSharedPtr<FHNE_ConnectionTracker> ConnectionTracker, FVector2D NodePos, FVector2D Margins, int& OutBranchHeight);
	static bool MakeChildNodeRecursive(UEdGraphPin* FromPin, UObject* InObject, TSharedPtr<FHNE_ConnectionTracker> ConnectionTracker, FVector2D NodePos, FVector2D Margins, int& OutBranchHeight);
};
