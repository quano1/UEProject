// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimInstanceProxy.h"
#include "AnimNode_TLLRN_ControlRig_ExternalSource.h"
#include "TLLRN_ControlRigLayerInstanceProxy.generated.h"

class UTLLRN_ControlRig;
class UAnimSequencerInstance;

/** Custom internal Input Pose node that handles any AnimInstance */
USTRUCT()
struct FAnimNode_TLLRN_ControlRigInputPose : public FAnimNode_Base
{
	GENERATED_BODY()

	FAnimNode_TLLRN_ControlRigInputPose()
		: InputProxy(nullptr)
		, InputAnimInstance(nullptr)
	{
	}

	/** Input pose, optionally linked dynamically to another graph */
	UPROPERTY()
	FPoseLink InputPose;

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	/** Called by linked instance nodes to dynamically link this to an outer graph */
	void Link(UAnimInstance* InInputInstance, FAnimInstanceProxy* InInputProxy);

	/** Called by linked instance nodes to dynamically unlink this to an outer graph */
	void Unlink();

private:
	/** The proxy to use when getting inputs, set when dynamically linked */
	FAnimInstanceProxy* InputProxy;
	UAnimInstance*		InputAnimInstance;
};

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_ControlRigLayerInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

public:
	FTLLRN_ControlRigLayerInstanceProxy()
		: CurrentRoot(nullptr)
		, CurrentSourceAnimInstance(nullptr)
	{
	}

	FTLLRN_ControlRigLayerInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
		, CurrentRoot(nullptr)
		, CurrentSourceAnimInstance(nullptr)
	{
	}

	virtual ~FTLLRN_ControlRigLayerInstanceProxy();

	// FAnimInstanceProxy interface
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual void CacheBones() override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	virtual void PreEvaluateAnimation(UAnimInstance* InAnimInstance) override;

	/** Anim Instance Source info - created externally and used here */
	void SetSourceAnimInstance(UAnimInstance* SourceAnimInstance, FAnimInstanceProxy* SourceAnimInputProxy);
	UAnimInstance* GetSourceAnimInstance() const { return CurrentSourceAnimInstance; }

	/** TLLRN_ControlRig related support */
	void AddTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID, UTLLRN_ControlRig* InTLLRN_ControlRig);
	void UpdateTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID, float Weight, const FTLLRN_ControlRigIOSettings& InputSettings, bool bExecute);
	void RemoveTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID);
	bool HasTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID);
	void ResetTLLRN_ControlRigTracks();

	/** Sequencer AnimInstance Interface */
	void AddAnimation(int32 SequenceId, UAnimSequenceBase* InAnimSequence);
	void UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InPosition, float Weight, bool bFireNotifies);
	void UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InFromPosition, float InToPosition, float Weight, bool bFireNotifies);
	void RemoveAnimation(int32 SequenceId);

	/** Reset all nodes in this instance */
	virtual void ResetNodes();
	/** Reset the pose in this instance*/
	virtual void ResetPose();
	/** Construct and link the base part of the blend tree */
	virtual void ConstructNodes();

	/** return first available control rig from the node it has */
	UTLLRN_ControlRig* GetFirstAvailableTLLRN_ControlRig() const;

	virtual void AddReferencedObjects(UAnimInstance* InAnimInstance, FReferenceCollector& Collector) override;

	// this doesn't work because this instance continuously change root
	// this will invalidate the evaluation
// 	virtual FAnimNode_Base* GetCustomRootNode() 

	friend struct FAnimNode_TLLRN_ControlRigInputPose;
	friend class UTLLRN_ControlRigLayerInstance;
protected:
	/** Sort Control Rig node*/
	void SortTLLRN_ControlRigNodes();

	/** Find TLLRN_ControlRig node of the */
	FAnimNode_TLLRN_ControlRig_ExternalSource* FindTLLRN_ControlRigNode(int32 TLLRN_ControlRigID) const;

	/** Input pose anim node */
	FAnimNode_TLLRN_ControlRigInputPose InputPose;

	/** Cuyrrent Root node - this changes whenever track changes */
	FAnimNode_Base* CurrentRoot;

	/** TLLRN_ControlRig Nodes */
	TArray<TSharedPtr<FAnimNode_TLLRN_ControlRig_ExternalSource>> TLLRN_ControlRigNodes;

	/** mapping from sequencer index to internal player index */
	TMap<int32, FAnimNode_TLLRN_ControlRig_ExternalSource*> SequencerToTLLRN_ControlRigNodeMap;

	/** Source Anim Instance */
	TObjectPtr<UAnimInstance> CurrentSourceAnimInstance;

	/** getter for Sequencer AnimInstance. It will return null if it's using AnimBP */
	UAnimSequencerInstance* GetSequencerAnimInstance();

	static void InitializeCustomProxy(FAnimInstanceProxy* InputProxy, UAnimInstance* InAnimInstance);
	static void GatherCustomProxyDebugData(FAnimInstanceProxy* InputProxy, FNodeDebugData& DebugData);
	static void CacheBonesCustomProxy(FAnimInstanceProxy* InputProxy);
	static void UpdateCustomProxy(FAnimInstanceProxy* InputProxy, const FAnimationUpdateContext& Context);
	static void EvaluateCustomProxy(FAnimInstanceProxy* InputProxy, FPoseContext& Output);
	/** reset internal Counters of given animinstance proxy */
	static void ResetCounter(FAnimInstanceProxy* InAnimInstanceProxy);
};
