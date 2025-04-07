// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_SetSpaceInitialTransform.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_SetSpaceInitialTransform)

FTLLRN_RigUnit_SetSpaceInitialTransform_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	UTLLRN_RigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy)
	{
		const FTLLRN_RigElementKey SpaceKey(SpaceName, ETLLRN_RigElementType::Null);
		if (!CachedSpaceIndex.UpdateCache(SpaceKey, Hierarchy))
		{
			UE_TLLRN_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Space '%s' is not valid."), *SpaceName.ToString());
			return;
		}

		FTransform InitialTransform = Transform;
		if (Space == ERigVMTransformSpace::GlobalSpace)
		{
			const FTransform ParentTransform = Hierarchy->GetParentTransformByIndex(CachedSpaceIndex, true);
			InitialTransform = InitialTransform.GetRelativeTransform(ParentTransform);
		}

		Hierarchy->SetInitialLocalTransform(CachedSpaceIndex, InitialTransform);
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SetSpaceInitialTransform::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_SetTransform NewNode;
	NewNode.Item = FTLLRN_RigElementKey(SpaceName, ETLLRN_RigElementType::Null);
	NewNode.Space = Space;
	NewNode.Value = Transform;
	NewNode.bInitial = true;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("SpaceName"), TEXT("Item.Name"));
	Info.AddRemappedPin(TEXT("Transform"), TEXT("Value"));
	return Info;
}

