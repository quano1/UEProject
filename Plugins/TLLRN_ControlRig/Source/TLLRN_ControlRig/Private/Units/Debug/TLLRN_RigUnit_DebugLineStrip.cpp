// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Debug/TLLRN_RigUnit_DebugLineStrip.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "RigVMFunctions/Debug/RigVMFunction_DebugLineStrip.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_DebugLineStrip)

FTLLRN_RigUnit_DebugLineStrip_Execute()
{
	FTLLRN_RigUnit_DebugLineStripItemSpace::StaticExecute(
		ExecuteContext, 
		Points,
		Color,
		Thickness,
		FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Bone), 
		WorldOffset, 
		bEnabled);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DebugLineStrip::GetUpgradeInfo() const
{
	FRigVMFunction_DebugLineStripNoSpace NewNode;
	NewNode.Points = Points;
	NewNode.Color = Color;
	NewNode.Thickness = Thickness;
	NewNode.WorldOffset = WorldOffset;
	NewNode.bEnabled = bEnabled;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FTLLRN_RigUnit_DebugLineStripItemSpace_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	if (ExecuteContext.GetDrawInterface() == nullptr || !bEnabled)
	{
		return;
	}

	if (Space.IsValid())
	{
		FTransform Transform = ExecuteContext.Hierarchy->GetGlobalTransform(Space);
		TArray<FVector> PointsTransformed;
		PointsTransformed.Reserve(Points.Num());
		for(const FVector& Point : Points)
		{
			PointsTransformed.Add(Transform.TransformPosition(Point));
		}
		ExecuteContext.GetDrawInterface()->DrawLineStrip(WorldOffset, PointsTransformed, Color, Thickness);
	}
	else
	{
		ExecuteContext.GetDrawInterface()->DrawLineStrip(WorldOffset, TArrayView<const FVector>(Points.GetData(), Points.Num()), Color, Thickness);
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DebugLineStripItemSpace::GetUpgradeInfo() const
{
	FRigVMFunction_DebugLineStripNoSpace NewNode;
	NewNode.Points = Points;
	NewNode.Color = Color;
	NewNode.Thickness = Thickness;
	NewNode.WorldOffset = WorldOffset;
	NewNode.bEnabled = bEnabled;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

