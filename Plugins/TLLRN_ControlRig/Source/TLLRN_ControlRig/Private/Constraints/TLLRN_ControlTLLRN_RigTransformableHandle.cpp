// Copyright Epic Games, Inc. All Rights Reserved.


#include "Constraints/TLLRN_ControlTLLRN_RigTransformableHandle.h"

#include "Components/SkeletalMeshComponent.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigComponent.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "TransformableHandleUtils.h"
#include "Rigs/TLLRN_RigHierarchyElements.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Sequencer/TLLRN_ControlRigSequencerHelpers.h"
#include "UObject/Package.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlTLLRN_RigTransformableHandle)

namespace ControlHandleLocals
{
	using RigGuard = TGuardValue_Bitfield_Cleanup<TFunction<void()>>;
	
	TSet<UTLLRN_ControlRig*> NotifyingRigs;

	bool IsRigNotifying(const UTLLRN_ControlRig* InTLLRN_ControlRig)
	{
		return InTLLRN_ControlRig ? NotifyingRigs.Contains(InTLLRN_ControlRig) : false;
	}
}

/**
 * UTLLRN_TransformableControlHandle
 */

UTLLRN_TransformableControlHandle::~UTLLRN_TransformableControlHandle()
{
	UnregisterDelegates();
}

void UTLLRN_TransformableControlHandle::PostLoad()
{
	Super::PostLoad();
	RegisterDelegates();
}

bool UTLLRN_TransformableControlHandle::IsValid(const bool bDeepCheck) const
{
	if (!TLLRN_ControlRig.IsValid() || ControlName == NAME_None)
	{
		return false;
	}

	const FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlName);
	if (!ControlElement)
	{
		return false;
	}

	if (bDeepCheck)
	{
		const USceneComponent* BoundComponent = GetBoundComponent();
		if (!BoundComponent)
		{
			return false;
		}
	}
	
	return true;
}

void UTLLRN_TransformableControlHandle::PreEvaluate(const bool bTick) const
{
	if (!TLLRN_ControlRig.IsValid() || TLLRN_ControlRig->IsEvaluating())
	{
		return;
	}

	if (TLLRN_ControlRig->IsAdditive())
	{
		if (ControlHandleLocals::IsRigNotifying(TLLRN_ControlRig.Get()))
		{
			return;
		}
		
		if (const USkeletalMeshComponent* SkeletalMeshComponent = GetSkeletalMesh())
		{
			if (!SkeletalMeshComponent->PoseTickedThisFrame())
			{
				return TickTarget();
			}
		}
	}
	
	return bTick ? TickTarget() : TLLRN_ControlRig->Evaluate_AnyThread();
}

void UTLLRN_TransformableControlHandle::TickTarget() const
{
	if (!TLLRN_ControlRig.IsValid())
	{
		return;
	}
	
	if (TLLRN_ControlRig->IsAdditive() && ControlHandleLocals::IsRigNotifying(TLLRN_ControlRig.Get()))
	{
		return;
	}
	
	if (const USkeletalMeshComponent* SkeletalMeshComponent = GetSkeletalMesh())
	{
		return TransformableHandleUtils::TickDependantComponents(SkeletalMeshComponent);
	}

	if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = GetTLLRN_ControlRigComponent())
	{
		TLLRN_ControlRigComponent->Update();
	}
}

// NOTE should we cache the skeletal mesh and the CtrlIndex to avoid looking for if every time
// probably not for handling runtime changes
void UTLLRN_TransformableControlHandle::SetGlobalTransform(const FTransform& InGlobal) const
{
	const FTLLRN_RigControlElement* ControlElement = GetControlElement();
	if (!ControlElement)
	{
		return;
	}

	const USceneComponent* BoundComponent = GetBoundComponent();
	if (!BoundComponent)
	{
		return;
	}
	
	const FTLLRN_RigElementKey& ControlKey = ControlElement->GetKey();
	const FTransform& ComponentTransform = BoundComponent->GetComponentTransform();

	static const FTLLRN_RigControlModifiedContext Context(ETLLRN_ControlRigSetKey::Never);
	static constexpr bool bNotify = false, bSetupUndo = false, bPrintPython = false, bFixEulerFlips = false;

	//use this function so we don't set the preferred angles
	TLLRN_ControlRig->SetControlGlobalTransform(ControlKey.Name, InGlobal.GetRelativeTransform(ComponentTransform),
		bNotify, Context, bSetupUndo, bPrintPython, bFixEulerFlips);
}

