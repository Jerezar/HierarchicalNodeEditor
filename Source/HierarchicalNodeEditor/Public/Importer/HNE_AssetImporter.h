#pragma once
#include "CoreMinimal.h"

class FHNE_AssetImporter {
public:
	static bool ImportObjectIntoGraph(UHierarchicalEditAsset* GraphAsset, UObject* InObject, bool bSetImportedAsTarget);
	static bool MakeArrayNodeRecursive(UEdGraphPin* FromPin, FProperty* InProperty, UObject* InObject, FVector2D NodePos, FVector2D Margins, int& OutBranchHeight);
	static bool MakeChildNodeRecursive(UEdGraphPin* FromPin, UObject* InObject, FVector2D NodePos, FVector2D Margins, int& OutBranchHeight);
};
