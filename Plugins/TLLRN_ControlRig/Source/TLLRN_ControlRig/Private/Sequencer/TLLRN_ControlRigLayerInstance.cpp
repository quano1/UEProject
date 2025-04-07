// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UTLLRN_ControlRigLayerInstance.cpp: Layer AnimInstance that support single Source Anim Instance and multiple control rigs
	The source AnimInstance can be any AnimBlueprint 
=============================================================================*/ 

#include "Sequencer/TLLRN_ControlRigLayerInstance.h"
#include "Sequencer/TLLRN_ControlRigLayerInstanceProxy.h"
#include "TLLRN_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigLayerInstance)

/////////////////////////////////////////////////////
// UTLLRN_ControlRigLayerInstance
/////////////////////////////////////////////////////

const FName UTLLRN_ControlRigLayerInstance::SequencerPoseName(TEXT("Sequencer_Pose_Name"));

UTLLRN_ControlRigLayerInstance::UTLLRN_ControlRigLayerInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseMultiThreadedAnimationUpdate = false;
}

FAnimInstanceProxy* UTLLRN_ControlRigLayerInstance::CreateAnimInstanceProxy()
{
	return new FTLLRN_ControlRigLayerInstanceProxy(this);
}

void UTLLRN_ControlRigLayerInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (UAnimInstance* SourceAnimInstance = GetSourceAnimInstance())
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = GetFirstAvailableTLLRN_ControlRig())
		{
			if (TLLRN_ControlRig->IsAdditive())
			{
				SourceAnimInstance->UpdateAnimation(DeltaSeconds, true, EUpdateAnimationFlag::Default);
			}
		}
	}
}

void UTLLRN_ControlRigLayerInstance::UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InPosition, float Weight, bool bFireNotifies)
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().UpdateAnimTrack(InAnimSequence, SequenceId, InPosition, Weight, bFireNotifies);
}

void UTLLRN_ControlRigLayerInstance::UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InFromPosition, float InToPosition, float Weight, bool bFireNotifies)
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().UpdateAnimTrack(InAnimSequence, SequenceId, InFromPosition, InToPosition, Weight, bFireNotifies);
}

void UTLLRN_ControlRigLayerInstance::ConstructNodes()
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().ConstructNodes();
}

void UTLLRN_ControlRigLayerInstance::ResetNodes()
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().ResetNodes();
}

void UTLLRN_ControlRigLayerInstance::ResetPose()
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().ResetPose();
}

UTLLRN_ControlRig* UTLLRN_ControlRigLayerInstance::GetFirstAvailableTLLRN_ControlRig() const
{
	return GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().GetFirstAvailableTLLRN_ControlRig();
}

/** Anim Instance Source info - created externally and used here */
void UTLLRN_ControlRigLayerInstance::SetSourceAnimInstance(UAnimInstance* SourceAnimInstance)
{
	USkeletalMeshComponent* MeshComponent = GetOwningComponent();
	ensure (MeshComponent->GetAnimInstance() != SourceAnimInstance);

	if (SourceAnimInstance)
	{
		// Add the current animation instance as a linked instance
		FLinkedInstancesAdapter::AddLinkedInstance(MeshComponent, SourceAnimInstance);

		// Direct the control rig instance to the current animation instance to evaluate as its source (input pose)
		GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().SetSourceAnimInstance(SourceAnimInstance, UAnimInstance::GetProxyOnGameThreadStatic<FAnimInstanceProxy>(SourceAnimInstance));
	}
	else
	{
		UAnimInstance* CurrentSourceAnimInstance = GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().GetSourceAnimInstance();		
		// Remove the original instances from the linked instances as it should be reinstated as the main anim instance
		FLinkedInstancesAdapter::RemoveLinkedInstance(MeshComponent, CurrentSourceAnimInstance);

		// Null out the animation instance used to as the input source for the control rig instance
		GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().SetSourceAnimInstance(nullptr, nullptr);
	}
}