void UTLLRN_TransformableControlHandle::SetLocalTransform(const FTransform& InLocal) const
{
	const FTLLRN_RigControlElement* ControlElement = GetControlElement();
	if (!ControlElement)
	{
		return;
	}
	
	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
	const FTLLRN_RigElementKey& ControlKey = ControlElement->GetKey();
	const int32 CtrlIndex = Hierarchy->GetIndex(ControlKey);
	
	Hierarchy->SetLocalTransform(CtrlIndex, InLocal);
}

// NOTE should we cache the skeletal mesh and the CtrlIndex to avoid looking for if every time
// probably not for handling runtime changes
FTransform UTLLRN_TransformableControlHandle::GetGlobalTransform() const
{
	const FTLLRN_RigControlElement* ControlElement = GetControlElement();
	if (!ControlElement)
	{
		return FTransform::Identity;
	}
	
	const USceneComponent* BoundComponent = GetBoundComponent();
	if (!BoundComponent)
	{
		return FTransform::Identity;
	}

	const FTLLRN_RigElementKey& ControlKey = ControlElement->GetKey();
	const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
	const int32 CtrlIndex = Hierarchy->GetIndex(ControlKey);

	const FTransform& ComponentTransform = BoundComponent->GetComponentTransform();
	return Hierarchy->GetGlobalTransform(CtrlIndex) * ComponentTransform;
}

FTransform UTLLRN_TransformableControlHandle::GetLocalTransform() const
{
	const FTLLRN_RigControlElement* ControlElement = GetControlElement();
	if (!ControlElement)
	{
		return FTransform::Identity;
	}

	if (TLLRN_ControlRig->IsAdditive())
	{
		return TLLRN_ControlRig->GetControlLocalTransform(ControlName);
	}
	
	const FTLLRN_RigElementKey& ControlKey = ControlElement->GetKey();
	const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
	const int32 CtrlIndex = Hierarchy->GetIndex(ControlKey);

	return Hierarchy->GetLocalTransform(CtrlIndex);
}

UObject* UTLLRN_TransformableControlHandle::GetPrerequisiteObject() const
{
	return GetBoundComponent(); 
}

FTickFunction* UTLLRN_TransformableControlHandle::GetTickFunction() const
{
	USceneComponent* BoundComponent = GetBoundComponent();
	return BoundComponent ? &BoundComponent->PrimaryComponentTick : nullptr;
}

uint32 UTLLRN_TransformableControlHandle::ComputeHash(const UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InControlName)
{
	return HashCombine(GetTypeHash(InTLLRN_ControlRig), GetTypeHash(InControlName));
}

uint32 UTLLRN_TransformableControlHandle::GetHash() const
{
	if (TLLRN_ControlRig.IsValid() && ControlName != NAME_None)
	{
		return ComputeHash(TLLRN_ControlRig.Get(), ControlName);
	}
	return 0;
}

TWeakObjectPtr<UObject> UTLLRN_TransformableControlHandle::GetTarget() const
{
	return GetBoundComponent();
}

USceneComponent* UTLLRN_TransformableControlHandle::GetBoundComponent() const
{
	if (USkeletalMeshComponent* SkeletalMeshComponent = GetSkeletalMesh())
	{
		return SkeletalMeshComponent;	
	}
	return GetTLLRN_ControlRigComponent();
}

USkeletalMeshComponent* UTLLRN_TransformableControlHandle::GetSkeletalMesh() const
{
	const TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig.IsValid() ? TLLRN_ControlRig->GetObjectBinding() : nullptr;
   	return ObjectBinding ? Cast<USkeletalMeshComponent>(ObjectBinding->GetBoundObject()) : nullptr;
}

UTLLRN_ControlRigComponent* UTLLRN_TransformableControlHandle::GetTLLRN_ControlRigComponent() const
{
	const TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig.IsValid() ? TLLRN_ControlRig->GetObjectBinding() : nullptr;
	return ObjectBinding ? Cast<UTLLRN_ControlRigComponent>(ObjectBinding->GetBoundObject()) : nullptr;
}

