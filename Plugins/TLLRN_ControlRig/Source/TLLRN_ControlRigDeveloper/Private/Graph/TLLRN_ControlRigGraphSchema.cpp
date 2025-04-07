// Copyright Epic Games, Inc. All Rights Reserved.

#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InteractionExecution.h"
#include "Units/Modules/TLLRN_RigUnit_ConnectorExecution.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigGraphSchema)



#define LOCTEXT_NAMESPACE "TLLRN_ControlRigGraphSchema"

UTLLRN_ControlRigGraphSchema::UTLLRN_ControlRigGraphSchema()
{
}

FLinearColor UTLLRN_ControlRigGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	const FName& TypeName = PinType.PinCategory;
	if (TypeName == UEdGraphSchema_K2::PC_Struct)
	{
		if (UStruct* Struct = Cast<UStruct>(PinType.PinSubCategoryObject))
		{
			if (Struct == FTLLRN_RigElementKey::StaticStruct() || Struct == FTLLRN_RigElementKeyCollection::StaticStruct())
			{
				return FLinearColor(0.0, 0.6588, 0.9490);
			}

			if (Struct == FTLLRN_RigElementKey::StaticStruct() || Struct == FTLLRN_RigPose::StaticStruct())
			{
				return FLinearColor(0.0, 0.3588, 0.5490);
			}
		}
	}
	
	return Super::GetPinTypeColor(PinType);
}

bool UTLLRN_ControlRigGraphSchema::IsRigVMDefaultEvent(const FName& InEventName) const
{
	if(Super::IsRigVMDefaultEvent(InEventName))
	{
		return true;
	}
	
	return InEventName == FTLLRN_RigUnit_BeginExecution::EventName ||
		InEventName == FTLLRN_RigUnit_PreBeginExecution::EventName ||
		InEventName == FTLLRN_RigUnit_PostBeginExecution::EventName ||
		InEventName == FTLLRN_RigUnit_InverseExecution::EventName ||
		InEventName == FTLLRN_RigUnit_PrepareForExecution::EventName ||
		InEventName == FTLLRN_RigUnit_PostPrepareForExecution::EventName ||
		InEventName == FTLLRN_RigUnit_InteractionExecution::EventName ||
		InEventName == FTLLRN_RigUnit_ConnectorExecution::EventName;
}

#undef LOCTEXT_NAMESPACE

