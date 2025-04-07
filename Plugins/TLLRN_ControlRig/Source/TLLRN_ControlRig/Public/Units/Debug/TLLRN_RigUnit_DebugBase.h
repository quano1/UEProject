// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_DebugBase.generated.h"

USTRUCT(meta=(Abstract, Category="Debug", NodeColor = "0.83077 0.846873 0.049707"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugBase : public FTLLRN_RigUnit
{
	GENERATED_BODY()
};

USTRUCT(meta=(Abstract, Category="Debug", NodeColor = "0.83077 0.846873 0.049707"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit_DebugBaseMutable : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()
};
