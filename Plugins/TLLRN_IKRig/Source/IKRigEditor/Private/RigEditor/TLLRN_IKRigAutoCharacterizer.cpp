// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigAutoCharacterizer.h"

#include "PBIK_Shared.h"
#include "Engine/SkeletalMesh.h"
#include "Algo/LevenshteinDistance.h"

#define LOCTEXT_NAMESPACE "TLLRN_AutoCharacterizer"

// core
const FName FTLLRN_CharacterizationStandard::Root = FName("Root");
const FName FTLLRN_CharacterizationStandard::Spine = FName("Spine");
const FName FTLLRN_CharacterizationStandard::Neck = FName("Neck");
const FName FTLLRN_CharacterizationStandard::Head = FName("Head");
const FName FTLLRN_CharacterizationStandard::Tail = FName("Tail");
// legs
const FName FTLLRN_CharacterizationStandard::LeftLeg = FName("LeftLeg");
const FName FTLLRN_CharacterizationStandard::RightLeg = FName("RightLeg");
// arms
const FName FTLLRN_CharacterizationStandard::LeftClavicle = FName("LeftClavicle");
const FName FTLLRN_CharacterizationStandard::RightClavicle = FName("RightClavicle");
const FName FTLLRN_CharacterizationStandard::LeftArm = FName("LeftArm");
const FName FTLLRN_CharacterizationStandard::RightArm = FName("RightArm");
// left hand
const FName FTLLRN_CharacterizationStandard::LeftThumbMetacarpal = FName("LeftThumbMetacarpal");
const FName FTLLRN_CharacterizationStandard::LeftIndexMetacarpal = FName("LeftIndexMetacarpal");
const FName FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal = FName("LeftMiddleMetacarpal");
const FName FTLLRN_CharacterizationStandard::LeftRingMetacarpal = FName("LeftRingMetacarpal");
const FName FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal = FName("LeftPinkyMetacarpal");
const FName FTLLRN_CharacterizationStandard::LeftThumb = FName("LeftThumb");
const FName FTLLRN_CharacterizationStandard::LeftIndex = FName("LeftIndex");
const FName FTLLRN_CharacterizationStandard::LeftMiddle = FName("LeftMiddle");
const FName FTLLRN_CharacterizationStandard::LeftRing = FName("LeftRing");
const FName FTLLRN_CharacterizationStandard::LeftPinky = FName("LeftPinky");
// right hand
const FName FTLLRN_CharacterizationStandard::RightThumbMetacarpal = FName("RightThumbMetacarpal");
const FName FTLLRN_CharacterizationStandard::RightIndexMetacarpal = FName("RightIndexMetacarpal");
const FName FTLLRN_CharacterizationStandard::RightMiddleMetacarpal = FName("RightMiddleMetacarpal");
const FName FTLLRN_CharacterizationStandard::RightRingMetacarpal = FName("RightRingMetacarpal");
const FName FTLLRN_CharacterizationStandard::RightPinkyMetacarpal = FName("RightPinkyMetacarpal");
const FName FTLLRN_CharacterizationStandard::RightThumb = FName("RightThumb");
const FName FTLLRN_CharacterizationStandard::RightIndex = FName("RightIndex");
const FName FTLLRN_CharacterizationStandard::RightMiddle = FName("RightMiddle");
const FName FTLLRN_CharacterizationStandard::RightRing = FName("RightRing");
const FName FTLLRN_CharacterizationStandard::RightPinky = FName("RightPinky");
// left foot
const FName FTLLRN_CharacterizationStandard::LeftBigToe = FName("LeftBigToe");
const FName FTLLRN_CharacterizationStandard::LeftIndexToe = FName("LeftIndexToe");
const FName FTLLRN_CharacterizationStandard::LeftMiddleToe = FName("LeftMiddleToe");
const FName FTLLRN_CharacterizationStandard::LeftRingToe = FName("LeftRingToe");
const FName FTLLRN_CharacterizationStandard::LeftPinkyToe = FName("LeftPinkyToe");
// right foot
const FName FTLLRN_CharacterizationStandard::RightBigToe = FName("RightBigToe");
const FName FTLLRN_CharacterizationStandard::RightIndexToe = FName("RightIndexToe");
const FName FTLLRN_CharacterizationStandard::RightMiddleToe = FName("RightMiddleToe");
const FName FTLLRN_CharacterizationStandard::RightRingToe = FName("RightRingToe");
const FName FTLLRN_CharacterizationStandard::RightPinkyToe = FName("RightPinkyToe");
// ik goal names
const FName FTLLRN_CharacterizationStandard::LeftHandIKGoal = FName("LeftHandIK");
const FName FTLLRN_CharacterizationStandard::LeftFootIKGoal = FName("LeftFootIK");
const FName FTLLRN_CharacterizationStandard::RightHandIKGoal = FName("RightHandIK");
const FName FTLLRN_CharacterizationStandard::RightFootIKGoal = FName("RightFootIK");

// extra limbs for creatures
const FName FTLLRN_CharacterizationStandard::LeftLegA = FName("LeftLegA");
const FName FTLLRN_CharacterizationStandard::LeftLegB = FName("LeftLegB");
const FName FTLLRN_CharacterizationStandard::LeftLegC = FName("LeftLegC");
const FName FTLLRN_CharacterizationStandard::RightLegA = FName("RightLegA");
const FName FTLLRN_CharacterizationStandard::RightLegB = FName("RightLegB");
const FName FTLLRN_CharacterizationStandard::RightLegC = FName("RightLegC");
//
const FName FTLLRN_CharacterizationStandard::LeftFootAIKGoal = FName("LeftFootAIKGoal");
const FName FTLLRN_CharacterizationStandard::LeftFootBIKGoal = FName("LeftFootBIKGoal");
const FName FTLLRN_CharacterizationStandard::LeftFootCIKGoal = FName("LeftFootCIKGoal");
const FName FTLLRN_CharacterizationStandard::RightFootAIKGoal = FName("RightFootAIKGoal");
const FName FTLLRN_CharacterizationStandard::RightFootBIKGoal = FName("RightFootBIKGoal");
const FName FTLLRN_CharacterizationStandard::RightFootCIKGoal = FName("RightFootCIKGoal");

FTLLRN_AbstractHierarchy::FTLLRN_AbstractHierarchy(USkeletalMesh* InMesh)
{
	if (!InMesh)
	{
		return;
	}

	const FReferenceSkeleton& RefSkeleton = InMesh->GetRefSkeleton();
	
	FullBoneNames.Reserve(RefSkeleton.GetNum());
	ParentIndices.Reserve(RefSkeleton.GetNum());
	for (int32 BoneIndex=0; BoneIndex<RefSkeleton.GetNum(); ++BoneIndex)
	{
		FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
		int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
		FullBoneNames.Add(BoneName);
		ParentIndices.Add(ParentIndex);
	}

	GenerateCleanBoneNames();
}

int32 FTLLRN_AbstractHierarchy::GetParentIndex(const FName BoneName, const ETLLRN_CleanOrFullName NameType) const
{
	const int32 BoneIndex = GetBoneIndex(BoneName, NameType);
	if (ParentIndices.IsValidIndex(BoneIndex))
	{
		return ParentIndices[BoneIndex];
	}
	return INDEX_NONE;
}

int32 FTLLRN_AbstractHierarchy::GetBoneIndex(const FName BoneName, const ETLLRN_CleanOrFullName NameType) const
{
	return GetBoneNames(NameType).Find(BoneName);
}

FName FTLLRN_AbstractHierarchy::GetBoneName(const int32 BoneIndex, const ETLLRN_CleanOrFullName NameType) const
{
	if (!ensure(ParentIndices.IsValidIndex(BoneIndex)))
	{
		return NAME_None;
	}
	
	return GetBoneNames(NameType)[BoneIndex];
}

bool FTLLRN_AbstractHierarchy::IsChildOf(const FName InParent, const FName PotentialChild, const ETLLRN_CleanOrFullName NameType) const
{
	const TArray<FName>& BoneNames = GetBoneNames(NameType);
	const bool bParentExists = BoneNames.Find(InParent) != INDEX_NONE;
	const bool bPotentialChildExists = BoneNames.Find(PotentialChild) != INDEX_NONE;
	if (!(bParentExists && bPotentialChildExists))
	{
		// can't be a child of parent unless both exist
		return false;
	}
	
	int32 ParentIndex = GetParentIndex(PotentialChild, NameType);
	while(ParentIndex != INDEX_NONE)
	{
		FName ParentName = BoneNames[ParentIndex];
		if (ParentName == InParent)
		{
			return true;
		}
		ParentIndex = GetParentIndex(ParentName, NameType);
	}
	return false;
}

void FTLLRN_AbstractHierarchy::GetBonesInChain(
	const FName Start,
	const FName End,
	const ETLLRN_CleanOrFullName NameType,
	TArray<FName>& OutBonesInChain) const
{
	OutBonesInChain.Reset();

	// optionally return either the full names or clean names
	TArray<FName> BoneNames = GetBoneNames(NameType);

	// can form a chain if either end doesn't exist
	const int32 StartBoneIndex = BoneNames.Find(Start);
	const int32 EndBoneIndex = BoneNames.Find(End);
	if (StartBoneIndex == INDEX_NONE || EndBoneIndex == INDEX_NONE)
	{
		return;
	}

	// single bone chain?
	if (Start == End)
	{
		OutBonesInChain.Add(Start);
		return;
	}

	// bones must be a valid chain in the hierarchy
	if (!IsChildOf(Start, End, NameType))
	{
		return;
	}

	// gather list of bones from end to start
	OutBonesInChain.Add(End);
	int32 ParentIndex = GetParentIndex(End, NameType);
	while(ParentIndex != StartBoneIndex)
	{
		FName ParentName = BoneNames[ParentIndex];
		OutBonesInChain.Add(ParentName);
		ParentIndex = GetParentIndex(ParentName, NameType);
	}
	OutBonesInChain.Add(Start);
	
	// we want root to leaf order
	Algo::Reverse(OutBonesInChain);
}

const TArray<FName>& FTLLRN_AbstractHierarchy::GetBoneNames(const ETLLRN_CleanOrFullName NameType) const
{
	return NameType == ETLLRN_CleanOrFullName::Clean ? CleanBoneNames : FullBoneNames;
}

void FTLLRN_AbstractHierarchy::GetImmediateChildren(
	const FName& BoneName,
	const ETLLRN_CleanOrFullName NameType,
	TArray<FName>& OutChildren) const
{
	const int32 ParentIndexToFind = GetBoneIndex(BoneName, NameType);
	const TArray<FName>& BoneNames = GetBoneNames(NameType);
	for (int32 BoneIndex=0; BoneIndex<ParentIndices.Num(); ++BoneIndex)
	{
		if (ParentIndices[BoneIndex] == ParentIndexToFind)
		{
			OutChildren.Add(BoneNames[BoneIndex]);
		}
	}
}

void FTLLRN_AbstractHierarchy::Compare(
	const FTLLRN_AbstractHierarchy& OtherHierarchy,
	TArray<FName>& OutMissingBones,
	TArray<FName>& OutBonesWithDifferentParent,
	int32& OutNumMatchingBones,
	float& OutPercentOfTemplateMatched) const
{
	OutMissingBones.Reset();
	OutBonesWithDifferentParent.Reset();
	OutNumMatchingBones = 0;
	OutPercentOfTemplateMatched = 0.f;
	
	if (OtherHierarchy.CleanBoneNames.IsEmpty())
	{
		// avoid divide by zero, empty hierarchy matches with anything
		return;
	}
	
	// calculate a simple score with equal weight for number of matching bones and parents
	// this score represents what percentage of the template is found in the input hierarchy
	FindBonesNotInOther(OtherHierarchy, OutMissingBones, OutBonesWithDifferentParent);
	const float NumBonesTotal = CleanBoneNames.Num();
	const float NumMatchingBones = NumBonesTotal - OutMissingBones.Num();
	const float NumMatchingParents = NumBonesTotal - OutBonesWithDifferentParent.Num();
	const float BoneMatchScore = NumMatchingBones / NumBonesTotal;
	const float ParentMatchScore = NumMatchingParents / NumBonesTotal;
	constexpr float InvNumScores = 1.0f / 2.f;
	OutPercentOfTemplateMatched = (BoneMatchScore + ParentMatchScore) * InvNumScores;
	
	// determine how many bones in other match with this template
	// this is an absolute score that indicates which template would maximize the number of retargeted bones
	OutNumMatchingBones = OtherHierarchy.GetNumMatchingBones(*this);
}

void FTLLRN_AbstractHierarchy::FindBonesNotInOther(
	const FTLLRN_AbstractHierarchy& OtherHierarchy,
	TArray<FName>& OutMissingBones,
	TArray<FName>& OutBonesWithDifferentParent) const
{
	OutMissingBones.Reset();
	OutBonesWithDifferentParent.Reset();
	
	for (const FName BoneName : CleanBoneNames)
	{
		bool bExists;
		bool bHasSameParent;
		CheckBoneExistsAndHasSameParent(BoneName, OtherHierarchy, bExists, bHasSameParent);

		if (!bExists)
		{
			OutMissingBones.Add(BoneName);
			continue;
		}

		if (!bHasSameParent)
		{
			OutBonesWithDifferentParent.Add(BoneName);
		}
	}
}

int32 FTLLRN_AbstractHierarchy::GetNumMatchingBones(const FTLLRN_AbstractHierarchy& OtherHierarchy) const
{
	int32 NumMatchingBones = 0;
	
	for (const FName BoneName : CleanBoneNames)
	{
		bool bExists;
		bool bHasSameParent;
		CheckBoneExistsAndHasSameParent(BoneName, OtherHierarchy, bExists, bHasSameParent);

		// increment score when bone exists in both with same parent in both
		NumMatchingBones += bExists && bHasSameParent ? 1 : 0;
	}
	
	return NumMatchingBones;
}

void FTLLRN_AbstractHierarchy::CheckBoneExistsAndHasSameParent(
	const FName& CleanBoneName,
	const FTLLRN_AbstractHierarchy& OtherHierarchy,
	bool& OutExists,
	bool& OutSameParent) const
{
	OutExists = false;
	OutSameParent = false;
	
	const int32 OtherBoneIndex = OtherHierarchy.GetBoneIndex(CleanBoneName, ETLLRN_CleanOrFullName::Clean);
	if (OtherBoneIndex == INDEX_NONE)
	{
		return;
	}
	
	OutExists = true;
	
	// check if parents are the same
	const int32 ThisParentIndex = GetParentIndex(CleanBoneName, ETLLRN_CleanOrFullName::Clean);
	const int32 OtherParentIndex = OtherHierarchy.GetParentIndex(CleanBoneName, ETLLRN_CleanOrFullName::Clean);
	const FName ThisParentName = (ThisParentIndex != INDEX_NONE) ? CleanBoneNames[ThisParentIndex] : NAME_None;
	const FName OtherParentName = (OtherParentIndex != INDEX_NONE) ? OtherHierarchy.CleanBoneNames[OtherParentIndex] : NAME_None;
	OutSameParent = ThisParentName == OtherParentName;
}

void FTLLRN_AbstractHierarchy::GenerateCleanBoneNames()
{
	// copy full names to make clean names from
	CleanBoneNames = FullBoneNames;
	
	const FString CommonPrefix = FindLargestCommonPrefix(CleanBoneNames);
	if (CommonPrefix.Len() == 0)
	{
		return;
	}

	// skeleton was prefixed, so remove the prefix from all the clean bone names
	for (int32 BoneIndex=0; BoneIndex<CleanBoneNames.Num(); ++BoneIndex)
	{
		FString BoneNameStr = CleanBoneNames[BoneIndex].ToString();
		BoneNameStr.RemoveFromStart(CommonPrefix);
		CleanBoneNames[BoneIndex] = FName(BoneNameStr);
	}
}

FString FTLLRN_AbstractHierarchy::FindLargestCommonPrefix(const TArray<FName>& ArrayOfNames)
{
	if (ArrayOfNames.Num() == 0)
	{
		return TEXT("");
	}

	// convert to strings
	TArray<FString> StrArray;
	for (const FName& Name : ArrayOfNames)
	{
		StrArray.Add(Name.ToString());
	}

	// sort the array to compare the first and last strings
	StrArray.Sort();

	FString FirstStr = StrArray[0];
	FString LastStr = StrArray.Last();
	FString CommonPrefix = TEXT("");
	for (int32 i = 0; i < FirstStr.Len(); ++i)
	{
		if (i < LastStr.Len() && FirstStr[i] == LastStr[i])
		{
			CommonPrefix.AppendChar(FirstStr[i]);
		}
		else
		{
			break;
		}
	}

	return CommonPrefix;
}

