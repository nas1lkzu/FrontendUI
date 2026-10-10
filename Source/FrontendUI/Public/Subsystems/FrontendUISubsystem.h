// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "FrontendUISubsystem.generated.h"

class UWidget_PrimaryLayout;
struct FGameplayTag;
class UWidget_ActivatableBase;
class UFrontendCommonButtonBase;

enum class ESyncPushWidgetState : uint8
{
	OnCreatedBeforePush,
	AfterPush
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnButtonTextDescriptionUpdatedDelegate, UFrontendCommonButtonBase*, BroadCastingButton, FText, DescriptionText);

UCLASS()
class FRONTENDUI_API UFrontendUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static	UFrontendUISubsystem* Get(const UObject* WorldContextObject);

	//~ Begin USubsystem Interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End USubsystem Interface
	UFUNCTION(BlueprintCallable)
	void RegisterCreatedPrimaryLayout(UWidget_PrimaryLayout* InCreatedWidget);
	void PushSoftWidgetToStackAsync
	(
		const FGameplayTag& InWidgetStackTag,
		TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass,
		TFunction<void (ESyncPushWidgetState, UWidget_ActivatableBase*)> ASyncPushStateCallback
	);
	void PushConfirmScreenToModalStackAsync(EConfirmScreenType InScreenType,const FText& InScreenTitle,const FText& InScreenMessage,TFunction<void(EConfirmScreenButtonType)> ButtonClickedCallback);
	UPROPERTY(BlueprintAssignable)
	FOnButtonTextDescriptionUpdatedDelegate OnButtonTextDescriptionUpdatedDelegate;
private:
	UPROPERTY(Transient)
	UWidget_PrimaryLayout* CreatedPrimaryLayout;
};
