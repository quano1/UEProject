// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Drawing/TLLRN_RigUnit_DrawContainer.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_DrawContainer)

FTLLRN_RigUnit_DrawContainerGetInstruction_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

    if(ExecuteContext.GetDrawContainer() == nullptr)
    {
    	return;
    }
	
	int32 Index = ExecuteContext.GetDrawContainer()->GetIndex(InstructionName);
	if (Index != INDEX_NONE)
	{
		const FRigVMDrawInstruction& Instruction = (*ExecuteContext.GetDrawContainer())[Index];
		Color = Instruction.Color;
		Transform = Instruction.Transform;
	}
	else
	{
		Color = FLinearColor::Red;
		Transform = FTransform::Identity;
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DrawContainerGetInstruction::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_DrawContainerSetThickness_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (ExecuteContext.GetDrawContainer() == nullptr)
	{
		return;
	}

	int32 Index = ExecuteContext.GetDrawContainer()->GetIndex(InstructionName);
	if (Index != INDEX_NONE)
	{
		FRigVMDrawInstruction& Instruction = (*ExecuteContext.GetDrawContainer())[Index];
		Instruction.Thickness = Thickness;
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DrawContainerSetThickness::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_DrawContainerSetColor_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (ExecuteContext.GetDrawContainer() == nullptr)
	{
		return;
	}

	int32 Index = ExecuteContext.GetDrawContainer()->GetIndex(InstructionName);
	if (Index != INDEX_NONE)
	{
		FRigVMDrawInstruction& Instruction = (*ExecuteContext.GetDrawContainer())[Index];
		Instruction.Color = Color;
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DrawContainerSetColor::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_DrawContainerSetTransform_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (ExecuteContext.GetDrawContainer() == nullptr)
	{
		return;
	}

	int32 Index = ExecuteContext.GetDrawContainer()->GetIndex(InstructionName);
	if (Index != INDEX_NONE)
	{
		FRigVMDrawInstruction& Instruction = (*ExecuteContext.GetDrawContainer())[Index];
		Instruction.Transform = Transform;
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DrawContainerSetTransform::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

