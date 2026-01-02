#pragma once

#include "CoreMinimal.h"
#include "HierarchicalEditAsset.generated.h"

USTRUCT()
struct FAssetTargetInfo {
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, meta = (ToolTip = "The asset to edit. If not set, uses OutAssetName and OutAssetPath to create a new asset or find an existing one."))
	UObject* OutAsset = nullptr;
};

UCLASS(BlueprintType)
class HIERARCHICALNODEEDITOR_API UHierarchicalEditAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class UEdGraph* WorkingGraph;

	UPROPERTY(EditAnywhere)
	FAssetTargetInfo TargetInfo;

public:
	void CompileGraphToAsset();
	bool ValidateInputPins(UEdGraph* Graph);
};