FVector FTLLRN_BoneSettingsForIK::GetPreferredAxisAsAngles() const
{
	switch (PreferredAxis)
	{
	case ETLLRN_PreferredAxis::None: return FVector::ZeroVector;
	case ETLLRN_PreferredAxis::PositiveX: return FVector::XAxisVector * PreferredAngleMagnitude;
	case ETLLRN_PreferredAxis::NegativeX: return FVector::XAxisVector * -PreferredAngleMagnitude;
	case ETLLRN_PreferredAxis::PositiveY: return FVector::YAxisVector * PreferredAngleMagnitude;
	case ETLLRN_PreferredAxis::NegativeY: return FVector::YAxisVector * -PreferredAngleMagnitude;
	case ETLLRN_PreferredAxis::PositiveZ: return FVector::ZAxisVector * PreferredAngleMagnitude;
	case ETLLRN_PreferredAxis::NegativeZ: return FVector::ZAxisVector * -PreferredAngleMagnitude;
	default:
		{
			checkNoEntry();
			return FVector();
		}
	}
}

void FTLLRN_BoneSettingsForIK::LockNonPreferredAxes(EPBIKLimitType& OutX, EPBIKLimitType& OutY, EPBIKLimitType& OutZ) const
{
	if (PreferredAxis == ETLLRN_PreferredAxis::None)
	{
		return;
	}

	if (PreferredAxis == ETLLRN_PreferredAxis::PositiveX || PreferredAxis == ETLLRN_PreferredAxis::NegativeX)
	{
		OutY = EPBIKLimitType::Locked;
		OutZ = EPBIKLimitType::Locked;
		return;
	}

	if (PreferredAxis == ETLLRN_PreferredAxis::PositiveY || PreferredAxis == ETLLRN_PreferredAxis::NegativeY)
	{
		OutX = EPBIKLimitType::Locked;
		OutZ = EPBIKLimitType::Locked;
		return;
	}

	if (PreferredAxis == ETLLRN_PreferredAxis::PositiveZ || PreferredAxis == ETLLRN_PreferredAxis::NegativeZ)
	{
		OutX = EPBIKLimitType::Locked;
		OutY = EPBIKLimitType::Locked;
		return;
	}
	
	checkNoEntry();
}

void FTLLRN_AllBoneSettingsForIK::SetPreferredAxis(const FName BoneName, const ETLLRN_PreferredAxis PreferredAxis, const bool bTreatAsHinge)
{
	FTLLRN_BoneSettingsForIK& Settings = GetOrAddBoneSettings(BoneName);
	Settings.PreferredAxis = PreferredAxis;
	Settings.bIsHinge = bTreatAsHinge;
}

void FTLLRN_AllBoneSettingsForIK::SetRotationStiffness(const FName BoneName, const float RotationStiffness)
{
	FTLLRN_BoneSettingsForIK& Settings = GetOrAddBoneSettings(BoneName);
	Settings.RotationStiffness = RotationStiffness;
}

void FTLLRN_AllBoneSettingsForIK::SetExcluded(const FName BoneName, const bool bExclude)
{
	FTLLRN_BoneSettingsForIK& Settings = GetOrAddBoneSettings(BoneName);
	Settings.bExcluded = bExclude;
}

FTLLRN_BoneSettingsForIK& FTLLRN_AllBoneSettingsForIK::GetOrAddBoneSettings(const FName BoneName)
{
	for (FTLLRN_BoneSettingsForIK& BoneSettings : AllBoneSettings)
	{
		if (BoneSettings.BoneToApplyTo == BoneName)
		{
			return BoneSettings;
		}
	}

	const int32 NewBoneSettingsIndex = AllBoneSettings.Emplace(BoneName);
	return AllBoneSettings[NewBoneSettingsIndex];
}

