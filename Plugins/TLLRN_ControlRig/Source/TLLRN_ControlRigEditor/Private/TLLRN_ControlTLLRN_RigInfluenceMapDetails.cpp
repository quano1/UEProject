// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlTLLRN_RigInfluenceMapDetails.h"
#include "Widgets/SWidget.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "TLLRN_ControlRig.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"

#define LOCTEXT_NAMESPACE "TLLRN_RigInfluenceMapDetails"

void FTLLRN_RigInfluenceMapPerEventDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	BlueprintBeingCustomized = GetBlueprintFromDetailBuilder(DetailBuilder);
	if (BlueprintBeingCustomized == nullptr)
	{
		return;
	}

	FTLLRN_RigInfluenceMapPerEvent& MapPerEvent = BlueprintBeingCustomized->Influences;

	// add default events
	MapPerEvent.FindOrAdd(FTLLRN_RigUnit_BeginExecution::EventName);
	MapPerEvent.FindOrAdd(FTLLRN_RigUnit_PrepareForExecution::EventName);
}

UTLLRN_ControlRigBlueprint* FTLLRN_RigInfluenceMapPerEventDetails::GetBlueprintFromDetailBuilder(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TSharedPtr<FStructOnScope>> StructsBeingCustomized;
	DetailBuilder.GetStructsBeingCustomized(StructsBeingCustomized);
	if (StructsBeingCustomized.Num() > 0)
	{
		TSharedPtr<FStructOnScope> StructBeingCustomized = StructsBeingCustomized[0];
		if (UPackage* Package = StructBeingCustomized->GetPackage())
		{
			TArray<UObject*> SubObjects;
			Package->GetDefaultSubobjects(SubObjects);

			for (UObject* SubObject : SubObjects)
			{
				if (UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(SubObject))
				{
					if(UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(Rig->GetClass()->ClassGeneratedBy))
					{
						return Blueprint;
					}
				}
			}
		}
	}
	return nullptr;
}


#undef LOCTEXT_NAMESPACE
