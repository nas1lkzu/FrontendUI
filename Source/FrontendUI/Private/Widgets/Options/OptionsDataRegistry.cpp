// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Options/OptionsDataRegistry.h"
#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"
#include "Data/FrontendOptionsTabRow.h"
#include "FrontendSettings/FrontendDeveloperSettings.h"
#include "Engine/DataTable.h"
#include "FrontendDebugHelper.h"

void UOptionsDataRegistry::InitOptionsDataRegistry(ULocalPlayer* InLocalPlayer)
{
	UDataTable* TabTable = LoadOptionsTabTable();
	if (!TabTable)
	{
		Debug::Print(TEXT("[OptionsDataRegistry] Failed to load OptionsTabTable. Check Project Settings."), 10, FColor::Red);
		return;
	}

	BuildTabCollectionsFromTable(TabTable);
}

UDataTable* UOptionsDataRegistry::LoadOptionsTabTable()
{
	if (CachedTabTable)
	{
		return CachedTabTable;
	}

	const UFrontendDeveloperSettings* Settings = GetDefault<UFrontendDeveloperSettings>();
	if (!Settings || Settings->OptionsTabTable.IsNull())
	{
		return nullptr;
	}

	CachedTabTable = Settings->OptionsTabTable.LoadSynchronous();
	return CachedTabTable;
}

void UOptionsDataRegistry::BuildTabCollectionsFromTable(UDataTable* InTable)
{
	if (!InTable)
	{
		return;
	}

	if (InTable->GetRowStruct() != FFrontendOptionsTabRow::StaticStruct())
	{
		ensureMsgf(false,
			TEXT("[OptionsDataRegistry] DataTable row struct mismatch: expected '%s', got '%s'. Aborting tab construction."),
			*FFrontendOptionsTabRow::StaticStruct()->GetName(),
			InTable->GetRowStruct() ? *InTable->GetRowStruct()->GetName() : TEXT("None"));
		return;
	}

	struct FTabRowEntry
	{
		FName RowName;
		FText DisplayName;
		int32 SortOrder = 0;
	};

	TArray<FTabRowEntry> SortedRows;
	SortedRows.Reserve(InTable->GetRowMap().Num());

	for (const TPair<FName, uint8*>& RowPair : InTable->GetRowMap())
	{
		if (!RowPair.Value)
		{
			continue;
		}

		const FFrontendOptionsTabRow* Row =
			reinterpret_cast<const FFrontendOptionsTabRow*>(RowPair.Value);

		FTabRowEntry& NewEntry = SortedRows.AddDefaulted_GetRef();
		NewEntry.RowName = RowPair.Key;
		NewEntry.DisplayName = Row->DisplayName;
		NewEntry.SortOrder = Row->SortOrder;
	}

	SortedRows.Sort([](const FTabRowEntry& A, const FTabRowEntry& B)
		{
			return A.SortOrder < B.SortOrder;
		});

	for (const FTabRowEntry& Entry : SortedRows)
	{
		UListDataObject_Collection* TabCollection = NewObject<UListDataObject_Collection>(this);
		TabCollection->SetDataID(Entry.RowName);
		TabCollection->SetDataDisplayName(Entry.DisplayName);
		RegisteredOptionsTabCollections.Add(TabCollection);
	}
}
