// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigBlueprintDetails.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "DetailLayoutBuilder.h"
#include "Rigs/TLLRN_RigHierarchy.h"

TSharedRef<IDetailCustomization> FTLLRN_ControlRigBlueprintDetails::MakeInstance()
{
	return MakeShareable(new FTLLRN_ControlRigBlueprintDetails);
}

void FTLLRN_ControlRigBlueprintDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	bool bIsValidTLLRN_RigModule = CVarTLLRN_ControlTLLRN_RigHierarchyEnableModules.GetValueOnAnyThread();
	if(bIsValidTLLRN_RigModule)
	{
		bIsValidTLLRN_RigModule = false;
		
		TArray<TWeakObjectPtr<UObject>> DetailObjects;
		DetailLayout.GetObjectsBeingCustomized(DetailObjects);
		for(TWeakObjectPtr<UObject> DetailObject : DetailObjects)
		{
			if(UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(DetailObject.Get()))
			{
				if(Blueprint->IsTLLRN_ControlTLLRN_RigModule())
				{
					Blueprint->UpdateExposedModuleConnectors();
					bIsValidTLLRN_RigModule = true;
				}
			}
		}
	}
	
	if(!bIsValidTLLRN_RigModule)
	{
		DetailLayout.HideProperty(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigBlueprint, TLLRN_RigModuleSettings));
	}
}