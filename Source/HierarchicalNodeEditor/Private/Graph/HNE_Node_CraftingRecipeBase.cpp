#include "HNE_Node_CraftingRecipeBase.h"

TArray<FString> UHNE_Node_CraftingRecipeBase::GetFieldNamesToCopy() const
{
    TArray<FString> CopyFields = Super::GetFieldNamesToCopy();
    CopyFields.Add("Guid");
    return CopyFields;
}
