// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_IKRetargeter.h"
#include "TLLRN_IKRetargetSettings.h"
#include "TLLRN_IKRigLogger.h"
#include "Kismet/KismetMathLibrary.h"

#include "TLLRN_IKRetargetProcessor.generated.h"

class UTLLRN_IKRetargetProcessor;
enum class ETLLRN_RetargetTranslationMode : uint8;
enum class ETLLRN_RetargetRotationMode : uint8;
class UTLLRN_RetargetChainSettings;
class UTLLRN_IKRigDefinition;
class UTLLRN_IKRigProcessor;
struct FReferenceSkeleton;
struct FTLLRN_BoneChain;
struct FTLLRN_IKRetargetPose;
class UObject;
class UTLLRN_IKRetargeter;
class USkeletalMesh;
class UTLLRN_RetargetOpBase;

struct TLLRN_IKRIG_API FTLLRN_RetargetSkeleton
{
	TArray<FName> BoneNames;				// list of all bone names in ref skeleton order
	TArray<int32> ParentIndices;			// per-bone indices of parent bones (the hierarchy)
	TArray<FTransform> RetargetLocalPose;	// local space retarget pose
	TArray<FTransform> RetargetGlobalPose;	// global space retarget pose
	FName RetargetPoseName;					// the name of the retarget pose this was initialized with
	int32 RetargetPoseVersion;				// the version of the retarget pose this was initialized with (transient)
	USkeletalMesh* SkeletalMesh;			// the skeletal mesh this was initialized with
	TArray<FName> ChainThatContainsBone;	// record which chain is actually controlling each bone

	void Initialize(
		USkeletalMesh* InSkeletalMesh,
		const TArray<FTLLRN_BoneChain>& BoneChains,
		const FName InRetargetPoseName,
		const FTLLRN_IKRetargetPose* RetargetPose,
		const FName RetargetRootBone);

	void Reset();

	void GenerateRetargetPose(
		const FName InRetargetPoseName,
		const FTLLRN_IKRetargetPose* InRetargetPose,
		const FName RetargetRootBone);

	int32 FindBoneIndexByName(const FName InName) const;

	int32 GetParentIndex(const int32 BoneIndex) const;

	void UpdateGlobalTransformsBelowBone(
		const int32 StartBoneIndex,
		const TArray<FTransform>& InLocalPose,
		TArray<FTransform>& OutGlobalPose) const;

	void UpdateLocalTransformsBelowBone(
		const int32 StartBoneIndex,
		TArray<FTransform>& OutLocalPose,
		const TArray<FTransform>& InGlobalPose) const;
	
	void UpdateGlobalTransformOfSingleBone(
		const int32 BoneIndex,
		const TArray<FTransform>& InLocalPose,
		TArray<FTransform>& OutGlobalPose) const;
	
	void UpdateLocalTransformOfSingleBone(
		const int32 BoneIndex,
		TArray<FTransform>& OutLocalPose,
		const TArray<FTransform>& InGlobalPose) const;

	FTransform GetGlobalRetargetPoseOfSingleBone(
		const int32 BoneIndex,
		const TArray<FTransform>& InGlobalPose) const;

	int32 GetCachedEndOfBranchIndex(const int32 InBoneIndex) const;

	void GetChildrenIndices(const int32 BoneIndex, TArray<int32>& OutChildren) const;

	void GetChildrenIndicesRecursive(const int32 BoneIndex, TArray<int32>& OutChildren) const;
	
	bool IsParentOfChild(const int32 PotentialParentIndex, const int32 ChildBoneIndex) const;

private:
	
	/** One index per-bone. Lazy-filled on request. Stores the last element of the branch below the bone.
	 * You can iterate between in the indices stored here and the bone in question to iterate over all children recursively */
	mutable TArray<int32> CachedEndOfBranchIndices;
};

