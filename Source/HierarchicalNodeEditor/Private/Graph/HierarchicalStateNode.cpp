#include "HierarchicalStateNode.h"
#include "Graph/HierarchicalNodeGraph.h"
#include "ActorStateID.h"

void UHierarchicalStateNode::SetUpInputPins()
{
	UHierarchicalChildNode::SetUpInputPins();

	FEdGraphPinType PinType;
	PinType.PinSubCategory = UHierarchicalGraphSchema::SC_StateTransition;
	PinType.ContainerType = EPinContainerType::None;

	this->CreatePin(
		EGPD_Input,
		PinType,
		FName("StateID")
	);
}

void UHierarchicalStateNode::SetUpOutputPins()
{
	UHierarchicalChildNode::SetUpOutputPins();


	for (TFieldIterator<FProperty> PropIterator(InnerClass); PropIterator; ++PropIterator) {

		UE_LOG(LogTemp, Warning, TEXT("Checking %s"), *(PropIterator->GetName()))

		if (PropIterator->GetName() == "UniqueId") {
			continue;
		}

		bool bIsArrayField = false;
		FProperty* TestProperty = *PropIterator;

		FArrayProperty* TestArray = CastField<FArrayProperty>(*PropIterator);
		if (TestArray != nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("%s is ArrayProperty."), *(PropIterator->GetName()))
				TestProperty = TestArray->Inner;
			bIsArrayField = true;
		}

		FStructProperty* TestStructProperty = CastField<FStructProperty>(TestProperty);

		if (TestStructProperty == nullptr) continue;

		UE_LOG(LogTemp, Warning, TEXT("%s is StructProperty."), *(PropIterator->GetName()))

			if (TestStructProperty->Struct == TBaseStructure<FActorStateID>::Get()) {

				FEdGraphPinType PinType;
				PinType.PinSubCategory = UHierarchicalGraphSchema::SC_StateTransition;
				PinType.ContainerType = bIsArrayField ? EPinContainerType::Array : EPinContainerType::None;

				this->CreatePin(
					EGPD_Output,
					PinType,
					PropIterator->GetName()
				);
			}
	}
}
