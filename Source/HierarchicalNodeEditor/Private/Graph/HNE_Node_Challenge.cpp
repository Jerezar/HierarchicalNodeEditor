#include "HNE_Node_Challenge.h"

TArray<FString> UHNE_Node_Challenge::GetFieldNamesToCopy() const
{
    TArray<FString> CopyFields = Super::GetFieldNamesToCopy();
    CopyFields.Add("ID");
    return CopyFields;
}