struct FTargetSkeleton : public FTLLRN_RetargetSkeleton
{
	TArray<FTransform> OutputGlobalPose;
	// true for bones that are in a target chain that is ALSO mapped to a source chain
	// ie, bones that are actually posed based on a mapped source chain
	TArray<bool> IsBoneRetargeted;

	void Initialize(
		USkeletalMesh* InSkeletalMesh,
		const TArray<FTLLRN_BoneChain>& BoneChains,
		const FName InRetargetPoseName,
		const FTLLRN_IKRetargetPose* RetargetPose,
		const FName RetargetRootBone);

	void Reset();

	void SetBoneIsRetargeted(const int32 BoneIndex, const bool IsRetargeted);

	void UpdateGlobalTransformsAllNonRetargetedBones(TArray<FTransform>& InOutGlobalPose);
};

/** resolving an FTLLRN_BoneChain to an actual skeleton, used to validate compatibility and get all chain indices */
struct TLLRN_IKRIG_API FTLLRN_ResolvedBoneChain
{
	FTLLRN_ResolvedBoneChain(const FTLLRN_BoneChain& BoneChain, const FTLLRN_RetargetSkeleton& Skeleton, TArray<int32> &OutBoneIndices);

	/* Does the START bone exist in the skeleton? */
	bool bFoundStartBone = false;
	/* Does the END bone exist in the skeleton? */
	bool bFoundEndBone = false;
	/* Is the END bone equals or a child of the START bone? */
	bool bEndIsStartOrChildOfStart  = false;

	bool IsValid() const
	{
		return bFoundStartBone && bFoundEndBone && bEndIsStartOrChildOfStart;
	}
};

struct FTLLRN_RootSource
{
	FName BoneName;
	int32 BoneIndex;
	FQuat InitialRotation;
	float InitialHeightInverse;
	FVector InitialPosition;
	FVector CurrentPosition;
	FVector CurrentPositionNormalized;
	FQuat CurrentRotation;
};

struct FTLLRN_RootTarget
{
	FName BoneName;
	int32 BoneIndex;
	FVector InitialPosition;
	FQuat InitialRotation;
	float InitialHeight;

	FVector RootTranslationDelta;
	FQuat RootRotationDelta;
};

struct FTLLRN_RootRetargeter
{	
	FTLLRN_RootSource Source;
	FTLLRN_RootTarget Target;
	FTLLRN_TargetRootSettings Settings;

	void Reset();
	
	bool InitializeSource(
		const FName SourceRootBoneName,
		const FTLLRN_RetargetSkeleton& SourceSkeleton,
		FTLLRN_IKRigLogger& Log);
	
	bool InitializeTarget(
		const FName TargetRootBoneName,
		const FTargetSkeleton& TargetSkeleton,
		FTLLRN_IKRigLogger& Log);

	void EncodePose(const TArray<FTransform> &SourceGlobalPose);
	
	void DecodePose(TArray<FTransform> &OutTargetGlobalPose);

	FVector GetGlobalScaleVector() const
	{
		return GlobalScaleFactor * FVector(Settings.ScaleHorizontal, Settings.ScaleHorizontal, Settings.ScaleVertical);
	}

private:
	FVector GlobalScaleFactor;
};

struct FTLLRN_PoleVectorMatcher
{
	EAxis::Type SourcePoleAxis;
	EAxis::Type TargetPoleAxis;
	float TargetToSourceAngularOffsetAtRefPose;
	TArray<int32> AllChildrenWithinChain;

	bool Initialize(
		const TArray<int32>& SourceIndices,
		const TArray<int32>& TargetIndices,
		const TArray<FTransform> &SourceGlobalPose,
		const TArray<FTransform> &TargetGlobalPose,
		const FTLLRN_RetargetSkeleton& TargetSkeleton);

	void MatchPoleVector(
		const FTLLRN_TargetChainFKSettings& Settings,
		const TArray<int32>& SourceIndices,
		const TArray<int32>& TargetIndices,
		const TArray<FTransform> &SourceGlobalPose,
		TArray<FTransform> &OutTargetGlobalPose,
		FTLLRN_RetargetSkeleton& TargetSkeleton);
	
private:

	EAxis::Type CalculateBestPoleAxisForChain(
		const TArray<int32>& BoneIndices,
		const TArray<FTransform>& GlobalPose);
	
	static FVector CalculatePoleVector(
		const EAxis::Type& PoleAxis,
		const TArray<int32>& BoneIndices,
		const TArray<FTransform>& GlobalPose);

	static EAxis::Type GetMostDifferentAxis(
		const FTransform& Transform,
		const FVector& InNormal);

	static FVector GetChainAxisNormalized(
		const TArray<int32>& BoneIndices,
		const TArray<FTransform>& GlobalPose);
};

struct FTLLRN_ChainFK
{
	TArray<FTransform> InitialGlobalTransforms;
	TArray<FTransform> InitialLocalTransforms;
	TArray<FTransform> CurrentGlobalTransforms;

	TArray<float> Params;
	TArray<int32> BoneIndices;

	int32 ChainParentBoneIndex;
	FTransform ChainParentInitialGlobalTransform;

	bool Initialize(
		const FTLLRN_RetargetSkeleton& Skeleton,
		const TArray<int32>& InBoneIndices,
		const TArray<FTransform> &InitialGlobalPose,
		FTLLRN_IKRigLogger& Log);

	FTransform GetTransformAtParam(const TArray<FTransform>& Transforms, const float& Param) const;

private:
	
	bool CalculateBoneParameters(FTLLRN_IKRigLogger& Log);

protected:

	static void FillTransformsWithLocalSpaceOfChain(
		const FTLLRN_RetargetSkeleton& Skeleton,
		const TArray<FTransform>& InGlobalPose,
		const TArray<int32>& BoneIndices,
		TArray<FTransform>& OutLocalTransforms);

	void PutCurrentTransformsInRefPose(
		const TArray<int32>& BoneIndices,
		const FTLLRN_RetargetSkeleton& Skeleton,
		const TArray<FTransform>& InCurrentGlobalPose);
};

struct FTLLRN_ChainEncoderFK : public FTLLRN_ChainFK
{
	TArray<FTransform> CurrentLocalTransforms;
	FTransform ChainParentCurrentGlobalTransform;
	
	void EncodePose(
		const FTLLRN_RetargetSkeleton& SourceSkeleton,
		const TArray<int32>& SourceBoneIndices,
		const TArray<FTransform> &InSourceGlobalPose);

	void TransformCurrentChainTransforms(const FTransform& NewParentTransform);
};

struct FTLLRN_ChainDecoderFK : public FTLLRN_ChainFK
{
	void InitializeIntermediateParentIndices(
		const int32 RetargetRootBoneIndex,
		const int32 ChainRootBoneIndex,
		const FTargetSkeleton& TargetSkeleton);
	
	void DecodePose(
		const FTLLRN_RootRetargeter& RootRetargeter,
		const FTLLRN_TargetChainFKSettings& Settings,
		const TArray<int32>& TargetBoneIndices,
		FTLLRN_ChainEncoderFK& SourceChain,
		const FTargetSkeleton& TargetSkeleton,
		TArray<FTransform> &InOutGlobalPose);

	void MatchPoleVector(
		const FTLLRN_TargetChainFKSettings& Settings,
		const TArray<int32>& TargetBoneIndices,
		FTLLRN_ChainEncoderFK& SourceChain,
		const FTargetSkeleton& TargetSkeleton,
		TArray<FTransform> &InOutGlobalPose);
	
private:
	
	void UpdateIntermediateParents(
		const FTargetSkeleton& TargetSkeleton,
		TArray<FTransform> &InOutGlobalPose);

	TArray<int32> IntermediateParentIndices;
};

