// Copyright Epic Games, Inc. All Rights Reserved.

#include "Retargeter/TLLRN_IKRetargeter.h"

#include "TLLRN_IKRigObjectVersion.h"
#include "Retargeter/TLLRN_IKRetargetOps.h"
#include "Retargeter/TLLRN_IKRetargetProfile.h"
#include "Engine/SkeletalMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRetargeter)

#if WITH_EDITOR
const FName UTLLRN_IKRetargeter::GetSourceIKRigPropertyName() { return GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_IKRetargeter, SourceIKRigAsset); };
const FName UTLLRN_IKRetargeter::GetTargetIKRigPropertyName() { return GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_IKRetargeter, TargetIKRigAsset); };
const FName UTLLRN_IKRetargeter::GetSourcePreviewMeshPropertyName() { return GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_IKRetargeter, SourcePreviewMesh); };
const FName UTLLRN_IKRetargeter::GetTargetPreviewMeshPropertyName() { return GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_IKRetargeter, TargetPreviewMesh); }

void UTLLRN_IKRetargeter::GetSpeedCurveNames(TArray<FName>& OutSpeedCurveNames) const
{
	for (const UTLLRN_RetargetChainSettings* ChainSetting : ChainSettings)
	{
		if (ChainSetting->Settings.SpeedPlanting.SpeedCurveName != NAME_None)
		{
			OutSpeedCurveNames.Add(ChainSetting->Settings.SpeedPlanting.SpeedCurveName);
		}
	}
}
#endif

UTLLRN_IKRetargeter::UTLLRN_IKRetargeter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootSettings = CreateDefaultSubobject<UTLLRN_RetargetRootSettings>(TEXT("RootSettings"));
	RootSettings->SetFlags(RF_Transactional);

	GlobalSettings = CreateDefaultSubobject<UTLLRN_IKRetargetGlobalSettings>(TEXT("GlobalSettings"));
	GlobalSettings->SetFlags(RF_Transactional);

	OpStack = CreateDefaultSubobject<UTLLRN_RetargetOpStack>(TEXT("PostSettings"));
	OpStack->SetFlags(RF_Transactional);

	CleanAndInitialize();
}

const UTLLRN_IKRigDefinition* UTLLRN_IKRetargeter::GetIKRig(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	const TSoftObjectPtr<UTLLRN_IKRigDefinition> SoftIKRig = SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? SourceIKRigAsset : TargetIKRigAsset;
	if (SoftIKRig.IsValid())
	{
		return SoftIKRig.Get();
	}
	
	return IsInGameThread() ? SoftIKRig.LoadSynchronous() : nullptr;
}

UTLLRN_IKRigDefinition* UTLLRN_IKRetargeter::GetIKRigWriteable(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	const TSoftObjectPtr<UTLLRN_IKRigDefinition> SoftIKRig = SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? SourceIKRigAsset : TargetIKRigAsset;
	if (SoftIKRig.IsValid())
	{
		return SoftIKRig.Get();
	}
	
	return IsInGameThread() ? SoftIKRig.LoadSynchronous() : nullptr;
}

#if WITH_EDITORONLY_DATA
USkeletalMesh* UTLLRN_IKRetargeter::GetPreviewMesh(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const
{
	if (!IsInGameThread())
	{
		return nullptr;
	}

	// the preview mesh override on the retarget takes precedence
	if (SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source)
	{
		if (SourcePreviewMesh.IsValid())
		{
			return SourcePreviewMesh.LoadSynchronous();
		}
	}
	else
	{
		if (TargetPreviewMesh.IsValid())
		{
			return TargetPreviewMesh.LoadSynchronous();
		}
	}

	// fallback to preview mesh from the IK Rig itself
	if (const UTLLRN_IKRigDefinition* IKRig = GetIKRig(SourceOrTarget))
	{
		return IKRig->GetPreviewMesh();
	}

	return nullptr;
}
#endif

void UTLLRN_IKRetargeter::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
#if WITH_EDITORONLY_DATA
	Controller = nullptr;
#endif
}

void UTLLRN_IKRetargeter::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FTLLRN_IKRigObjectVersion::GUID);
};

