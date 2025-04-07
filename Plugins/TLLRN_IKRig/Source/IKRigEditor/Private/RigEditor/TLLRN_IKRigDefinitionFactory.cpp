// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigDefinitionFactory.h"
#include "Rig/TLLRN_IKRigDefinition.h"
#include "AssetTypeCategories.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRigDefinitionFactory)

#define LOCTEXT_NAMESPACE "TLLRN_IKRigDefinitionFactory"


UTLLRN_IKRigDefinitionFactory::UTLLRN_IKRigDefinitionFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UTLLRN_IKRigDefinition::StaticClass();
}

UObject* UTLLRN_IKRigDefinitionFactory::FactoryCreateNew(
	UClass* Class,
	UObject* InParent,
	FName InName,
	EObjectFlags InFlags,
	UObject* Context, 
	FFeedbackContext* Warn)
{
	// create the IK Rig asset
	return NewObject<UTLLRN_IKRigDefinition>(InParent, InName, InFlags | RF_Transactional);
}

bool UTLLRN_IKRigDefinitionFactory::ShouldShowInNewMenu() const
{
	return true;
}

FText UTLLRN_IKRigDefinitionFactory::GetDisplayName() const
{
	return LOCTEXT("TLLRN_IKRigDefinition_DisplayName", "TLLRN IK Rig");
}

uint32 UTLLRN_IKRigDefinitionFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Animation;
}

FText UTLLRN_IKRigDefinitionFactory::GetToolTip() const
{
	return LOCTEXT("TLLRN_IKRigDefinition_Tooltip", "Defines a set of IK Solvers and Effectors to pose a skeleton with Goals.");
}

FString UTLLRN_IKRigDefinitionFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("IK_NewIKRig"));
}
#undef LOCTEXT_NAMESPACE

