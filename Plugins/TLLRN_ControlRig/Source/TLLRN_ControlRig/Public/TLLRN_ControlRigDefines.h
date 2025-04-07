// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyPathHelpers.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Rigs/TLLRN_RigHierarchyCache.h"
#include "Stats/StatsHierarchical.h"
#include "RigVMCore/RigVMExecuteContext.h"
#include "TLLRN_ControlRigDefines.generated.h"

#define DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
//#define DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT() \
//	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

#define UE_TLLRN_CONTROLRIG_LLM_SCOPE_BYNAME(InLabel)
//#define UE_TLLRN_CONTROLRIG_LLM_SCOPE_BYNAME(InLabel) \
//	LLM_SCOPE_BYNAME(TEXT(InLabel));

UENUM()
enum class ETLLRN_TransformSpaceMode : uint8
{
	/** Apply in parent space */
	LocalSpace,

	/** Apply in rig space*/
	GlobalSpace,

	/** Apply in Base space */
	BaseSpace,

	/** Apply in base bone */
	BaseJoint,

	/** MAX - invalid */
	Max UMETA(Hidden),
};

UENUM()
enum class ETLLRN_TransformGetterType : uint8
{
	Initial,
	Current,
	Max UMETA(Hidden),
};

// thought of mixing this with execution on
// the problem is execution on is transient state, and 
// this execution type is something to be set per rig
UENUM()
enum class ETLLRN_RigExecutionType : uint8
{
	Runtime,
	Editing, // editing time
	Max UMETA(Hidden),
};