void UTLLRN_IKRetargeter::PostLoad()
{
	Super::PostLoad();

	// very early versions of the asset may not have been set as standalone
	SetFlags(RF_Standalone);

	// load deprecated chain mapping (pre UStruct to UObject refactor)
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (!ChainMapping_DEPRECATED.IsEmpty())
	{
		for (const FTLLRN_RetargetChainMap& OldChainMap : ChainMapping_DEPRECATED)
		{
			if (OldChainMap.TargetChain == NAME_None)
			{
				continue;
			}
			
			TObjectPtr<UTLLRN_RetargetChainSettings>* MatchingChain = ChainSettings.FindByPredicate([&](const UTLLRN_RetargetChainSettings* Chain)
			{
				return Chain ? Chain->TargetChain == OldChainMap.TargetChain : false;
			});
			
			if (MatchingChain)
			{
				(*MatchingChain)->SourceChain = OldChainMap.SourceChain;
			}
			else
			{
				TObjectPtr<UTLLRN_RetargetChainSettings> NewChainMap = NewObject<UTLLRN_RetargetChainSettings>(this, UTLLRN_RetargetChainSettings::StaticClass(), NAME_None, RF_Transactional);
				NewChainMap->TargetChain = OldChainMap.TargetChain;
				NewChainMap->SourceChain = OldChainMap.SourceChain;
				ChainSettings.Add(NewChainMap);
			}
		}
	}

	#if WITH_EDITORONLY_DATA
		// load deprecated target actor offset
		if (!FMath::IsNearlyZero(TargetActorOffset_DEPRECATED))
		{
			TargetMeshOffset.X = TargetActorOffset_DEPRECATED;
		}

		// load deprecated target actor scale
		if (!FMath::IsNearlyZero(TargetActorScale_DEPRECATED))
		{
			TargetMeshScale = TargetActorScale_DEPRECATED;
		}

		// load deprecated global settings
		if (!bRetargetRoot_DEPRECATED)
		{
			GlobalSettings->Settings.bEnableRoot = false;
		}
		if (!bRetargetFK_DEPRECATED)
		{
			GlobalSettings->Settings.bEnableFK = false;
		}
		if (!bRetargetIK_DEPRECATED)
		{
			GlobalSettings->Settings.bEnableIK = false;
		}
	#endif

	// load deprecated retarget poses (pre adding retarget poses for source)
	if (!RetargetPoses_DEPRECATED.IsEmpty())
	{
		TargetRetargetPoses = RetargetPoses_DEPRECATED;
	}

	// load deprecated current retarget pose (pre adding retarget poses for source)
	if (CurrentRetargetPose_DEPRECATED != NAME_None)
	{
		CurrentTargetRetargetPose = CurrentRetargetPose_DEPRECATED;
	}
	
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	CleanAndInitialize();
}

#if WITH_EDITORONLY_DATA
void UTLLRN_IKRetargeter::DeclareConstructClasses(TArray<FTopLevelAssetPath>& OutConstructClasses, const UClass* SpecificSubclass)
{
	Super::DeclareConstructClasses(OutConstructClasses, SpecificSubclass);
	OutConstructClasses.Add(FTopLevelAssetPath(UTLLRN_RetargetChainSettings::StaticClass()));
}
#endif

void UTLLRN_IKRetargeter::CleanAndInitialize()
{
	// remove null retarget ops
	OpStack->RetargetOps.Remove(nullptr);
	
	// remove null settings
	ChainSettings.Remove(nullptr);

	// use default pose as current pose unless set to something else
	if (CurrentSourceRetargetPose == NAME_None)
	{
		CurrentSourceRetargetPose = GetDefaultPoseName();
	}
	if (CurrentTargetRetargetPose == NAME_None)
	{
		CurrentTargetRetargetPose = GetDefaultPoseName();
	}

	// enforce the existence of a default pose
	if (!SourceRetargetPoses.Contains(GetDefaultPoseName()))
	{
		SourceRetargetPoses.Emplace(GetDefaultPoseName());
	}
	if (!TargetRetargetPoses.Contains(GetDefaultPoseName()))
	{
		TargetRetargetPoses.Emplace(GetDefaultPoseName());
	}

	// ensure current pose exists, otherwise set it to the default pose
	if (!SourceRetargetPoses.Contains(CurrentSourceRetargetPose))
	{
		CurrentSourceRetargetPose = GetDefaultPoseName();
	}
	if (!TargetRetargetPoses.Contains(CurrentTargetRetargetPose))
	{
		CurrentTargetRetargetPose = GetDefaultPoseName();
	}
};

