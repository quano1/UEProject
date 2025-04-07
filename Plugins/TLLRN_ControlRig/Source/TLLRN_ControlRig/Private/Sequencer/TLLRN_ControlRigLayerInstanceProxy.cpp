// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/TLLRN_ControlRigLayerInstanceProxy.h"
#include "AnimNode_TLLRN_ControlRig_ExternalSource.h"
#include "Sequencer/TLLRN_ControlRigLayerInstance.h"
#include "AnimSequencerInstance.h"
#include "TLLRN_ControlRig.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigLayerInstanceProxy)

void FTLLRN_ControlRigLayerInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	ConstructNodes();

	FAnimInstanceProxy::Initialize(InAnimInstance);

	UpdateCounter.Reset();
}

void FTLLRN_ControlRigLayerInstanceProxy::CacheBones()
{
	if (bBoneCachesInvalidated)
	{
		FAnimationCacheBonesContext Context(this);
		check(CurrentRoot);
		CurrentRoot->CacheBones_AnyThread(Context);

		bBoneCachesInvalidated = false;
	}
}

void FTLLRN_ControlRigLayerInstanceProxy::PreEvaluateAnimation(UAnimInstance* InAnimInstance)
{
	Super::PreEvaluateAnimation(InAnimInstance);

	if (CurrentSourceAnimInstance)
	{
		CurrentSourceAnimInstance->PreEvaluateAnimation();
	}
}

void FTLLRN_ControlRigLayerInstanceProxy::SortTLLRN_ControlRigNodes()
{
	//remove any invalid nodes
	TArray<int32> NodesToRemove;
	for (int32 Index = 0; Index < TLLRN_ControlRigNodes.Num(); ++Index)
	{
		FAnimNode_TLLRN_ControlRig_ExternalSource* CurrentNode = TLLRN_ControlRigNodes[Index].Get();
		if (CurrentNode && CurrentNode->GetTLLRN_ControlRig() == nullptr)
		{
			//need to find the control rig by manually finding it in the sequencer map
			for (TPair<int32, FAnimNode_TLLRN_ControlRig_ExternalSource*>& Pair : SequencerToTLLRN_ControlRigNodeMap)
			{
				if (Pair.Value == CurrentNode)
				{
					NodesToRemove.Add(Pair.Key);
				}
			}
		}
	}
	for (int32 ID : NodesToRemove)
	{
		RemoveTLLRN_ControlRigTrack(ID);
	}

	auto SortPredicate = [](TSharedPtr<FAnimNode_TLLRN_ControlRig_ExternalSource>& A, TSharedPtr<FAnimNode_TLLRN_ControlRig_ExternalSource>& B)
	{
		FAnimNode_TLLRN_ControlRig_ExternalSource* APtr = A.Get();
		FAnimNode_TLLRN_ControlRig_ExternalSource* BPtr = B.Get();
		if (APtr && BPtr && APtr->GetTLLRN_ControlRig() && B->GetTLLRN_ControlRig())
		{
			const bool AIsAdditive = A->GetTLLRN_ControlRig()->IsAdditive();
			const bool BIsAdditive = B->GetTLLRN_ControlRig()->IsAdditive();
			if (AIsAdditive != BIsAdditive)
			{
				return AIsAdditive; // if additive then first(first goes last) so true if AIsAdditive;
			}
			UMovieSceneTLLRN_ControlRigParameterTrack* TrackA = APtr->GetTLLRN_ControlRig()->GetTypedOuter<UMovieSceneTLLRN_ControlRigParameterTrack>();
			UMovieSceneTLLRN_ControlRigParameterTrack* TrackB = BPtr->GetTLLRN_ControlRig()->GetTypedOuter<UMovieSceneTLLRN_ControlRigParameterTrack>();
			if (TrackA && TrackB)
			{
				return TrackA->GetPriorityOrder() < TrackB->GetPriorityOrder();
			}
		}
		else if (APtr && APtr->GetTLLRN_ControlRig())
		{
			return false;
		}
		return true;
	};

	Algo::Sort(TLLRN_ControlRigNodes, SortPredicate);

	ConstructNodes(); //we need to sort since prority may change at any time 
}

