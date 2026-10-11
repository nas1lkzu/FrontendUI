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
	TArray<FName> RowNames;
	InTable->GetRowMap().GenerateKeyArray(RowNames);

	RowNames.Sort([InTable](const FName& A, const FName& B)
		{
			const FFrontendOptionsTabRow* RowA = InTable->FindRow<FFrontendOptionsTabRow>(A, TEXT("OptionsTabSort"), false);
			const FFrontendOptionsTabRow* RowB = InTable->FindRow<FFrontendOptionsTabRow>(B, TEXT("OptionsTabSort"), false);
			if (!RowA || !RowB)
			{
				return false;
			}
			return RowA->SortOrder < RowB->SortOrder;
		});

	for (const FName& RowName : RowNames)
	{
		const FFrontendOptionsTabRow* Row = InTable->FindRow<FFrontendOptionsTabRow>(RowName, TEXT("OptionsTabInit"), false);
		if (!Row)
		{
			continue;
		}

		UListDataObject_Collection* TabCollection = NewObject<UListDataObject_Collection>(this);
		TabCollection->SetDataID(RowName);					
		TabCollection->SetDataDisplayName(Row->DisplayName);
		RegisteredOptionsTabCollections.Add(TabCollection);
	}
}
