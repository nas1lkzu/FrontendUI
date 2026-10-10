// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widget_OptionsScreen.generated.h"

class UOptionsDataRegistry;
class UFrontendTabListWidgetBase;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class FRONTENDUI_API UWidget_OptionsScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()
protected:
	//~Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~End UUserWidget Interface

	//~Begin UCommonActivatableWidget Interface
	virtual void NativeOnActivated() override;
	//~End UCommonActivatableWidget Interface
private:
	UFUNCTION()
	void OnOptionsTabSelected(FName TabID);

	UOptionsDataRegistry* GetOrCreateDataRegistry();
	UPROPERTY(meta = (BindWidget))
	UFrontendTabListWidgetBase* TabListWidget_OptionsTabs;
	void OnResetBoundActionTriggered();
	void OnBackBoundActionTriggered();
	UPROPERTY(Transient)
	UOptionsDataRegistry* CreatedOwningDataRegistry;
	UPROPERTY(EditDefaultsOnly, Category = "Frontend Options Screen", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle ResetAction;
	FUIActionBindingHandle ResetActionBindingHandle;
};