FTLLRN_KnownTemplateHierarchies::FTLLRN_KnownTemplateHierarchies()
{
	// TODO in the future we can make templates extensible.

	// UE4 Mannequin
	{
		static FName UE4Name = "UE4 Mannequin";
		static TArray<FName> UE4Bones = {"root", "pelvis", "spine_01", "spine_02", "spine_03", "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "index_01_l", "index_02_l", "index_03_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l", "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r", "index_01_r", "index_02_r", "index_03_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r", "neck_01", "head", "thigh_l", "calf_l", "foot_l", "ball_l", "thigh_r", "calf_r", "foot_r", "ball_r"};
		static TArray<int32> UE4ParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 8, 12, 13, 8, 15, 16, 8, 18, 19, 8, 21, 22, 4, 24, 25, 26, 27, 28, 29, 27, 31, 32, 27, 34, 35, 27, 37, 38, 27, 40, 41, 4, 43, 1, 45, 46, 47, 1, 49, 50, 51};
		FTLLRN_TemplateHierarchy& UE4 = AddTemplateHierarchy(UE4Name, UE4Bones, UE4ParentIndices);
		FTLLRN_RetargetDefinition& UE4Retarget = UE4.AutoRetargetDefinition.RetargetDefinition;
		// core
		UE4Retarget.RootBone = FName("pelvis");
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Root, FName("root"), FName("root"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("spine_01"), FName("spine_02"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("neck_01"), FName("neck_01"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("head"), FName("head"));
		// left
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("thigh_l"), FName("ball_l"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("clavicle_l"), FName("clavicle_l"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("upperarm_l"), FName("hand_l"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("thumb_01_l"), FName("thumb_03_l"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("index_01_l"), FName("index_03_l"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("middle_01_l"), FName("middle_03_l"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("ring_01_l"), FName("ring_03_l"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("pinky_01_l"), FName("pinky_03_l"));
		// right
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("thigh_r"), FName("ball_r"), FTLLRN_CharacterizationStandard::RightFootIKGoal);
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("clavicle_r"), FName("clavicle_r"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("upperarm_r"), FName("hand_r"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("thumb_01_r"), FName("thumb_03_r"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("index_01_r"), FName("index_03_r"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("middle_01_r"), FName("middle_03_r"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("ring_01_r"), FName("ring_03_r"));
		UE4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("pinky_01_r"), FName("pinky_03_r"));
		// bone settings for IK
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("pelvis"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("clavicle_l"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("clavicle_r"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("foot_l"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("foot_r"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("calf_l"), ETLLRN_PreferredAxis::PositiveZ);
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("calf_r"), ETLLRN_PreferredAxis::PositiveZ);
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lowerarm_l"), ETLLRN_PreferredAxis::PositiveZ);
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lowerarm_r"), ETLLRN_PreferredAxis::PositiveZ);
		UE4.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("spine_03"), true);
		// unreal ik bones
		UE4.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_root", "root");
		UE4.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_l", "foot_l");
		UE4.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_r", "foot_r");
		UE4.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_root", "root");
		UE4.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_gun", "hand_r");
		UE4.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_l", "hand_l");
		UE4.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_r", "hand_r");
		// exclude feet from auto-pose
		UE4.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("foot_l");
		UE4.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("foot_r");
	}

	// UE5 Mannequin
	{
		static FName UE5Name = "UE5 Mannequin";
		static TArray<FName> UE5Bones = {"root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "neck_01", "neck_02", "head", "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "index_metacarpal_l", "index_01_l", "index_02_l", "index_03_l", "middle_metacarpal_l", "middle_01_l", "middle_02_l", "middle_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l", "pinky_metacarpal_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "ring_metacarpal_l", "ring_01_l", "ring_02_l", "ring_03_l", "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r", "pinky_metacarpal_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "ring_metacarpal_r", "ring_01_r", "ring_02_r", "ring_03_r", "middle_metacarpal_r", "middle_01_r", "middle_02_r", "middle_03_r", "index_metacarpal_r", "index_01_r", "index_02_r", "index_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r", "thigh_r", "calf_r", "foot_r", "ball_r", "thigh_l", "calf_l", "foot_l", "ball_l"};
		static TArray<int32> UE5ParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 6, 10, 11, 12, 13, 14, 15, 16, 13, 18, 19, 20, 13, 22, 23, 13, 25, 26, 27, 13, 29, 30, 31, 6, 33, 34, 35, 36, 37, 38, 39, 36, 41, 42, 43, 36, 45, 46, 47, 36, 49, 50, 51, 36, 53, 54, 1, 56, 57, 58, 1, 60, 61, 62};
		FTLLRN_TemplateHierarchy& UE5 = AddTemplateHierarchy(UE5Name, UE5Bones, UE5ParentIndices); 
		FTLLRN_RetargetDefinition& UE5Retarget = UE5.AutoRetargetDefinition.RetargetDefinition;
		// core
		UE5Retarget.RootBone = FName("pelvis");
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Root, FName("root"), FName("root"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("spine_01"), FName("spine_02"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("neck_01"), FName("neck_02"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("head"), FName("head"));
		// left
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("thigh_l"), FName("ball_l"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("clavicle_l"), FName("clavicle_l"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("upperarm_l"), FName("hand_l"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("index_metacarpal_l"), FName("index_metacarpal_l"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("middle_metacarpal_l"), FName("middle_metacarpal_l"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("ring_metacarpal_l"), FName("ring_metacarpal_l"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("pinky_metacarpal_l"), FName("pinky_metacarpal_l"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("thumb_01_l"), FName("thumb_03_l"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("index_01_l"), FName("index_03_l"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("middle_01_l"), FName("middle_03_l"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("ring_01_l"), FName("ring_03_l"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("pinky_01_l"), FName("pinky_03_l"));
		// right
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("thigh_r"), FName("ball_r"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("clavicle_r"), FName("clavicle_r"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("upperarm_r"), FName("hand_r"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("index_metacarpal_r"), FName("index_metacarpal_r"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("middle_metacarpal_r"), FName("middle_metacarpal_r"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("ring_metacarpal_r"), FName("ring_metacarpal_r"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("pinky_metacarpal_r"), FName("pinky_metacarpal_r"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("thumb_01_r"), FName("thumb_03_r"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("index_01_r"), FName("index_03_r"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("middle_01_r"), FName("middle_03_r"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("ring_01_r"), FName("ring_03_r"));
		UE5Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("pinky_01_r"), FName("pinky_03_r"));
		// bone settings for IK
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("pelvis"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("clavicle_l"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("clavicle_r"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("foot_l"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("foot_r"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("calf_l"), ETLLRN_PreferredAxis::PositiveZ);
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("calf_r"), ETLLRN_PreferredAxis::PositiveZ);
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lowerarm_l"), ETLLRN_PreferredAxis::PositiveZ);
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lowerarm_r"), ETLLRN_PreferredAxis::PositiveZ);
		UE5.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("spine_05"), true);
		// unreal ik bones
		UE5.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_root", "root");
		UE5.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_l", "foot_l");
		UE5.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_r", "foot_r");
		UE5.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_root", "root");
		UE5.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_gun", "hand_r");
		UE5.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_l", "hand_l");
		UE5.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_r", "hand_r");
		// exclude feet from auto-pose
		UE5.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("foot_l");
		UE5.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("foot_r");
	}
	
	// Daz 3d
	{
		static FName Daz3dName = "Daz_3d";
		static TArray<FName> Daz3dBones = {"hip", "pelvis", "lThighBend", "lThighTwist", "lShin", "lFoot", "lMetatarsals", "lToe", "lSmallToe4", "lSmallToe4_2", "lSmallToe3", "lSmallToe3_2", "lSmallToe2", "lSmallToe2_2", "lSmallToe1", "lSmallToe1_2", "lBigToe", "lBigToe_2", "lHeel", "rThighBend", "rThighTwist", "rShin", "rFoot", "rMetatarsals", "rToe", "rSmallToe4", "rSmallToe4_2", "rSmallToe3", "rSmallToe3_2", "rSmallToe2", "rSmallToe2_2", "rSmallToe1", "rSmallToe1_2", "rBigToe", "rBigToe_2", "rHeel", "abdomenLower", "abdomenUpper", "chestLower", "chestUpper", "lCollar", "lShldrBend", "lShldrTwist", "lForearmBend", "lForearmTwist", "lHand", "lThumb1", "lThumb2", "lThumb3", "lCarpal1", "lIndex1", "lIndex2", "lIndex3", "lCarpal2", "lMid1", "lMid2", "lMid3", "lCarpal3", "lRing1", "lRing2", "lRing3", "lCarpal4", "lPinky1", "lPinky2", "lPinky3", "rCollar", "rShldrBend", "rShldrTwist", "rForearmBend", "rForearmTwist", "rHand", "rThumb1", "rThumb2", "rThumb3", "rCarpal1", "rIndex1", "rIndex2", "rIndex3", "rCarpal2", "rMid1", "rMid2", "rMid3", "rCarpal3", "rRing1", "rRing2", "rRing3", "rCarpal4", "rPinky1", "rPinky2", "rPinky3", "neckLower", "neckUpper", "head"};
		static TArray<int32> Daz3dParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 6, 8, 6, 10, 6, 12, 6, 14, 6, 16, 5, 1, 19, 20, 21, 22, 23, 23, 25, 23, 27, 23, 29, 23, 31, 23, 33, 22, 0, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 45, 49, 50, 51, 45, 53, 54, 55, 45, 57, 58, 59, 45, 61, 62, 63, 39, 65, 66, 67, 68, 69, 70, 71, 72, 70, 74, 75, 76, 70, 78, 79, 80, 70, 82, 83, 84, 70, 86, 87, 88, 39, 90, 91};
		FTLLRN_TemplateHierarchy& Daz3d = AddTemplateHierarchy(Daz3dName, Daz3dBones, Daz3dParentIndices);
		FTLLRN_RetargetDefinition& DazRetarget = Daz3d.AutoRetargetDefinition.RetargetDefinition;
		// core
		DazRetarget.RootBone = FName("hip");
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("abdomenLower"), FName("chestUpper"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("neckLower"), FName("neckUpper"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("head"), FName("head"));
		// left
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("lThighBend"), FName("lToe"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("lCollar"), FName("lCollar"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("lShldrBend"), FName("lHand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("lCarpal1"), FName("lCarpal1"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("lCarpal2"), FName("lCarpal2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("lCarpal3"), FName("lCarpal3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("lCarpal4"), FName("lCarpal4"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("lThumb1"), FName("lThumb3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("lIndex1"), FName("lIndex3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("lMid1"), FName("lMid3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("lRing1"), FName("lRing3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("lPinky1"), FName("lPinky3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftBigToe, FName("lBigToe"), FName("lBigToe_2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexToe, FName("lSmallToe1"), FName("lSmallToe1_2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleToe, FName("lSmallToe2"), FName("lSmallToe2_2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingToe, FName("lSmallToe3"), FName("lSmallToe3_2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyToe, FName("lSmallToe4"), FName("lSmallToe4_2"));
		// right
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("rThighBend"), FName("rToe"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("rCollar"), FName("rCollar"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("rShldrBend"), FName("rHand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("rCarpal1"), FName("rCarpal1"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("rCarpal2"), FName("rCarpal2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("rCarpal3"), FName("rCarpal3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("rCarpal4"), FName("rCarpal4"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("rThumb1"), FName("rThumb3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("rIndex1"), FName("rIndex3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("rMid1"), FName("rMid3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("rRing1"), FName("rRing3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("rPinky1"), FName("rPinky3"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightBigToe, FName("rBigToe"), FName("rBigToe_2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexToe, FName("rSmallToe1"), FName("rSmallToe1_2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleToe, FName("rSmallToe2"), FName("rSmallToe2_2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingToe, FName("rSmallToe3"), FName("rSmallToe3_2"));
		DazRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyToe, FName("rSmallToe4"), FName("rSmallToe4_2"));
		// bone settings for IK
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("hip"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("lCollar"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("rCollar"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("lFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("rFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lShin"), ETLLRN_PreferredAxis::PositiveX);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("rShin"), ETLLRN_PreferredAxis::PositiveX);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lForearmBend"), ETLLRN_PreferredAxis::PositiveY);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("rForearmBend"), ETLLRN_PreferredAxis::NegativeY);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("chestUpper"), true);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("lShldrTwist"), true);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("rShldrTwist"), true);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("lForearmTwist"), true);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("rForearmTwist"), true);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("lThighTwist"), true);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("rThighTwist"), true);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("lMetatarsals"), true);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("rMetatarsals"), true);
		Daz3d.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("pelvis"), true);
		// exclude feet from auto-pose
		Daz3d.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("lFoot");
		Daz3d.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("rFoot");
	}

	// Mixamo
	{
		static FName Mixamo_Name = "Mixamo";
		static TArray<FName> Mixamo_Bones = {"Hips", "Spine", "Spine1", "Spine2", "Neck", "Head", "HeadTop_End", "RightShoulder", "RightArm", "RightForeArm", "RightHand", "RightHandThumb1", "RightHandThumb2", "RightHandThumb3", "RightHandThumb4", "RightHandIndex1", "RightHandIndex2", "RightHandIndex3", "RightHandIndex4", "RightHandMiddle1", "RightHandMiddle2", "RightHandMiddle3", "RightHandMiddle4", "RightHandRing1", "RightHandRing2", "RightHandRing3", "RightHandRing4", "RightHandPinky1", "RightHandPinky2", "RightHandPinky3", "RightHandPinky4", "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand", "LeftHandThumb1", "LeftHandThumb2", "LeftHandThumb3", "LeftHandThumb4", "LeftHandIndex1", "LeftHandIndex2", "LeftHandIndex3", "LeftHandIndex4", "LeftHandMiddle1", "LeftHandMiddle2", "LeftHandMiddle3", "LeftHandMiddle4", "LeftHandRing1", "LeftHandRing2", "LeftHandRing3", "LeftHandRing4", "LeftHandPinky1", "LeftHandPinky2", "LeftHandPinky3", "LeftHandPinky4", "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase", "RightToe_End", "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase", "LeftToe_End"};
		static TArray<int32> Mixamo_ParentIndices = {-1, 0, 1, 2, 3, 4, 5, 3, 7, 8, 9, 10, 11, 12, 13, 10, 15, 16, 17, 10, 19, 20, 21, 10, 23, 24, 25, 10, 27, 28, 29, 3, 31, 32, 33, 34, 35, 36, 37, 34, 39, 40, 41, 34, 43, 44, 45, 34, 47, 48, 49, 34, 51, 52, 53, 0, 55, 56, 57, 58, 0, 60, 61, 62, 63};
		FTLLRN_TemplateHierarchy& Mixamo = AddTemplateHierarchy(Mixamo_Name, Mixamo_Bones, Mixamo_ParentIndices);
		FTLLRN_RetargetDefinition& MixamoRetarget = Mixamo.AutoRetargetDefinition.RetargetDefinition;
		// core
		MixamoRetarget.RootBone = FName("Hips");
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("Spine"), FName("Spine2"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("Neck"), FName("Neck"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("Head"), FName("Head"));
		// left
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("LeftUpLeg"), FName("LeftToeBase"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("LeftShoulder"), FName("LeftShoulder"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("LeftArm"), FName("LeftHand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("LeftHandThumb1"), FName("LeftHandThumb3"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("LeftHandIndex1"), FName("LeftHandIndex3"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("LeftHandMiddle1"), FName("LeftHandMiddle3"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("LeftHandRing1"), FName("LeftHandRing3"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("LeftHandPinky1"), FName("LeftHandPinky3"));
		// right
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("RightUpLeg"), FName("RightToeBase"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("RightShoulder"), FName("RightShoulder"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("RightArm"), FName("RightHand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("RightHandThumb1"), FName("RightHandThumb3"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("RightHandIndex1"), FName("RightHandIndex3"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("RightHandMiddle1"), FName("RightHandMiddle3"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("RightHandRing1"), FName("RightHandRing3"));
		MixamoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("RightHandPinky1"), FName("RightHandPinky3"));
		// bone settings for IK
		Mixamo.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Hips"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		Mixamo.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Mixamo.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Mixamo.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Mixamo.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Mixamo.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftLeg"), ETLLRN_PreferredAxis::PositiveX);
		Mixamo.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightLeg"), ETLLRN_PreferredAxis::PositiveX);
		Mixamo.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftForeArm"), ETLLRN_PreferredAxis::PositiveY);
		Mixamo.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightForeArm"), ETLLRN_PreferredAxis::NegativeY);
		// exclude feet from auto-pose
		Mixamo.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("LeftFoot");
		Mixamo.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("RightFoot");
	}
	
	// Reallusions Character Creator 4
	{
		static FName CC4Name = "Reallusion Character Creator 4";
		static TArray<FName> CC4Bones = {"BoneRoot", "Hip", "Pelvis", "L_Thigh", "L_Calf", "L_Foot", "L_ToeBase", "L_PinkyToe1", "L_RingToe1", "L_MidToe1", "L_IndexToe1", "L_BigToe1", "L_ThighTwist01", "L_ThighTwist02", "R_Thigh", "R_Calf", "R_Foot", "R_ToeBase", "R_BigToe1", "R_PinkyToe1", "R_RingToe1", "R_IndexToe1", "R_MidToe1", "Waist", "Spine01", "Spine02", "NeckTwist01", "NeckTwist02", "Head", "L_Clavicle", "L_Upperarm", "L_Forearm", "L_Hand", "L_Pinky1", "L_Pinky2", "L_Pinky3", "L_Ring1", "L_Ring2", "L_Ring3", "L_Mid1", "L_Mid2", "L_Mid3", "L_Index1", "L_Index2", "L_Index3", "L_Thumb1", "L_Thumb2", "L_Thumb3", "R_Clavicle", "R_Upperarm", "R_Forearm", "R_Hand", "R_Ring1", "R_Ring2", "R_Ring3", "R_Mid1", "R_Mid2", "R_Mid3", "R_Thumb1", "R_Thumb2", "R_Thumb3", "R_Index1", "R_Index2", "R_Index3", "R_Pinky1", "R_Pinky2", "R_Pinky3"};
		static TArray<int32> CC4ParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 3, 12, 2, 14, 15, 16, 17, 17, 17, 17, 17, 1, 23, 24, 25, 26, 27, 25, 29, 30, 31, 32, 33, 34, 32, 36, 37, 32, 39, 40, 32, 42, 43, 32, 45, 46, 25, 48, 49, 50, 51, 52, 53, 51, 55, 56, 51, 58, 59, 51, 61, 62, 51, 64, 65};
		FTLLRN_TemplateHierarchy& CC4 = AddTemplateHierarchy(CC4Name, CC4Bones, CC4ParentIndices);
		FTLLRN_RetargetDefinition& CC4Retarget = CC4.AutoRetargetDefinition.RetargetDefinition;
		// core
		CC4Retarget.RootBone = FName("Hip");
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Root, FName("BoneRoot"), FName("BoneRoot"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("Waist"), FName("Spine02"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("NeckTwist01"), FName("NeckTwist02"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("Head"), FName("Head"));
		// left
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("L_Thigh"), FName("L_ToeBase"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("L_Clavicle"), FName("L_Clavicle"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("L_Upperarm"), FName("L_Hand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("L_Thumb1"), FName("L_Thumb3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("L_Index1"), FName("L_Index3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("L_Mid1"), FName("L_Mid3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("L_Ring1"), FName("L_Ring3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("L_Pinky1"), FName("L_Pinky3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftBigToe, FName("L_BigToe1"), FName("L_BigToe1"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexToe, FName("L_IndexToe1"), FName("L_IndexToe1"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleToe, FName("L_MidToe1"), FName("L_MidToe1"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingToe, FName("L_RingToe1"), FName("L_RingToe1"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyToe, FName("L_PinkyToe1"), FName("L_PinkyToe1"));
		// right
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("R_Thigh"), FName("R_ToeBase"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("R_Clavicle"), FName("R_Clavicle"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("R_Upperarm"), FName("R_Hand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("R_Thumb1"), FName("R_Thumb3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("R_Index1"), FName("R_Index3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("R_Mid1"), FName("R_Mid3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("R_Ring1"), FName("R_Ring3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("R_Pinky1"), FName("R_Pinky3"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightBigToe, FName("R_BigToe1"), FName("R_BigToe1"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexToe, FName("R_IndexToe1"), FName("R_IndexToe1"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleToe, FName("R_MidToe1"), FName("R_MidToe1"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingToe, FName("R_RingToe1"), FName("R_RingToe1"));
		CC4Retarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyToe, FName("R_PinkyToe1"), FName("R_PinkyToe1"));
		// bone settings for IK
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Hip"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("L_Clavicle"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("R_Clavicle"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("L_Foot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("R_Foot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("L_Calf"), ETLLRN_PreferredAxis::NegativeX);
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("R_Calf"), ETLLRN_PreferredAxis::NegativeX);
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("L_Forearm"), ETLLRN_PreferredAxis::PositiveX);
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("R_Forearm"), ETLLRN_PreferredAxis::PositiveX);
		CC4.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("Pelvis"), true);
		// exclude feet from auto-pose
		CC4.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("CC_Base_L_Foot");
		CC4.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("CC_Base_R_Foot");
	}

	// Xsens
	{
		static FName XsensName = "Xsens";
		static TArray<FName> XsenseBones = {"Reference", "Hips", "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase", "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase", "Spine", "Spine1", "Spine2", "Spine3", "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand", "LeftHandThumb1", "LeftHandThumb2", "LeftHandThumb3", "LeftHandIndex1", "LeftHandIndex2", "LeftHandIndex3", "LeftHandMiddle1", "LeftHandMiddle2", "LeftHandMiddle3", "LeftHandRing1", "LeftHandRing2", "LeftHandRing3", "LeftHandPinky1", "LeftHandPinky2", "LeftHandPinky3", "RightShoulder", "RightArm", "RightForeArm", "RightHand", "RightHandThumb1", "RightHandThumb2", "RightHandThumb3", "RightHandIndex1", "RightHandIndex2", "RightHandIndex3", "RightHandMiddle1", "RightHandMiddle2", "RightHandMiddle3", "RightHandRing1", "RightHandRing2", "RightHandRing3", "RightHandPinky1", "RightHandPinky2", "RightHandPinky3", "Neck", "Head"};
		static TArray<int32> XsensParentIndices = {-1, 0, 1, 2, 3, 4, 1, 6, 7, 8, 1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 17, 21, 22, 17, 24, 25, 17, 27, 28, 17, 30, 31, 13, 33, 34, 35, 36, 37, 38, 36, 40, 41, 36, 43, 44, 36, 46, 47, 36, 49, 50, 13, 52};
		FTLLRN_TemplateHierarchy& Xsens = AddTemplateHierarchy(XsensName, XsenseBones, XsensParentIndices);
		FTLLRN_RetargetDefinition& XsensRetarget = Xsens.AutoRetargetDefinition.RetargetDefinition;
		// core
		XsensRetarget.RootBone = FName("Hips");
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Root, FName("Reference"), FName("Reference"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("Spine"), FName("Spine3"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("Neck"), FName("Neck"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("Head"), FName("Head"));
		// left
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("LeftUpLeg"), FName("LeftToeBase"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("LeftShoulder"), FName("LeftShoulder"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("LeftArm"), FName("LeftHand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("LeftHandThumb1"), FName("LeftHandThumb3"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("LeftHandIndex1"), FName("LeftHandIndex3"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("LeftHandMiddle1"), FName("LeftHandMiddle3"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("LeftHandRing1"), FName("LeftHandRing3"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("LeftHandPinky1"), FName("LeftHandPinky3"));
		// right
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("RightUpLeg"), FName("RightToeBase"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("RightShoulder"), FName("RightShoulder"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("RightArm"), FName("RightHand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("RightHandThumb1"), FName("RightHandThumb3"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("RightHandIndex1"), FName("RightHandIndex3"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("RightHandMiddle1"), FName("RightHandMiddle3"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("RightHandRing1"), FName("RightHandRing3"));
		XsensRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("RightHandPinky1"), FName("RightHandPinky3"));
		// bone settings for IK
		Xsens.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Hips"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		Xsens.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Xsens.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Xsens.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Xsens.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Xsens.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftLeg"), ETLLRN_PreferredAxis::PositiveX);
		Xsens.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightLeg"), ETLLRN_PreferredAxis::PositiveX);
		Xsens.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftForeArm"), ETLLRN_PreferredAxis::PositiveY);
		Xsens.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightForeArm"), ETLLRN_PreferredAxis::NegativeY);
		// exclude feet from auto-pose
		Xsens.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("LeftFoot");
		Xsens.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("RightFoot");
	}
	
	// mGear
	{
		static FName mGearName = "mGear";
		static TArray<FName> mGearBones = {"root_C0_0_jnt", "spine_C0_pelvis_jnt", "spine_C0_01_jnt", "spine_C0_02_jnt", "spine_C0_03_jnt", "spine_C0_04_jnt", "spine_C0_05_jnt", "neck_C0_01_jnt", "neck_C0_02_jnt", "neck_C0_head_jnt", "headBendNose_C0_0_jnt", "headBendJaw_C0_0_jnt", "jaw_C0_0_jnt", "shoulder_L0_0_jnt", "arm_L0_upperarm_jnt", "arm_L0_lowerarm_jnt", "arm_L0_hand_jnt", "thumb_L0_0_jnt", "thumb_L0_1_jnt", "thumb_L0_2_jnt", "meta_L0_0_jnt", "finger_L0_0_jnt", "finger_L0_1_jnt", "finger_L0_2_jnt", "meta_L0_1_jnt", "finger_L1_0_jnt", "finger_L1_1_jnt", "finger_L1_2_jnt", "meta_L0_2_jnt", "finger_L2_0_jnt", "finger_L2_1_jnt", "finger_L2_2_jnt", "meta_L0_3_jnt", "finger_L3_0_jnt", "finger_L3_1_jnt", "finger_L3_2_jnt", "shoulder_R0_0_jnt", "arm_R0_upperarm_jnt", "arm_R0_lowerarm_jnt", "arm_R0_hand_jnt", "thumb_R0_0_jnt", "thumb_R0_1_jnt", "thumb_R0_2_jnt", "meta_R0_0_jnt", "finger_R0_0_jnt", "finger_R0_1_jnt", "finger_R0_2_jnt", "meta_R0_1_jnt", "finger_R1_0_jnt", "finger_R1_1_jnt", "finger_R1_2_jnt", "meta_R0_2_jnt", "finger_R2_0_jnt", "finger_R2_1_jnt", "finger_R2_2_jnt", "meta_R0_3_jnt", "finger_R3_0_jnt", "finger_R3_1_jnt", "finger_R3_2_jnt", "leg_L0_thigh_jnt", "leg_L0_calf_jnt", "leg_L0_foot_jnt", "foot_L0_ball_jnt", "leg_R0_thigh_jnt", "leg_R0_calf_jnt", "leg_R0_foot_jnt", "foot_R0_ball_jnt", "ik_hand_root_C0_0_jnt", "ik_hand_gun_C0_0_jnt", "ik_hand_L0_0_jnt", "ik_hand_R0_0_jnt", "ik_foot_root_C0_0_jnt", "ik_foot_L0_0_jnt", "ik_foot_R0_0_jnt"};
		static TArray<int32> mGearParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 6, 13, 14, 15, 16, 17, 18, 16, 20, 21, 22, 16, 24, 25, 26, 16, 28, 29, 30, 16, 32, 33, 34, 6, 36, 37, 38, 39, 40, 41, 39, 43, 44, 45, 39, 47, 48, 49, 39, 51, 52, 53, 39, 55, 56, 57, 1, 59, 60, 61, 1, 63, 64, 65, 0, 67, 68, 68, 0, 71, 71};
		FTLLRN_TemplateHierarchy& mGear = AddTemplateHierarchy(mGearName, mGearBones, mGearParentIndices);
		FTLLRN_RetargetDefinition& mGearRetarget = mGear.AutoRetargetDefinition.RetargetDefinition;
		// core
		mGearRetarget.RootBone = FName("spine_C0_pelvis_jnt");
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Root, FName("Reference"), FName("Reference"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("spine_C0_01_jnt"), FName("spine_C0_05_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("neck_C0_01_jnt"), FName("neck_C0_02_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("neck_C0_head_jnt"), FName("neck_C0_head_jnt"));
		// left
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("leg_L0_thigh_jnt"), FName("foot_L0_ball_jnt"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("shoulder_L0_0_jnt"), FName("shoulder_L0_0_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("arm_L0_upperarm_jnt"), FName("arm_L0_hand_jnt"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("thumb_L0_0_jnt"), FName("thumb_L0_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("finger_L0_0_jnt"), FName("finger_L0_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("finger_L1_0_jnt"), FName("finger_L1_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("finger_L2_0_jnt"), FName("finger_L2_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("finger_L3_0_jnt"), FName("finger_L3_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("meta_L0_0_jnt"), FName("meta_L0_0_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("meta_L0_1_jnt"), FName("meta_L0_1_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("meta_L0_2_jnt"), FName("meta_L0_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("meta_L0_3_jnt"), FName("meta_L0_3_jnt"));
		// right
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("leg_R0_thigh_jnt"), FName("foot_R0_ball_jnt"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("shoulder_R0_0_jnt"), FName("shoulder_R0_0_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("arm_R0_upperarm_jnt"), FName("arm_R0_hand_jnt"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("thumb_R0_0_jnt"), FName("thumb_R0_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("finger_R0_0_jnt"), FName("finger_R0_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("finger_R1_0_jnt"), FName("finger_R1_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("finger_R2_0_jnt"), FName("finger_R2_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("finger_R3_0_jnt"), FName("finger_R3_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("meta_R0_0_jnt"), FName("meta_R0_0_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("meta_R0_1_jnt"), FName("meta_R0_1_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("meta_R0_2_jnt"), FName("meta_R0_2_jnt"));
		mGearRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("meta_R0_3_jnt"), FName("meta_R0_3_jnt"));
		// bone settings for IK
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("spine_C0_pelvis_jnt"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("shoulder_L0_0_jnt"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("shoulder_R0_0_jnt"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("leg_L0_foot_jnt"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("leg_R0_foot_jnt"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("leg_L0_calf_jnt"), ETLLRN_PreferredAxis::PositiveZ);
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("leg_R0_calf_jnt"), ETLLRN_PreferredAxis::PositiveZ);
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("arm_L0_lowerarm_jnt"), ETLLRN_PreferredAxis::PositiveZ);
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("arm_R0_lowerarm_jnt"), ETLLRN_PreferredAxis::PositiveZ);
		mGear.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("spine_C0_05_jnt"), true);
		// exclude feet from auto-pose
		mGear.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("leg_L0_foot_jnt");
		mGear.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("leg_R0_foot_jnt");
		// ik bones
		mGear.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_root_C0_0_jnt", "root_C0_0_jnt");
		mGear.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_L0_0_jnt", "leg_L0_foot_jnt");
		mGear.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_R0_0_jnt", "leg_R0_foot_jnt");
		mGear.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_root_C0_0_jnt", "arm_R0_hand_jnt");
		mGear.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_gun_C0_0_jnt", "arm_R0_hand_jnt");
		mGear.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_R0_0_jnt", "arm_R0_hand_jnt");
		mGear.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_L0_0_jnt", "arm_L0_hand_jnt");
	}
	
	// Motionbuilder / Human IK
	{
		static FName HumanIKName = "HumanIK / Motionbuilder";
		static TArray<FName> HumanIKBones = {"Hips", "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase", "Spine", "Spine1", "Spine2", "Spine3", "Neck", "Head", "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand", "LeftHandThumb1", "LeftHandThumb2", "LeftHandThumb3", "LeftHandAttach", "LeftFingerBase", "LeftHandMiddle1", "LeftHandMiddle2", "LeftHandMiddle3", "LeftHandIndex1", "LeftHandIndex2", "LeftHandIndex3", "LeftHandRing1", "LeftHandRing2", "LeftHandRing3", "LeftHandPinky1", "LeftHandPinky2", "LeftHandPinky3", "RightShoulder", "RightArm", "RightForeArm", "RightHand", "RightHandThumb1", "RightHandThumb2", "RightHandThumb3", "RightHandAttach", "RightFingerBase", "RightHandMiddle1", "RightHandMiddle2", "RightHandMiddle3", "RightHandIndex1", "RightHandIndex2", "RightHandIndex3", "RightHandRing1", "RightHandRing2", "RightHandRing3", "RightHandPinky1", "RightHandPinky2", "RightHandPinky3", "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase"};
		static TArray<int32> HumanIKParentIndices = {-1, 0, 1, 2, 3, 0, 5, 6, 7, 8, 9, 8, 11, 12, 13, 14, 15, 16, 14, 14, 19, 20, 21, 19, 23, 24, 19, 26, 27, 19, 29, 30, 8, 32, 33, 34, 35, 36, 37, 35, 35, 40, 41, 42, 40, 44, 45, 40, 47, 48, 40, 50, 51, 0, 53, 54, 55};
		FTLLRN_TemplateHierarchy& HumanIK = AddTemplateHierarchy(HumanIKName, HumanIKBones, HumanIKParentIndices);
		FTLLRN_RetargetDefinition& HumanIKRetarget = HumanIK.AutoRetargetDefinition.RetargetDefinition;
		// core
		HumanIKRetarget.RootBone = FName("Hips");
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("Spine"), FName("Spine3"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("Neck"), FName("Neck"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("Head"), FName("Head"));
		// left
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("LeftUpLeg"), FName("LeftToeBase"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("LeftShoulder"), FName("LeftShoulder"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("LeftArm"), FName("LeftHand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("LeftHandThumb1"), FName("LeftHandThumb3"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("LeftHandIndex1"), FName("LeftHandIndex3"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("LeftHandMiddle1"), FName("LeftHandMiddle3"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("LeftHandRing1"), FName("LeftHandRing3"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("LeftHandPinky1"), FName("LeftHandPinky3"));
		// right
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("RightUpLeg"), FName("RightToeBase"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("RightShoulder"), FName("RightShoulder"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("RightArm"), FName("RightHand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("RightHandThumb1"), FName("RightHandThumb3"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("RightHandIndex1"), FName("RightHandIndex3"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("RightHandMiddle1"), FName("RightHandMiddle3"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("RightHandRing1"), FName("RightHandRing3"));
		HumanIKRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("RightHandPinky1"), FName("RightHandPinky3"));
		// bone settings for IK
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Hips"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftLeg"), ETLLRN_PreferredAxis::NegativeY);
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightLeg"), ETLLRN_PreferredAxis::NegativeY);
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftForeArm"), ETLLRN_PreferredAxis::PositiveY);
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightForeArm"), ETLLRN_PreferredAxis::PositiveY);
		HumanIK.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("Spine3"), true);
		// exclude feet from auto-pose
		HumanIK.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("LeftFoot");
		HumanIK.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("RightFoot");
	}

	// Vicon
	{
		static FName ViconName = "Vicon";
		static TArray<FName> ViconBones = {"Hips", "Spine", "Spine1", "Spine2", "Spine3", "Neck", "Neck1", "Head", "HeadEnd", "RightShoulder", "RightArm", "RightForeArm", "RightHand", "RightHandMiddle1", "RightHandMiddle2", "RightHandMiddle3", "RightHandMiddle4", "RightHandRing", "RightHandRing1", "RightHandRing2", "RightHandRing3", "RightHandRing4", "RightHandPinky", "RightHandPinky1", "RightHandPinky2", "RightHandPinky3", "RightHandPinky4", "RightHandIndex", "RightHandIndex1", "RightHandIndex2", "RightHandIndex3", "RightHandIndex4", "RightHandThumb1", "RightHandThumb2", "RightHandThumb3", "RightHandThumb4", "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand", "LeftHandMiddle1", "LeftHandMiddle2", "LeftHandMiddle3", "LeftHandMiddle4", "LeftHandRing", "LeftHandRing1", "LeftHandRing2", "LeftHandRing3", "LeftHandRing4", "LeftHandPinky", "LeftHandPinky1", "LeftHandPinky2", "LeftHandPinky3", "LeftHandPinky4", "LeftHandIndex", "LeftHandIndex1", "LeftHandIndex2", "LeftHandIndex3", "LeftHandIndex4", "LeftHandThumb1", "LeftHandThumb2", "LeftHandThumb3", "LeftHandThumb4", "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase", "RightToeBaseEnd", "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase", "LeftToeBaseEnd"};
		static TArray<int32> ViconParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 4, 9, 10, 11, 12, 13, 14, 15, 12, 17, 18, 19, 20, 17, 22, 23, 24, 25, 12, 27, 28, 29, 30, 27, 32, 33, 34, 4, 36, 37, 38, 39, 40, 41, 42, 39, 44, 45, 46, 47, 44, 49, 50, 51, 52, 39, 54, 55, 56, 57, 54, 59, 60, 61, 0, 63, 64, 65, 66, 0, 68, 69, 70, 71};
		FTLLRN_TemplateHierarchy& Vicon = AddTemplateHierarchy(ViconName, ViconBones, ViconParentIndices);
		FTLLRN_RetargetDefinition& ViconRetarget = Vicon.AutoRetargetDefinition.RetargetDefinition;
		// core
		ViconRetarget.RootBone = FName("Hips");
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("Spine"), FName("Spine3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("Neck"), FName("Neck1"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("Head"), FName("Head"));
		// left
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("LeftUpLeg"), FName("LeftToeBase"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("LeftShoulder"), FName("LeftShoulder"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("LeftArm"), FName("LeftHand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("LeftHandThumb1"), FName("LeftHandThumb3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("LeftHandIndex1"), FName("LeftHandIndex3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("LeftHandMiddle1"), FName("LeftHandMiddle3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("LeftHandRing1"), FName("LeftHandRing3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("LeftHandPinky1"), FName("LeftHandPinky3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("LeftHandIndex"), FName("LeftHandIndex"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("LeftHandPinky"), FName("LeftHandPinky"));
		// right
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("RightUpLeg"), FName("RightToeBase"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("RightShoulder"), FName("RightShoulder"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("RightArm"), FName("RightHand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("RightHandThumb1"), FName("RightHandThumb3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("RightHandIndex1"), FName("RightHandIndex3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("RightHandMiddle1"), FName("RightHandMiddle3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("RightHandRing1"), FName("RightHandRing3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("RightHandPinky1"), FName("RightHandPinky3"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("RightHandIndex"), FName("RightHandIndex"));
		ViconRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("RightHandPinky"), FName("RightHandPinky"));
		// bone settings for IK
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Hips"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftHand"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightHand"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftLeg"), ETLLRN_PreferredAxis::NegativeX);
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightLeg"), ETLLRN_PreferredAxis::NegativeX);
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftForeArm"), ETLLRN_PreferredAxis::NegativeX);
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightForeArm"), ETLLRN_PreferredAxis::NegativeX);
		Vicon.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("Spine3"), true);
		// exclude feet from auto-pose
		Vicon.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("LeftFoot");
		Vicon.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("RightFoot");
	}

	// Optitrack / Motive
	{
		static FName OptitrakName = "OptiTrack / Motive";
		static TArray<FName> OptitrakBones = {"root", "pelvis", "spine_01", "spine_02", "neck_01", "head", "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r", "thigh_l", "calf_l", "foot_l", "ball_l", "thigh_r", "calf_r", "foot_r", "ball_r"};
		static TArray<int32> OptitrakParentIndices = {-1, 0, 1, 2, 3, 4, 3, 6, 7, 8, 3, 10, 11, 12, 1, 14, 15, 16, 1, 18, 19, 20};
		FTLLRN_TemplateHierarchy& Optitrak = AddTemplateHierarchy(OptitrakName, OptitrakBones, OptitrakParentIndices);
		FTLLRN_RetargetDefinition& OptitrakRetarget = Optitrak.AutoRetargetDefinition.RetargetDefinition;
		// core
		OptitrakRetarget.RootBone = FName("pelvis");
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Root, FName("root"), FName("root"));
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("spine_01"), FName("spine_02"));
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("neck_01"), FName("neck_01"));
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("head"), FName("head"));
		// left
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("thigh_l"), FName("ball_l"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("clavicle_l"), FName("clavicle_l"));
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("upperarm_l"), FName("hand_l"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		// right
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("thigh_r"), FName("ball_r"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("clavicle_r"), FName("clavicle_r"));
		OptitrakRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("upperarm_r"), FName("hand_r"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		// bone settings for IK
		Optitrak.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("pelvis"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		Optitrak.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("clavicle_l"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Optitrak.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("clavicle_r"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Optitrak.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("foot_l"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Optitrak.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("foot_r"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Optitrak.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("calf_l"), ETLLRN_PreferredAxis::NegativeY);
		Optitrak.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("calf_r"), ETLLRN_PreferredAxis::NegativeY);
		Optitrak.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lowerarm_l"), ETLLRN_PreferredAxis::PositiveZ);
		Optitrak.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lowerarm_r"), ETLLRN_PreferredAxis::NegativeZ);
		// exclude feet from auto-pose
		Optitrak.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("foot_l");
		Optitrak.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("foot_r");
	}

	// Sony Mocopi
	{
		static FName MocopiName = "Sony Mocopi";
		static TArray<FName> MocopiBones = {"root", "torso_1", "torso_2", "torso_3", "torso_4", "torso_5", "torso_6", "torso_7", "neck_1", "neck_2", "head", "head_End", "l_shoulder", "l_up_arm", "l_low_arm", "l_hand", "l_hand_End", "r_shoulder", "r_up_arm", "r_low_arm", "r_hand", "r_hand_End", "l_up_leg", "l_low_leg", "l_foot", "l_toes", "l_toes_End", "r_up_leg", "r_low_leg", "r_foot", "r_toes", "r_toes_End"};
		static TArray<int32> MocopiParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 7, 12, 13, 14, 15, 7, 17, 18, 19, 20, 0, 22, 23, 24, 25, 0, 27, 28, 29, 30};
		FTLLRN_TemplateHierarchy& Mocopi = AddTemplateHierarchy(MocopiName, MocopiBones, MocopiParentIndices);
		FTLLRN_RetargetDefinition& MocopiRetarget = Mocopi.AutoRetargetDefinition.RetargetDefinition;
		MocopiRetarget.RootBone = FName("root");
		MocopiRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("torso_1"), FName("torso_7"));
		MocopiRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("neck_1"), FName("neck_2"));
		MocopiRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("head"), FName("head"));
		// left
		MocopiRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("l_up_leg"), FName("l_toes"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		MocopiRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("l_shoulder"), FName("l_shoulder"));
		MocopiRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("l_up_arm"), FName("l_hand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		// right
		MocopiRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("r_up_leg"), FName("r_toes"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		MocopiRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("r_shoulder"), FName("r_shoulder"));
		MocopiRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("r_up_arm"), FName("r_hand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		// bone settings for IK
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("root"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("l_shoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("r_shoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("l_foot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("r_foot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("l_low_leg"), ETLLRN_PreferredAxis::PositiveZ);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("r_low_leg"), ETLLRN_PreferredAxis::NegativeZ);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("l_low_arm"), ETLLRN_PreferredAxis::NegativeZ);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("r_low_arm"), ETLLRN_PreferredAxis::PositiveZ);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("torso_6"), true);
		Mocopi.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("torso_7"), true);
		// exclude feet from auto-pose
		Mocopi.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("l_foot");
		Mocopi.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("r_foot");
	}
	
	// Advanced skeleton
	{
		static FName AdvancedSkeletonName = "Advanced Skeleton";
		static TArray<FName> AdvancedSkeletonBones = {"Root_M", "Hip_R", "HipPart1_R", "HipPart2_R", "Knee_R", "Ankle_R", "Toes_R", "RootPart1_M", "RootPart2_M", "Spine1_M", "Spine1Part1_M", "Spine1Part2_M", "Chest_M", "Neck_M", "NeckPart1_M", "NeckPart2_M", "Head_M", "Scapula_R", "Shoulder_R", "ShoulderPart1_R", "ShoulderPart2_R", "Elbow_R", "ElbowPart1_R", "ElbowPart2_R", "Wrist_R", "MiddleFinger1_R", "MiddleFinger2_R", "MiddleFinger3_R", "MiddleFinger4_R", "ThumbFinger1_R", "ThumbFinger2_R", "ThumbFinger3_R", "ThumbFinger4_R", "IndexFinger1_R", "IndexFinger2_R", "IndexFinger3_R", "IndexFinger4_R", "Cup_R", "PinkyFinger1_R", "PinkyFinger2_R", "PinkyFinger3_R", "PinkyFinger4_R", "RingFinger1_R", "RingFinger2_R", "RingFinger3_R", "RingFinger4_R", "Scapula_L", "Shoulder_L", "ShoulderPart1_L", "ShoulderPart2_L", "Elbow_L", "ElbowPart1_L", "ElbowPart2_L", "Wrist_L", "MiddleFinger1_L", "MiddleFinger2_L", "MiddleFinger3_L", "MiddleFinger4_L", "ThumbFinger1_L", "ThumbFinger2_L", "ThumbFinger3_L", "ThumbFinger4_L", "IndexFinger1_L", "IndexFinger2_L", "IndexFinger3_L", "IndexFinger4_L", "Cup_L", "PinkyFinger1_L", "PinkyFinger2_L", "PinkyFinger3_L", "PinkyFinger4_L", "RingFinger1_L", "RingFinger2_L", "RingFinger3_L", "RingFinger4_L", "Hip_L", "HipPart1_L", "HipPart2_L", "Knee_L", "Ankle_L", "Toes_L"};
		static TArray<int32> AdvancedSkeletonParentIndices = {-1, 0, 1, 2, 3, 4, 5, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 12, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 24, 29, 30, 31, 24, 33, 34, 35, 24, 37, 38, 39, 40, 37, 42, 43, 44, 12, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 53, 58, 59, 60, 53, 62, 63, 64, 53, 66, 67, 68, 69, 66, 71, 72, 73, 0, 75, 76, 77, 78, 79};
		FTLLRN_TemplateHierarchy& AdvancedSkeleton = AddTemplateHierarchy(AdvancedSkeletonName, AdvancedSkeletonBones, AdvancedSkeletonParentIndices);
		FTLLRN_RetargetDefinition& AdvSkelRetarget = AdvancedSkeleton.AutoRetargetDefinition.RetargetDefinition;
		// core
		AdvSkelRetarget.RootBone = FName("Root_M");
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("RootPart1_M"), FName("Chest_M"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("Neck_M"), FName("NeckPart2_M"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("Head_M"), FName("Head_M"));
		// left
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("Hip_L"), FName("Toes_L"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("Scapula_L"), FName("Scapula_L"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("Shoulder_L"), FName("Wrist_L"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("ThumbFinger2_L"), FName("ThumbFinger4_L"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("IndexFinger1_L"), FName("IndexFinger3_L"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("MiddleFinger1_L"), FName("MiddleFinger3_L"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("RingFinger1_L"), FName("RingFinger3_L"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("PinkyFinger1_L"), FName("PinkyFinger3_L"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("Cup_L"), FName("Cup_L"));
		// right
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("Hip_R"), FName("Toes_R"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("Scapula_R"), FName("Scapula_R"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("Shoulder_R"), FName("Wrist_R"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("ThumbFinger2_R"), FName("ThumbFinger4_R"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("IndexFinger1_R"), FName("IndexFinger3_R"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("MiddleFinger1_R"), FName("MiddleFinger3_R"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("RingFinger1_R"), FName("RingFinger3_R"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("PinkyFinger1_R"), FName("PinkyFinger3_R"));
		AdvSkelRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("Cup_R"), FName("Cup_R"));
		// bone settings for IK
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Root_M"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Scapula_L"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Scapula_R"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Ankle_L"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Ankle_R"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("Knee_L"), ETLLRN_PreferredAxis::PositiveZ);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("Knee_R"), ETLLRN_PreferredAxis::PositiveZ);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("Elbow_L"), ETLLRN_PreferredAxis::NegativeZ);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("Elbow_R"), ETLLRN_PreferredAxis::NegativeZ);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("ShoulderPart1_L"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("ShoulderPart2_L"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("ElbowPart1_L"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("ElbowPart2_L"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("HipPart1_L"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("HipPart2_L"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("ShoulderPart1_R"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("ShoulderPart2_R"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("ElbowPart1_R"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("ElbowPart2_R"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("HipPart1_R"), true);
		AdvancedSkeleton.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("HipPart2_R"), true);
		// exclude feet from auto-pose
		AdvancedSkeleton.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("Ankle_L");
		AdvancedSkeleton.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("Ankle_R");
	}
	
	// MoveAI
	{
		static FName MoveOneName = "Move One";
		static TArray<FName> MoveOneBones = {"Root", "Hips", "Spine", "Spine1", "RightShoulder", "RightArm", "RightForeArm", "RightHand", "palm_01_R", "RightHandIndex1", "RightHandIndex2", "RightHandIndex3", "RightHandThumb1", "RightHandThumb2", "RightHandThumb3", "palm_02_R", "RightHandMiddle1", "RightHandMiddle2", "RightHandMiddle3", "palm_03_R", "RightHandRing1", "RightHandRing2", "RightHandRing3", "palm_04_R", "RightHandPinky1", "RightHandPinky2", "RightHandPinky3", "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand", "palm_02_L", "LeftHandMiddle1", "LeftHandMiddle2", "LeftHandMiddle3", "palm_03_L", "LeftHandRing1", "LeftHandRing2", "LeftHandRing3", "palm_04_L", "LeftHandPinky1", "LeftHandPinky2", "LeftHandPinky3", "palm_01_L", "LeftHandThumb1", "LeftHandThumb2", "LeftHandThumb3", "LeftHandIndex1", "LeftHandIndex2", "LeftHandIndex3", "Neck", "Head", "Head_end", "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase", "heel_02_L", "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase", "heel_02_R"};
		static TArray<int32> MoveOneParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 8, 12, 13, 7, 15, 16, 17, 7, 19, 20, 21, 7, 23, 24, 25, 3, 27, 28, 29, 30, 31, 32, 33, 30, 35, 36, 37, 30, 39, 40, 41, 30, 43, 44, 45, 43, 47, 48, 3, 50, 51, 1, 53, 54, 55, 55, 1, 58, 59, 60, 60};
		FTLLRN_TemplateHierarchy& MoveOne = AddTemplateHierarchy(MoveOneName, MoveOneBones, MoveOneParentIndices);
		FTLLRN_RetargetDefinition& MoveOneRetarget = MoveOne.AutoRetargetDefinition.RetargetDefinition;
		// core
		MoveOneRetarget.RootBone = FName("Hips");
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("Spine"), FName("Spine1"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("Neck"), FName("Neck"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("Head"), FName("Head"));
		// left
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("LeftUpLeg"), FName("LeftToeBase"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("LeftShoulder"), FName("LeftShoulder"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("LeftArm"), FName("LeftHand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("LeftHandThumb1"), FName("LeftHandThumb3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("LeftHandIndex1"), FName("LeftHandIndex3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("LeftHandMiddle1"), FName("LeftHandMiddle3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("LeftHandRing1"), FName("LeftHandRing3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("LeftHandPinky1"), FName("LeftHandPinky3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("palm_01_L"), FName("palm_01_L"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("palm_02_L"), FName("palm_02_L"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("palm_03_L"), FName("palm_03_L"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("palm_04_L"), FName("palm_04_L"));
		// right
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("RightUpLeg"), FName("RightToeBase"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("RightShoulder"), FName("RightShoulder"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("RightArm"), FName("RightHand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("RightHandThumb1"), FName("RightHandThumb3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("RightHandIndex1"), FName("RightHandIndex3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("RightHandMiddle1"), FName("RightHandMiddle3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("RightHandRing1"), FName("RightHandRing3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("RightHandPinky1"), FName("RightHandPinky3"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("palm_01_R"), FName("palm_01_R"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("palm_02_R"), FName("palm_02_R"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("palm_03_R"), FName("palm_03_R"));
		MoveOneRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("palm_04_R"), FName("palm_04_R"));
		// bone settings for IK
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Hips"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftLeg"), ETLLRN_PreferredAxis::NegativeY);
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightLeg"), ETLLRN_PreferredAxis::NegativeY);
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftForeArm"), ETLLRN_PreferredAxis::PositiveY);
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightForeArm"), ETLLRN_PreferredAxis::PositiveY);
		MoveOne.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("Spine3"), true);
		// exclude feet from auto-pose
		MoveOne.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("LeftFoot");
		MoveOne.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("RightFoot");
	}

	// Qualisys
	{
		static FName QualisysName = "Qualisys";
		static TArray<FName> QualisysBones = {"Reference", "Hips", "Spine", "Spine1", "Spine2", "Neck", "Head", "Head_end", "LeftShoulder", "LeftArm", "LeftForeArm", "LeftForeArmRoll", "LeftHand", "LeftHand_end", "LeftInHandThumb", "LeftHandThumb0", "LeftHandThumb1", "LeftHandThumb2", "LeftHandThumb3", "LeftHandThumb3_end", "LeftInHandIndex", "LeftHandIndex1", "LeftHandIndex2", "LeftHandIndex3", "LeftHandIndex3_end", "LeftInHandMiddle", "LeftHandMiddle1", "LeftHandMiddle2", "LeftHandMiddle3", "LeftHandMiddle3_end", "LeftInHandRing", "LeftHandRing1", "LeftHandRing2", "LeftHandRing3", "LeftHandRing3_end", "LeftInHandPinky", "LeftHandPinky1", "LeftHandPinky2", "LeftHandPinky3", "LeftHandPinky3_end", "RightShoulder", "RightArm", "RightForeArm", "RightForeArmRoll", "RightHand", "RightHand_end", "RightInHandPinky", "RightHandPinky1", "RightHandPinky2", "RightHandPinky3", "RightHandPinky3_end", "RightInHandRing", "RightHandRing1", "RightHandRing2", "RightHandRing3", "RightHandRing3_end", "RightInHandMiddle", "RightHandMiddle1", "RightHandMiddle2", "RightHandMiddle3", "RightHandMiddle3_end", "RightInHandIndex", "RightHandIndex1", "RightHandIndex2", "RightHandIndex3", "RightHandIndex3_end", "RightInHandThumb", "RightInHandThumb0", "RightHandThumb1", "RightHandThumb2", "RightHandThumb3", "RightHandThumb3_end", "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase", "LeftToeBase_end", "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase", "RightToeBase_end"};
		static TArray<int32> QualisysParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 4, 8, 9, 10, 11, 12, 12, 14, 15, 16, 17, 18, 12, 20, 21, 22, 23, 12, 25, 26, 27, 28, 12, 30, 31, 32, 33, 12, 35, 36, 37, 38, 4, 40, 41, 42, 43, 44, 44, 46, 47, 48, 49, 44, 51, 52, 53, 54, 44, 56, 57, 58, 59, 44, 61, 62, 63, 64, 44, 66, 67, 68, 69, 70, 1, 72, 73, 74, 75, 1, 77, 78, 79, 80};
		FTLLRN_TemplateHierarchy& Qualisys = AddTemplateHierarchy(QualisysName, QualisysBones, QualisysParentIndices);
		FTLLRN_RetargetDefinition& QualisysRetarget = Qualisys.AutoRetargetDefinition.RetargetDefinition;
		// core
		QualisysRetarget.RootBone = FName("Hips");
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("Spine"), FName("Spine2"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("Neck"), FName("Neck"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("Head"), FName("Head"));
		// left
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("LeftUpLeg"), FName("LeftToeBase"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("LeftShoulder"), FName("LeftShoulder"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("LeftArm"), FName("LeftHand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("LeftHandThumb1"), FName("LeftHandThumb3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("LeftHandIndex1"), FName("LeftHandIndex3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("LeftHandMiddle1"), FName("LeftHandMiddle3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("LeftHandRing1"), FName("LeftHandRing3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("LeftHandPinky1"), FName("LeftHandPinky3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumbMetacarpal, FName("LeftInHandThumb"), FName("LeftInHandThumb"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("LeftInHandIndex"), FName("LeftInHandIndex"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("LeftInHandMiddle"), FName("LeftInHandMiddle"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("LeftInHandRing"), FName("LeftInHandRing"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("LeftInHandPinky"), FName("LeftInHandPinky"));
		// right
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("RightUpLeg"), FName("RightToeBase"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("RightShoulder"), FName("RightShoulder"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("RightArm"), FName("RightHand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("RightHandThumb1"), FName("RightHandThumb3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("RightHandIndex1"), FName("RightHandIndex3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("RightHandMiddle1"), FName("RightHandMiddle3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("RightHandRing1"), FName("RightHandRing3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("RightHandPinky1"), FName("RightHandPinky3"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumbMetacarpal, FName("RightInHandThumb"), FName("RightInHandThumb"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("RightInHandIndex"), FName("RightInHandIndex"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("RightInHandMiddle"), FName("RightInHandMiddle"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("RightInHandRing"), FName("RightInHandRing"));
		QualisysRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("RightInHandPinky"), FName("RightInHandPinky"));
		// bone settings for IK
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Hips"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightFoot"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftLeg"), ETLLRN_PreferredAxis::NegativeY);
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightLeg"), ETLLRN_PreferredAxis::NegativeY);
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftForeArm"), ETLLRN_PreferredAxis::PositiveY);
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightForeArm"), ETLLRN_PreferredAxis::PositiveY);
		Qualisys.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("Spine2"), true);
		// exclude feet from auto-pose
		Qualisys.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("LeftFoot");
		Qualisys.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("RightFoot");
	}
	
	// FN skeleton
	{
		static FName FNHumanName = "Fortnite Humanoid";
		static TArray<FName> FNHumanBones = {"root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "index_metacarpal_l", "index_01_l", "index_02_l", "index_03_l", "middle_metacarpal_l", "middle_01_l", "middle_02_l", "middle_03_l", "pinky_metacarpal_l", "pinky_01_l", "pinky_02_l", "pinky_03_l", "ring_metacarpal_l", "ring_01_l", "ring_02_l", "ring_03_l", "thumb_01_l", "thumb_02_l", "thumb_03_l", "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r", "index_metacarpal_r", "index_01_r", "index_02_r", "index_03_r", "middle_metacarpal_r", "middle_01_r", "middle_02_r", "middle_03_r", "pinky_metacarpal_r", "pinky_01_r", "pinky_02_r", "pinky_03_r", "ring_metacarpal_r", "ring_01_r", "ring_02_r", "ring_03_r", "thumb_01_r", "thumb_02_r", "thumb_03_r", "neck_01", "neck_02", "head", "thigh_l", "calf_l", "foot_l", "ball_l", "thigh_r", "calf_r", "foot_r", "ball_r", "ik_foot_root", "ik_foot_l", "ik_foot_r", "ik_hand_root", "ik_hand_gun", "ik_hand_l", "ik_hand_r"};
		static TArray<int32> FNHumanParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 10, 15, 16, 17, 10, 19, 20, 21, 10, 23, 24, 25, 10, 27, 28, 6, 30, 31, 32, 33, 34, 35, 36, 33, 38, 39, 40, 33, 42, 43, 44, 33, 46, 47, 48, 33, 50, 51, 6, 53, 54, 1, 56, 57, 58, 1, 60, 61, 62, 0, 64, 64, 0, 67, 68, 68};
		FTLLRN_TemplateHierarchy& FNHuman = AddTemplateHierarchy(FNHumanName, FNHumanBones, FNHumanParentIndices);
		FTLLRN_RetargetDefinition& FNHumanRetarget = FNHuman.AutoRetargetDefinition.RetargetDefinition;
		// core
		FNHumanRetarget.RootBone = FName("pelvis");
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Root, FName("root"), FName("root"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("spine_01"), FName("spine_05"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("neck_01"), FName("neck_02"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("head"), FName("head"));
		// left
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("thigh_l"), FName("ball_l"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("clavicle_l"), FName("clavicle_l"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("upperarm_l"), FName("hand_l"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("thumb_01_l"), FName("thumb_03_l"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("index_01_l"), FName("index_03_l"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("middle_01_l"), FName("middle_03_l"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("ring_01_l"), FName("ring_03_l"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("pinky_01_l"), FName("pinky_03_l"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("index_metacarpal_l"), FName("index_metacarpal_l"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("middle_metacarpal_l"), FName("middle_metacarpal_l"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("ring_metacarpal_l"), FName("ring_metacarpal_l"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("pinky_metacarpal_l"), FName("pinky_metacarpal_l"));
		// right
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("thigh_r"), FName("ball_r"), FTLLRN_CharacterizationStandard::RightFootIKGoal);
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("clavicle_r"), FName("clavicle_r"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("upperarm_r"), FName("hand_r"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("thumb_01_r"), FName("thumb_03_r"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("index_01_r"), FName("index_03_r"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("middle_01_r"), FName("middle_03_r"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("ring_01_r"), FName("ring_03_r"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("pinky_01_r"), FName("pinky_03_r"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("index_metacarpal_r"), FName("index_metacarpal_r"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("middle_metacarpal_r"), FName("middle_metacarpal_r"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("ring_metacarpal_r"), FName("ring_metacarpal_r"));
		FNHumanRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("pinky_metacarpal_r"), FName("pinky_metacarpal_r"));
		// bone settings for IK
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("pelvis"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("clavicle_l"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("clavicle_r"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("foot_l"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("foot_r"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("calf_l"), ETLLRN_PreferredAxis::PositiveZ);
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("calf_r"), ETLLRN_PreferredAxis::PositiveZ);
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lowerarm_l"), ETLLRN_PreferredAxis::PositiveZ);
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("lowerarm_r"), ETLLRN_PreferredAxis::PositiveZ);
		FNHuman.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("spine_05"), true);
		// unreal ik bones
		FNHuman.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_root", "root");
		FNHuman.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_l", "foot_l");
		FNHuman.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_foot_r", "foot_r");
		FNHuman.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_root", "root");
		FNHuman.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_gun", "hand_r");
		FNHuman.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_l", "hand_l");
		FNHuman.AutoRetargetDefinition.BonesToPin.AddBoneToPin("ik_hand_r", "hand_r");
		// exclude feet from auto-pose
		FNHuman.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("foot_l");
		FNHuman.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("foot_r");
	}

	// FN Quadruped
	{
		static FName FNQuadrupedName = "FN Quadruped";
		static TArray<FName> FNQuadrupedBones = {"root", "QuadSpine_A_Pelvis_C", "QuadSpine_A_Spine1_C", "QuadSpine_A_Spine2_C", "QuadSpine_A_Spine3_C", "QuadSpine_A_Chest_C", "PawedArm_A_Shoulder_L", "PawedArm_A_Elbow_L", "PawedArm_A_Wrist_L", "PawedArm_A_Ball_L", "PawedArm_A_Toe_L", "PawedArm_A_Scapula_L", "BipedHeadNeck_A_NeckBase_C", "BipedHeadNeck_A_NeckMid_C", "BipedHeadNeck_A_Head_C", "PawedArm_A_Shoulder_R", "PawedArm_A_Elbow_R", "PawedArm_A_Wrist_R", "PawedArm_A_Ball_R", "PawedArm_A_Toe_R", "PawedArm_A_Scapula_R", "Leaf_Hips_Leaf_C", "PawedLeg_A_Thigh_L", "PawedLeg_A_Knee_L", "PawedLeg_A_Ankle_L", "PawedLeg_A_Ball_L", "PawedLeg_A_Toe_L", "Tail_A_TailBase_C", "Tail_A_Tail1_C", "Tail_A_Tail2_C", "Tail_A_Tail3_C", "Tail_A_Tail4_C", "Tail_A_Tail5_C", "Tail_A_Tail6_C", "C_Tail_A_Tail7_Jnt", "PawedLeg_A_Thigh_R", "PawedLeg_A_Knee_R", "PawedLeg_A_Ankle_R", "PawedLeg_A_Ball_R", "PawedLeg_A_Toe_R"};
		static TArray<int32> FNQuadrupedParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 5, 12, 13, 5, 15, 16, 17, 18, 15, 1, 21, 22, 23, 24, 25, 21, 27, 28, 29, 30, 31, 32, 33, 21, 35, 36, 37, 38};
		FTLLRN_TemplateHierarchy& FNQuadruped = AddTemplateHierarchy(FNQuadrupedName, FNQuadrupedBones, FNQuadrupedParentIndices);
		FTLLRN_RetargetDefinition& FNQuadrupedRetarget = FNQuadruped.AutoRetargetDefinition.RetargetDefinition;
		// core
		FNQuadrupedRetarget.RootBone = FName("QuadSpine_A_Pelvis_C");
		FNQuadrupedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("QuadSpine_A_Spine1_C"), FName("QuadSpine_A_Spine3_C"));
		FNQuadrupedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("BipedHeadNeck_A_NeckBase_C"), FName("BipedHeadNeck_A_NeckMid_C"));
		FNQuadrupedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("BipedHeadNeck_A_Head_C"), FName("BipedHeadNeck_A_Head_C"));
		// left
		FNQuadrupedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("PawedLeg_A_Thigh_L"), FName("PawedLeg_A_Toe_L"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		FNQuadrupedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("PawedArm_A_Shoulder_L"), FName("PawedArm_A_Ball_L"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		// right
		FNQuadrupedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("PawedLeg_A_Thigh_R"), FName("PawedLeg_A_Toe_R"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		FNQuadrupedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("PawedArm_A_Shoulder_R"), FName("PawedArm_A_Ball_R"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		// tail
		FNQuadrupedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Tail, FName("Tail_A_TailBase_C"), FName("C_Tail_A_Tail2_Jnt"));
		// unreal ik bones
		FNQuadruped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Front_Root", "root");
		FNQuadruped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Front_L", "PawedArm_A_Ball_L");
		FNQuadruped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Front_R", "PawedArm_A_Ball_R");
		FNQuadruped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Rear_Root", "root");
		FNQuadruped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Rear_L", "PawedLeg_A_Ball_L");
		FNQuadruped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Rear_R", "PawedLeg_A_Ball_R");
	}

	// FN Raptor
	{
		static FName FNRaptorName = "FN Raptor";
		static TArray<FName> FNRaptorBones = {"root", "QuadSpine_A_Pelvis_C", "QuadSpine_A_Spine1_C", "QuadSpine_A_Spine2_C", "QuadSpine_A_Spine3_C", "QuadSpine_A_Chest_C", "BipedArm_A_Clavicle_L", "BipedArm_A_Shoulder_L", "BipedArm_A_Elbow_L", "BipedArm_A_Wrist_L", "BipedHand_A_IndexCarpal_L", "BipedHand_A_IndexBase_L", "BipedHand_A_IndexMid_L", "BipedHand_A_IndexTip_L", "BipedHand_A_MidCarpal_L", "BipedHand_A_MidBase_L", "BipedHand_A_MidMid_L", "BipedHand_A_MidTip_L", "BipedHand_A_RingCarpal_L", "BipedHand_A_RingBase_L", "BipedHand_A_RingMid_L", "BipedHand_A_RingTip_L", "BipedArm_A_LowerA_L", "BipedArm_A_LowerB_L", "BipedArm_A_UpperA_L", "BipedArm_A_UpperB_L", "BipedHeadNeck_A_NeckBase_C", "BipedHeadNeck_A_NeckMid_C", "BipedHeadNeck_A_Head_C", "BipedArm_A_Clavicle_R", "BipedArm_A_Shoulder_R", "BipedArm_A_Elbow_R", "BipedArm_A_Wrist_R", "BipedHand_A_IndexCarpal_R", "BipedHand_A_IndexBase_R", "BipedHand_A_IndexMid_R", "BipedHand_A_IndexTip_R", "BipedHand_A_MidCarpal_R", "BipedHand_A_MidBase_R", "BipedHand_A_MidMid_R", "BipedHand_A_MidTip_R", "BipedHand_A_RingCarpal_R", "BipedHand_A_RingBase_R", "BipedHand_A_RingMid_R", "BipedHand_A_RingTip_R", "HoofedLeg_A_Thigh_L", "HoofedLeg_A_Knee_L", "HoofedLeg_A_Ankle_L", "HoofedLeg_A_Ball_L", "HoofedLeg_A_Toe1_L", "HoofedLeg_A_Toe2_L", "HoofedLeg_A_ToeTip_L", "Leaf_BackToe_Leaf_L", "Leaf_LongToe_Leaf1_L", "Leaf_LongToe_Leaf2_L", "Leaf_ShortToe_Leaf_L", "Tail_A_TailBase_C", "Tail_A_Tail1_C", "Tail_A_Tail2_C", "Tail_A_Tail3_C", "Tail_A_Tail4_C", "Tail_A_Tail5_C", "Tail_A_Tail6_C", "Tail_A_Tail7_C", "Tail_A_Tail8_C", "Tail_A_Tail9_C", "HoofedLeg_A_Thigh_R", "HoofedLeg_A_Knee_R", "HoofedLeg_A_Ankle_R", "HoofedLeg_A_Ball_R", "HoofedLeg_A_Toe1_R", "HoofedLeg_A_Toe2_R", "HoofedLeg_A_ToeTip_R", "IK_Foot_Root", "IK_Foot_L", "IK_Foot_R"};
		static TArray<int32> FNRaptorParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 9, 14, 15, 16, 9, 18, 19, 20, 8, 8, 7, 7, 5, 26, 27, 5, 29, 30, 31, 32, 33, 34, 35, 32, 37, 38, 39, 32, 41, 42, 43, 1, 45, 46, 47, 48, 49, 50, 49, 49, 53, 49, 1, 56, 57, 58, 59, 60, 61, 62, 63, 64, 1, 66, 67, 68, 69, 70, 71, 0, 73, 73};
		FTLLRN_TemplateHierarchy& FNRaptor = AddTemplateHierarchy(FNRaptorName, FNRaptorBones, FNRaptorParentIndices);
		FTLLRN_RetargetDefinition& FNRaptorRetarget = FNRaptor.AutoRetargetDefinition.RetargetDefinition;
		// core
		FNRaptorRetarget.RootBone = FName("QuadSpine_A_Pelvis_C");
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("QuadSpine_A_Spine1_C"), FName("QuadSpine_A_Spine3_C"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("BipedHeadNeck_A_NeckBase_C"), FName("BipedHeadNeck_A_NeckMid_C"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("BipedHeadNeck_A_Head_C"), FName("BipedHeadNeck_A_Head_C"));
		// left
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("BipedArm_A_Clavicle_L"), FName("BipedArm_A_Clavicle_L"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("BipedArm_A_Shoulder_L"), FName("BipedArm_A_Wrist_L"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("HoofedLeg_A_Thigh_L"), FName("HoofedLeg_A_Toe1_L"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("BipedHand_A_IndexBase_L"), FName("BipedHand_A_IndexTip_L"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("BipedHand_A_MidBase_L"), FName("BipedHand_A_MidTip_L"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("BipedHand_A_RingBase_L"), FName("BipedHand_A_RingTip_L"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("BipedHand_A_IndexCarpal_L"), FName("BipedHand_A_IndexCarpal_L"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("BipedHand_A_MidCarpal_L"), FName("BipedHand_A_MidCarpal_L"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("BipedHand_A_RingCarpal_L"), FName("BipedHand_A_RingCarpal_L"));
		// right
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("BipedArm_A_Clavicle_R"), FName("BipedArm_A_Clavicle_R"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("BipedArm_A_Shoulder_R"), FName("BipedArm_A_Wrist_R"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("HoofedLeg_A_Thigh_R"), FName("HoofedLeg_A_Toe1_R"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("BipedHand_A_IndexBase_R"), FName("BipedHand_A_IndexTip_R"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("BipedHand_A_MidBase_R"), FName("BipedHand_A_MidTip_R"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("BipedHand_A_RingBase_R"), FName("BipedHand_A_RingTip_R"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("BipedHand_A_IndexCarpal_R"), FName("BipedHand_A_IndexCarpal_R"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("BipedHand_A_MidCarpal_R"), FName("BipedHand_A_MidCarpal_R"));
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("BipedHand_A_RingCarpal_R"), FName("BipedHand_A_RingCarpal_R"));
		// tail
		FNRaptorRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Tail, FName("Tail_A_TailBase_C"), FName("Tail_A_Tail2_C"));
		// unreal ik bones
		FNRaptor.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Root", "root");
		FNRaptor.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_L", "HoofedLeg_A_Ball_L");
		FNRaptor.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_R", "HoofedLeg_A_Ball_R");
	}

	// FN Amphibian
	{
		static FName FNAmphibianName = "FN Amphibian";
		static TArray<FName> FNAmphibianBones = {"root", "QuadSpine_A_Pelvis_C", "QuadSpine_A_Spine1_C", "QuadSpine_A_Spine2_C", "QuadSpine_A_Spine3_C", "QuadSpine_A_Spine4_C", "QuadSpine_A_Spine5_C", "QuadSpine_A_Chest_C", "BipedHeadNeck_A_NeckBase_C", "BipedHeadNeck_A_NeckMid_C", "BipedHeadNeck_A_Head_C", "Leaf_TongueScaleA_Leaf_C", "Tail_Tongue_TailBase_C", "Tail_Tongue_Tail1_C", "Tail_Tongue_Tail2_C", "Tail_Tongue_Tail3_C", "Tail_Tongue_Tail4_C", "Tail_Tongue_Tail5_C", "vocalSac_01", "vocalSac_02", "BipedArm_A_Clavicle_L", "BipedArm_A_Shoulder_L", "BipedArm_A_Elbow_L", "BipedArm_A_Wrist_L", "BipedHand_A_IndexBase_L", "BipedHand_A_IndexMid_L", "BipedHand_A_IndexTip_L", "BipedHand_A_MidBase_L", "BipedHand_A_MidMid_L", "BipedHand_A_MidTip_L", "BipedHand_A_RingBase_L", "BipedHand_A_RingMid_L", "BipedHand_A_RingTip_L", "BipedArm_A_Clavicle_R", "BipedArm_A_Shoulder_R", "BipedArm_A_Elbow_R", "BipedArm_A_Wrist_R", "BipedHand_A_IndexBase_R", "BipedHand_A_IndexMid_R", "BipedHand_A_IndexTip_R", "BipedHand_A_MidBase_R", "BipedHand_A_MidMid_R", "BipedHand_A_MidTip_R", "BipedHand_A_RingBase_R", "BipedHand_A_RingMid_R", "BipedHand_A_RingTip_R", "BipedLeg_A_Thigh_L", "BipedLeg_A_Knee_L", "BipedLeg_A_Ankle_L", "BipedLeg_A_Ball_L", "Leaf_Toe_Leaf_L", "BipedLeg_A_Thigh_R", "BipedLeg_A_Knee_R", "BipedLeg_A_Ankle_R", "BipedLeg_A_Ball_R", "Leaf_Toe_Leaf_R", "IK_Foot_Front_Root", "IK_Foot_Front_L", "IK_Foot_Front_R", "IK_Foot_Rear_Root", "IK_Foot_Rear_L", "IK_Foot_Rear_R"};
		static TArray<int32> FNAmphibianParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 11, 12, 13, 14, 15, 16, 7, 18, 7, 20, 21, 22, 23, 24, 25, 23, 27, 28, 23, 30, 31, 7, 33, 34, 35, 36, 37, 38, 36, 40, 41, 36, 43, 44, 1, 46, 47, 48, 49, 1, 51, 52, 53, 54, 0, 56, 56, 0, 59, 59};
		FTLLRN_TemplateHierarchy& FNAmphibian = AddTemplateHierarchy(FNAmphibianName, FNAmphibianBones, FNAmphibianParentIndices);
		FTLLRN_RetargetDefinition& FNAmphibianRetarget = FNAmphibian.AutoRetargetDefinition.RetargetDefinition;
		// core
		FNAmphibianRetarget.RootBone = FName("QuadSpine_A_Pelvis_C");
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("QuadSpine_A_Spine1_C"), FName("QuadSpine_A_Spine3_C"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("BipedHeadNeck_A_NeckBase_C"), FName("BipedHeadNeck_A_NeckMid_C"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("BipedHeadNeck_A_Head_C"), FName("BipedHeadNeck_A_Head_C"));
		// left
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("BipedArm_A_Clavicle_L"), FName("BipedArm_A_Clavicle_L"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("BipedArm_A_Shoulder_L"), FName("BipedArm_A_Wrist_L"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("BipedLeg_A_Thigh_L"), FName("BipedLeg_A_Ball_L"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("BipedHand_A_IndexBase_L"), FName("BipedHand_A_IndexTip_L"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("BipedHand_A_MidBase_L"), FName("BipedHand_A_MidTip_L"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("BipedHand_A_RingBase_L"), FName("BipedHand_A_RingTip_L"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("BipedHand_A_IndexCarpal_L"), FName("BipedHand_A_IndexCarpal_L"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("BipedHand_A_MidCarpal_L"), FName("BipedHand_A_MidCarpal_L"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("BipedHand_A_RingCarpal_L"), FName("BipedHand_A_RingCarpal_L"));
		// right
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("BipedArm_A_Clavicle_R"), FName("BipedArm_A_Clavicle_R"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("BipedArm_A_Shoulder_R"), FName("BipedArm_A_Wrist_R"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("BipedLeg_A_Thigh_R"), FName("BipedLeg_A_Ball_R"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("BipedHand_A_IndexBase_R"), FName("BipedHand_A_IndexTip_R"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("BipedHand_A_MidBase_R"), FName("BipedHand_A_MidTip_R"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("BipedHand_A_RingBase_R"), FName("BipedHand_A_RingTip_R"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("BipedHand_A_IndexCarpal_R"), FName("BipedHand_A_IndexCarpal_R"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("BipedHand_A_MidCarpal_R"), FName("BipedHand_A_MidCarpal_R"));
		FNAmphibianRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("BipedHand_A_RingCarpal_R"), FName("BipedHand_A_RingCarpal_R"));
		// unreal ik bones
		FNAmphibian.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Front_Root", "root");
		FNAmphibian.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Front_L", "BipedArm_A_Wrist_L");
		FNAmphibian.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Front_R", "BipedArm_A_Wrist_R");
		FNAmphibian.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Rear_Root", "root");
		FNAmphibian.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Rear_L", "BipedLeg_A_Ankle_L");
		FNAmphibian.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Rear_R", "BipedLeg_A_Ankle_R");
	}

	// FN Buttercake
	{
		static FName FNButtercakeName = "FN Buttercake";
		static TArray<FName> FNButtercakeBones = {"C_Root_Main_Root_Jnt", "C_QuadSpine_A_Pelvis_Jnt", "C_QuadSpine_A_Spine1_Jnt", "C_QuadSpine_A_Spine2_Jnt", "C_QuadSpine_A_Spine3_Jnt", "C_QuadSpine_A_Chest_Jnt", "L_PawedArm_A_Shoulder_Jnt", "L_PawedArm_A_Elbow_Jnt", "L_PawedArm_A_Wrist_Jnt", "L_PawedArm_A_Ball_Jnt", "L_PawedArm_A_Toe_Jnt", "L_PawedArm_A_ToeTip_Jnt", "C_BipedHeadNeck_A_NeckBase_Jnt", "C_BipedHeadNeck_A_NeckMid_Jnt", "C_BipedHeadNeck_A_Head_Jnt", "R_PawedArm_A_Shoulder_Jnt", "R_PawedArm_A_Elbow_Jnt", "R_PawedArm_A_Wrist_Jnt", "R_PawedArm_A_Ball_Jnt", "R_PawedArm_A_Toe_Jnt", "L_PawedArm_B_Shoulder_Jnt", "L_PawedArm_B_Elbow_Jnt", "L_PawedArm_B_Wrist_Jnt", "L_PawedArm_B_Ball_Jnt", "L_PawedArm_B_Toe_Jnt", "R_PawedArm_B_Shoulder_Jnt", "R_PawedArm_B_Elbow_Jnt", "R_PawedArm_B_Wrist_Jnt", "R_PawedArm_B_Ball_Jnt", "R_PawedArm_B_Toe_Jnt", "C_Tail_A_TailBase_Jnt", "C_Tail_A_Tail1_Jnt", "C_Tail_A_Tail2_Jnt", "C_Tail_A_Tail3_Jnt", "C_Tail_A_Tail4_Jnt", "C_Tail_A_Tail5_Jnt", "C_Tail_A_Tail6_Jnt", "L_PawedLeg_C_Thigh_Jnt", "L_PawedLeg_C_Knee_Jnt", "L_PawedLeg_C_Ankle_Jnt", "L_PawedLeg_C_Ball_Jnt", "L_PawedLeg_C_Toe_Jnt", "R_PawedLeg_C_Thigh_Jnt", "R_PawedLeg_C_Knee_Jnt", "R_PawedLeg_C_Ankle_Jnt", "R_PawedLeg_C_Ball_Jnt", "R_PawedLeg_C_Toe_Jnt"};
		static TArray<int32> FNButtercakeParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 5, 12, 13, 5, 15, 16, 17, 18, 3, 20, 21, 22, 23, 3, 25, 26, 27, 28, 1, 30, 31, 32, 33, 34, 35, 1, 37, 38, 39, 40, 1, 42, 43, 44, 45};
		FTLLRN_TemplateHierarchy& FNButtercake = AddTemplateHierarchy(FNButtercakeName, FNButtercakeBones, FNButtercakeParentIndices);
		FTLLRN_RetargetDefinition& FNButtercakeRetarget = FNButtercake.AutoRetargetDefinition.RetargetDefinition;
		// core
		FNButtercakeRetarget.RootBone = FName("C_QuadSpine_A_Pelvis_Jnt");
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("C_QuadSpine_A_Spine1_Jnt"), FName("C_QuadSpine_A_Chest_Jnt"));
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("C_BipedHeadNeck_A_NeckBase_Jnt"), FName("C_BipedHeadNeck_A_NeckMid_Jnt"));
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("C_BipedHeadNeck_A_Head_Jnt"), FName("C_BipedHeadNeck_A_Head_Jnt"));
		// left
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("L_PawedLeg_C_Thigh_Jnt"), FName("L_PawedLeg_C_Ball_Jnt"), FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLegA, FName("L_PawedArm_B_Shoulder_Jnt"), FName("L_PawedArm_B_Ball_Jnt"), FTLLRN_CharacterizationStandard::LeftFootAIKGoal);
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("L_PawedArm_A_Shoulder_Jnt"), FName("L_PawedArm_A_Wrist_Jnt"), FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		// right
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("R_PawedLeg_C_Thigh_Jnt"), FName("R_PawedLeg_C_Ball_Jnt"), FTLLRN_CharacterizationStandard::RightFootIKGoal);
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLegA, FName("R_PawedArm_B_Shoulder_Jnt"), FName("R_PawedArm_B_Ball_Jnt"), FTLLRN_CharacterizationStandard::RightFootAIKGoal);
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("R_PawedArm_A_Shoulder_Jnt"), FName("R_PawedArm_A_Wrist_Jnt"), FTLLRN_CharacterizationStandard::RightHandIKGoal);
		// tail
		FNButtercakeRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Tail, FName("C_Tail_A_TailBase_Jnt"), FName("C_Tail_A_Tail6_Jnt"));
	}
	
	// FN biped
	{
		static FName FNBipedName = "Fortnite Biped";
		static TArray<FName> FNBipedBones = {"C_Root_Main_Root_Jnt", "C_BipedSpine_Main_Pelvis_Jnt", "C_BipedSpine_Main_Spine1_Jnt", "C_BipedSpine_Main_Spine2_Jnt", "C_BipedSpine_Main_Spine3_Jnt", "C_BipedSpine_Main_Chest_Jnt", "C_BipedHeadNeck_A_NeckBase_Jnt", "C_BipedHeadNeck_A_NeckMid_Jnt", "C_BipedHeadNeck_A_Head_Jnt", "L_BipedArm_A_Clavicle_Jnt", "L_BipedArm_A_Shoulder_Jnt", "L_BipedArm_A_Elbow_Jnt", "L_BipedArm_A_Wrist_Jnt", "L_BipedHand_A_ThumbBase_Jnt", "L_BipedHand_A_ThumbMid_Jnt", "L_BipedHand_A_ThumbTip_Jnt", "L_BipedHand_A_IndexCarpal_Jnt", "L_BipedHand_A_IndexBase_Jnt", "L_BipedHand_A_IndexMid_Jnt", "L_BipedHand_A_IndexTip_Jnt", "L_BipedHand_A_MidCarpal_Jnt", "L_BipedHand_A_MidBase_Jnt", "L_BipedHand_A_MidMid_Jnt", "L_BipedHand_A_MidTip_Jnt", "L_BipedHand_A_RingCarpal_Jnt", "L_BipedHand_A_RingBase_Jnt", "L_BipedHand_A_RingMid_Jnt", "L_BipedHand_A_RingTip_Jnt", "L_BipedHand_A_PinkyCarpal_Jnt", "L_BipedHand_A_PinkyBase_Jnt", "L_BipedHand_A_PinkyMid_Jnt", "L_BipedHand_A_PinkyTip_Jnt", "R_BipedArm_A_Clavicle_Jnt", "R_BipedArm_A_Shoulder_Jnt", "R_BipedArm_A_Elbow_Jnt", "R_BipedArm_A_Wrist_Jnt", "R_BipedHand_A_ThumbBase_Jnt", "R_BipedHand_A_ThumbMid_Jnt", "R_BipedHand_A_ThumbTip_Jnt", "R_BipedHand_A_IndexCarpal_Jnt", "R_BipedHand_A_IndexBase_Jnt", "R_BipedHand_A_IndexMid_Jnt", "R_BipedHand_A_IndexTip_Jnt", "R_BipedHand_A_MidCarpal_Jnt", "R_BipedHand_A_MidBase_Jnt", "R_BipedHand_A_MidMid_Jnt", "R_BipedHand_A_MidTip_Jnt", "R_BipedHand_A_RingCarpal_Jnt", "R_BipedHand_A_RingBase_Jnt", "R_BipedHand_A_RingMid_Jnt", "R_BipedHand_A_RingTip_Jnt", "R_BipedHand_A_PinkyCarpal_Jnt", "R_BipedHand_A_PinkyBase_Jnt", "R_BipedHand_A_PinkyMid_Jnt", "R_BipedHand_A_PinkyTip_Jnt", "L_BipedLeg_A_Thigh_Jnt", "L_BipedLeg_A_Knee_Jnt", "L_BipedLeg_A_Ankle_Jnt", "L_BipedLeg_A_Ball_Jnt", "R_BipedLeg_A_Thigh_Jnt", "R_BipedLeg_A_Knee_Jnt", "R_BipedLeg_A_Ankle_Jnt", "R_BipedLeg_A_Ball_Jnt", "IK_Foot_Root", "IK_Foot_L", "IK_Foot_R", "IK_Foot_Prediction_L", "IK_Foot_Prediciton_R", "IK_Hand_Root", "IK_Hand_L", "IK_Hand_R", "IK_Hand_Prediction_L", "IK_Hand_Prediciton_R"};
		static TArray<int32> FNBipedParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 7, 5, 9, 10, 11, 12, 13, 14, 12, 16, 17, 18, 12, 20, 21, 22, 12, 24, 25, 26, 12, 28, 29, 30, 5, 32, 33, 34, 35, 36, 37, 35, 39, 40, 41, 35, 43, 44, 45, 35, 47, 48, 49, 35, 51, 52, 53, 1, 55, 56, 57, 1, 59, 60, 61, 0, 63, 63, 63, 63, 0, 68, 68, 68, 68};
		FTLLRN_TemplateHierarchy& FNBiped = AddTemplateHierarchy(FNBipedName, FNBipedBones, FNBipedParentIndices);
		FTLLRN_RetargetDefinition& FNBipedRetarget = FNBiped.AutoRetargetDefinition.RetargetDefinition;
		// core
		FNBipedRetarget.RootBone = FName("C_BipedSpine_Main_Pelvis_Jnt");
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Root, FName("C_Root_Main_Root_Jnt"), FName("C_Root_Main_Root_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("C_BipedSpine_Main_Spine1_Jnt"), FName("C_BipedSpine_Main_Spine3_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("C_BipedHeadNeck_A_NeckBase_Jnt"), FName("C_BipedHeadNeck_A_NeckMid_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("C_BipedHeadNeck_A_Head_Jnt"), FName("C_BipedHeadNeck_A_Head_Jnt"));
		// left
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("L_BipedLeg_A_Thigh_Jnt"), FName("L_BipedLeg_A_Ball_Jnt"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("L_BipedArm_A_Clavicle_Jnt"), FName("L_BipedArm_A_Clavicle_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("L_BipedArm_A_Shoulder_Jnt"), FName("L_BipedArm_A_Wrist_Jnt"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("L_BipedHand_A_ThumbBase_Jnt"), FName("L_BipedHand_A_ThumbTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("L_BipedHand_A_IndexBase_Jnt"), FName("L_BipedHand_A_IndexTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("L_BipedHand_A_MidBase_Jnt"), FName("L_BipedHand_A_MidTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("L_BipedHand_A_RingBase_Jnt"), FName("L_BipedHand_A_RingTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("L_BipedHand_A_PinkyBase_Jnt"), FName("L_BipedHand_A_PinkyTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("L_BipedHand_A_IndexCarpal_Jnt"), FName("L_BipedHand_A_IndexCarpal_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("L_BipedHand_A_MidCarpal_Jnt"), FName("L_BipedHand_A_MidCarpal_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("L_BipedHand_A_RingCarpal_Jnt"), FName("L_BipedHand_A_RingCarpal_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("L_BipedHand_A_PinkyCarpal_Jnt"), FName("L_BipedHand_A_PinkyCarpal_Jnt"));
		// right
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("R_BipedLeg_A_Thigh_Jnt"), FName("R_BipedLeg_A_Ball_Jnt"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("R_BipedArm_A_Clavicle_Jnt"), FName("R_BipedArm_A_Clavicle_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("R_BipedArm_A_Shoulder_Jnt"), FName("R_BipedArm_A_Wrist_Jnt"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("R_BipedHand_A_ThumbBase_Jnt"), FName("R_BipedHand_A_ThumbTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("R_BipedHand_A_IndexBase_Jnt"), FName("R_BipedHand_A_IndexTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("R_BipedHand_A_MidBase_Jnt"), FName("R_BipedHand_A_MidTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("R_BipedHand_A_RingBase_Jnt"), FName("R_BipedHand_A_RingTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("R_BipedHand_A_PinkyBase_Jnt"), FName("R_BipedHand_A_PinkyTip_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("R_BipedHand_A_IndexCarpal_Jnt"), FName("R_BipedHand_A_IndexCarpal_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("R_BipedHand_A_MidCarpal_Jnt"), FName("R_BipedHand_A_MidCarpal_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("R_BipedHand_A_RingCarpal_Jnt"), FName("R_BipedHand_A_RingCarpal_Jnt"));
		FNBipedRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("R_BipedHand_A_PinkyCarpal_Jnt"), FName("R_BipedHand_A_PinkyCarpal_Jnt"));
		// bone settings for IK
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("C_BipedSpine_Main_Pelvis_Jnt"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("L_BipedArm_A_Clavicle_Jnt"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("R_BipedArm_A_Clavicle_Jnt"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("L_BipedLeg_A_Ball_Jnt"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("R_BipedLeg_A_Ball_Jnt"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("L_BipedLeg_A_Knee_Jnt"), ETLLRN_PreferredAxis::NegativeZ);
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("R_BipedLeg_A_Knee_Jnt"), ETLLRN_PreferredAxis::NegativeZ);
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("L_BipedArm_A_Elbow_Jnt"), ETLLRN_PreferredAxis::PositiveZ);
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("R_BipedArm_A_Elbow_Jnt"), ETLLRN_PreferredAxis::PositiveZ);
		FNBiped.AutoRetargetDefinition.BoneSettingsForIK.SetExcluded(FName("C_BipedSpine_Main_Chest_Jnt"), true);
		// unreal ik bones
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Front_Root", "root");
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_L", "L_BipedLeg_A_Ankle_Jnt");
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_R", "R_BipedLeg_A_Ankle_Jnt");
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Prediciton_L", "L_BipedLeg_A_Ankle_Jnt");
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Foot_Prediciton_R", "R_BipedLeg_A_Ankle_Jnt");
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Hand_Root", "root");
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Hand_L", "L_BipedArm_A_Wrist_Jnt");
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Hand_R", "R_BipedArm_A_Wrist_Jnt");
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Hand_Prediciton_L", "L_BipedArm_A_Wrist_Jnt");
		FNBiped.AutoRetargetDefinition.BonesToPin.AddBoneToPin("IK_Hand_Prediciton_R", "R_BipedArm_A_Wrist_Jnt");
		// exclude feet from auto-pose
		FNBiped.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("L_BipedLeg_A_Ankle_Jnt");
		FNBiped.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("R_BipedLeg_A_Ankle_Jnt");
	}

	// Rokoko v2
	{
		static FName RokokoName = "Rokoko";
		static TArray<FName> RokokoBones = {"Root", "Hips", "Spine1", "Spine2", "Spine3", "Spine4", "Neck", "Head", "RightShoulder", "RightArm", "RightForeArm", "RightHand", "RightFinger1Metacarpal", "RightFinger1Proximal", "RightFinger1Distal", "RightFinger5Metacarpal", "RightFinger5Proximal", "RightFinger5Medial", "RightFinger5Distal", "RightFinger4Metacarpal", "RightFinger4Proximal", "RightFinger4Medial", "RightFinger4Distal", "RightFinger3Metacarpal", "RightFinger3Proximal", "RightFinger3Medial", "RightFinger3Distal", "RightFinger2Metacarpal", "RightFinger2Proximal", "RightFinger2Medial", "RightFinger2Distal", "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand", "LeftFinger5Metacarpal", "LeftFinger5Proximal", "LeftFinger5Medial", "LeftFinger5Distal", "LeftFinger4Metacarpal", "LeftFinger4Proximal", "LeftFinger4Medial", "LeftFinger4Distal", "LeftFinger3Metacarpal", "LeftFinger3Proximal", "LeftFinger3Medial", "LeftFinger3Distal", "LeftFinger2Metacarpal", "LeftFinger2Proximal", "LeftFinger2Medial", "LeftFinger2Distal", "LeftFinger1Metacarpal", "LeftFinger1Proximal", "LeftFinger1Distal", "RightThigh", "RightShin", "RightFoot", "RightToe", "LeftThigh", "LeftShin", "LeftFoot", "LeftToe"};
		static TArray<int32> RokokoParentIndices = {-1, 0, 1, 2, 3, 4, 5, 6, 5, 8, 9, 10, 11, 12, 13, 11, 15, 16, 17, 11, 19, 20, 21, 11, 23, 24, 25, 11, 27, 28, 29, 5, 31, 32, 33, 34, 35, 36, 37, 34, 39, 40, 41, 34, 43, 44, 45, 34, 47, 48, 49, 34, 51, 52, 1, 54, 55, 56, 1, 58, 59, 60};
		FTLLRN_TemplateHierarchy& Rokoko = AddTemplateHierarchy(RokokoName, RokokoBones, RokokoParentIndices);
		FTLLRN_RetargetDefinition& RokokoRetarget = Rokoko.AutoRetargetDefinition.RetargetDefinition;
		// core
		RokokoRetarget.RootBone = FName("Hips");
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Spine, FName("Spine1"), FName("Spine4"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Neck, FName("Neck"), FName("Neck"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::Head, FName("Head"), FName("Head"));
		// left
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftLeg, FName("LeftThigh"), FName("LeftToe"),FTLLRN_CharacterizationStandard::LeftFootIKGoal);
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftClavicle, FName("LeftShoulder"), FName("LeftShoulder"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftArm, FName("LeftArm"), FName("LeftHand"),FTLLRN_CharacterizationStandard::LeftHandIKGoal);
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftThumb, FName("LeftFinger1Metacarpal"), FName("LeftFinger1Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndex, FName("LeftFinger2Proximal"), FName("LeftFinger2Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddle, FName("LeftFinger3Proximal"), FName("LeftFinger3Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRing, FName("LeftFinger4Proximal"), FName("LeftFinger4Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinky, FName("LeftFinger5Proximal"), FName("LeftFinger5Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftIndexMetacarpal, FName("LeftFinger2Metacarpal"), FName("LeftFinger2Metacarpal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftMiddleMetacarpal, FName("LeftFinger3Metacarpal"), FName("LeftFinger3Metacarpal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftRingMetacarpal, FName("LeftFinger4Metacarpal"), FName("LeftFinger4Metacarpal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::LeftPinkyMetacarpal, FName("LeftFinger5Metacarpal"), FName("LeftFinger5Metacarpal"));
		// right
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightLeg, FName("RightThigh"), FName("RightToe"),FTLLRN_CharacterizationStandard::RightFootIKGoal);
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightClavicle, FName("RightShoulder"), FName("RightShoulder"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightArm, FName("RightArm"), FName("RightHand"),FTLLRN_CharacterizationStandard::RightHandIKGoal);
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightThumb, FName("RightFinger1Metacarpal"), FName("RightFinger1Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndex, FName("RightFinger2Proximal"), FName("RightFinger2Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddle, FName("RightFinger3Proximal"), FName("RightFinger3Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRing, FName("RightFinger4Proximal"), FName("RightFinger4Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinky, FName("RightFinger5Proximal"), FName("RightFinger5Distal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightIndexMetacarpal, FName("RightFinger2Metacarpal"), FName("RightFinger2Metacarpal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightMiddleMetacarpal, FName("RightFinger3Metacarpal"), FName("RightFinger3Metacarpal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightRingMetacarpal, FName("RightFinger4Metacarpal"), FName("RightFinger4Metacarpal"));
		RokokoRetarget.AddBoneChain(FTLLRN_CharacterizationStandard::RightPinkyMetacarpal, FName("RightFinger5Metacarpal"), FName("RightFinger5Metacarpal"));
		// bone settings for IK
		Rokoko.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("Hips"), FTLLRN_CharacterizationStandard::PelvisRotationStiffness);
		Rokoko.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Rokoko.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightShoulder"), FTLLRN_CharacterizationStandard::ClavicleRotationStiffness);
		Rokoko.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("LeftHand"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Rokoko.AutoRetargetDefinition.BoneSettingsForIK.SetRotationStiffness(FName("RightHand"), FTLLRN_CharacterizationStandard::FootRotationStiffness);
		Rokoko.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftLeg"), ETLLRN_PreferredAxis::NegativeZ);
		Rokoko.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightLeg"), ETLLRN_PreferredAxis::PositiveZ);
		Rokoko.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("LeftForeArm"), ETLLRN_PreferredAxis::PositiveZ);
		Rokoko.AutoRetargetDefinition.BoneSettingsForIK.SetPreferredAxis(FName("RightForeArm"), ETLLRN_PreferredAxis::NegativeZ);
		// exclude feet from auto-pose
		Rokoko.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("LeftFoot");
		Rokoko.AutoRetargetDefinition.BonesToExcludeFromAutoPose.Add("RightFoot");
	}
}

void FTLLRN_KnownTemplateHierarchies::GetClosestMatchingKnownHierarchy(
	const FTLLRN_AbstractHierarchy& InHierarchy,
	FTLLRN_AutoCharacterizeResults& Results) const
{
	Results.BestNumMatchingBones = 0;
	Results.BestPercentageOfTemplateScore = 0.f;
	Results.BestTemplateName = NAME_None;

	TArray<FName> MissingBones;
	TArray<FName> BonesWithMissingParent;
	for (const FTLLRN_TemplateHierarchy& KnownHierarchy : KnownHierarchies)
	{
		int32 NumMatchingBones = 0;
		float PercentageOfTemplateFound  = 0.f;
		KnownHierarchy.Hierarchy.Compare(InHierarchy, MissingBones, BonesWithMissingParent, NumMatchingBones, PercentageOfTemplateFound);
		if (NumMatchingBones > Results.BestNumMatchingBones)
		{
			Results.BestNumMatchingBones = NumMatchingBones;
			Results.BestPercentageOfTemplateScore = PercentageOfTemplateFound;
			Results.BestTemplateName = KnownHierarchy.Name;
			Results.MissingBones = MissingBones;
			Results.BonesWithMissingParent = BonesWithMissingParent;
		}
	}
}

const FTLLRN_TemplateHierarchy* FTLLRN_KnownTemplateHierarchies::GetKnownHierarchyByName(const FName InName) const
{
	for (const FTLLRN_TemplateHierarchy& KnownHierarchy : KnownHierarchies)
	{
		if (KnownHierarchy.Name == InName)
		{
			return &KnownHierarchy;
		}
	}

	return nullptr;
}

FTLLRN_TemplateHierarchy& FTLLRN_KnownTemplateHierarchies::AddTemplateHierarchy(const FName& Label, const TArray<FName>& BoneNames, const TArray<int32>& ParentIndices)
{
	const int32 NewTemplateIndex = KnownHierarchies.Emplace(Label, FTLLRN_AbstractHierarchy(BoneNames, ParentIndices));
	return KnownHierarchies[NewTemplateIndex];
}

void FTLLRN_AutoCharacterizer::GenerateRetargetDefinitionFromMesh(USkeletalMesh* Mesh, FTLLRN_AutoCharacterizeResults& Results) const
{
	// reset
	Results = FTLLRN_AutoCharacterizeResults();
	
	if (!Mesh)
	{
		return;
	}
	
	// first check to see if there are any known popular skeleton formats that closely match the input mesh
	const FTLLRN_AbstractHierarchy TargetAbstractHierarchy(Mesh);
	KnownHierarchies.GetClosestMatchingKnownHierarchy(TargetAbstractHierarchy, Results);

	// if we found a known hierarchy with high enough certainty, then use it!
	constexpr float MinScoreThreshold = 0.1f;
	if (Results.BestTemplateName != NAME_None && Results.BestPercentageOfTemplateScore > MinScoreThreshold)
	{
		// adapt the retarget definition from the best matching template to the target hierarchy
		const FTLLRN_TemplateHierarchy* ClosestTemplateHierarchy = KnownHierarchies.GetKnownHierarchyByName(Results.BestTemplateName);
		check(ClosestTemplateHierarchy);
		AdaptTemplateToHierarchy(*ClosestTemplateHierarchy, TargetAbstractHierarchy, Results);
		// record that a template was indeed used
		Results.bUsedTemplate = true;
		return;
	}

	// TODO
	// skeleton does not match closely enough to any known skeleton,
	// so we fall back on analyzing the skeleton and procedurally generating a retarget definition
	Results.bUsedTemplate = false;
}

const FTLLRN_TemplateHierarchy* FTLLRN_AutoCharacterizer::GetKnownTemplateHierarchy(const FName& TemplateName) const
{
	return KnownHierarchies.GetKnownHierarchyByName(TemplateName);
}

void FTLLRN_AutoCharacterizer::AdaptTemplateToHierarchy(
	const FTLLRN_TemplateHierarchy& Template,
	const FTLLRN_AbstractHierarchy& TargetHierarchy,
	FTLLRN_AutoCharacterizeResults& Results) const
{
	// copy the template into the results
	Results.AutoRetargetDefinition = Template.AutoRetargetDefinition;

	// reset the retarget definition, we're about to adapt it to the given hierarchy
	Results.AutoRetargetDefinition.RetargetDefinition = FTLLRN_RetargetDefinition();

	// adapt the retarget root bone
	const int32 RetargetRootIndex = TargetHierarchy.GetBoneIndex(Template.AutoRetargetDefinition.RetargetDefinition.RootBone, ETLLRN_CleanOrFullName::Clean);
	if (RetargetRootIndex != INDEX_NONE)
	{
		// convert to the full, non-clean name to use for the actual bone chain
		const FName RootBoneName =  TargetHierarchy.GetBoneName(RetargetRootIndex, ETLLRN_CleanOrFullName::Full);
		Results.AutoRetargetDefinition.RetargetDefinition.RootBone = RootBoneName;
	}

	// adapt the bone chains by shrinking from start and end until a valid chain is found on the target hierarchy
	for (const FTLLRN_BoneChain& BoneChain : Template.AutoRetargetDefinition.RetargetDefinition.BoneChains)
	{
		// get the bones that are expected to be in this chain according to the template (ordered root to leaf)
		TArray<FName> BonesInChainInTemplate;
		Template.Hierarchy.GetBonesInChain(
			BoneChain.StartBone.BoneName,
			BoneChain.EndBone.BoneName,
			ETLLRN_CleanOrFullName::Clean,
			BonesInChainInTemplate);

		// find closest start bone in target hierarchy
		int32 StartBoneIndexInTarget = INDEX_NONE;
		for (const FName& BoneInChain : BonesInChainInTemplate)
		{
			const int32 BoneInTargetIndex = TargetHierarchy.GetBoneIndex(BoneInChain, ETLLRN_CleanOrFullName::Clean);
			if (BoneInTargetIndex != INDEX_NONE)
			{
				StartBoneIndexInTarget = BoneInTargetIndex;
				break;
			}
		}

		// find closest end bone in target hierarchy
		int32 EndBoneIndexInTarget = INDEX_NONE;
		for (int32 ChainIndex=BonesInChainInTemplate.Num()-1; ChainIndex>=0; --ChainIndex)
		{
			const FName& BoneInChain = BonesInChainInTemplate[ChainIndex];
			const int32 BoneInTargetIndex = TargetHierarchy.GetBoneIndex(BoneInChain, ETLLRN_CleanOrFullName::Clean);
			if (BoneInTargetIndex != INDEX_NONE)
			{
				EndBoneIndexInTarget = BoneInTargetIndex;
				break;
			}
		}

		// couldn't find both a start AND end bone in the target hierarchy
		if (StartBoneIndexInTarget == INDEX_NONE || EndBoneIndexInTarget == INDEX_NONE)
		{
			continue;
		}

		// chain must be either a single bone, OR end must be a child of start
		const FName StartBoneFullName = TargetHierarchy.GetBoneName(StartBoneIndexInTarget, ETLLRN_CleanOrFullName::Full);
		const FName EndBoneFullName = TargetHierarchy.GetBoneName(EndBoneIndexInTarget, ETLLRN_CleanOrFullName::Full);
		const bool bIsEndChildOfStart = TargetHierarchy.IsChildOf(StartBoneFullName, EndBoneFullName, ETLLRN_CleanOrFullName::Full);
		if (StartBoneIndexInTarget == EndBoneIndexInTarget || bIsEndChildOfStart)
		{
			// convert to the full, non-clean name to use for the actual bone chain
			Results.AutoRetargetDefinition.RetargetDefinition.AddBoneChain(BoneChain.ChainName, StartBoneFullName, EndBoneFullName, BoneChain.IKGoalName);
			continue;
		}
	}
	
	// since the templates usually only account for 1-3 neck/spine bones, but some target skeletons have more,
	// we need to grow these chains or risk leaving some of the bones out of the retarget chain
	TArray<FName> ChainsToExpand = {FTLLRN_CharacterizationStandard::Spine, FTLLRN_CharacterizationStandard::Neck, FTLLRN_CharacterizationStandard::Tail};
	for (const FName ChainName : ChainsToExpand)
	{
		// grow the chain to include "N" bones
		FTLLRN_BoneChain* ChainToExpand = Results.AutoRetargetDefinition.RetargetDefinition.GetEditableBoneChainByName(ChainName);
		if (int32 NumExpanded = ExpandChain(ChainToExpand, TargetHierarchy))
		{
			Results.ExpandedChains.Add(ChainName, NumExpanded);
		}
	}
}

int32 FTLLRN_AutoCharacterizer::ExpandChain(
	FTLLRN_BoneChain* ChainToExpand, 
	const FTLLRN_AbstractHierarchy& Hierarchy) const
{
	if (!ChainToExpand)
	{
		return 0;
	}
	
	// get all bones in chain
	TArray<FName> FullNamesOfAllBonesInChain;
	Hierarchy.GetBonesInChain(ChainToExpand->StartBone.BoneName, ChainToExpand->EndBone.BoneName, ETLLRN_CleanOrFullName::Full, FullNamesOfAllBonesInChain);

	// convert to clean strings for searching
	TArray<FName> BonesInChain;
	for (const FName& FullName : FullNamesOfAllBonesInChain)
	{
		const int32 BoneIndex = Hierarchy.GetBoneIndex(FullName, ETLLRN_CleanOrFullName::Full);
		BonesInChain.Add(Hierarchy.GetBoneName(BoneIndex, ETLLRN_CleanOrFullName::Clean));
	}

	// keep searching down the hierarchy for more joint that belong to this chain
	FName NewEndBone = BonesInChain.Last();
	int32 NumBonesExpanded = 0;
	while(NewEndBone != NAME_None)
	{
		TArray<FName> Children;
		Hierarchy.GetImmediateChildren(NewEndBone, ETLLRN_CleanOrFullName::Clean, Children);
		FName BestMatchingName;
		float Score;
		FindClosestNameInArray(NewEndBone, Children, BestMatchingName, Score);
		constexpr float MinScoreThreshold = 0.9f;
		if (Score < MinScoreThreshold)
		{
			break;
		}
		NewEndBone = BestMatchingName;
		++NumBonesExpanded;
	}

	// assign the new end bone
	const int32 NewEndIndex = Hierarchy.GetBoneIndex(NewEndBone, ETLLRN_CleanOrFullName::Clean);
	ChainToExpand->EndBone = Hierarchy.GetBoneName(NewEndIndex, ETLLRN_CleanOrFullName::Full);

	return NumBonesExpanded;
}

void FTLLRN_AutoCharacterizer::FindClosestNameInArray(
	const FName& NameToMatch,
	const TArray<FName>& NamesToCheck,
	FName& OutClosestName,
	float& OutBestScore) const
{
	OutClosestName = NAME_None;
	OutBestScore = -1.0f;
	
	if (NamesToCheck.IsEmpty())
	{
		return;
	}
	
	const FString NameToMatchStr = NameToMatch.ToString().ToLower();
	for (const FName BoneName : NamesToCheck)
	{
		FString CurrentNameStr = BoneName.ToString().ToLower();
		float WorstCase = NameToMatchStr.Len() + CurrentNameStr.Len();
		WorstCase = WorstCase < 1.0f ? 1.0f : WorstCase;
		const float Score = 1.0f - (Algo::LevenshteinDistance(NameToMatchStr, CurrentNameStr) / WorstCase);
		if (Score > OutBestScore)
		{
			OutBestScore = Score;
			OutClosestName = BoneName;
		}
	}
}

#undef LOCTEXT_NAMESPACE
