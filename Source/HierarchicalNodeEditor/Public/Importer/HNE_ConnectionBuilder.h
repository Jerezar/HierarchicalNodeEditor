#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"

class FHNE_ConnectionTracker {
public:
	virtual bool RegisterValuePin(UEdGraphPin* InPin, UObject* InObject) { return false; };
	virtual bool RegisterArrayPins(TArray<UEdGraphPin*> InPins, FArrayProperty* InProperty, UObject* InObject) { return false; };
	virtual void SetUpConnections() {};
};

class FHNE_MultiConnectionBuilder : public FHNE_ConnectionTracker {
public:
	FHNE_MultiConnectionBuilder(TMap<FName, TSharedPtr<FHNE_ConnectionTracker>> Trackers) {TrackersBySubcategory = Trackers; }

	virtual bool RegisterValuePin(UEdGraphPin* InPin, UObject* InObject) override;
	virtual bool RegisterArrayPins(TArray<UEdGraphPin*> InPins, FArrayProperty* InProperty, UObject* InObject) override;
	virtual void SetUpConnections() override;
protected:
	TMap<FName, TSharedPtr<FHNE_ConnectionTracker>> TrackersBySubcategory;
};