struct FTLLRN_DecodedIKChain
{
	FVector EndEffectorPosition = FVector::ZeroVector;
	FQuat EndEffectorRotation = FQuat::Identity;
	FVector PoleVectorPosition = FVector::ZeroVector;
};

struct FTLLRN_SourceChainIK
{
	int32 StartBoneIndex = INDEX_NONE;
	int32 EndBoneIndex = INDEX_NONE;
	
	FVector InitialEndPosition = FVector::ZeroVector;
	FQuat InitialEndRotation = FQuat::Identity;
	float InvInitialLength = 1.0f;

	// results after encoding...
	FVector PreviousEndPosition = FVector::ZeroVector;
	FVector CurrentEndPosition = FVector::ZeroVector;
	FVector CurrentEndDirectionNormalized = FVector::ZeroVector;
	FQuat CurrentEndRotation = FQuat::Identity;
	float CurrentHeightFromGroundNormalized = 0.0f;
};

struct FTLLRN_TargetChainIK
{
	int32 BoneIndexA = INDEX_NONE;
	int32 BoneIndexC = INDEX_NONE;
	
	float InitialLength = 1.0f;
	FVector InitialEndPosition = FVector::ZeroVector;
	FQuat InitialEndRotation = FQuat::Identity;
	FVector PrevEndPosition = FVector::ZeroVector;
};

struct FTLLRN_ChainDebugData
{
	FTransform InputTransformStart = FTransform::Identity;
	FTransform InputTransformEnd = FTransform::Identity;
	FTransform OutputTransformEnd = FTransform::Identity;
};

struct FTLLRN_ChainRetargeterIK
{
	FTLLRN_SourceChainIK Source;
	FTLLRN_TargetChainIK Target;
	
	FTLLRN_DecodedIKChain Results;

	bool ResetThisTick;
	FVectorSpringState PlantingSpringState;

	#if WITH_EDITOR
	FTLLRN_ChainDebugData DebugData;
	#endif

	bool InitializeSource(
		const TArray<int32>& BoneIndices,
		const TArray<FTransform> &SourceInitialGlobalPose,
		FTLLRN_IKRigLogger& Log);
	
	bool InitializeTarget(
		const TArray<int32>& BoneIndices,
		const TArray<FTransform> &TargetInitialGlobalPose,
		FTLLRN_IKRigLogger& Log);

	void EncodePose(const TArray<FTransform> &SourceInputGlobalPose);
	
	void DecodePose(
		const FTLLRN_TargetChainIKSettings& Settings,
		const FTLLRN_TargetChainSpeedPlantSettings& SpeedPlantSettings,
		const FTLLRN_RootRetargeter& RootRetargeter,
		const TMap<FName, float>& SpeedValuesFromCurves,
		const float DeltaTime,
		const TArray<FTransform>& InGlobalPose);

	void SaveDebugInfo(const TArray<FTransform>& InGlobalPose);
};

struct FTLLRN_RetargetChainPair
{
	TArray<int32> SourceBoneIndices;
	TArray<int32> TargetBoneIndices;
	
	FName SourceBoneChainName;
	FName TargetBoneChainName;

	virtual ~FTLLRN_RetargetChainPair() = default;
	
	virtual bool Initialize(
		const FTLLRN_BoneChain& SourceBoneChain,
		const FTLLRN_BoneChain& TargetBoneChain,
		const FTLLRN_RetargetSkeleton& SourceSkeleton,
		const FTargetSkeleton& TargetSkeleton,
		FTLLRN_IKRigLogger& Log);

private:

	bool ValidateBoneChainWithSkeletalMesh(
		const bool IsSource,
		const FTLLRN_BoneChain& BoneChain,
		const FTLLRN_RetargetSkeleton& RetargetSkeleton,
		FTLLRN_IKRigLogger& Log);
};