bool FTLLRN_ControlRigLayerInstanceProxy::Evaluate(FPoseContext& Output)
{
	SortTLLRN_ControlRigNodes();  //mz todo once we move over to ECS see if we can avoid this and trigger it as needed.
	check(CurrentRoot);
	CurrentRoot->Evaluate_AnyThread(Output);
	return true;
}

void FTLLRN_ControlRigLayerInstanceProxy::UpdateAnimationNode(const FAnimationUpdateContext& InContext)
{
	UpdateCounter.Increment();

	check(CurrentRoot);
	CurrentRoot->Update_AnyThread(InContext);
}

void FTLLRN_ControlRigLayerInstanceProxy::ConstructNodes()
{
	if (TLLRN_ControlRigNodes.Num() > 0)
	{
		FAnimNode_TLLRN_ControlRig_ExternalSource* ParentNode = TLLRN_ControlRigNodes[0].Get();
		CurrentRoot = static_cast<FAnimNode_Base*>(ParentNode);

		// index 0 - (N-1) is the order of operation
		for (int32 Index = 1; Index < TLLRN_ControlRigNodes.Num(); ++Index)
		{
			FAnimNode_TLLRN_ControlRig_ExternalSource* CurrentNode = TLLRN_ControlRigNodes[Index].Get();
			ParentNode->Source.SetLinkNode(CurrentNode);
			ParentNode = CurrentNode;
		}

		// last parent node has to link to input pose
		ParentNode->Source.SetLinkNode(&InputPose);
	}
	else
	{
		CurrentRoot = &InputPose;
	}

// 	if (UAnimSequencerInstance* SeqInstance = GetSequencerAnimInstance())
// 	{
// 		SeqInstance->ConstructNodes();
// 	}
}

void FTLLRN_ControlRigLayerInstanceProxy::ResetPose()
{
	if (UAnimSequencerInstance* SeqInstance = GetSequencerAnimInstance())
	{
		SeqInstance->ResetPose();
	}
}	

void FTLLRN_ControlRigLayerInstanceProxy::ResetNodes()
{
	if (UAnimSequencerInstance* SeqInstance = GetSequencerAnimInstance())
	{
		SeqInstance->ResetNodes();
	}
}

FTLLRN_ControlRigLayerInstanceProxy::~FTLLRN_ControlRigLayerInstanceProxy()
{
}

void FTLLRN_ControlRigLayerInstanceProxy::UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InPosition, float Weight, bool bFireNotifies)
{
	if (UAnimSequencerInstance* SeqInstance = GetSequencerAnimInstance())
	{
		SeqInstance->UpdateAnimTrack(InAnimSequence, SequenceId, InPosition, Weight, bFireNotifies);
	}
}

void FTLLRN_ControlRigLayerInstanceProxy::UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InFromPosition, float InToPosition, float Weight, bool bFireNotifies)
{
	if (UAnimSequencerInstance* SeqInstance = GetSequencerAnimInstance())
	{
		SeqInstance->UpdateAnimTrack(InAnimSequence, SequenceId, InFromPosition, InToPosition, Weight, bFireNotifies);
	}
}

FAnimNode_TLLRN_ControlRig_ExternalSource* FTLLRN_ControlRigLayerInstanceProxy::FindTLLRN_ControlRigNode(int32 TLLRN_ControlRigID) const
{
	return SequencerToTLLRN_ControlRigNodeMap.FindRef(TLLRN_ControlRigID);
}


/** Anim Instance Source info - created externally and used here */
void FTLLRN_ControlRigLayerInstanceProxy::SetSourceAnimInstance(UAnimInstance* SourceAnimInstance, FAnimInstanceProxy* SourceAnimInputProxy)
{
	CurrentSourceAnimInstance = SourceAnimInstance;
	InputPose.Unlink();

	if (CurrentSourceAnimInstance)
	{
		InputPose.Link(CurrentSourceAnimInstance, SourceAnimInputProxy);
	}
}

