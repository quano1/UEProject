// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimBulkCurves.h"
#include "Animation/AnimNode_CustomProperty.h"
#include "Rigs/TLLRN_RigHierarchyPoseAdapter.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "AnimNode_TLLRN_ControlRigBase.generated.h"

class UNodeMappingContainer;
class UTLLRN_ControlRig;

/** Struct defining the settings to override when driving a control rig */
USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigIOSettings
{
	GENERATED_BODY()

	FTLLRN_ControlRigIOSettings()
		: bUpdatePose(true)
		, bUpdateCurves(true)
	{}

	static FTLLRN_ControlRigIOSettings MakeEnabled()
	{
		return FTLLRN_ControlRigIOSettings();
	}

	static FTLLRN_ControlRigIOSettings MakeDisabled()
	{
		FTLLRN_ControlRigIOSettings Settings;
		Settings.bUpdatePose = Settings.bUpdateCurves = false;
		return Settings;
	}

	UPROPERTY()
	bool bUpdatePose;

	UPROPERTY()
	bool bUpdateCurves;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigAnimNodeEventName
{
	GENERATED_BODY()

	FTLLRN_ControlRigAnimNodeEventName()
	: EventName(NAME_None)
	{}

	UPROPERTY(EditAnywhere, Category = Links)
	FName EventName;	
};

class TLLRN_CONTROLRIG_API FAnimNode_TLLRN_ControlRig_PoseAdapter : public FTLLRN_RigHierarchyPoseAdapter
{
public:

	FAnimNode_TLLRN_ControlRig_PoseAdapter()
	: bTransferInLocalSpace(true)
	, LastTopologyVersion(UINT32_MAX)
	, bRequiresResetPoseToInitial(true)
	, Hierarchy(nullptr)
	, bEnabled(true)
	{
	}
	
protected:

	virtual void PostLinked(UTLLRN_RigHierarchy* InHierarchy) override;
	virtual void PreUnlinked(UTLLRN_RigHierarchy* InHierarchy) override;

	void ConvertToLocalPose();
	void ConvertToGlobalPose();
	const FTransform& GetLocalTransform(int32 InIndex);
	const FTransform& GetGlobalTransform(int32 InIndex);
	
	void UpdateDirtyStates(const TOptional<bool> InLocalIsPrimary = TOptional<bool>());
	void ComputeDependentTransforms();
	void MarkDependentsDirty();
	
	void UnlinkTransformStorage();

	bool bTransferInLocalSpace;

	TArray<int32> ParentPoseIndices;
	TArray<bool> RequiresHierarchyForSpaceConversion;
	TArray<FTransform> LocalPose;
	TArray<FTransform> GlobalPose;
	TArray<bool> LocalPoseIsDirty;
	TArray<bool> GlobalPoseIsDirty;
	TArray<int32> PoseCurveToHierarchyCurve;
	TArray<bool> HierarchyCurveCopied;
	TMap<FName, int32> HierarchyCurveLookup;
	uint32 LastTopologyVersion;
	bool bRequiresResetPoseToInitial;
	TArray<int32> BonesToResetToInitial;

	TMap<uint16, uint16> ElementIndexToPoseIndex;
	TArray<int32> PoseIndexToElementIndex;

	struct FDependentTransform
	{
		FDependentTransform()
			: KeyAndIndex()
			, TransformType(ETLLRN_RigTransformType::InitialLocal)
			, StorageType(ETLLRN_RigTransformStorageType::Pose)
			, DirtyState(nullptr)
		{}
		
		FDependentTransform(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, ETLLRN_RigTransformType::Type InTransformType, ETLLRN_RigTransformStorageType::Type InStorageType, FTLLRN_RigLocalAndGlobalDirtyState* InDirtyState)
			: KeyAndIndex(InKeyAndIndex)
			, TransformType(InTransformType)
			, StorageType(InStorageType)
			, DirtyState(InDirtyState)
		{}

		FTLLRN_RigElementKeyAndIndex KeyAndIndex;
		ETLLRN_RigTransformType::Type TransformType;
		ETLLRN_RigTransformStorageType::Type StorageType;
		FTLLRN_RigLocalAndGlobalDirtyState* DirtyState;
	};
	TArray<FDependentTransform> Dependents;

	UTLLRN_RigHierarchy* Hierarchy;
	bool bEnabled;;

	friend struct FAnimNode_TLLRN_ControlRigBase;
};

/**
 * Animation node that allows animation TLLRN_ControlRig output to be used in an animation graph
 */
USTRUCT()
struct TLLRN_CONTROLRIG_API FAnimNode_TLLRN_ControlRigBase : public FAnimNode_CustomProperty
{
	GENERATED_BODY()

	FAnimNode_TLLRN_ControlRigBase();

	/* return Control Rig of current object */
	virtual UTLLRN_ControlRig* GetTLLRN_ControlRig() const PURE_VIRTUAL(FAnimNode_TLLRN_ControlRigBase::GetTLLRN_ControlRig, return nullptr; );
	virtual TSubclassOf<UTLLRN_ControlRig> GetTLLRN_ControlRigClass() const PURE_VIRTUAL(FAnimNode_TLLRN_ControlRigBase::GetTLLRN_ControlRigClass, return nullptr; );
	
	// FAnimNode_Base interface
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;

protected:

	void UpdateInputOutputMappingIfRequired(UTLLRN_ControlRig* InTLLRN_ControlRig, const FBoneContainer& RequiredBones); 

	UPROPERTY(EditAnywhere, Category = Links)
	FPoseLink Source;

	/**
	 * If this is checked the rig's pose needs to be reset to its initial
	 * prior to evaluating the rig.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Settings)
	bool bResetInputPoseToInitial;

	/**
	 * If this is checked the bone pose coming from the AnimBP will be
	 * transferred into the Control Rig.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Settings)
	bool bTransferInputPose;

	/**
	 * If this is checked the curves coming from the AnimBP will be
	 * transferred into the Control Rig.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Settings)
	bool bTransferInputCurves;

	/**
	 * Transferring the pose in global space guarantees a global pose match,
	 * while transferring in local space ensures a match of the local transforms.
	 * In general transforms only differ if the hierarchy topology differs
	 * between the Control Rig and the skeleton used in the AnimBP.
	 * Note: Turning this off can potentially improve performance.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Settings)
	bool bTransferPoseInGlobalSpace;

	/**
	 * An inclusive list of bones to transfer as part
	 * of the input pose transfer phase.
	 * If this list is empty all bones will be transferred.
	 */
	UPROPERTY()
	TArray<FBoneReference> InputBonesToTransfer;

	/**
	 * An inclusive list of bones to transfer as part
	 * of the output pose transfer phase.
	 * If this list is empty all bones will be transferred.
	 */
	UPROPERTY()
	TArray<FBoneReference> OutputBonesToTransfer;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Settings)
	TArray<TObjectPtr<UAssetUserData>> AssetUserData;

	TMap<FName, int32> InputToControlIndex;
	
	/** Node Mapping Container */
	UPROPERTY(transient)
	TWeakObjectPtr<UNodeMappingContainer> NodeMappingContainer;

	UPROPERTY(transient)
	FTLLRN_ControlRigIOSettings InputSettings;

	UPROPERTY(transient)
	FTLLRN_ControlRigIOSettings OutputSettings;

	/** Complete mapping from skeleton to control rig bone index */
	TArray<TPair<uint16, uint16>> TLLRN_ControlTLLRN_RigBoneInputMappingByIndex;
	TArray<TPair<uint16, uint16>> TLLRN_ControlTLLRN_RigBoneOutputMappingByIndex;

	/** Complete mapping from skeleton to curve name */
	TArray<TPair<uint16, FName>> TLLRN_ControlTLLRN_RigCurveMappingByIndex;

	/** Rig Hierarchy bone name to required array index mapping */
	TMap<FName, uint16> TLLRN_ControlTLLRN_RigBoneInputMappingByName;
	TMap<FName, uint16> TLLRN_ControlTLLRN_RigBoneOutputMappingByName;

	/** Rig Curve name to Curve mapping */
	TMap<FName, FName> TLLRN_ControlTLLRN_RigCurveMappingByName;
	
	TSharedPtr<FAnimNode_TLLRN_ControlRig_PoseAdapter> PoseAdapter;

	UPROPERTY(transient)
	bool bExecute;

	// The below is alpha value support for control rig
	float InternalBlendAlpha;

	// The customized event queue to run
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Settings)
	TArray<FTLLRN_ControlRigAnimNodeEventName> EventQueue;

	bool bClearEventQueueRequired = false;

	virtual bool CanExecute();
	// update input/output to control rig
	virtual void UpdateInput(UTLLRN_ControlRig* TLLRN_ControlRig, FPoseContext& InOutput);
	virtual void UpdateOutput(UTLLRN_ControlRig* TLLRN_ControlRig, FPoseContext& InOutput);
	virtual UClass* GetTargetClass() const override;
	
	// execute control rig on the input pose and outputs the result
	void ExecuteTLLRN_ControlRig(FPoseContext& InOutput);

	void QueueTLLRN_ControlRigDrawInstructions(UTLLRN_ControlRig* TLLRN_ControlRig, FAnimInstanceProxy* Proxy) const;

	TArray<TObjectPtr<UAssetUserData>> GetAssetUserData() const { return AssetUserData; }
	void UpdateGetAssetUserDataDelegate(UTLLRN_ControlRig* InTLLRN_ControlRig) const;

	bool bTLLRN_ControlRigRequiresInitialization;
	bool bEnablePoseAdapter;
	uint16 LastBonesSerialNumberForCacheBones;
	TWeakObjectPtr<const UAnimInstance> WeakAnimInstanceObject;

	friend struct FTLLRN_ControlRigSequencerAnimInstanceProxy;
	friend struct FTLLRN_ControlRigLayerInstanceProxy;
};
template<>
struct TStructOpsTypeTraits<FAnimNode_TLLRN_ControlRigBase> : public TStructOpsTypeTraitsBase2<FAnimNode_TLLRN_ControlRigBase>
{
	enum
	{
		WithPureVirtual = true,
	};
};