struct FTLLRN_RetargetChainPairFK : FTLLRN_RetargetChainPair
{
	FTLLRN_TargetChainFKSettings Settings;
	FTLLRN_ChainEncoderFK FKEncoder;
	FTLLRN_ChainDecoderFK FKDecoder;
	FTLLRN_PoleVectorMatcher PoleVectorMatcher;
	
	virtual bool Initialize(
        const FTLLRN_BoneChain& SourceBoneChain,
        const FTLLRN_BoneChain& TargetBoneChain,
        const FTLLRN_RetargetSkeleton& SourceSkeleton,
        const FTargetSkeleton& TargetSkeleton,
        FTLLRN_IKRigLogger& Log) override;
};

struct FTLLRN_RetargetChainPairIK : FTLLRN_RetargetChainPair
{
	FTLLRN_TargetChainIKSettings Settings;
	FTLLRN_TargetChainSpeedPlantSettings SpeedPlantSettings;
	FTLLRN_ChainRetargeterIK IKChainRetargeter;
	FName IKGoalName;
	FName PoleVectorGoalName;

	virtual bool Initialize(
        const FTLLRN_BoneChain& SourceBoneChain,
        const FTLLRN_BoneChain& TargetBoneChain,
        const FTLLRN_RetargetSkeleton& SourceSkeleton,
        const FTargetSkeleton& TargetSkeleton,
        FTLLRN_IKRigLogger& Log) override;
};

struct FTLLRN_RetargetDebugData
{
	FTransform StrideWarpingFrame;
};

/** The runtime processor that converts an input pose from a source skeleton into an output pose on a target skeleton.
 * To use:
 * 1. Initialize a processor with a Source/Target skeletal mesh and a UTLLRN_IKRetargeter asset.
 * 2. Call RunRetargeter and pass in a source pose as an array of global-space transforms
 * 3. RunRetargeter returns an array of global space transforms for the target skeleton.
 */
UCLASS()
class TLLRN_IKRIG_API UTLLRN_IKRetargetProcessor : public UObject
{
	GENERATED_BODY()

public:

	UTLLRN_IKRetargetProcessor();
	
	/**
	* Initialize the retargeter to enable running it.
	* @param SourceSkeleton - the skeletal mesh to poses FROM
	* @param TargetSkeleton - the skeletal mesh to poses TO
	* @param InRetargeterAsset - the source asset to use for retargeting settings
	* @param bSuppressWarnings - if true, will not output warnings during initialization
	* @warning - Initialization does a lot of validation and can fail for many reasons. Check bIsLoadedAndValid afterwards.
	*/
	void Initialize(
		USkeletalMesh *SourceSkeleton,
		USkeletalMesh *TargetSkeleton,
		UTLLRN_IKRetargeter* InRetargeterAsset,
		const FTLLRN_RetargetProfile& Settings,
		const bool bSuppressWarnings=false);

	/**
	* Run the retarget to generate a new pose.
	* @param InSourceGlobalPose -  is the source mesh input pose in Component/Global space
	* @param SpeedValuesFromCurves - the speed of each curve used by speed planting (blended in anim graph)
	* @param DeltaTime -  time since last tick in seconds (used by speed planting)
	* @param Settings -  the retarget profile to use for this update
	* @return The retargeted Component/Global space pose for the target skeleton
	*/
	TArray<FTransform>& RunRetargeter(
		const TArray<FTransform>& InSourceGlobalPose,
		const TMap<FName, float>& SpeedValuesFromCurves,
		const float DeltaTime,
		const FTLLRN_RetargetProfile& Settings);
	
	/** Apply the settings stored in the IKRig asset. */
	void CopyIKRigSettingsFromAsset();

	/** Does a partial reinitialization (at runtime) whenever the retarget pose is swapped to a different or if the
	 * pose has been modified. Does nothing if the pose has not changed. */
	void UpdateRetargetPoseAtRuntime(const FName NewRetargetPoseName, ETLLRN_RetargetSourceOrTarget SourceOrTarget);

