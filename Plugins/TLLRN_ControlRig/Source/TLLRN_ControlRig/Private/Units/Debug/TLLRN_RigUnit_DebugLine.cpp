// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Debug/TLLRN_RigUnit_DebugLine.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "RigVMFunctions/Debug/RigVMFunction_DebugLine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_DebugLine)

FTLLRN_RigUnit_DebugLine_Execute()
{
	FTLLRN_RigUnit_DebugLineItemSpace::StaticExecute(
		ExecuteContext, 
		A,
		B,
		Color,
		Thickness,
		FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Bone), 
		WorldOffset, 
		bEnabled);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DebugLine::GetUpgradeInfo() const
{
	FRigVMFunction_DebugLineNoSpace NewNode;
	NewNode.A = A;
	NewNode.B = B;
	NewNode.Color = Color;
	NewNode.Thickness = Thickness;
	NewNode.WorldOffset = WorldOffset;
	NewNode.bEnabled = bEnabled;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FTLLRN_RigUnit_DebugLineItemSpace_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	if (ExecuteContext.GetDrawInterface() == nullptr || !bEnabled)
	{
		return;
	}

	FVector DrawA = A, DrawB = B;
	if (Space.IsValid())
	{
		FTransform Transform = ExecuteContext.Hierarchy->GetGlobalTransform(Space);
		DrawA = Transform.TransformPosition(DrawA);
		DrawB = Transform.TransformPosition(DrawB);
	}

	ExecuteContext.GetDrawInterface()->DrawLine(WorldOffset, DrawA, DrawB, Color, Thickness);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DebugLineItemSpace::GetUpgradeInfo() const
{
	FRigVMFunction_DebugLineNoSpace NewNode;
	NewNode.A = A;
	NewNode.B = B;
	NewNode.Color = Color;
	NewNode.Thickness = Thickness;
	NewNode.WorldOffset = WorldOffset;
	NewNode.bEnabled = bEnabled;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}