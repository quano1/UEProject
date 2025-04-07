// Copyright Epic Games, Inc. All Rights Reserved.
#include "Tools/TLLRN_ControlTLLRN_RigPoseMirrorSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlTLLRN_RigPoseMirrorSettings)

UTLLRN_ControlTLLRN_RigPoseMirrorSettings::UTLLRN_ControlTLLRN_RigPoseMirrorSettings()
{
	RightSide = TEXT("_r_");
	LeftSide = TEXT("_l_");
	MirrorAxis = EAxis::X;
	AxisToFlip = EAxis::X;
}