/** TLLRN_ControlRig related support */
void FTLLRN_ControlRigLayerInstanceProxy::AddTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID, UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	FAnimNode_TLLRN_ControlRig_ExternalSource* Node = FindTLLRN_ControlRigNode(TLLRN_ControlRigID);
	const int32 DefaultPriorityOrder = 100;
	if(!Node)
	{
		UMovieSceneTLLRN_ControlRigParameterTrack* Track = InTLLRN_ControlRig->GetTypedOuter<UMovieSceneTLLRN_ControlRigParameterTrack>();

		if (Track)
		{
			int32 PriorityOrder = Track->GetPriorityOrder();
			if (PriorityOrder == INDEX_NONE) //track has no order so need to find a number such that it runs at the end, will happen on creation
			{
				PriorityOrder = DefaultPriorityOrder;
				
				SortTLLRN_ControlRigNodes();
			
				// find the first node with the same type and make sure we set a even lower priority
				for (int32 Index = 0; Index < TLLRN_ControlRigNodes.Num(); ++Index)
				{
					FAnimNode_TLLRN_ControlRig_ExternalSource* NodeToCheck = TLLRN_ControlRigNodes[Index].Get();
				
					if (!NodeToCheck ||
						!NodeToCheck->GetTLLRN_ControlRig() ||
						NodeToCheck->GetTLLRN_ControlRig()->IsAdditive() != InTLLRN_ControlRig->IsAdditive())
					{
						continue;
					}
				
					if (UMovieSceneTLLRN_ControlRigParameterTrack* OtherTrack = TLLRN_ControlRigNodes[0]->GetTLLRN_ControlRig()->GetTypedOuter<UMovieSceneTLLRN_ControlRigParameterTrack>())
					{
						PriorityOrder = OtherTrack->GetPriorityOrder() - 1;
					}
					else
					{
						// Best guess of the lowest priority
						PriorityOrder = DefaultPriorityOrder - TLLRN_ControlRigNodes.Num();
					}
				}

				Track->SetPriorityOrder(PriorityOrder);
			}	
		}
		

		// Simply add to the end, we will sort again
		Node = TLLRN_ControlRigNodes.Add_GetRef(MakeShared<FAnimNode_TLLRN_ControlRig_ExternalSource>()).Get();
		SortTLLRN_ControlRigNodes();
		
		//this will set up the link nodes
		ConstructNodes();
		SequencerToTLLRN_ControlRigNodeMap.FindOrAdd(TLLRN_ControlRigID) = Node;
	}

	Node->SetTLLRN_ControlRig(InTLLRN_ControlRig);
	Node->OnInitializeAnimInstance(this, CastChecked<UAnimInstance>(GetAnimInstanceObject()));
	//mz removed this due to crash since Skeleton is not set up on a previous linked node 
	// see FORT-630426
	//but leaving in case it's needed for something else in which case need
	//to call AnimInstance::UpdateAnimation(via TickAnimation perhaps
	// Also, this initialize will remove any source animations that additive control rigs might depend on
	//Node->Initialize_AnyThread(FAnimationInitializeContext(this));
}

bool FTLLRN_ControlRigLayerInstanceProxy::HasTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID)
{
	FAnimNode_TLLRN_ControlRig_ExternalSource* Node = FindTLLRN_ControlRigNode(TLLRN_ControlRigID);
	return (Node != nullptr) ? true : false;
}

void FTLLRN_ControlRigLayerInstanceProxy::ResetTLLRN_ControlRigTracks()
{
	SequencerToTLLRN_ControlRigNodeMap.Reset();
	TLLRN_ControlRigNodes.Reset();

}
void FTLLRN_ControlRigLayerInstanceProxy::UpdateTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID, float Weight, const FTLLRN_ControlRigIOSettings& InputSettings, bool bExecute)
{
	if (FAnimNode_TLLRN_ControlRig_ExternalSource* Node = FindTLLRN_ControlRigNode(TLLRN_ControlRigID))
	{
		Node->InternalBlendAlpha = FMath::Clamp(Weight, 0.f, 1.f);
		Node->InputSettings = InputSettings;
		Node->bExecute = bExecute;
	}
}

