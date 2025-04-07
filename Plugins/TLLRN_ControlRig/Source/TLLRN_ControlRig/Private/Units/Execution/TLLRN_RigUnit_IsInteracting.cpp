// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Execution/TLLRN_RigUnit_IsInteracting.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_IsInteracting)

FTLLRN_RigUnit_IsInteracting_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	bIsInteracting = ExecuteContext.UnitContext.IsInteracting();
	bIsTranslating = ( ExecuteContext.UnitContext.InteractionType & (uint8)ETLLRN_ControlRigInteractionType::Translate) != 0;
	bIsRotating = ( ExecuteContext.UnitContext.InteractionType & (uint8)ETLLRN_ControlRigInteractionType::Rotate) != 0;
	bIsScaling = ( ExecuteContext.UnitContext.InteractionType & (uint8)ETLLRN_ControlRigInteractionType::Scale) != 0;
	Items = ExecuteContext.UnitContext.ElementsBeingInteracted;
}