void UTLLRN_RetargetChainSettings::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FTLLRN_IKRigObjectVersion::GUID);

#if WITH_EDITORONLY_DATA
	if (Ar.IsLoading())
	{
		// load the old chain settings into the new struct format
		if (Ar.CustomVer(FTLLRN_IKRigObjectVersion::GUID) < FTLLRN_IKRigObjectVersion::ChainSettingsConvertedToStruct)
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			Settings.FK.EnableFK =  CopyPoseUsingFK_DEPRECATED;
			Settings.FK.RotationMode =  RotationMode_DEPRECATED;
			Settings.FK.RotationAlpha =  RotationAlpha_DEPRECATED;
			Settings.FK.TranslationMode =  TranslationMode_DEPRECATED;
			Settings.FK.TranslationAlpha =  TranslationAlpha_DEPRECATED;
			Settings.IK.EnableIK =  DriveIKGoal_DEPRECATED;
			Settings.IK.BlendToSource =  BlendToSource_DEPRECATED;
			Settings.IK.BlendToSourceWeights =  BlendToSourceWeights_DEPRECATED;
			Settings.IK.StaticOffset =  StaticOffset_DEPRECATED;
			Settings.IK.StaticLocalOffset =  StaticLocalOffset_DEPRECATED;
			Settings.IK.StaticRotationOffset =  StaticRotationOffset_DEPRECATED;
			Settings.IK.Extension =  Extension_DEPRECATED;
			Settings.SpeedPlanting.EnableSpeedPlanting =  UseSpeedCurveToPlantIK_DEPRECATED;
			Settings.SpeedPlanting.SpeedCurveName =  SpeedCurveName_DEPRECATED;
			Settings.SpeedPlanting.SpeedThreshold =  VelocityThreshold_DEPRECATED;
			Settings.SpeedPlanting.UnplantStiffness =  UnplantStiffness_DEPRECATED;
			Settings.SpeedPlanting.UnplantCriticalDamping =  UnplantCriticalDamping_DEPRECATED;
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
	}
#endif
}

void UTLLRN_RetargetRootSettings::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FTLLRN_IKRigObjectVersion::GUID);

#if WITH_EDITORONLY_DATA
	if (Ar.IsLoading())
	{
		// load the old root settings into the new struct format
		if (Ar.CustomVer(FTLLRN_IKRigObjectVersion::GUID) < FTLLRN_IKRigObjectVersion::ChainSettingsConvertedToStruct)
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			Settings.ScaleHorizontal = GlobalScaleHorizontal_DEPRECATED;
			Settings.ScaleVertical = GlobalScaleVertical_DEPRECATED;
			Settings.BlendToSource = BlendToSource_DEPRECATED.Size();
			Settings.TranslationOffset = StaticOffset_DEPRECATED;
			Settings.RotationOffset = StaticRotationOffset_DEPRECATED;
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
	}
#endif
}

FQuat FTLLRN_IKRetargetPose::GetDeltaRotationForBone(const FName BoneName) const
{
	const FQuat* BoneRotationOffset = BoneRotationOffsets.Find(BoneName);
	return BoneRotationOffset != nullptr ? *BoneRotationOffset : FQuat::Identity;
}

void FTLLRN_IKRetargetPose::SetDeltaRotationForBone(FName BoneName, const FQuat& RotationDelta)
{
	IncrementVersion();

	FQuat* RotOffset = BoneRotationOffsets.Find(BoneName);
	if (RotOffset == nullptr)
	{
		// first time this bone has been modified in this pose
		BoneRotationOffsets.Emplace(BoneName, RotationDelta);
		return;
	}

	*RotOffset = RotationDelta;
}

FVector FTLLRN_IKRetargetPose::GetRootTranslationDelta() const
{
	return RootTranslationOffset;
}

void FTLLRN_IKRetargetPose::SetRootTranslationDelta(const FVector& TranslationDelta)
{
	IncrementVersion();
	
	RootTranslationOffset = TranslationDelta;
	// only allow vertical offset of root in retarget pose
	RootTranslationOffset.X = 0.0f;
	RootTranslationOffset.Y = 0.0f;
}

