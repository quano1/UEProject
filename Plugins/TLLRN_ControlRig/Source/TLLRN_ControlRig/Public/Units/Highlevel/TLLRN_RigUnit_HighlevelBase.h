// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_RigUnit_HighlevelBase.generated.h"

UENUM()
enum class ETLLRN_ControlTLLRN_RigVectorKind : uint8
{
	Direction,
	Location
};

USTRUCT(meta=(Abstract, Category="Debug", NodeColor = "0.462745 1.0 0.329412"))
struct FTLLRN_RigUnit_HighlevelBase : public FTLLRN_RigUnit
{
	GENERATED_BODY()
};

USTRUCT(meta=(Abstract, Category="Debug", NodeColor = "0 0.364706 1.0"))
struct FTLLRN_RigUnit_HighlevelBaseMutable : public FTLLRN_RigUnitMutable
{
	GENERATED_BODY()
};
