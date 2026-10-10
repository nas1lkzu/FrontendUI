// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Widget_ConfirmScreen.h"
#include "CommonTextBlock.h"
#include "Components/DynamicEntryBox.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "ICommonInputModule.h"

UConfirmScreenInfoObject* UConfirmScreenInfoObject::CreateOkScreen(const FText& InScreenTitle, const FText& InScreenMessage)
{
    UConfirmScreenInfoObject* InfoObject = NewObject<UConfirmScreenInfoObject>();
    InfoObject->ScreenTitle = InScreenTitle;
    InfoObject->ScreenMessage = InScreenMessage;
    
    FConfirmScreenButtonInfo OkButtonInfo;
    OkButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Closed;
    OkButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("Ok"));
    
    InfoObject->AvailableScreenButtons.Add(OkButtonInfo);
    return InfoObject;
}

UConfirmScreenInfoObject* UConfirmScreenInfoObject::CreateYesNoScreen(const FText& InScreenTitle, const FText& InScreenMessage)
{
    UConfirmScreenInfoObject* InfoObject = NewObject<UConfirmScreenInfoObject>();
    InfoObject->ScreenTitle = InScreenTitle;
    InfoObject->ScreenMessage = InScreenMessage;

    FConfirmScreenButtonInfo YesButtonInfo;
    YesButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Confirmed;
    YesButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("Yes"));

    FConfirmScreenButtonInfo NoButtonInfo;
    NoButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Cancelled;
    NoButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("No"));

    InfoObject->AvailableScreenButtons.Add(YesButtonInfo);
    InfoObject->AvailableScreenButtons.Add(NoButtonInfo);
    return InfoObject;
}

UConfirmScreenInfoObject* UConfirmScreenInfoObject::CreateOkCancelScreen(const FText& InScreenTitle, const FText& InScreenMessage)
{
    UConfirmScreenInfoObject* InfoObject = NewObject<UConfirmScreenInfoObject>();
    InfoObject->ScreenTitle = InScreenTitle;
    InfoObject->ScreenMessage = InScreenMessage;

    FConfirmScreenButtonInfo OkButtonInfo;
    OkButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Confirmed;
    OkButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("Ok"));

    FConfirmScreenButtonInfo CancelButtonInfo;
    CancelButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Cancelled;
    CancelButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("Cancel"));

    InfoObject->AvailableScreenButtons.Add(OkButtonInfo);
    InfoObject->AvailableScreenButtons.Add(CancelButtonInfo);
    return InfoObject;
}

void UWidget_ConfirmScreen::InitScreen(UConfirmScreenInfoObject* InScreenInfoObject, TFunction<void(EConfirmScreenButtonType)> ClickedButtonCallBack)
{
    if (!IsValid(InScreenInfoObject))
    {
        UE_LOG(LogTemp, Error, TEXT("InScreenInfoObject is nullptr"));
        return;
    }
    CommonTextBlock_Title->SetText(InScreenInfoObject->ScreenTitle);
    CommonTextBlock_Message->SetText(InScreenInfoObject->ScreenMessage);

    if (UDynamicEntryBox_Buttons->GetNumEntries() != 0)
    {
        UDynamicEntryBox_Buttons->Reset<UFrontendCommonButtonBase>(
            [](UFrontendCommonButtonBase& ExistingButton)
            {
                ExistingButton.OnClicked().Clear();
            }
        );
    }

    check(!InScreenInfoObject->AvailableScreenButtons.IsEmpty());

    for (const FConfirmScreenButtonInfo& AvailableButtonInfo : InScreenInfoObject->AvailableScreenButtons)
    {
        FDataTableRowHandle InputActionRowHandle;
        switch (AvailableButtonInfo.ConfirmScreenButtonType)
        {
        case EConfirmScreenButtonType::Cancelled:
            InputActionRowHandle = ICommonInputModule::GetSettings().GetDefaultBackAction();
            break;
        case EConfirmScreenButtonType::Closed:
            InputActionRowHandle = ICommonInputModule::GetSettings().GetDefaultBackAction();

        }
        UFrontendCommonButtonBase* AddedButton = UDynamicEntryBox_Buttons->CreateEntry<UFrontendCommonButtonBase>();
        AddedButton->SetButtonText(AvailableButtonInfo.ButtonTextToDisplay);
        AddedButton->SetTriggeringInputAction(InputActionRowHandle);
        AddedButton->OnClicked().AddLambda(
            [this, ClickedButtonCallBack, AvailableButtonInfo]()
            {
                ClickedButtonCallBack(AvailableButtonInfo.ConfirmScreenButtonType);
                DeactivateWidget();
            }

        );
    }

    if (UDynamicEntryBox_Buttons->GetNumEntries() != 0)
    {
        UDynamicEntryBox_Buttons->GetAllEntries().Last()->SetFocus();
    }
}