void FTLLRN_IKRetargetPose::AddToRootTranslationDelta(const FVector& TranslateDelta)
{
	IncrementVersion();
	
	RootTranslationOffset += TranslateDelta;
	// only allow vertical offset of root in retarget pose
	RootTranslationOffset.X = 0.0f;
	RootTranslationOffset.Y = 0.0f;
}

void FTLLRN_IKRetargetPose::SortHierarchically(const FTLLRN_IKRigSkeleton& Skeleton)
{
	// sort offsets hierarchically so that they are applied in leaf to root order
	// when generating the component space retarget pose in the processor
	BoneRotationOffsets.KeySort([Skeleton](FName A, FName B)
	{
		return Skeleton.GetBoneIndexFromName(A) > Skeleton.GetBoneIndexFromName(B);
	});
}

const TObjectPtr<UTLLRN_RetargetChainSettings> UTLLRN_IKRetargeter::GetChainMapByName(const FName& TargetChainName) const
{
	const TObjectPtr<UTLLRN_RetargetChainSettings>* ChainMap = ChainSettings.FindByPredicate(
		[TargetChainName](const TObjectPtr<UTLLRN_RetargetChainSettings> ChainMap)
		{
			return ChainMap->TargetChain == TargetChainName;
		});
	
	return !ChainMap ? nullptr : ChainMap->Get();
}

const FTLLRN_TargetChainSettings* UTLLRN_IKRetargeter::GetChainSettingsByName(const FName& TargetChainName) const
{
	const TObjectPtr<UTLLRN_RetargetChainSettings> ChainMap = GetChainMapByName(TargetChainName);
	if (ChainMap)
	{
		return &ChainMap->Settings;
	}

	return nullptr;
}

const FTLLRN_IKRetargetPose* UTLLRN_IKRetargeter::GetCurrentRetargetPose(const ETLLRN_RetargetSourceOrTarget& SourceOrTarget) const
{
	return SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? &SourceRetargetPoses[CurrentSourceRetargetPose] : &TargetRetargetPoses[CurrentTargetRetargetPose];
}

FName UTLLRN_IKRetargeter::GetCurrentRetargetPoseName(const ETLLRN_RetargetSourceOrTarget& SourceOrTarget) const
{
	return SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? CurrentSourceRetargetPose : CurrentTargetRetargetPose;
}

const FTLLRN_IKRetargetPose* UTLLRN_IKRetargeter::GetRetargetPoseByName(
	const ETLLRN_RetargetSourceOrTarget& SourceOrTarget,
	const FName PoseName) const
{
	return SourceOrTarget == ETLLRN_RetargetSourceOrTarget::Source ? SourceRetargetPoses.Find(PoseName) : TargetRetargetPoses.Find(PoseName);
}

const FName UTLLRN_IKRetargeter::GetDefaultPoseName()
{
	static const FName DefaultPoseName = "Default Pose";
	return DefaultPoseName;
}

void UTLLRN_IKRetargeter::FillProfileWithAssetSettings(FTLLRN_RetargetProfile& InOutProfile) const
{
	// first copy all the asset settings into the profile
	InOutProfile.bApplyTargetRetargetPose = true;
	InOutProfile.TargetRetargetPoseName = GetCurrentRetargetPoseName(ETLLRN_RetargetSourceOrTarget::Target);
	InOutProfile.bApplySourceRetargetPose = true;
	InOutProfile.SourceRetargetPoseName = GetCurrentRetargetPoseName(ETLLRN_RetargetSourceOrTarget::Source);
	InOutProfile.bApplyChainSettings = true;
	for (const TObjectPtr<UTLLRN_RetargetChainSettings>& Chain : ChainSettings)
	{
		InOutProfile.ChainSettings.Add(Chain->TargetChain, Chain->Settings);
	}
	InOutProfile.bApplyRootSettings = true;
	InOutProfile.RootSettings = RootSettings->Settings;
	InOutProfile.bApplyGlobalSettings = true;
	InOutProfile.GlobalSettings = GlobalSettings->Settings;

	// now override any settings in the asset's current profile
	if (const FTLLRN_RetargetProfile* ProfileToUse = GetCurrentProfile())
	{
		InOutProfile.MergeWithOtherProfile(*ProfileToUse);
	}
}