/** TLLRN_ControlRig related support */
void UTLLRN_ControlRigLayerInstance::AddTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID, UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if (InTLLRN_ControlRig->IsAdditive())
	{
		if (UAnimInstance* SourceAnimInstance = GetSourceAnimInstance())
		{
			SourceAnimInstance->bUseMultiThreadedAnimationUpdate = false;
		}
	}
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().AddTLLRN_ControlRigTrack(TLLRN_ControlRigID, InTLLRN_ControlRig);
}

bool UTLLRN_ControlRigLayerInstance::HasTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID)
{
	return GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().HasTLLRN_ControlRigTrack(TLLRN_ControlRigID);
}

void UTLLRN_ControlRigLayerInstance::UpdateTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID, float Weight, const FTLLRN_ControlRigIOSettings& InputSettings, bool bExecute)
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().UpdateTLLRN_ControlRigTrack(TLLRN_ControlRigID, Weight, InputSettings, bExecute);
}

void UTLLRN_ControlRigLayerInstance::RemoveTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID)
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().RemoveTLLRN_ControlRigTrack(TLLRN_ControlRigID);
	if (UAnimInstance* SourceAnimInstance = GetSourceAnimInstance())
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = GetFirstAvailableTLLRN_ControlRig())
		{
			SourceAnimInstance->bUseMultiThreadedAnimationUpdate = !TLLRN_ControlRig->IsAdditive();
		}
		else
		{
			SourceAnimInstance->bUseMultiThreadedAnimationUpdate = true;
		}
	}
}

void UTLLRN_ControlRigLayerInstance::ResetTLLRN_ControlRigTracks()
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().ResetTLLRN_ControlRigTracks();
}

/** Sequencer AnimInstance Interface */
void UTLLRN_ControlRigLayerInstance::AddAnimation(int32 SequenceId, UAnimSequenceBase* InAnimSequence)
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().AddAnimation(SequenceId, InAnimSequence);
}

void UTLLRN_ControlRigLayerInstance::RemoveAnimation(int32 SequenceId)
{
	GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().RemoveAnimation(SequenceId);
}

void UTLLRN_ControlRigLayerInstance::SavePose()
{
	if (USkeletalMeshComponent* SkeletalMeshComponent = GetSkelMeshComponent())
	{
		if (SkeletalMeshComponent->GetSkeletalMeshAsset() && SkeletalMeshComponent->GetComponentSpaceTransforms().Num() > 0)
		{
			SavePoseSnapshot(UTLLRN_ControlRigLayerInstance::SequencerPoseName);
		}
	}
}

UAnimInstance* UTLLRN_ControlRigLayerInstance::GetSourceAnimInstance()  
{	
	return GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>().GetSourceAnimInstance();
}

#if WITH_EDITOR
void UTLLRN_ControlRigLayerInstance::HandleObjectsReinstanced(const TMap<UObject*, UObject*>& OldToNewInstanceMap)
{
	Super::HandleObjectsReinstanced(OldToNewInstanceMap);

	static IConsoleVariable* UseLegacyAnimInstanceReinstancingBehavior = IConsoleManager::Get().FindConsoleVariable(TEXT("bp.UseLegacyAnimInstanceReinstancingBehavior"));
	if(UseLegacyAnimInstanceReinstancingBehavior == nullptr || !UseLegacyAnimInstanceReinstancingBehavior->GetBool())
	{
		// Forward to control rig nodes
		FTLLRN_ControlRigLayerInstanceProxy& Proxy = GetProxyOnGameThread<FTLLRN_ControlRigLayerInstanceProxy>();
		for(TSharedPtr<FAnimNode_TLLRN_ControlRig_ExternalSource>& TLLRN_ControlRigNode : Proxy.TLLRN_ControlRigNodes)
		{
			if(TLLRN_ControlRigNode.IsValid())
			{
				TLLRN_ControlRigNode->HandleObjectsReinstanced(OldToNewInstanceMap);
			}
		}
	}
}
#endif

