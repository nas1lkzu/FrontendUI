// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OptionsDataRegistry.generated.h"

class UListDataObject_Collection;
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
	void InitGameplayTabCollection();
	void InitAudioTabCollection();
	void InitVideoTabCollection();
	void InitControlTabCollection();

	UPROPERTY(Transient)
	TArray<UListDataObject_Collection*> RegisteredOptionsTabCollections;
};
