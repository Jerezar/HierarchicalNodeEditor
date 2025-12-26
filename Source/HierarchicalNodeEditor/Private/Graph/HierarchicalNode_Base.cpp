
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

UObject* UHierarchicalNode_Base::GetFinalizedAssetRecursive() const
{

	UE_LOG(LogTemp, Warning, TEXT("Getting Finalized"))

	//No caching, 
	UObject* OutObject = DuplicateObject<UObject>(GetInnerObject(), GetTransientPackage(), NAME_None);

	for (UEdGraphPin* Pin : Pins) {
		if (Pin->Direction != EGPD_Output) continue; // only consider output pins
		if (!Pin->LinkedTo.Num()) continue; //only consider pins with actual connections

		FEdGraphPinType PinType = Pin->PinType;

		if (PinType.PinSubCategory != UHierarchicalGraphSchema::SC_ChildNode) continue;

		FProperty* Property = InnerClass->FindPropertyByName(Pin->PinName);

		if (PinType.ContainerType == EPinContainerType::Array) {
			TArray<UObject*> ChildObjects;
			
			UEdGraphNode* ArrayNode = nullptr;

			//Get connected array node from pin
			if (Pin->LinkedTo.Num()) {
				UEdGraphPin* ConnectedPin = *Pin->LinkedTo.begin();
				ArrayNode = ConnectedPin->GetOwningNode();
			}

			if (ArrayNode == nullptr) continue;

			//get connected child nodes from array node and add respective finalized assets
			for (UEdGraphPin* ArrayPin : ArrayNode->Pins) {
				if (ArrayPin->Direction != EGPD_Output) continue;
				if (!ArrayPin->LinkedTo.Num()) continue;

				UEdGraphPin* ConnectedPin = *ArrayPin->LinkedTo.begin();
				UEdGraphNode* ChildNode = ConnectedPin->GetOwningNode();

				UHierarchicalNode_Base* NodeAsBase = Cast< UHierarchicalNode_Base>(ChildNode);

				if (NodeAsBase == nullptr) continue;

				ChildObjects.Add(NodeAsBase->GetFinalizedAssetRecursive());
			}

			for (UObject* Child : ChildObjects) {
				Child->Rename(nullptr, OutObject);
			}

			TArray<UObject*>* ArrayPointer = Property->ContainerPtrToValuePtr<TArray<UObject*>>(OutObject);
			*ArrayPointer = ChildObjects;
		}
		else {

			UEdGraphNode* ChildNode = nullptr;

			if (Pin->LinkedTo.Num()) {
				UEdGraphPin* ConnectedPin = *Pin->LinkedTo.begin();
				ChildNode = ConnectedPin->GetOwningNode();
			}

			if (ChildNode == nullptr) continue;

			UHierarchicalNode_Base* ChildAsBase = Cast< UHierarchicalNode_Base>(ChildNode);

			if (ChildAsBase == nullptr) continue;

			UObject** ObjectPointer = Property->ContainerPtrToValuePtr<UObject*>(OutObject);

			UObject* ChildObject = ChildAsBase->GetFinalizedAssetRecursive();

			ChildObject->Rename(nullptr, OutObject);

			*ObjectPointer = ChildObject;
		}
	}
	
	return OutObject;
}
