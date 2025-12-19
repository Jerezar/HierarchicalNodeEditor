
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

	for (TFieldIterator<FArrayProperty> PropIterator(InnerClass); PropIterator; ++PropIterator) {


		FObjectPropertyBase* InnerAsObjectProperty = Cast<FObjectPropertyBase>(PropIterator->Inner);
		FStructProperty* InnerAsStructProperty = Cast<FStructProperty>(PropIterator->Inner);
		//FOptionalProperty* InnerAsOptionalProperty = Cast<FOptionalProperty>(PropIterator->Inner);


		UE_LOG(LogTemp, Warning, TEXT("Array property: %s; inner: %s"), *(PropIterator->GetName()), *(PropIterator->Inner->GetCPPType()))

		UE_LOG(LogTemp, Warning, TEXT("bInnerExists: %d; bInnerIsObject: %d; bInnerIsStruct: %d"), !(PropIterator->Inner == nullptr), !(InnerAsObjectProperty == nullptr), !(InnerAsStructProperty == nullptr))

		if (InnerAsObjectProperty == nullptr) continue;

		if (InnerAsObjectProperty->PropertyClass->ImplementsInterface(UHierarchicalEditInterface::StaticClass())) {

			FEdGraphPinType PinType;
			PinType.PinSubCategory = UHierarchicalGraphSchema::SC_ChildNodeArray;
			PinType.PinSubCategoryObject = InnerAsObjectProperty->PropertyClass;

			this->CreatePin(
				EGPD_Output,
				PinType,
				PropIterator->GetName()
			);
		}
	}

	for (TFieldIterator<FObjectProperty> PropIterator(InnerClass); PropIterator; ++PropIterator) {
		if (PropIterator->PropertyClass->ImplementsInterface(UHierarchicalEditInterface::StaticClass())) {

			FEdGraphPinType PinType;
			PinType.PinSubCategory = UHierarchicalGraphSchema::SC_ChildNode;
			PinType.PinSubCategoryObject = PropIterator->PropertyClass;

			this->CreatePin(
				EGPD_Output,
				PinType,
				PropIterator->GetName()
			);
		}
	}
}
