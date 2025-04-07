// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Hierarchy/TLLRN_RigUnit_BoneName.h"
#include "Units/TLLRN_RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_BoneName)

FTLLRN_RigUnit_Item_Execute()
{
}

FTLLRN_RigUnit_ItemArray_Execute()
{
}

FTLLRN_RigUnit_BoneName_Execute()
{
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_BoneName::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_Item NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Bone, ETLLRN_RigElementType::Bone);

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Bone"), TEXT("Item.Name"));
	return Info;
}

FTLLRN_RigUnit_SpaceName_Execute()
{
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_SpaceName::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_Item NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Space, ETLLRN_RigElementType::Null);

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Space"), TEXT("Item.Name"));
	return Info;
}

FTLLRN_RigUnit_ControlName_Execute()
{
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ControlName::GetUpgradeInfo() const
{
	FTLLRN_RigUnit_Item NewNode;
	NewNode.Item = FTLLRN_RigElementKey(Control, ETLLRN_RigElementType::Control);

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Control"), TEXT("Item.Name"));
	return Info;
}

