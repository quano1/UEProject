// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VisualGraphUtils.h"
#include "Rigs/TLLRN_RigHierarchy.h"

struct TLLRN_CONTROLRIGDEVELOPER_API FTLLRN_ControlRigVisualGraphUtils
{
	static FString DumpTLLRN_RigHierarchyToDotGraph(UTLLRN_RigHierarchy* InHierarchy, const FName& InEventName = NAME_None);
};
