// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_IKRetargetFactory.h"
#include "Retargeter/TLLRN_IKRetargeter.h"
#include "AssetTypeCategories.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRetargetFactory)

#define LOCTEXT_NAMESPACE "TLLRN_IKRetargeterFactory"

UTLLRN_IKRetargetFactory::UTLLRN_IKRetargetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UTLLRN_IKRetargeter::StaticClass();
}

UObject *UTLLRN_IKRetargetFactory::FactoryCreateNew(
	UClass *Class,
	UObject *InParent,
	FName Name,
	EObjectFlags Flags,
	UObject *Context,
	FFeedbackContext *Warn)
{
	return NewObject<UTLLRN_IKRetargeter>(InParent, Class, Name, Flags);
}

bool UTLLRN_IKRetargetFactory::ShouldShowInNewMenu() const
{
	return true;
}

FText UTLLRN_IKRetargetFactory::GetDisplayName() const
{
	return LOCTEXT("TLLRN_IKRetargeter_DisplayName", "TLLRN IK Retargeter");
}

uint32 UTLLRN_IKRetargetFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Animation;
}

FText UTLLRN_IKRetargetFactory::GetToolTip() const
{
	return LOCTEXT("TLLRN_IKRetargeter_Tooltip", "Defines a pair of Source/Target Retarget Rigs and the mapping between them.");
}

FString UTLLRN_IKRetargetFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("RTG_NewRetargeter"));
}

#undef LOCTEXT_NAMESPACE