void FTLLRN_ControlRigLayerInstanceProxy::RemoveTLLRN_ControlRigTrack(int32 TLLRN_ControlRigID)
{
	if (FAnimNode_TLLRN_ControlRig_ExternalSource* Node = FindTLLRN_ControlRigNode(TLLRN_ControlRigID))
	{
		FAnimNode_TLLRN_ControlRig_ExternalSource* Parent = nullptr;

		// "TLLRN_ControlRigNodes" should have nodes sorted from parent(last to evaluate) to child(first to evaluate)
		for (int32 Index = 0; Index < TLLRN_ControlRigNodes.Num(); ++Index)
		{
			FAnimNode_TLLRN_ControlRig_ExternalSource* Current = TLLRN_ControlRigNodes[Index].Get();

			if (Current == Node)
			{
				// we need to delete this one
				// find next child one
				FAnimNode_TLLRN_ControlRig_ExternalSource* Child = (TLLRN_ControlRigNodes.IsValidIndex(Index + 1)) ? TLLRN_ControlRigNodes[Index + 1].Get() : nullptr;

				if (Parent)
				{
					if (Child)
					{
						Parent->Source.SetLinkNode(Child);
					}
					else
					{
						Parent->Source.SetLinkNode(&InputPose);
					}
				}

				TLLRN_ControlRigNodes.RemoveAt(Index);
				break;
			}

			Parent = Current;
		}

		if (TLLRN_ControlRigNodes.IsEmpty())
		{
			CurrentRoot = &InputPose;
		}
		else
		{
			// stay consistent with ConstructNodes()
			CurrentRoot = TLLRN_ControlRigNodes[0].Get();
		}
		SequencerToTLLRN_ControlRigNodeMap.Remove(TLLRN_ControlRigID);
	}
}

/** Sequencer AnimInstance Interface - don't need these right now */
void FTLLRN_ControlRigLayerInstanceProxy::AddAnimation(int32 SequenceId, UAnimSequenceBase* InAnimSequence)
{

}

void FTLLRN_ControlRigLayerInstanceProxy::RemoveAnimation(int32 SequenceId)
{

}

UAnimSequencerInstance* FTLLRN_ControlRigLayerInstanceProxy::GetSequencerAnimInstance()
{
	return Cast<UAnimSequencerInstance>(CurrentSourceAnimInstance);
}

UTLLRN_ControlRig* FTLLRN_ControlRigLayerInstanceProxy::GetFirstAvailableTLLRN_ControlRig() const
{
	for (const TSharedPtr<FAnimNode_TLLRN_ControlRig_ExternalSource>& TLLRN_ControlRigNode : TLLRN_ControlRigNodes)
	{
		if (UTLLRN_ControlRig* Rig = TLLRN_ControlRigNode->GetTLLRN_ControlRig())
		{
			return Rig;
		}
	}

	return nullptr;
}

void FTLLRN_ControlRigLayerInstanceProxy::AddReferencedObjects(UAnimInstance* InAnimInstance, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InAnimInstance, Collector);

	if (CurrentSourceAnimInstance)
	{
		Collector.AddReferencedObject(CurrentSourceAnimInstance);
	}

	for (TSharedPtr<FAnimNode_TLLRN_ControlRig_ExternalSource>& Node : TLLRN_ControlRigNodes)
	{
#if WITH_EDITORONLY_DATA
		Collector.AddReferencedObject(Node->SourceInstance);
#endif
		Collector.AddReferencedObject(Node->TargetInstance);
	}
}

void FTLLRN_ControlRigLayerInstanceProxy::InitializeCustomProxy(FAnimInstanceProxy* InputProxy, UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::InitializeInputProxy(InputProxy, InAnimInstance);
}

void FTLLRN_ControlRigLayerInstanceProxy::GatherCustomProxyDebugData(FAnimInstanceProxy* InputProxy, FNodeDebugData& DebugData)
{
	FAnimInstanceProxy::GatherInputProxyDebugData(InputProxy, DebugData);
}

void FTLLRN_ControlRigLayerInstanceProxy::CacheBonesCustomProxy(FAnimInstanceProxy* InputProxy)
{
	FAnimInstanceProxy::CacheBonesInputProxy(InputProxy);
}

void FTLLRN_ControlRigLayerInstanceProxy::UpdateCustomProxy(FAnimInstanceProxy* InputProxy, const FAnimationUpdateContext& Context)
{
	FAnimInstanceProxy::UpdateInputProxy(InputProxy, Context);
}

void FTLLRN_ControlRigLayerInstanceProxy::EvaluateCustomProxy(FAnimInstanceProxy* InputProxy, FPoseContext& Output)
{
	FAnimInstanceProxy::EvaluateInputProxy(InputProxy, Output);
}

