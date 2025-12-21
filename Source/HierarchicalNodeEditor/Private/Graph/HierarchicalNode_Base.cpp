
#include "HierarchicalNode_Base.h"
#include "Graph/HierarchicalNodeGraph.h"
#include "HierarchicalEditInterface.h"
//#include "UObject/PropertyOptional.h"

void UHierarchicalNode_Base::InitializeHierarchicalNode()
{

	this->CreateNewGuid();

	if (InnerClass != nullptr) {
		UObject* NewInnerObject = NewObject<UObject>(this, InnerClass, NAME_None);
		if (InnerObject != nullptr) {

		}

		InnerObject = NewInnerObject;
	}

	SetUpInputPins();
	SetUpOutputPins();
}

void UHierarchicalNode_Base::SetUpOutputPins()
{

	for (TFieldIterator<FProperty> PropIterator(InnerClass); PropIterator; ++PropIterator) {

		UE_LOG(LogTemp, Warning, TEXT("Checking %s"), *(PropIterator->GetName()))

		bool bIsArrayField = false;
		FProperty* TestProperty = *PropIterator;

		FArrayProperty* TestArray = CastField<FArrayProperty>(*PropIterator);
		if (TestArray != nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("%s is ArrayProperty."), *(PropIterator->GetName()))
			TestProperty = TestArray->Inner;
			bIsArrayField = true;
		}

		FObjectPropertyBase* TestObjectProperty = CastField<FObjectPropertyBase>(TestProperty);

		if (TestObjectProperty == nullptr) continue;

		UE_LOG(LogTemp, Warning, TEXT("%s is ObjectProperty."), *(PropIterator->GetName()))

		if (TestObjectProperty->PropertyClass->ImplementsInterface(UHierarchicalEditInterface::StaticClass())) {

			FEdGraphPinType PinType;
			PinType.PinSubCategory = UHierarchicalGraphSchema::SC_ChildNode;
			PinType.ContainerType = bIsArrayField ? EPinContainerType::Array : EPinContainerType::None;
			PinType.PinSubCategoryObject = TestObjectProperty->PropertyClass;

			this->CreatePin(
				EGPD_Output,
				PinType,
				PropIterator->GetName()
			);
		}
	}
}