const FTLLRN_RetargetProfile* UTLLRN_IKRetargeter::GetCurrentProfile() const
{
	return GetProfileByName(CurrentProfile);
}

const FTLLRN_RetargetProfile* UTLLRN_IKRetargeter::GetProfileByName(const FName& ProfileName) const
{
	return Profiles.Find(ProfileName);
}

FTLLRN_TargetChainSettings UTLLRN_IKRetargeter::GetChainUsingGoalFromRetargetAsset(
	const UTLLRN_IKRetargeter* RetargetAsset,
	const FName IKGoalName)
{
	FTLLRN_TargetChainSettings EmptySettings;

	if (!RetargetAsset)
	{
		return EmptySettings;
	}

	const UTLLRN_IKRigDefinition* IKRig = RetargetAsset->GetIKRig(ETLLRN_RetargetSourceOrTarget::Target);
	if (!IKRig)
	{
		return EmptySettings;
	}

	const TArray<FTLLRN_BoneChain>& RetargetChains = IKRig->GetRetargetChains();
	const FTLLRN_BoneChain* ChainWithGoal = nullptr;
	for (const FTLLRN_BoneChain& RetargetChain : RetargetChains)
	{
		if (RetargetChain.IKGoalName == IKGoalName)
		{
			ChainWithGoal = &RetargetChain;
			break;
		}
	}

	if (!ChainWithGoal)
	{
		return EmptySettings;
	}

	// found a chain using the specified goal, return a copy of it's settings
	const FTLLRN_TargetChainSettings* ChainSettings = RetargetAsset->GetChainSettingsByName(ChainWithGoal->ChainName);
	return ChainSettings ? *ChainSettings : EmptySettings;
}

FTLLRN_TargetChainSettings UTLLRN_IKRetargeter::GetChainSettingsFromRetargetAsset(
	const UTLLRN_IKRetargeter* RetargetAsset,
	const FName TargetChainName,
	const FName OptionalProfileName)
{
	FTLLRN_TargetChainSettings OutSettings;
	
	if (!RetargetAsset)
	{
		return OutSettings;
	}
	
	// optionally get the chain settings from a profile
	if (OptionalProfileName != NAME_None)
	{
		if (const FTLLRN_RetargetProfile* RetargetProfile = RetargetAsset->GetProfileByName(OptionalProfileName))
		{
			if (const FTLLRN_TargetChainSettings* ProfileChainSettings = RetargetProfile->ChainSettings.Find(TargetChainName))
			{
				return *ProfileChainSettings;
			}
		}

		// no profile with this chain found, return default settings
		return OutSettings;
	}
	
	// return the chain settings stored in the retargeter (if it has one matching specified name)
	if (const FTLLRN_TargetChainSettings* AssetChainSettings = RetargetAsset->GetChainSettingsByName(TargetChainName))
	{
		return *AssetChainSettings;
	}

	// no chain map with the given target chain, so return default settings
	return OutSettings;
}

FTLLRN_TargetChainSettings UTLLRN_IKRetargeter::GetChainSettingsFromRetargetProfile(
	FTLLRN_RetargetProfile& RetargetProfile,
	const FName TargetChainName)
{
	return RetargetProfile.ChainSettings.FindOrAdd(TargetChainName);
}

void UTLLRN_IKRetargeter::GetRootSettingsFromRetargetAsset(
	const UTLLRN_IKRetargeter* RetargetAsset,
	const FName OptionalProfileName,
	FTLLRN_TargetRootSettings& OutSettings)
{
	if (!RetargetAsset)
	{
		OutSettings = FTLLRN_TargetRootSettings();
		return;
	}
	
	// optionally get the root settings from a profile
	if (OptionalProfileName != NAME_None)
	{
		if (const FTLLRN_RetargetProfile* RetargetProfile = RetargetAsset->GetProfileByName(OptionalProfileName))
		{
			if (RetargetProfile->bApplyRootSettings)
			{
				OutSettings =  RetargetProfile->RootSettings;
				return;
			}
		}
		
		// could not find profile, so return default settings
		OutSettings = FTLLRN_TargetRootSettings();
		return;
	}

	// return the base root settings
	OutSettings =  RetargetAsset->GetRootSettingsUObject()->Settings;
}

