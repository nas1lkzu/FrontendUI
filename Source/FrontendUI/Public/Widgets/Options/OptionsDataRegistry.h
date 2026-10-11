// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OptionsDataRegistry.generated.h"

class UListDataObject_Collection;
class UDataTable;
/**
 * 
 */
UCLASS()
class FRONTENDUI_API UOptionsDataRegistry : public UObject
{
	GENERATED_BODY()
public:
	void InitOptionsDataRegistry(ULocalPlayer* InLocalPlayer);
	const TArray<UListDataObject_Collection*>& GetRegisteredOptionsTabCollections() const {return RegisteredOptionsTabCollections;}
private:
	UDataTable* LoadOptionsTabTable();
	void BuildTabCollectionsFromTable(UDataTable* InTable);

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedTabTable;

	UPROPERTY(Transient)
	TArray<UListDataObject_Collection*> RegisteredOptionsTabCollections;
};
