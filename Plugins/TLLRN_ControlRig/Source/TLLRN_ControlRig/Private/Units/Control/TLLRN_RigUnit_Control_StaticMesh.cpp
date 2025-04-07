// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Control/TLLRN_RigUnit_Control_StaticMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_Control_StaticMesh)

FTLLRN_RigUnit_Control_StaticMesh::FTLLRN_RigUnit_Control_StaticMesh()
{
}

FTLLRN_RigUnit_Control_StaticMesh_Execute()
{
	FTLLRN_RigUnit_Control::StaticExecute(ExecuteContext, Transform, Base, InitTransform, Result, Filter, bIsInitialized);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_Control_StaticMesh::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}