FTLLRN_TargetRootSettings UTLLRN_IKRetargeter::GetRootSettingsFromRetargetProfile(FTLLRN_RetargetProfile& RetargetProfile)
{
	return RetargetProfile.RootSettings;
}

void UTLLRN_IKRetargeter::GetGlobalSettingsFromRetargetAsset(
	const UTLLRN_IKRetargeter* RetargetAsset,
	const FName OptionalProfileName,
	FTLLRN_RetargetGlobalSettings& OutSettings)
{
	if (!RetargetAsset)
	{
		OutSettings = FTLLRN_RetargetGlobalSettings();
		return;
	}
	
	// optionally get the root settings from a profile
	if (OptionalProfileName != NAME_None)
	{
		if (const FTLLRN_RetargetProfile* RetargetProfile = RetargetAsset->GetProfileByName(OptionalProfileName))
		{
			if (RetargetProfile->bApplyGlobalSettings)
			{
				OutSettings = RetargetProfile->GlobalSettings;
				return;
			}
		}
		
		// could not find profile, so return default settings
		OutSettings = FTLLRN_RetargetGlobalSettings();
		return;
	}

	// return the base root settings
	OutSettings = RetargetAsset->GetGlobalSettings();
}

FTLLRN_RetargetGlobalSettings UTLLRN_IKRetargeter::GetGlobalSettingsFromRetargetProfile(FTLLRN_RetargetProfile& RetargetProfile)
{
	return RetargetProfile.GlobalSettings;
}

void UTLLRN_IKRetargeter::SetGlobalSettingsInRetargetProfile(
	FTLLRN_RetargetProfile& RetargetProfile,
	const FTLLRN_RetargetGlobalSettings& GlobalSettings)
{
	RetargetProfile.GlobalSettings = GlobalSettings;
	RetargetProfile.bApplyGlobalSettings = true;
}

void UTLLRN_IKRetargeter::SetRootSettingsInRetargetProfile(
	FTLLRN_RetargetProfile& RetargetProfile,
	const FTLLRN_TargetRootSettings& RootSettings)
{
	RetargetProfile.RootSettings = RootSettings;
	RetargetProfile.bApplyRootSettings = true;
}

void UTLLRN_IKRetargeter::SetChainSettingsInRetargetProfile(
	FTLLRN_RetargetProfile& RetargetProfile,
	const FTLLRN_TargetChainSettings& ChainSettings,
	const FName TargetChainName)
{
	RetargetProfile.ChainSettings.Add(TargetChainName, ChainSettings);
	RetargetProfile.bApplyChainSettings = true;
}

void UTLLRN_IKRetargeter::SetChainFKSettingsInRetargetProfile(
	FTLLRN_RetargetProfile& RetargetProfile,
	const FTLLRN_TargetChainFKSettings& FKSettings,
	const FName TargetChainName)
{
	FTLLRN_TargetChainSettings& ChainSettings = RetargetProfile.ChainSettings.FindOrAdd(TargetChainName);
	ChainSettings.FK = FKSettings;
	RetargetProfile.bApplyChainSettings = true;
}

void UTLLRN_IKRetargeter::SetChainIKSettingsInRetargetProfile(
	FTLLRN_RetargetProfile& RetargetProfile,
	const FTLLRN_TargetChainIKSettings& IKSettings,
	const FName TargetChainName)
{
	FTLLRN_TargetChainSettings& ChainSettings = RetargetProfile.ChainSettings.FindOrAdd(TargetChainName);
	ChainSettings.IK = IKSettings;
	RetargetProfile.bApplyChainSettings = true;
}

void UTLLRN_IKRetargeter::SetChainSpeedPlantSettingsInRetargetProfile(
	FTLLRN_RetargetProfile& RetargetProfile,
	const FTLLRN_TargetChainSpeedPlantSettings& SpeedPlantSettings,
	const FName TargetChainName)
{
	FTLLRN_TargetChainSettings& ChainSettings = RetargetProfile.ChainSettings.FindOrAdd(TargetChainName);
	ChainSettings.SpeedPlanting = SpeedPlantSettings;
	RetargetProfile.bApplyChainSettings = true;
}
