// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Options/Widget_OptionsScreen.h"
#include "Input/CommonUIInputTypes.h"
#include "ICommonInputModule.h"
#include "Widgets/Options/OptionsDataRegistry.h"
#include "Widgets/Components/FrontendTabListWidgetBase.h"
#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"

#include "FrontendDebugHelper.h"

void UWidget_OptionsScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ResetAction.IsNull())
	{
		ResetActionBindingHandle = RegisterUIActionBinding(
			FBindUIActionArgs(
				ResetAction,
				true,
				FSimpleDelegate::CreateUObject(this,&UWidget_OptionsScreen::OnResetBoundActionTriggered)
			)
		);
	}
	RegisterUIActionBinding(
		FBindUIActionArgs(
			ICommonInputModule::GetSettings().GetDefaultBackAction(),
			true,
			FSimpleDelegate::CreateUObject(this, &UWidget_OptionsScreen::OnBackBoundActionTriggered)
		)
	);
	// BindWidget 缺失属于蓝图配置错误：这里报一次，运行期各入口再做兜底，避免直接崩溃
	ensureMsgf(TabListWidget_OptionsTabs, TEXT("[OptionsScreen] TabListWidget_OptionsTabs is not bound in the widget blueprint."));

	if (IsValid(TabListWidget_OptionsTabs))
	{
		TabListWidget_OptionsTabs->OnTabSelected.AddUniqueDynamic(this, &UWidget_OptionsScreen::OnOptionsTabSelected);
	}
}

void UWidget_OptionsScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (!IsValid(TabListWidget_OptionsTabs))
	{
		UE_LOG(LogTemp, Error, TEXT("[OptionsScreen] TabListWidget_OptionsTabs is not bound. Options tabs cannot be registered."));
		return;
	}

	const TArray<UListDataObject_Collection*>& TabCollections = GetOrCreateDataRegistry()->GetRegisteredOptionsTabCollections();

	for (UListDataObject_Collection* TabCollection : TabCollections)
	{
		if (!TabCollection)
		{
			continue;
		}

		const FName TabID = TabCollection->GetDataID();
		if (TabListWidget_OptionsTabs->GetTabButtonBaseByID(TabID) != nullptr)
		{
			continue;
		}

		TabListWidget_OptionsTabs->RequestRegisterTab(TabID, TabCollection->GetDataDisplayName());
	}
}

void UWidget_OptionsScreen::OnOptionsTabSelected(FName TabID)
{
	Debug::Print(TEXT("New Tab Selected. Tab ID : ") + TabID.ToString());
}

UOptionsDataRegistry* UWidget_OptionsScreen::GetOrCreateDataRegistry()
{
	if (!CreatedOwningDataRegistry)
	{
		CreatedOwningDataRegistry = NewObject<UOptionsDataRegistry>(this);
		CreatedOwningDataRegistry->InitOptionsDataRegistry(GetOwningLocalPlayer());
	}

	ensureMsgf(CreatedOwningDataRegistry, TEXT("[OptionsScreen] Failed to create the options data registry."));
	return CreatedOwningDataRegistry;
}

void UWidget_OptionsScreen::OnResetBoundActionTriggered()
{
	Debug::Print(TEXT("reset button triggered"));
}

void UWidget_OptionsScreen::OnBackBoundActionTriggered()
{
	DeactivateWidget();
}