void FTLLRN_ControlRigLayerInstanceProxy::ResetCounter(FAnimInstanceProxy* InAnimInstanceProxy)
{
	FAnimInstanceProxy::ResetCounterInputProxy(InAnimInstanceProxy);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void FAnimNode_TLLRN_ControlRigInputPose::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	if (InputProxy)
	{
		FAnimationInitializeContext InputContext(InputProxy);
		if (InputPose.GetLinkNode())
		{
			InputPose.Initialize(InputContext);
		}
 		else 
 		{
			FTLLRN_ControlRigLayerInstanceProxy::InitializeCustomProxy(InputProxy, InputAnimInstance);
 		}
	}
}

void FAnimNode_TLLRN_ControlRigInputPose::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	if (InputProxy)
	{
		FAnimationCacheBonesContext InputContext(InputProxy);
		if (InputPose.GetLinkNode())
		{
			InputPose.CacheBones(InputContext);
		}
		else
		{
			FTLLRN_ControlRigLayerInstanceProxy::CacheBonesCustomProxy(InputProxy);
		}
	}
}

void FAnimNode_TLLRN_ControlRigInputPose::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	// if (InputProxy)
	// {
	// 	FAnimationUpdateContext InputContext = Context.WithOtherProxy(InputProxy);
	// 	if (FAnimNode_Base* InputNode = InputPose.GetLinkNode())
	// 	{
	// 		InputProxy->UpdateAnimation_WithRoot(InputContext, InputNode, TEXT("AnimGraph"));
	// 	}
	// 	else if(InputProxy->HasRootNode())
	// 	{
	// 		InputProxy->UpdateAnimationNode(InputContext);
	// 	}
	// 	else
	// 	{
	// 		FTLLRN_ControlRigLayerInstanceProxy::UpdateCustomProxy(InputProxy, InputContext);
	// 	}
	// }
}

void FAnimNode_TLLRN_ControlRigInputPose::Evaluate_AnyThread(FPoseContext& Output)
{
	// if (InputProxy)
	// {
	// 	FBoneContainer& RequiredBones = InputProxy->GetRequiredBones();
	// 	if (RequiredBones.IsValid())
	// 	{
	// 		FPoseContext InnerOutput(InputProxy, Output.ExpectsAdditivePose());
			
	// 		// if no linked node, just use Evaluate of proxy
	// 		if (FAnimNode_Base* InputNode = InputPose.GetLinkNode())
	// 		{
	// 			InputProxy->EvaluateAnimation_WithRoot(InnerOutput, InputNode);
	// 		}
	// 		else if(InputProxy->HasRootNode())
	// 		{
	// 			InputProxy->EvaluateAnimationNode(InnerOutput);
	// 		}
	// 		else
	// 		{
	// 			FTLLRN_ControlRigLayerInstanceProxy::EvaluateCustomProxy(InputProxy, InnerOutput);
	// 		}

	// 		Output.Pose.MoveBonesFrom(InnerOutput.Pose);
	// 		Output.Curve.MoveFrom(InnerOutput.Curve);
	// 		Output.CustomAttributes.MoveFrom(InnerOutput.CustomAttributes);
	// 		return;
	// 	}
	// }

	// Output.ResetToRefPose();
}

void FAnimNode_TLLRN_ControlRigInputPose::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugData.AddDebugItem(DebugLine);

	if (InputProxy)
	{
		if (InputPose.GetLinkNode())
		{
			InputPose.GatherDebugData(DebugData);
		}
		else
		{
			FTLLRN_ControlRigLayerInstanceProxy::GatherCustomProxyDebugData(InputProxy, DebugData);
		}
	}
}

void FAnimNode_TLLRN_ControlRigInputPose::Link(UAnimInstance* InInputInstance, FAnimInstanceProxy* InInputProxy)
{
	Unlink();

	if (InInputInstance)
	{
		InputAnimInstance = InInputInstance;
		InputProxy = InInputProxy;
		InputPose.SetLinkNode(InputProxy->GetRootNode());
		
		// reset counter, so that input proxy can restart
		FTLLRN_ControlRigLayerInstanceProxy::ResetCounter(InputProxy);
	}
}

void FAnimNode_TLLRN_ControlRigInputPose::Unlink()
{
	InputProxy = nullptr;
	InputAnimInstance = nullptr;
	InputPose.SetLinkNode(nullptr);
}


