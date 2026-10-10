// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/DataObjects/ListDataObject_Base.h"
#include "ListDataObject_Collection.generated.h"

/**
 * 
 */
UCLASS()
class FRONTENDUI_API UListDataObject_Collection : public UListDataObject_Base
{
	GENERATED_BODY()
public:
	//~Begin UListDataObject_Base Interface
	const TArray<UListDataObject_Base*>& GetAllChildListData() const override;
	bool HasAnyChildListData() const override;
	//~End UListDataObject_Base Interface
	void AddChildListData(UListDataObject_Base* InChildData);
private:
	UPROPERTY(Transient)
	TArray<UListDataObject_Base*> ChildListDataArray;
};
