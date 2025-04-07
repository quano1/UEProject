// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigHierarchyPose.h"
#include "TLLRN_TLLRN_RigBoneHierarchy.h"
#include "TLLRN_TLLRN_RigSpaceHierarchy.h"
#include "TLLRN_TLLRN_RigControlHierarchy.h"
#include "TLLRN_TLLRN_RigCurveContainer.h"
#include "TLLRN_RigHierarchyCache.h"
#include "TLLRN_RigInfluenceMap.h"
#include "TLLRN_RigHierarchyContainer.generated.h"

class UTLLRN_ControlRig;
struct FTLLRN_RigHierarchyContainer;

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyContainer
{
public:

	GENERATED_BODY()

	FTLLRN_RigHierarchyContainer();
	FTLLRN_RigHierarchyContainer(const FTLLRN_RigHierarchyContainer& InOther);
	FTLLRN_RigHierarchyContainer& operator= (const FTLLRN_RigHierarchyContainer& InOther);

	UPROPERTY()
	FTLLRN_TLLRN_RigBoneHierarchy BoneHierarchy;

	UPROPERTY()
	FTLLRN_TLLRN_RigSpaceHierarchy SpaceHierarchy;

	UPROPERTY()
	FTLLRN_TLLRN_RigControlHierarchy ControlHierarchy;

	UPROPERTY()
	FTLLRN_TLLRN_RigCurveContainer CurveContainer;

private:

	TArray<FTLLRN_RigElementKey> ImportFromText(const FTLLRN_RigHierarchyCopyPasteContent& InData);

	friend class STLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
};

// this struct is still here for backwards compatibility - but not used anywhere
USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyRef
{
	GENERATED_BODY()
};