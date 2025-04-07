// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_ControlRigSkeletalMeshComponent.h"
#include "Sequencer/TLLRN_ControlRigLayerInstance.h" 
#include "SkeletalDebugRendering.h"
#include "TLLRN_ControlRig.h"
#include "AnimPreviewInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigSkeletalMeshComponent)

UTLLRN_ControlRigSkeletalMeshComponent::UTLLRN_ControlRigSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DebugDrawSkeleton(false)
	, HierarchyInteractionBracket(0)
	, bRebuildDebugDrawSkeletonRequired(false)
	, bIsConstructionEventRunning(false)
{
	SetDisablePostProcessBlueprint(true);
}

void UTLLRN_ControlRigSkeletalMeshComponent::InitAnim(bool bForceReinit)
{
	UAnimInstance* AnimInstance = GetAnimInstance();
	TObjectPtr<class UAnimPreviewInstance> LastPreviewInstance = PreviewInstance;

	// Super::InitAnim might trigger an evaluation, in which case the source animation instance must be set
	UTLLRN_ControlRigLayerInstance* TLLRN_ControlRigInstance = Cast<UTLLRN_ControlRigLayerInstance>(AnimInstance);
	if (TLLRN_ControlRigInstance)
	{
		TLLRN_ControlRigInstance->SetSourceAnimInstance(PreviewInstance);
	}
	
	// skip preview init entirely, just init the super class
	Super::InitAnim(bForceReinit);

	// The preview instance or anim instance might have been created in Super::InitAnim, in which case
	// the source animation instance must be set now.
	// we also must ensure that the anim instance is linked as InitAnim can reset the linked instances
	TLLRN_ControlRigInstance = Cast<UTLLRN_ControlRigLayerInstance>(GetAnimInstance());
	if (TLLRN_ControlRigInstance)
	{
		const bool bHaveInstancesChanged = (AnimInstance != GetAnimInstance()) || (LastPreviewInstance != PreviewInstance);

		const USkeletalMeshComponent* MeshComponent = TLLRN_ControlRigInstance->GetOwningComponent();
		const bool bIsAnimInstanceLinked = PreviewInstance && MeshComponent && MeshComponent->GetLinkedAnimInstances().Contains(PreviewInstance);
		if (bHaveInstancesChanged || !bIsAnimInstanceLinked)
		{
			TLLRN_ControlRigInstance->SetSourceAnimInstance(PreviewInstance);
		}
	}

	bRebuildDebugDrawSkeletonRequired = true;
	RebuildDebugDrawSkeleton();
}

bool UTLLRN_ControlRigSkeletalMeshComponent::IsPreviewOn() const
{
	return (PreviewInstance != nullptr);
}

void UTLLRN_ControlRigSkeletalMeshComponent::SetCustomDefaultPose()
{
	ShowReferencePose(false);
}

