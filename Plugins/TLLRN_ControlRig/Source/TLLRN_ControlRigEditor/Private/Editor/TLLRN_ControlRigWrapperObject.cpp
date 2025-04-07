// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_ControlRigWrapperObject.h"

#if WITH_EDITOR
#include "TLLRN_ControlTLLRN_RigElementDetails.h"
#include "TLLRN_ControlTLLRN_RigModuleDetails.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#endif

UClass* UTLLRN_ControlRigWrapperObject::GetClassForStruct(UScriptStruct* InStruct, bool bCreateIfNeeded) const
{
	UClass* Class = Super::GetClassForStruct(InStruct, bCreateIfNeeded);
	if(Class == nullptr)
	{
		return nullptr;
	}
	
	const FName WrapperClassName = Class->GetFName();

#if WITH_EDITOR
	if(InStruct->IsChildOf(FTLLRN_RigBaseElement::StaticStruct()))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		if (!PropertyEditorModule.GetClassNameToDetailLayoutNameMap().Contains(WrapperClassName))
		{
			if(InStruct == FTLLRN_RigBoneElement::StaticStruct())
			{
				PropertyEditorModule.RegisterCustomClassLayout(WrapperClassName, FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_RigBoneElementDetails::MakeInstance));
			}
			else if(InStruct == FTLLRN_RigNullElement::StaticStruct())
			{
				PropertyEditorModule.RegisterCustomClassLayout(WrapperClassName, FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_RigNullElementDetails::MakeInstance));
			}
			else if(InStruct == FTLLRN_RigControlElement::StaticStruct())
			{
				PropertyEditorModule.RegisterCustomClassLayout(WrapperClassName, FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_RigControlElementDetails::MakeInstance));
			}
			else if(InStruct == FTLLRN_RigConnectorElement::StaticStruct())
			{
				PropertyEditorModule.RegisterCustomClassLayout(WrapperClassName, FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_RigConnectorElementDetails::MakeInstance));
			}
			else if(InStruct == FTLLRN_RigSocketElement::StaticStruct())
			{
				PropertyEditorModule.RegisterCustomClassLayout(WrapperClassName, FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_RigSocketElementDetails::MakeInstance));
			}
			else if(InStruct == FTLLRN_RigPhysicsElement::StaticStruct())
			{
				PropertyEditorModule.RegisterCustomClassLayout(WrapperClassName, FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_RigPhysicsElementDetails::MakeInstance));
			}
		}
	}
#endif

	return Class;
}
