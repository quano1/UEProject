// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_TLLRN_RigBoneHierarchy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_TLLRN_RigBoneHierarchy)

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_TLLRN_RigBoneHierarchy
////////////////////////////////////////////////////////////////////////////////

FTLLRN_TLLRN_RigBoneHierarchy::FTLLRN_TLLRN_RigBoneHierarchy()
{
}

FTLLRN_RigBone& FTLLRN_TLLRN_RigBoneHierarchy::Add(const FName& InNewName, const FName& InParentName, ETLLRN_RigBoneType InType, const FTransform& InInitTransform, const FTransform& InLocalTransform, const FTransform& InGlobalTransform)
{
	FTLLRN_RigBone NewBone;
	NewBone.Name = InNewName;
	NewBone.ParentIndex = INDEX_NONE; // we no longer support parent index lookup
	NewBone.ParentName = InParentName;
	NewBone.InitialTransform = InInitTransform;
	NewBone.LocalTransform = InLocalTransform;
	NewBone.GlobalTransform = InGlobalTransform;
	NewBone.Type = InType;

	const int32 Index = Bones.Add(NewBone);
	return Bones[Index];
}
