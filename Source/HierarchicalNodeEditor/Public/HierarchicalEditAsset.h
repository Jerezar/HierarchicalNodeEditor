#pragma once

#include "CoreMinimal.h"
#include "HierarchicalEditAsset.generated.h"

USTRUCT()
struct FAssetTargetInfo {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, meta = (ToolTip = "The name of the generated asset/file. Must not be 'None'."))
	FName OutAssetName;


	UPROPERTY(EditAnywhere, meta = (ToolTip = "The folder path relative to the content folder. If 'None', uses the folder containing this graph"))
	FName OutAssetPath;
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
