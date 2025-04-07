// Copyright Epic Games, Inc. All Rights Reserved.

#include "Retargeter/TLLRN_IKRetargetSettings.h"
#include "Retargeter/TLLRN_IKRetargeter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRetargetSettings)

bool FTLLRN_TargetChainSpeedPlantSettings::operator==(const FTLLRN_TargetChainSpeedPlantSettings& Other) const
{
	return EnableSpeedPlanting == Other.EnableSpeedPlanting
	&& SpeedCurveName == Other.SpeedCurveName
	&& FMath::IsNearlyEqualByULP(SpeedThreshold, Other.SpeedThreshold)
	&& FMath::IsNearlyEqualByULP(UnplantStiffness, Other.UnplantStiffness)
	&& FMath::IsNearlyEqualByULP(UnplantCriticalDamping, Other.UnplantCriticalDamping);
}

bool FTLLRN_TargetChainFKSettings::operator==(const FTLLRN_TargetChainFKSettings& Other) const
{
	return EnableFK == Other.EnableFK
	   && RotationMode == Other.RotationMode
	   && FMath::IsNearlyEqualByULP(RotationAlpha,Other.RotationAlpha)
	   && TranslationMode == Other.TranslationMode
	   && FMath::IsNearlyEqualByULP(TranslationAlpha, Other.TranslationAlpha)
	   && PoleVectorMatching == Other.PoleVectorMatching
	   && FMath::IsNearlyEqualByULP(PoleVectorOffset, Other.PoleVectorOffset);
}

bool FTLLRN_TargetChainIKSettings::operator==(const FTLLRN_TargetChainIKSettings& Other) const
{
	return EnableIK == Other.EnableIK
	&& FMath::IsNearlyEqualByULP(BlendToSource, Other.BlendToSource)
	&& BlendToSourceWeights.Equals(Other.BlendToSourceWeights)
	&& StaticOffset.Equals(Other.StaticOffset)
	&& StaticLocalOffset.Equals(Other.StaticLocalOffset)
	&& StaticRotationOffset.Equals(Other.StaticRotationOffset)
	&& FMath::IsNearlyEqualByULP(ScaleVertical, Other.ScaleVertical)
	&& FMath::IsNearlyEqualByULP(Extension, Other.Extension)
	&& bAffectedByIKWarping == Other.bAffectedByIKWarping;
}

bool FTLLRN_TargetChainSettings::operator==(const FTLLRN_TargetChainSettings& Other) const
{
	return FK == Other.FK && IK == Other.IK && SpeedPlanting == Other.SpeedPlanting;
}
