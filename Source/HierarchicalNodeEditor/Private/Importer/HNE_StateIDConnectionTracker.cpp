#include "HNE_StateIDConnectionTracker.h"
#include "ActorState.h"
#include "ActorStateID.h"
#include "Graph/HierarchicalStateNode.h"

bool FHNE_StateIDConnectionTracker::RegisterValuePin(UEdGraphPin* InPin, FProperty* InProperty, void* ValuePtr)
{
    UE_LOG(LogTemp, Log, TEXT("Registering id pin: %s"), *(InPin->GetFName().ToString()));

    if (InProperty == nullptr) return false;

    FStructProperty* PropAsStruct = CastField< FStructProperty>(InProperty);

    if (PropAsStruct == nullptr) return false;

    if (PropAsStruct->Struct != TBaseStructure<FActorStateID>::Get()) return false;

    FActorStateID* StateID = (FActorStateID * )ValuePtr;

    FGuid StateIDInner = StateID->Guid;

    if (!StateIDInner.IsValid()) return true;

    TArray<UEdGraphPin*>* PinsOfID = PinsByID.Find(StateIDInner);

    if (PinsOfID == nullptr) {
        PinsByID.Add({ StateIDInner, TArray<UEdGraphPin*>{ InPin } });
    }
    else {
        PinsOfID->AddUnique(InPin);
    }
    //UE_LOG(LogTemp, Log, TEXT("Registering guid: %s"), *(StateIDInner.ToString()));

    return true;
}

bool FHNE_StateIDConnectionTracker::RegisterArrayPins(TArray<UEdGraphPin*> InPins, FArrayProperty* InProperty, void* ValuePtr)
{
    //UE_LOG(LogTemp, Log, TEXT("Registering id array: %s"), *(InProperty->GetFName().ToString()));

    if (InProperty == nullptr) return false;

    FStructProperty* InnerAsStruct = CastField< FStructProperty>(InProperty->Inner);

    if (InnerAsStruct == nullptr) return false;

    if (InnerAsStruct->Struct != TBaseStructure<FActorStateID>::Get()) return false;

    TArray<FActorStateID>* StateIDs = (TArray<FActorStateID>*)ValuePtr;

    if (InPins.Num() != StateIDs->Num()) return false;

    for (int i = 0; i < InPins.Num(); ++i) {
        FActorStateID StateID = (*StateIDs)[i];
        FGuid StateIDInner = StateID.Guid;

        if (!StateIDInner.IsValid()) continue;

        UEdGraphPin* InPin = InPins[i];

        TArray<UEdGraphPin*>* PinsOfID = PinsByID.Find(StateIDInner);

        if (PinsOfID == nullptr) {
            PinsByID.Add({ StateIDInner, TArray<UEdGraphPin*>{ InPin } });
        }
        else {
            PinsOfID->AddUnique(InPin);
        }
        //UE_LOG(LogTemp, Log, TEXT("Registering guid: %s"), *(StateIDInner.ToString()));
    }

    return true;
}

void FHNE_StateIDConnectionTracker::SetUpConnections()
{
    UE_LOG(LogTemp, Log, TEXT("Attempting to setup state id connections."))
    for (TPair<FGuid, TArray<UEdGraphPin*>> PinsOfID : PinsByID) {
        TArray<UEdGraphPin*> Pins = PinsOfID.Value;

        TArray<UEdGraphPin*> InputPins = Pins.FilterByPredicate([](UEdGraphPin* InspectPin) {return InspectPin->Direction == EGPD_Input; });

        if (InputPins.Num() != 1) { 
            SetErrorMessages(Pins, PinsOfID.Key);
            continue; 
        }

        UEdGraphPin* TargetInputPin = *(InputPins.begin());

        const UEdGraphSchema* Schema = TargetInputPin->GetSchema();

        TArray<UEdGraphPin*> OutputPins = Pins.FilterByPredicate([](UEdGraphPin* InspectPin) {return !(InspectPin->Direction == EGPD_Input); });

        for (UEdGraphPin* Pin : OutputPins) {
            Schema->TryCreateConnection(Pin, TargetInputPin);
        }
    }
}

void FHNE_StateIDConnectionTracker::SetErrorMessages(TArray<UEdGraphPin*> Pins, FGuid StateID)
{
    TArray<UEdGraphPin*> InputPins = Pins.FilterByPredicate([](UEdGraphPin* InspectPin) {return InspectPin->Direction == EGPD_Input; });
    TArray<FString> AffectedNameIds;

    for (UEdGraphPin* InputPin : InputPins) {
        UEdGraphNode* InspectNode = InputPin->GetOwningNode();
        if (const UHierarchicalStateNode* InspectAsState = Cast< UHierarchicalStateNode>(InspectNode)) {
            UActorState* InnerAsState = Cast<UActorState>(InspectAsState->GetInnerObject());
            if (InnerAsState) {
                AffectedNameIds.Add(InnerAsState->NameID.ToString());
            }
        }
    }



    UE_LOG(LogTemp, Warning, TEXT("Ambiguous StateID '%s' may refer to the following: %s (%d pins affected)"), *(StateID.ToString()), *(FString::Join(AffectedNameIds, TEXT(", "))), Pins.Num());
    for (UEdGraphPin* Pin : Pins) {
        FString ErrorMessage = FString::Printf(TEXT("Ambiguous StateID %s on Pin %s"), *(StateID.ToString()), *(Pin->GetFName().ToString())) + "\n";
        UEdGraphNode* OwningNode = Pin->GetOwningNode();
        OwningNode->ErrorMsg += ErrorMessage;
        OwningNode->bHasCompilerMessage = true;
    }
}