void UTLLRN_ControlRigSkeletalMeshComponent::RebuildDebugDrawSkeleton()
{
	if ((HierarchyInteractionBracket != 0) || bIsConstructionEventRunning || !bRebuildDebugDrawSkeletonRequired)
	{
		return;
	}
	
	UTLLRN_ControlRigLayerInstance* TLLRN_ControlRigInstance = Cast<UTLLRN_ControlRigLayerInstance>(GetAnimInstance());

	bool bHasValidTLLRN_ControlRig = false;
	
	if (TLLRN_ControlRigInstance)
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigInstance->GetFirstAvailableTLLRN_ControlRig();
		if (TLLRN_ControlRig)
		{
			bHasValidTLLRN_ControlRig = true;
			
			// we are trying to poke into running instances of Control Rigs
			// on the anim thread and query data, using a lock here to make sure
			// we don't get an inconsistent view of the rig at some
			// intermediate stage of evaluation, for example, during evaluate, we can have a call
			// to copy hierarchy, which empties the hierarchy for a short period of time.
			// if we did not have this lock and try to grab it, we could get an empty bone array
			FScopeLock EvaluateLock(&TLLRN_ControlRig->GetEvaluateMutex());
			
			// just copy it because this is not thread safe
			UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();

			DebugDrawSkeleton.Empty();
			DebugDrawBones.Reset();
			DebugDrawBoneIndexInHierarchy.Reset();

			// create ref modifier
			FReferenceSkeletonModifier RefSkelModifier(DebugDrawSkeleton, nullptr);

			TMap<FName, int32> AddedBoneMap;
			TArray<FTLLRN_RigBoneElement*> BoneElements = Hierarchy->GetBones(true);
			for(FTLLRN_RigBoneElement* BoneElement : BoneElements)
			{
				AddedBoneMap.FindOrAdd(BoneElement->GetFName(), DebugDrawBones.Num());
				DebugDrawBones.Add(DebugDrawBones.Num());
				DebugDrawBoneIndexInHierarchy.Add(BoneElement->GetIndex());
			}

			for(FTLLRN_RigBoneElement* BoneElement : BoneElements)
			{
				const int32 Index = BoneElement->GetIndex();

				FName ParentName = NAME_None;

				// find the first parent that is a bone
				Hierarchy->Traverse(BoneElement, false, [&ParentName, BoneElement](FTLLRN_RigBaseElement* InElement, bool& bContinue)
				{
					bContinue = true;
					
					if(InElement == BoneElement)
					{
						return;
					}

					if(FTLLRN_RigBoneElement* InBoneElement = Cast<FTLLRN_RigBoneElement>(InElement))
					{
						if(ParentName.IsNone())
						{
							ParentName = InBoneElement->GetFName();
						}
						bContinue = false;
					}
				});

				int32 ParentIndex = INDEX_NONE; 
				if(!ParentName.IsNone())
				{
					ParentIndex = AddedBoneMap.FindChecked(ParentName);
				}
					
				FMeshBoneInfo NewMeshBoneInfo;
				NewMeshBoneInfo.Name = BoneElement->GetFName();
				NewMeshBoneInfo.ParentIndex = ParentIndex; 
				// give ref pose here
				RefSkelModifier.Add(NewMeshBoneInfo, Hierarchy->GetInitialGlobalTransform(Index), true);
			}
		}
	}
	
	if (!bHasValidTLLRN_ControlRig)
	{
		DebugDrawSkeleton.Empty();
		DebugDrawBones.Reset();
		DebugDrawBoneIndexInHierarchy.Reset();
		return;
	}
	
	bRebuildDebugDrawSkeletonRequired = false;
}

FTransform UTLLRN_ControlRigSkeletalMeshComponent::GetDrawTransform(int32 BoneIndex) const
{
	UTLLRN_ControlRigLayerInstance* TLLRN_ControlRigInstance = Cast<UTLLRN_ControlRigLayerInstance>(GetAnimInstance());

	if (TLLRN_ControlRigInstance)
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigInstance->GetFirstAvailableTLLRN_ControlRig();
		if (TLLRN_ControlRig && DebugDrawBoneIndexInHierarchy.IsValidIndex(BoneIndex))
		{
			// just copy it because this is not thread safe
			UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
			return Hierarchy->GetGlobalTransform(DebugDrawBoneIndexInHierarchy[BoneIndex]);
		}
	}

	return FTransform::Identity;
}


void UTLLRN_ControlRigSkeletalMeshComponent::EnablePreview(bool bEnable, UAnimationAsset* PreviewAsset)
{
	if (PreviewInstance)
	{
		PreviewInstance->SetAnimationAsset(PreviewAsset);
	}
}

