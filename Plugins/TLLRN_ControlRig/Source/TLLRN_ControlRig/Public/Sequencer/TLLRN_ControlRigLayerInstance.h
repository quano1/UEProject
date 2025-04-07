// Copyright Epic Games, Inc. All Rights Reserved.

/**
 *
 * This Instance only contains one AnimationAsset, and produce poses
 * Used by Preview in AnimGraph, Playing single animation in Kismet2 and etc
 */

#pragma once
#include "Animation/AnimInstance.h"
#include "SequencerAnimationSupport.h"
#include "AnimNode_TLLRN_ControlRigBase.h"
#include "TLLRN_ControlRigLayerInstance.generated.h"

class UTLLRN_ControlRig;

UCLASS(transient, NotBlueprintable)
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigLayerInstance : public UAnimInstance, public ISequencerAnimationSupport
{
	GENERATED_UCLASS_BODY()

public:
	/** TLLRN_ControlRig related support */
	void AddTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID, UTLLRN_ControlRig* InTLLRN_ControlRig);
	void UpdateTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID, float Weight, const FTLLRN_ControlRigIOSettings& InputSettings, bool bExecute);
	void RemoveTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID);
	bool HasTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID);
	void ResetTLLRN_ControlRigTracks();

	/** Sequencer AnimInstance Interface */
	void AddAnimation(int32 SequenceId, UAnimSequenceBase* InAnimSequence);
	virtual void UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InPosition, float Weight, bool bFireNotifies) override;
	virtual void UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InFromPosition, float InToPosition, float Weight, bool bFireNotifies) override;
	void RemoveAnimation(int32 SequenceId);

	/** Construct all nodes in this instance */
	virtual void ConstructNodes() override;
	/** Reset all nodes in this instance */
	virtual void ResetNodes() override;
	/** Reset the pose in this instance*/
	virtual void ResetPose() override;
	/** Saved the named pose to restore after */
	virtual void SavePose() override;

	/** Return the first available control rig */
	UTLLRN_ControlRig* GetFirstAvailableTLLRN_ControlRig() const;

	virtual UAnimInstance* GetSourceAnimInstance() override;
	virtual void SetSourceAnimInstance(UAnimInstance* SourceAnimInstance) override;
	virtual bool DoesSupportDifferentSourceAnimInstance() const override { return true; }

#if WITH_EDITOR
	virtual void HandleObjectsReinstanced(const TMap<UObject*, UObject*>& OldToNewInstanceMap) override;
#endif

protected:
	// UAnimInstance interface
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	static const FName SequencerPoseName;
};
