// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Debug/TLLRN_RigUnit_VisualDebug.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "RigVMFunctions/Debug/RigVMFunction_VisualDebug.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_VisualDebug)

FTLLRN_RigUnit_VisualDebugVector_Execute()
{
	FTLLRN_RigUnit_VisualDebugVectorItemSpace::StaticExecute(ExecuteContext, Value, bEnabled, Mode, Color, Thickness, Scale, FTLLRN_RigElementKey(BoneSpace, ETLLRN_RigElementType::Bone));
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_VisualDebugVector::GetUpgradeInfo() const
{
	FRigVMFunction_VisualDebugVectorNoSpace NewNode;
	NewNode.Value = Value;
	NewNode.bEnabled = bEnabled;
	NewNode.Mode = Mode;
	NewNode.Color = Color;
	NewNode.Thickness = Thickness;
	NewNode.Scale = Scale;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FTLLRN_RigUnit_VisualDebugVectorItemSpace_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (ExecuteContext.GetDrawInterface() == nullptr || !bEnabled)
	{
		return;
	}

	FTransform WorldOffset = FTransform::Identity;
	if (Space.IsValid())
	{
		WorldOffset = ExecuteContext.Hierarchy->GetGlobalTransform(Space);
	}

	switch(Mode)
	{
		case ERigUnitVisualDebugPointMode::Point:
		{
			ExecuteContext.GetDrawInterface()->DrawPoint(WorldOffset, Value, Thickness, Color);
			break;
		}
		case ERigUnitVisualDebugPointMode::Vector:
		{
			ExecuteContext.GetDrawInterface()->DrawLine(WorldOffset, FVector::ZeroVector, Value * Scale, Color, Thickness);
			break;
		}
		default:
		{
			checkNoEntry();
		}
	}
}

FTLLRN_RigUnit_VisualDebugQuat_Execute()
{
	FTLLRN_RigUnit_VisualDebugQuatItemSpace::StaticExecute(ExecuteContext, Value, bEnabled, Thickness, Scale, FTLLRN_RigElementKey(BoneSpace, ETLLRN_RigElementType::Bone));
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_VisualDebugVectorItemSpace::GetUpgradeInfo() const
{
	FRigVMFunction_VisualDebugVectorNoSpace NewNode;
	NewNode.Value = Value;
	NewNode.bEnabled = bEnabled;
	NewNode.Mode = Mode;
	NewNode.Color = Color;
	NewNode.Thickness = Thickness;
	NewNode.Scale = Scale;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_VisualDebugQuat::GetUpgradeInfo() const
{
	FRigVMFunction_VisualDebugQuatNoSpace NewNode;
	NewNode.Value = Value;
	NewNode.Thickness = Thickness;
	NewNode.Scale = Scale;
	NewNode.bEnabled = bEnabled;
	
	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FTLLRN_RigUnit_VisualDebugQuatItemSpace_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

    FTransform Transform = FTransform::Identity;
    Transform.SetRotation(Value);

	FTLLRN_RigUnit_VisualDebugTransformItemSpace::StaticExecute(ExecuteContext, Transform, bEnabled, Thickness, Scale, Space);
}

FTLLRN_RigUnit_VisualDebugTransform_Execute()
{
	FTLLRN_RigUnit_VisualDebugTransformItemSpace::StaticExecute(ExecuteContext, Value, bEnabled, Thickness, Scale, FTLLRN_RigElementKey(BoneSpace, ETLLRN_RigElementType::Bone));
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_VisualDebugQuatItemSpace::GetUpgradeInfo() const
{
	FRigVMFunction_VisualDebugQuatNoSpace NewNode;
	NewNode.Value = Value;
	NewNode.Thickness = Thickness;
	NewNode.Scale = Scale;
	NewNode.bEnabled = bEnabled;
	
	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_VisualDebugTransform::GetUpgradeInfo() const
{
	FRigVMFunction_VisualDebugTransformNoSpace NewNode;
	NewNode.Value = Value;
	NewNode.Thickness = Thickness;
	NewNode.Scale = Scale;
	NewNode.bEnabled = bEnabled;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_VisualDebugTransformItemSpace::GetUpgradeInfo() const
{
	FRigVMFunction_VisualDebugTransformNoSpace NewNode;
	NewNode.Value = Value;
	NewNode.Thickness = Thickness;
	NewNode.Scale = Scale;
	NewNode.bEnabled = bEnabled;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	return Info;
}

FTLLRN_RigUnit_VisualDebugTransformItemSpace_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (ExecuteContext.GetDrawInterface() == nullptr || !bEnabled)
	{
		return;
	}

	FTransform WorldOffset = FTransform::Identity;
	if (Space.IsValid())
	{
		WorldOffset = ExecuteContext.Hierarchy->GetGlobalTransform(Space);
	}

	ExecuteContext.GetDrawInterface()->DrawAxes(WorldOffset, Value, Scale, Thickness);
}
