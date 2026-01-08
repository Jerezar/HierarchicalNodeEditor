#include "HNE_Node_ChallengeObjectiveBase.h"

TArray<FString> UHNE_Node_ChallengeObjectiveBase::GetFieldNamesToCopy() const
{
    TArray<FString> CopyFields = Super::GetFieldNamesToCopy();
    CopyFields.Add("ID");
    return CopyFields;
}