bool UTLLRN_TransformableControlHandle::HasDirectDependencyWith(const UTransformableHandle& InOther) const
{
	const uint32 OtherHash = InOther.GetHash();
	if (OtherHash == 0)
	{
		return false;
	}

	// check whether the other handle is one of the skeletal mesh parent
	if (const USceneComponent* BoundComponent = GetBoundComponent())
	{
		if (GetTypeHash(BoundComponent) == OtherHash)
		{
			// we cannot constrain the skeletal mesh component to one of TLLRN_ControlRig's controls
			return true;
		}
		
		for (const USceneComponent* Comp=BoundComponent->GetAttachParent(); Comp!=nullptr; Comp=Comp->GetAttachParent() )
		{
			const uint32 AttachParentHash = GetTypeHash(Comp);
			if (AttachParentHash == OtherHash)
			{
				return true;
			}
		}
	}
	
	FTLLRN_RigControlElement* ControlElement = GetControlElement();
	if (!ControlElement)
	{
		return false;
	}

	// check whether the other handle is one of the control parent within the CR hierarchy
	static constexpr bool bRecursive = true;
	const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
	const FTLLRN_RigBaseElementParentArray AllParents = Hierarchy->GetParents(ControlElement, bRecursive);
	const bool bIsParent = AllParents.ContainsByPredicate([this, OtherHash](const FTLLRN_RigBaseElement* Parent)
	{
		const uint32 ParentHash = ComputeHash(TLLRN_ControlRig.Get(), Parent->GetFName());
		return ParentHash == OtherHash;		
	});

	if (bIsParent)
	{
		return true;
	}

	// otherwise, check if there are any dependency in the graph that would cause a cycle
	const TArray<FTLLRN_RigControlElement*> AllControls = Hierarchy->GetControls();
	const int32 IndexOfPossibleParent = AllControls.IndexOfByPredicate([this, OtherHash](const FTLLRN_RigBaseElement* Parent)
	{
		const uint32 ChildHash = ComputeHash(TLLRN_ControlRig.Get(), Parent->GetFName());
		return ChildHash == OtherHash;
	});

	if (IndexOfPossibleParent != INDEX_NONE)
	{
		FTLLRN_RigControlElement* Parent = AllControls[IndexOfPossibleParent];

		FString FailureReason;

#if WITH_EDITOR
		const UTLLRN_RigHierarchy::TElementDependencyMap DependencyMap = Hierarchy->GetDependenciesForVM(TLLRN_ControlRig->GetVM());
		const bool bIsParentedTo = Hierarchy->IsParentedTo(ControlElement, Parent, DependencyMap);
#else
		const bool bIsParentedTo = Hierarchy->IsParentedTo(ControlElement, Parent);
#endif
		
		if (bIsParentedTo)
		{
			return true;
		}
	}

	return false;
}

// if there's no skeletal mesh bound then the handle is not valid so no need to do anything else
FTickPrerequisite UTLLRN_TransformableControlHandle::GetPrimaryPrerequisite() const
{
	if (FTickFunction* TickFunction = GetTickFunction())
	{
		return FTickPrerequisite( GetBoundComponent(), *TickFunction); 
	}
	
	static const FTickPrerequisite DummyPrerex;
	return DummyPrerex;
}

FTLLRN_RigControlElement* UTLLRN_TransformableControlHandle::GetControlElement() const
{
	if (!TLLRN_ControlRig.IsValid() || ControlName == NAME_None)
	{
		return nullptr;
	}

	return TLLRN_ControlRig->FindControl(ControlName);
}

