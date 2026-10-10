// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Options/OptionsDataRegistry.h"
#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"

void UOptionsDataRegistry::InitOptionsDataRegistry(ULocalPlayer* InLocalPlayer)
{
	InitGameplayTabCollection();
	InitAudioTabCollection();
	InitVideoTabCollection();
	InitControlTabCollection();
}

void UOptionsDataRegistry::InitGameplayTabCollection()
{
	UListDataObject_Collection* GameplayTabCollection = NewObject<UListDataObject_Collection>(this);
	GameplayTabCollection->SetDataID(FName("GameplayTabCollection"));
	GameplayTabCollection->SetDataDisplayName(FText::FromString("Gameplay"));
	RegisteredOptionsTabCollections.Add(GameplayTabCollection);
}

void UOptionsDataRegistry::InitAudioTabCollection()
{
	UListDataObject_Collection* AudioTabCollection = NewObject<UListDataObject_Collection>(this);
	AudioTabCollection->SetDataID(FName("AudioTabCollection"));
	AudioTabCollection->SetDataDisplayName(FText::FromString("Audio"));
	RegisteredOptionsTabCollections.Add(AudioTabCollection);
}

void UOptionsDataRegistry::InitVideoTabCollection()
{
	UListDataObject_Collection* VideoTabCollection = NewObject<UListDataObject_Collection>(this);
	VideoTabCollection->SetDataID(FName("VideoTabCollection"));
	VideoTabCollection->SetDataDisplayName(FText::FromString("Video"));
	RegisteredOptionsTabCollections.Add(VideoTabCollection);
}

void UOptionsDataRegistry::InitControlTabCollection()
{
	UListDataObject_Collection* ControlTabCollection = NewObject<UListDataObject_Collection>(this);
	ControlTabCollection->SetDataID(FName("ControlTabCollection"));
	ControlTabCollection->SetDataDisplayName(FText::FromString("Control"));
	RegisteredOptionsTabCollections.Add(ControlTabCollection);
}
