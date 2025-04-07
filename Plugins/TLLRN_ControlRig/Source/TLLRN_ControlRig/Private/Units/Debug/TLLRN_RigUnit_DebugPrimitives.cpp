// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Debug/TLLRN_RigUnit_DebugPrimitives.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "RigVMFunctions/Debug/RigVMFunction_DebugPrimitives.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_DebugPrimitives)

FTLLRN_RigUnit_DebugRectangle_Execute()
{
	FTLLRN_RigUnit_DebugRectangleItemSpace::StaticExecute(
		ExecuteContext, 
		Transform,
		Color,
		Scale,
		Thickness,
		FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Bone), 
		WorldOffset, 
		bEnabled);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DebugRectangle::GetUpgradeInfo() const
{
	FRigVMFunction_DebugRectangleNoSpace NewNode;
	NewNode.Transform = Transform;
	NewNode.Color = Color;
	NewNode.Scale = Scale;
	NewNode.Thickness = Thickness;
	NewNode.WorldOffset = WorldOffset;
	NewNode.bEnabled = bEnabled;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DebugRectangleItemSpace::GetUpgradeInfo() const
{
	FRigVMFunction_DebugRectangleNoSpace NewNode;
	NewNode.Transform = Transform;
	NewNode.Color = Color;
	NewNode.Scale = Scale;
	NewNode.Thickness = Thickness;
	NewNode.WorldOffset = WorldOffset;
	NewNode.bEnabled = bEnabled;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_DebugRectangleItemSpace_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	if (ExecuteContext.GetDrawInterface() == nullptr || !bEnabled)
	{
		return;
	}

	FTransform DrawTransform = Transform;
	if (Space.IsValid())
	{
		DrawTransform = DrawTransform * ExecuteContext.Hierarchy->GetGlobalTransform(Space);
	}

	ExecuteContext.GetDrawInterface()->DrawRectangle(WorldOffset, DrawTransform, Scale, Color, Thickness);
}

FTLLRN_RigUnit_DebugArc_Execute()
{
	FTLLRN_RigUnit_DebugArcItemSpace::StaticExecute(
		ExecuteContext, 
		Transform,
		Color,
		Radius,
		MinimumDegrees,
		MaximumDegrees,
		Thickness,
		Detail,
		FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Bone), 
		WorldOffset, 
		bEnabled);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DebugArc::GetUpgradeInfo() const
{
	FRigVMFunction_DebugArcNoSpace NewNode;
	NewNode.Transform = Transform;
	NewNode.Color = Color;
	NewNode.Radius = Radius;
	NewNode.MinimumDegrees = MinimumDegrees;
	NewNode.MaximumDegrees = MaximumDegrees;
	NewNode.Thickness = Thickness;
	NewNode.Detail = Detail;
	NewNode.WorldOffset = WorldOffset;
	NewNode.bEnabled = bEnabled;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_DebugArcItemSpace::GetUpgradeInfo() const
{
	FRigVMFunction_DebugArcNoSpace NewNode;
	NewNode.Transform = Transform;
	NewNode.Color = Color;
	NewNode.Radius = Radius;
	NewNode.MinimumDegrees = MinimumDegrees;
	NewNode.MaximumDegrees = MaximumDegrees;
	NewNode.Thickness = Thickness;
	NewNode.Detail = Detail;
	NewNode.WorldOffset = WorldOffset;
	NewNode.bEnabled = bEnabled;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

FTLLRN_RigUnit_DebugArcItemSpace_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	if (ExecuteContext.GetDrawInterface() == nullptr || !bEnabled)
	{
		return;
	}

	FTransform DrawTransform = Transform;
	if (Space.IsValid())
	{
		DrawTransform = DrawTransform * ExecuteContext.Hierarchy->GetGlobalTransform(Space);
	}

	ExecuteContext.GetDrawInterface()->DrawArc(WorldOffset, DrawTransform, Radius, FMath::DegreesToRadians(MinimumDegrees), FMath::DegreesToRadians(MaximumDegrees), Color, Thickness, Detail);
}
