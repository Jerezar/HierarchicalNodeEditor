#include "HNE_Node_InheritableNamedObject.h"

TArray<FString> UHNE_Node_InheritableNamedObject::GetFieldNamesToCopy() const
{
    TArray<FString> CopyFields = Super::GetFieldNamesToCopy();
    CopyFields.Add("UniqueId");
    return CopyFields;
}