void UTLLRN_TransformableControlHandle::UnregisterDelegates() const
{
#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
#endif
	
	if (TLLRN_ControlRig.IsValid())
	{
		if (UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
		{
			Hierarchy->OnModified().RemoveAll(this);
		}
		TLLRN_ControlRig->ControlModified().RemoveAll(this);

		if (const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig->GetObjectBinding())
		{
			Binding->OnTLLRN_ControlRigBind().RemoveAll(this);
		}
		TLLRN_ControlRig->TLLRN_ControlRigBound().RemoveAll(this);
	}
}

void UTLLRN_TransformableControlHandle::RegisterDelegates()
{
	UnregisterDelegates();

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectsReplaced.AddUObject(this, &UTLLRN_TransformableControlHandle::OnObjectsReplaced);
#endif

	// make sure the CR is loaded so that we can register delegates
	if (TLLRN_ControlRig.IsPending())
	{
		TLLRN_ControlRig.LoadSynchronous();
	}
	
	if (TLLRN_ControlRig.IsValid())
	{
		if (UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
		{
			Hierarchy->OnModified().AddUObject(this, &UTLLRN_TransformableControlHandle::OnHierarchyModified);
		}

		// NOTE BINDER: this has to be done before binding UTLLRN_TransformableControlHandle::OnControlModified
		if (!TLLRN_ControlRig->ControlModified().IsBoundToObject(&GetEvaluationBinding()))
		{
			TLLRN_ControlRig->ControlModified().AddRaw(&GetEvaluationBinding(), &FControlEvaluationGraphBinding::HandleControlModified);
		}
		
		TLLRN_ControlRig->ControlModified().AddUObject(this, &UTLLRN_TransformableControlHandle::OnControlModified);
		if (!TLLRN_ControlRig->TLLRN_ControlRigBound().IsBoundToObject(this))
		{
			TLLRN_ControlRig->TLLRN_ControlRigBound().AddUObject(this, &UTLLRN_TransformableControlHandle::OnTLLRN_ControlRigBound);
		}
		OnTLLRN_ControlRigBound(TLLRN_ControlRig.Get());
	}
}

void UTLLRN_TransformableControlHandle::OnHierarchyModified(
	ETLLRN_RigHierarchyNotification InNotif,
	UTLLRN_RigHierarchy* InHierarchy,
	const FTLLRN_RigBaseElement* InElement)
{
	if (!TLLRN_ControlRig.IsValid())
	{
	 	return;
	}

	const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
	if (!Hierarchy || InHierarchy != Hierarchy)
	{
		return;
	}

	switch (InNotif)
	{
		case ETLLRN_RigHierarchyNotification::ElementRemoved:
		{
			// FIXME this leaves the constraint invalid as the element won't exist anymore
			// find a way to remove this from the constraints list 
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementRenamed:
		{
			const FName OldName = Hierarchy->GetPreviousName(InElement->GetKey());
			if (OldName == ControlName)
			{
				ControlName = InElement->GetFName();
			}
			break;
		}
		default:
			break;
	}
}

void UTLLRN_TransformableControlHandle::OnControlModified(
	UTLLRN_ControlRig* InTLLRN_ControlRig,
	FTLLRN_RigControlElement* InControl,
	const FTLLRN_RigControlModifiedContext& InContext)
{
	if (!InTLLRN_ControlRig || !InControl)
	{
		return;
	}

	if (bNotifying)
	{
		return;
	}

	if (!TLLRN_ControlRig.IsValid() || ControlName == NAME_None)
	{
		return;
	}

	if (HandleModified().IsBound() && (TLLRN_ControlRig == InTLLRN_ControlRig))
	{
		const EHandleEvent Event = InContext.bConstraintUpdate ?
			EHandleEvent::GlobalTransformUpdated : EHandleEvent::LocalTransformUpdated;

		if (InControl->GetFName() == ControlName)
		{	// if that handle is wrapping InControl
			if (InContext.bConstraintUpdate)
			{
				GetEvaluationBinding().bPendingFlush = true;
			}
			
			// guard from re-entrant notification
			const ControlHandleLocals::RigGuard NotificationGuard([TLLRN_ControlRig = TLLRN_ControlRig.Get()]()
			{
				ControlHandleLocals::NotifyingRigs.Remove(TLLRN_ControlRig);
			});
			ControlHandleLocals::NotifyingRigs.Add(TLLRN_ControlRig.Get());
			
			Notify(Event);
		}
		else if (Event == EHandleEvent::GlobalTransformUpdated)
		{
			// the control being modified is not the one wrapped by this handle 
			if (const FTLLRN_RigControlElement* Control = TLLRN_ControlRig->FindControl(ControlName))
			{
				if (InContext.bConstraintUpdate)
				{
					GetEvaluationBinding().bPendingFlush = true;
				}

				// guard from re-entrant notification 
				const ControlHandleLocals::RigGuard NotificationGuard([TLLRN_ControlRig = TLLRN_ControlRig.Get()]()
				{
					ControlHandleLocals::NotifyingRigs.Remove(TLLRN_ControlRig);
				});
				ControlHandleLocals::NotifyingRigs.Add(TLLRN_ControlRig.Get());
				
				const bool bPreTick = !TLLRN_ControlRig->IsAdditive();
				Notify(EHandleEvent::UpperDependencyUpdated, bPreTick);
			}
		}
	}
}

void UTLLRN_TransformableControlHandle::OnTLLRN_ControlRigBound(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if (!InTLLRN_ControlRig)
	{
		return;
	}

	if (!TLLRN_ControlRig.IsValid() || TLLRN_ControlRig != InTLLRN_ControlRig)
	{
		return;
	}

	if (const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig->GetObjectBinding())
	{
		if (!Binding->OnTLLRN_ControlRigBind().IsBoundToObject(this))
		{
			Binding->OnTLLRN_ControlRigBind().AddUObject(this, &UTLLRN_TransformableControlHandle::OnObjectBoundToTLLRN_ControlRig);
		}
	}
}

void UTLLRN_TransformableControlHandle::OnObjectBoundToTLLRN_ControlRig(UObject* InObject)
{
	if (!TLLRN_ControlRig.IsValid() || !InObject)
	{
		return;
	}

	const TSharedPtr<ITLLRN_ControlRigObjectBinding> Binding = TLLRN_ControlRig->GetObjectBinding();
	const UObject* CurrentObject = Binding ? Binding->GetBoundObject() : nullptr;
	if (CurrentObject == InObject)
	{
		const UWorld* World = GetWorld();
		if (!World)
		{
			if (const USceneComponent* BoundComponent = GetBoundComponent())
			{
				World = BoundComponent->GetWorld();
			}
		}
		
		if (World && InObject->GetWorld() == World)
		{
			Notify(EHandleEvent::ComponentUpdated);
		}
	}
}

TArrayView<FMovieSceneFloatChannel*>  UTLLRN_TransformableControlHandle::GetFloatChannels(const UMovieSceneSection* InSection) const
{
	return FTLLRN_ControlRigSequencerHelpers::GetFloatChannels(TLLRN_ControlRig.Get(),
		ControlName, InSection);
}

TArrayView<FMovieSceneDoubleChannel*>  UTLLRN_TransformableControlHandle::GetDoubleChannels(const UMovieSceneSection* InSection) const
{
	static const TArrayView<FMovieSceneDoubleChannel*> EmptyChannelsView;
	return EmptyChannelsView;
}

bool UTLLRN_TransformableControlHandle::AddTransformKeys(const TArray<FFrameNumber>& InFrames,
	const TArray<FTransform>& InTransforms,
	const EMovieSceneTransformChannel& InChannels,
	const FFrameRate& InTickResolution,
	UMovieSceneSection*,
	const bool bLocal) const
{
	if (!TLLRN_ControlRig.IsValid() || ControlName == NAME_None || InFrames.IsEmpty() || InFrames.Num() != InTransforms.Num())
	{
		return false;
	}
	auto KeyframeFunc = [this, bLocal](const FTransform& InTransform, const FTLLRN_RigControlModifiedContext& InKeyframeContext)
	{
		UTLLRN_ControlRig* InTLLRN_ControlRig = TLLRN_ControlRig.Get();
		static constexpr bool bNotify = true;
		static constexpr bool bUndo = false;
		static constexpr bool bFixEuler = true;

		if (bLocal)
		{
			InTLLRN_ControlRig->SetControlLocalTransform(ControlName, InTransform, bNotify, InKeyframeContext, bUndo, bFixEuler);
			if (InTLLRN_ControlRig->IsAdditive())
			{
				InTLLRN_ControlRig->Evaluate_AnyThread();
			}
			return;
		}
		
		InTLLRN_ControlRig->SetControlGlobalTransform(ControlName, InTransform, bNotify, InKeyframeContext, bUndo, bFixEuler);
		if (InTLLRN_ControlRig->IsAdditive())
		{
			InTLLRN_ControlRig->Evaluate_AnyThread();
		}
	};

	FTLLRN_RigControlModifiedContext KeyframeContext;
	KeyframeContext.SetKey = ETLLRN_ControlRigSetKey::Always;
	KeyframeContext.KeyMask = static_cast<uint32>(InChannels);

	for (int32 Index = 0; Index < InFrames.Num(); ++Index)
	{
		const FFrameNumber& Frame = InFrames[Index];
		KeyframeContext.LocalTime = InTickResolution.AsSeconds(FFrameTime(Frame));

		KeyframeFunc(InTransforms[Index], KeyframeContext);
	}

	return true;
}

//for control rig need to check to see if the control rig is different then we may need to update it based upon what we are now bound to
void UTLLRN_TransformableControlHandle::ResolveBoundObjects(FMovieSceneSequenceID LocalSequenceID, TSharedRef<UE::MovieScene::FSharedPlaybackState> SharedPlaybackState, UObject* SubObject)
{
	if (const UTLLRN_ControlRig* InTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(SubObject))
	{
		// nothing to do
		if (TLLRN_ControlRig == InTLLRN_ControlRig)
		{
			return;
		}

		// skip resolving if the rigs don't share the same class type 
		if (TLLRN_ControlRig && TLLRN_ControlRig->GetClass() != InTLLRN_ControlRig->GetClass())
		{
			return;
		}
		
		for (const TWeakObjectPtr<> ParentObject : ConstraintBindingID.ResolveBoundObjects(LocalSequenceID, SharedPlaybackState))
		{
			const UObject* Bindable = FTLLRN_ControlRigObjectBinding::GetBindableObject(ParentObject.Get());
			if (InTLLRN_ControlRig->GetObjectBinding() && InTLLRN_ControlRig->GetObjectBinding()->GetBoundObject() == Bindable)
			{
				UnregisterDelegates();
				TLLRN_ControlRig = const_cast<UTLLRN_ControlRig*>(InTLLRN_ControlRig);
				RegisterDelegates();
			}
			break; //just do one
		}
	}
}

UTransformableHandle* UTLLRN_TransformableControlHandle::Duplicate(UObject* NewOuter) const
{
	UTLLRN_TransformableControlHandle* HandleCopy = DuplicateObject<UTLLRN_TransformableControlHandle>(this, NewOuter, GetFName());
	HandleCopy->TLLRN_ControlRig = TLLRN_ControlRig;
	HandleCopy->ControlName = ControlName;
	return HandleCopy;
}
#if WITH_EDITOR

FString UTLLRN_TransformableControlHandle::GetLabel() const
{
	return ControlName.ToString();
}

FString UTLLRN_TransformableControlHandle::GetFullLabel() const
{
	const USceneComponent* BoundComponent = GetBoundComponent();
	if (!BoundComponent)
	{
		static const FString DummyLabel;
		return DummyLabel;
	}
	
	const AActor* Actor = BoundComponent->GetOwner();
	const FString TLLRN_ControlRigLabel = Actor ? Actor->GetActorLabel() : BoundComponent->GetName();
	return FString::Printf(TEXT("%s/%s"), *TLLRN_ControlRigLabel, *ControlName.ToString() );
}

void UTLLRN_TransformableControlHandle::OnObjectsReplaced(const TMap<UObject*, UObject*>& InOldToNewInstances)
{
	if (UObject* NewObject = InOldToNewInstances.FindRef(TLLRN_ControlRig.Get()))
	{
		if (UTLLRN_ControlRig* NewTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(NewObject))
		{
			UnregisterDelegates();
			TLLRN_ControlRig = NewTLLRN_ControlRig;
			RegisterDelegates();
		}
	}
}

#endif

FControlEvaluationGraphBinding& UTLLRN_TransformableControlHandle::GetEvaluationBinding()
{
	static FControlEvaluationGraphBinding EvaluationBinding;
	return EvaluationBinding;
}

void FControlEvaluationGraphBinding::HandleControlModified(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_RigControlElement* InControl, const FTLLRN_RigControlModifiedContext& InContext)
{
	if (!bPendingFlush || !InContext.bConstraintUpdate)
	{
		return;
	}
	
	if (!InTLLRN_ControlRig || !InControl)
	{
		return;
	}

	// flush all pending evaluations if any
	if (UWorld* World = InTLLRN_ControlRig->GetWorld())
	{
		const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(World);
		Controller.FlushEvaluationGraph();
	}
	bPendingFlush = false;
}
