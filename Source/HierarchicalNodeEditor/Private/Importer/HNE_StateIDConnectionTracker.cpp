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
        UHierarchicalStateNode* TargetNode = Cast< UHierarchicalStateNode>(TargetInputPin->GetOwningNode());

        if (TargetNode == nullptr) {
            UE_LOG(LogTemp, Error, TEXT("Input node for %s was not a state node. %d nodes affected."), *(PinsOfID.Key.ToString()), Pins.Num());
            continue;
        }

        UActorState* InnerState = Cast< UActorState>(TargetNode->GetInnerObject());
        FName NameID = InnerState->NameID;

        UEdGraph* ParentGraph = TargetNode->GetGraph();

        const UEdGraphSchema* Schema = TargetInputPin->GetSchema();

        TArray<UEdGraphPin*> OutputPins = Pins.FilterByPredicate([](UEdGraphPin* InspectPin) {return !(InspectPin->Direction == EGPD_Input); });

        if (!OutputPins.Num()) continue;

        TSortedMap<int32, UEdGraphNode*> ReroutesByOffset;

        for (UEdGraphPin* Pin : OutputPins) {
            UEdGraphNode* OwnerNode = Pin->GetOwningNode();
            TArray<UEdGraphPin*> OwnerOutputs = OwnerNode->GetAllPins().FilterByPredicate([](UEdGraphPin* InspectPin) {return InspectPin->Direction == EGPD_Output; });

            int32 OutputIndex;
            OwnerOutputs.Find(Pin, OutputIndex);
            ++OutputIndex;

            UHNE_RerouteNode* Reroute = NewObject< UHNE_RerouteNode >(ParentGraph);
            Reroute->NodePosX = OwnerNode->NodePosX + 256;
            Reroute->NodePosY = OwnerNode->NodePosY + 32 * OutputIndex;

            ParentGraph->Modify();
            ParentGraph->AddNode(Reroute, true, false);

            Reroute->PinTypeTemplate = FEdGraphPinType(Pin->PinType);
            Reroute->InitializeNode();

            int32 OffsetY = Reroute->NodePosY - TargetNode->NodePosY;

            ReroutesByOffset.Add(OffsetY, Reroute);

            UEdGraphPin* RerouteInputPin = Reroute->FindPin(NAME_None, EGPD_Input);

            Schema->TryCreateConnection(Pin, RerouteInputPin);

            Reroute->NodeComment = NameID.ToString();
            Reroute->bCommentBubbleVisible = true;
        }

        bool bEverSwapped = false;
        int32 PreviousOffset = 0;
        UEdGraphNode* PreviousNode = nullptr;
        for (TPair<int32, UEdGraphNode*> Entry : ReroutesByOffset) {
            UEdGraphNode* CurrentReroute = Entry.Value;

            bool bIsAbove = Entry.Key < 0;

            bool bSwappedFromAboveToBelow = (PreviousOffset < 0) && !bIsAbove;
            bEverSwapped |= bSwappedFromAboveToBelow;

            if (bSwappedFromAboveToBelow) {
                //Connect previous node to target pin
                Schema->TryCreateConnection(TargetInputPin, PreviousNode->FindPin(NAME_None, EGPD_Output));
                PreviousNode = nullptr;

                //Also connect new node to target pin
                Schema->TryCreateConnection(TargetInputPin, CurrentReroute->FindPin(NAME_None, EGPD_Output));
            }

            if (PreviousNode != nullptr) {
                //Connect current node with previous node

                Schema->TryCreateConnection(
                    PreviousNode->FindPin(NAME_None, (bIsAbove ? EGPD_Output : EGPD_Input)),
                    CurrentReroute->FindPin(NAME_None, (bIsAbove ? EGPD_Input : EGPD_Output))
                );
            }

            PreviousNode = CurrentReroute;
            PreviousOffset = Entry.Key;
        }

        if (!bEverSwapped) {
            TArray<int32> Keys;
            ReroutesByOffset.GetKeys(Keys);
            bool bPurelyAbove = (PreviousOffset < 0);

            int32 RerouteKey = bPurelyAbove ? Keys.Last() : (*Keys.begin());

            UEdGraphNode** Reroute = ReroutesByOffset.Find(RerouteKey);

            if (Reroute == nullptr) {
                UE_LOG(LogTemp, Error, TEXT("Cannot find reroute node of correct offset, despite just making it."))
            }
            else {
                Schema->TryCreateConnection(
                    TargetInputPin,
                    (*Reroute)->FindPin(NAME_None, EGPD_Output)
                );
                
            }
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
