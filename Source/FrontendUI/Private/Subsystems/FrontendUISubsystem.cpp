// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/FrontendUISubsystem.h"
#include "Engine/AssetManager.h"
#include "Widgets/Widget_PrimaryLayout.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Widgets/Widget_ConfirmScreen.h"
#include "FrontendGameplayTags.h"
#include "FrontendFunctionLibrary.h"

#include "FrontendDebugHelper.h"

UFrontendUISubsystem* UFrontendUISubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject,EGetWorldErrorMode::Assert);

		return UGameInstance::GetSubsystem<UFrontendUISubsystem>(World->GetGameInstance());
	}

	return nullptr;
}

bool UFrontendUISubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!CastChecked<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{
		TArray<UClass*> FoundClasses;
		GetDerivedClasses(GetClass(), FoundClasses);
		return FoundClasses.IsEmpty();
	}
	return false;
}

void UFrontendUISubsystem::RegisterCreatedPrimaryLayout(UWidget_PrimaryLayout* InCreatedWidget)
{
	check(InCreatedWidget);
	CreatedPrimaryLayout = InCreatedWidget;
}

void UFrontendUISubsystem::PushSoftWidgetToStackAsync
(
	const FGameplayTag& InWidgetStackTag,
	TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass,
	TFunction<void(ESyncPushWidgetState, UWidget_ActivatableBase*)> ASyncPushStateCallback
)
{
	check(!InSoftWidgetClass.IsNull());
	UAssetManager::Get().GetStreamableManager().RequestAsyncLoad
	(
		InSoftWidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda
		(
			[InSoftWidgetClass, this, InWidgetStackTag, ASyncPushStateCallback]()
			{
				UClass* LoadedWidgetClass = InSoftWidgetClass.Get();
				if (!LoadedWidgetClass || !IsValid(CreatedPrimaryLayout))
				{
					UE_LOG(LogTemp, Error, TEXT("[PushSoftWidget] Primary layout or widget class is not valid"));
					return;
				}
				UCommonActivatableWidgetContainerBase* FoundWidgetStack = CreatedPrimaryLayout->FindWidgetStackByTag(InWidgetStackTag);
				UWidget_ActivatableBase* CreatedWidget = FoundWidgetStack->AddWidget<UWidget_ActivatableBase>
					(
						LoadedWidgetClass,
						[ASyncPushStateCallback](UWidget_ActivatableBase& CreatedWidgetInstance)
						{
							ASyncPushStateCallback(ESyncPushWidgetState::OnCreatedBeforePush, &CreatedWidgetInstance);
						}
					);
				ASyncPushStateCallback(ESyncPushWidgetState::AfterPush, CreatedWidget);
			}
		)
	);
}

void UFrontendUISubsystem::PushConfirmScreenToModalStackAsync(EConfirmScreenType InScreenType, const FText& InScreenTitle, const FText& InScreenMessage, TFunction<void(EConfirmScreenButtonType)> ButtonClickedCallback)
{
	UConfirmScreenInfoObject* CreatedScreenInfoObject = nullptr;
	switch (InScreenType)
	{
	case EConfirmScreenType::Ok:
		CreatedScreenInfoObject = UConfirmScreenInfoObject::CreateOkScreen(InScreenTitle,InScreenMessage);
		break;
	case EConfirmScreenType::YesNo:
		CreatedScreenInfoObject = UConfirmScreenInfoObject::CreateYesNoScreen(InScreenTitle,InScreenMessage);
		break;
	case EConfirmScreenType::OkCancel:
		CreatedScreenInfoObject = UConfirmScreenInfoObject::CreateOkCancelScreen(InScreenTitle, InScreenMessage);
		break;
	case EConfirmScreenType::Unknown:
		break;
	}

	check(CreatedScreenInfoObject);
	PushSoftWidgetToStackAsync(FrontendGameplayTags::Frontend_WidgetStack_Modal,
		UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(FrontendGameplayTags::Frontend_Widget_ConfirmScreen),
		[CreatedScreenInfoObject, ButtonClickedCallback](ESyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			if (InPushState == ESyncPushWidgetState::OnCreatedBeforePush)
			{
				UWidget_ConfirmScreen* CreatedConfirmScreen = CastChecked<UWidget_ConfirmScreen>(PushedWidget);
				CreatedConfirmScreen->InitScreen(CreatedScreenInfoObject, ButtonClickedCallback);
			}
		}
	);
}