	/** Get read-only access to either source or target skeleton. */
	const FTLLRN_RetargetSkeleton& GetSkeleton(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;

	/** Get name of the root bone of the given skeleton. */
	FName GetRetargetRoot(ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	
	/** Get whether this processor is ready to call RunRetargeter() and generate new poses. */
	bool IsInitialized() const { return bIsInitialized; };

	/** Get whether this processor was initialized with these skeletal meshes and retarget asset*/
	bool WasInitializedWithTheseAssets(
		const TObjectPtr<USkeletalMesh> InSourceMesh,
		const TObjectPtr<USkeletalMesh> InTargetMesh,
		const TObjectPtr<UTLLRN_IKRetargeter> InRetargetAsset);

	/** Get the currently running IK Rig processor for the target */
	UTLLRN_IKRigProcessor* GetTargetTLLRN_IKRigProcessor() const { return TLLRN_IKRigProcessor; };
	
	/** Get read only access to the core IK retarget chains */
	const TArray<FTLLRN_RetargetChainPairIK>& GetIKChainPairs() const { return ChainPairsIK; };
	
	/** Get read only access to the core FK retarget chains */
	const TArray<FTLLRN_RetargetChainPairFK>& GetFKChainPairs() const { return ChainPairsFK; };
	
	/** Get read only access to the core root retargeter */
	const FTLLRN_RootRetargeter& GetRootRetargeter() const { return RootRetargeter; };
	
	// Get read only access to the retarget ops currently running in processor
	const TArray<TObjectPtr<UTLLRN_RetargetOpBase>>& GetRetargetOps() const {return OpStack; };

	// Get read only access to the current global settings
	const FTLLRN_RetargetGlobalSettings& GetGlobalSettings() const {return GlobalSettings; };
	
	/** Reset the IK planting state. */
	void ResetPlanting();

	/** logging system */
	FTLLRN_IKRigLogger Log;

	/** Set that this processor needs to be reinitialized. */
	void SetNeedsInitialized();
	
#if WITH_EDITOR
private:
	DECLARE_MULTICAST_DELEGATE(FOnRetargeterInitialized);
	FOnRetargeterInitialized RetargeterInitialized;
public:
	/** Returns true if the bone is part of a retarget chain or root bone, false otherwise. */
	bool IsBoneRetargeted(const FName BoneName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	/** Returns index of the bone with the given name in either Source or Target skeleton. */
	int32 GetBoneIndexFromName(const FName BoneName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	/** Returns name of the chain associated with this bone. Returns NAME_None if bone is not in a chain. */
	FName GetChainNameForBone(const FName BoneName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	/** Get a transform at a given param in a chain */
	FTransform GetGlobalRetargetPoseAtParam(const FName InChainName, const float Param, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	/** Get access to the internal chain on the given skeleton */
	const FTLLRN_ChainFK* GetChain(const FName InChainName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	/** Get the param of the bone in it's retarget chain. Ranges from 0 to NumBonesInChain. */
	float GetParamOfBoneInChain(const FName InBoneName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	/** Get the bone in the chain at the given param. */
	FName GetBoneAtParam(const FName InChainName, const float InParam, const ETLLRN_RetargetSourceOrTarget SourceOrTarget) const;
	/** Get the chain mapped to this one. */
	FName GetMappedChainName(const FName InChainName, const ETLLRN_RetargetSourceOrTarget SourceOrTarget);
	/** store data for debug drawing */
	FTLLRN_RetargetDebugData DebugData;
	/** Attach a delegate to be notified whenever this processor is initialized. */
	FOnRetargeterInitialized& OnRetargeterInitialized(){ return RetargeterInitialized; };
#endif

private:

	/** Only true once Initialize() has successfully completed.*/
	bool bIsInitialized = false;
	int32 AssetVersionInitializedWith = -2;
	/** true when roots are able to be retargeted */
	bool bRootsInitialized = false;
	/** true when at least one pair of bone chains is able to be retargeted */
	bool bAtLeastOneValidBoneChainPair = false;
	/** true when IK Rig has been initialized and is ready to run*/
	bool bIKRigInitialized = false;

	/** The source asset this processor was initialized with. */
	UTLLRN_IKRetargeter* RetargeterAsset = nullptr;

	/** The internal data structures used to represent the SOURCE skeleton / pose during retargeter.*/
	FTLLRN_RetargetSkeleton SourceSkeleton;

	/** The internal data structures used to represent the TARGET skeleton / pose during retargeter.*/
	FTargetSkeleton TargetSkeleton;

	/** The IK Rig processor for running IK on the target */
	UPROPERTY(Transient) // must be property to keep from being GC'd
	TObjectPtr<UTLLRN_IKRigProcessor> TLLRN_IKRigProcessor = nullptr;

	/** The Source/Target pairs of Bone Chains retargeted using the FK algorithm */
	TArray<FTLLRN_RetargetChainPairFK> ChainPairsFK;

	/** The Source/Target pairs of Bone Chains retargeted using the IK Rig */
	TArray<FTLLRN_RetargetChainPairIK> ChainPairsIK;

	/** The Source/Target pair of Root Bones retargeted with scaled translation */
	FTLLRN_RootRetargeter RootRetargeter;

	/** The currently used global settings (driven either by source asset or a profile) */
	FTLLRN_RetargetGlobalSettings GlobalSettings;

	/** The collection of operations to run in the final phase of retargeting */
	UPROPERTY(Transient) // must be property to keep from being GC'd
	TArray<TObjectPtr<UTLLRN_RetargetOpBase>> OpStack;

	/** Apply the settings stored in a retarget profile. Called inside RunRetargeter(). */
	void ApplySettingsFromProfile(const FTLLRN_RetargetProfile& Profile);
	
	/** Update chain settings at runtime (for use with a profile)
	 * Will queue an IK Rig reinitialization if the "Enable IK" state is modified. */
	void UpdateChainSettingsAtRuntime(const FName ChainName, const FTLLRN_TargetChainSettings& NewChainSettings);
	
	/** Initializes the FTLLRN_RootRetargeter */
	bool InitializeRoots();

	/** Initializes the all the chain pairs */
	bool InitializeBoneChainPairs(const TMap<FName, FTLLRN_TargetChainSettings>& ChainSettings);

	/** Initializes the IK Rig that evaluates the IK solve for the target IK chains */
	bool InitializeIKRig(UObject* Outer, const USkeletalMesh* InSkeletalMesh);

	// Initialize the retarget op stack
	bool InitializeOpStack(const TArray<TObjectPtr<UTLLRN_RetargetOpBase>>& OpStackFromAsset);
	
	/** Internal retarget phase for the root. */
	void RunRootRetarget(const TArray<FTransform>& InGlobalTransforms, TArray<FTransform>& OutGlobalTransforms);

	/** Internal retarget phase for the FK chains. */
	void RunFKRetarget(const TArray<FTransform>& InGlobalTransforms, TArray<FTransform>& OutGlobalTransforms);

	/** Internal retarget phase for the IK chains. */
	void RunIKRetarget(
		const TArray<FTransform>& InSourceGlobalPose,
		TArray<FTransform>& OutTargetGlobalPose,
		const TMap<FName, float>& SpeedValuesFromCurves,
		const float DeltaTime);

	/** Internal retarget phase for the pole matching feature of FK chains. */
	void RunPoleVectorMatching(const TArray<FTransform>& InGlobalTransforms, TArray<FTransform>& OutGlobalTransforms);

	/** Runs in the after the base IK retarget to apply stride warping to IK goals. */
	void RunStrideWarping(const TArray<FTransform>& InTargetGlobalPose);

	/** Run all post process operations on the retargeted result. */
	void RunRetargetOps(const TArray<FTransform>& InSourceGlobalPose, TArray<FTransform>& OutTargetGlobalPose);
};
