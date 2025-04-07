// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_TLLRN_RigSpaceHierarchy.h"
#include "Rigs/TLLRN_RigHierarchyContainer.h"
#include "HelperUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_TLLRN_RigSpaceHierarchy)

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_TLLRN_RigSpaceHierarchy
////////////////////////////////////////////////////////////////////////////////

FTLLRN_TLLRN_RigSpaceHierarchy::FTLLRN_TLLRN_RigSpaceHierarchy()
{
}

FTLLRN_RigSpace& FTLLRN_TLLRN_RigSpaceHierarchy::Add(const FName& InNewName, ETLLRN_RigSpaceType InSpaceType, const FName& InParentName, const FTransform& InTransform)
{
	FTLLRN_RigSpace NewSpace;
	NewSpace.Name = InNewName;
	NewSpace.ParentIndex = INDEX_NONE; // we no longer support indices 
	NewSpace.SpaceType = InSpaceType;
	NewSpace.ParentName = InParentName;
	NewSpace.InitialTransform = InTransform;
	NewSpace.LocalTransform = InTransform;
	const int32 Index = Spaces.Add(NewSpace);
	return Spaces[Index];
}