void UTLLRN_ControlRigSkeletalMeshComponent::SetTLLRN_ControlRigBeingDebugged(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if(TLLRN_ControlRigBeingDebuggedPtr.Get() == InTLLRN_ControlRig)
	{
		return;
	}

	if(TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		if(UTLLRN_ControlRig* TLLRN_ControlRigBeingDebugged = TLLRN_ControlRigBeingDebuggedPtr.Get())
		{
			if(!URigVMHost::IsGarbageOrDestroyed(TLLRN_ControlRigBeingDebugged))
			{
				TLLRN_ControlRigBeingDebugged->GetHierarchy()->OnModified().RemoveAll(this);
			}

#if WITH_EDITOR
			TLLRN_ControlRigBeingDebugged->OnPreConstructionForUI_AnyThread().RemoveAll(this);
			TLLRN_ControlRigBeingDebugged->OnPostConstruction_AnyThread().RemoveAll(this);
#endif
		}
	}

	TLLRN_ControlRigBeingDebuggedPtr.Reset();
	bRebuildDebugDrawSkeletonRequired = true;
	
	if(InTLLRN_ControlRig)
	{
		TLLRN_ControlRigBeingDebuggedPtr = InTLLRN_ControlRig;

		InTLLRN_ControlRig->GetHierarchy()->OnModified().RemoveAll(this);
 		InTLLRN_ControlRig->GetHierarchy()->OnModified().AddUObject(this, &UTLLRN_ControlRigSkeletalMeshComponent::OnHierarchyModified_AnyThread);

#if WITH_EDITOR
		InTLLRN_ControlRig->OnPreConstructionForUI_AnyThread().RemoveAll(this);
		InTLLRN_ControlRig->OnPreConstructionForUI_AnyThread().AddUObject(this, &UTLLRN_ControlRigSkeletalMeshComponent::OnPreConstruction_AnyThread);
		
		InTLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
		InTLLRN_ControlRig->OnPostConstruction_AnyThread().AddUObject(this, &UTLLRN_ControlRigSkeletalMeshComponent::OnPostConstruction_AnyThread);
#endif
	}

	RebuildDebugDrawSkeleton();
}

void UTLLRN_ControlRigSkeletalMeshComponent::OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif,
    UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	switch(InNotif)
	{
		case ETLLRN_RigHierarchyNotification::ElementAdded:
		case ETLLRN_RigHierarchyNotification::ElementRemoved:
		case ETLLRN_RigHierarchyNotification::ElementRenamed:
		case ETLLRN_RigHierarchyNotification::ElementReordered:
		case ETLLRN_RigHierarchyNotification::ParentChanged:
		case ETLLRN_RigHierarchyNotification::HierarchyReset:
		{
			bRebuildDebugDrawSkeletonRequired = true;
			if(HierarchyInteractionBracket == 0)
			{
				RebuildDebugDrawSkeleton();
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::InteractionBracketOpened:
		{
			HierarchyInteractionBracket++;
			break;
		}
		case ETLLRN_RigHierarchyNotification::InteractionBracketClosed:
		{
			HierarchyInteractionBracket = FMath::Max(HierarchyInteractionBracket - 1, 0);
			if(HierarchyInteractionBracket == 0)
			{
				RebuildDebugDrawSkeleton();
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

void UTLLRN_ControlRigSkeletalMeshComponent::OnHierarchyModified_AnyThread(ETLLRN_RigHierarchyNotification InNotif,
    UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	if (bIsConstructionEventRunning)
	{
		return;
	}
	
	FTLLRN_RigElementKey Key;
	if(InElement)
	{
		Key = InElement->GetKey();
	}

	TWeakObjectPtr<UTLLRN_RigHierarchy> WeakHierarchy = InHierarchy;
	
	auto Task = [this, InNotif, WeakHierarchy, Key]()
	{
		if(!WeakHierarchy.IsValid())
		{
			return;
		}
		if (const FTLLRN_RigBaseElement* Element = WeakHierarchy.Get()->Find(Key))
		{
			OnHierarchyModified(InNotif, WeakHierarchy.Get(), Element);
		}
	};

	if (IsInGameThread())
	{
		Task();
	}
	else
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([Task]()
		{
			Task();		
		}, TStatId(), NULL, ENamedThreads::GameThread);
	}
}

void UTLLRN_ControlRigSkeletalMeshComponent::OnPreConstruction_AnyThread(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
{
	bIsConstructionEventRunning = true;
}

void UTLLRN_ControlRigSkeletalMeshComponent::OnPostConstruction_AnyThread(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName)
{
	bIsConstructionEventRunning = false;

	if (IsInGameThread())
	{
		RebuildDebugDrawSkeleton();
	}
	else
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([this]()
		{
			RebuildDebugDrawSkeleton();
		}, TStatId(), NULL, ENamedThreads::GameThread);
	}
}