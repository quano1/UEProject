// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNode_TLLRN_ControlRigBase.h"
#include "TLLRN_ControlRig.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/NodeMappingContainer.h"
#include "AnimationRuntime.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SkeletalMesh.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Algo/Transform.h"
#include "Animation/AnimCurveUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_TLLRN_ControlRigBase)


DECLARE_CYCLE_STAT(TEXT("TLLRN_ControlRig_UpdateInput"), STAT_TLLRN_ControlRig_UpdateInput, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("TLLRN_ControlRig_Evaluate"), STAT_TLLRN_ControlRig_Evaluate, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("TLLRN_ControlRig_UpdateOutput"), STAT_TLLRN_ControlRig_UpdateOutput, STATGROUP_Anim);

#if ENABLE_ANIM_DEBUG
TAutoConsoleVariable<int32> CVarAnimNodeTLLRN_ControlRigDebug(TEXT("a.AnimNode.TLLRN_ControlRig.Debug"), 0, TEXT("Set to 1 to turn on debug drawing for AnimNode_TLLRN_ControlRigBase"));
#endif

// CVar to disable control rig execution within an anim node
static TAutoConsoleVariable<int32> CVarTLLRN_ControlRigDisableExecutionAnimNode(TEXT("TLLRN_ControlRig.DisableExecutionInAnimNode"), 0, TEXT("if nonzero we disable the execution of Control Rigs inside an anim node."));

// CVar to enable performance optimizations within the anim node pose exchange
static TAutoConsoleVariable<int32> CVarTLLRN_ControlRigEnableAnimNodePerformanceOptimizations(TEXT("TLLRN_ControlRig.EnableAnimNodePerformanceOptimizations"), 0, TEXT("if nonzero we enable the (experimental) execution performance optimizations of Control Rig AnimNodes."));

void FAnimNode_TLLRN_ControlRig_PoseAdapter::PostLinked(UTLLRN_RigHierarchy* InHierarchy)
{
	FTLLRN_RigHierarchyPoseAdapter::PostLinked(InHierarchy);

	Hierarchy = InHierarchy;

	// 1. sort the hierarchies storage so that we can make sure initial and local is grouped correctly
	SortHierarchyStorage();

	// 2. make sure to compute all local transforms in initial and local
	const TArray<FTLLRN_RigTransformElement*> TransformElements = Hierarchy->GetElementsOfType<FTLLRN_RigTransformElement>(false);
	for(FTLLRN_RigTransformElement* TransformElement : TransformElements)
	{
		(void)Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::InitialLocal);
		(void)Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentLocal);
	}
	
	// 3. set up a list of dependents in the hierarchy of the rig to be reset to initial on execute
	TArray<FTLLRN_RigTransformElement*> DependentTransformElements;

	struct Local
	{
		static bool ProcessTransformElement(FAnimNode_TLLRN_ControlRig_PoseAdapter* InPoseAdapter, TArray<FTLLRN_RigTransformElement*>& InDependentTransformElements, FTLLRN_RigTransformElement* InTransformElement)
		{
			if(InTransformElement == nullptr)
			{
				return false;
			}
			
			if(InPoseAdapter->ElementIndexToPoseIndex.Contains((uint16)InTransformElement->GetIndex()))
			{
				return true;
			}

			if(InDependentTransformElements.Contains(InTransformElement))
			{
				return true;
			}

			const FTLLRN_RigBaseElementParentArray& ParentElements = InPoseAdapter->Hierarchy->GetParents(InTransformElement);
			for(const FTLLRN_RigBaseElement* ParentElement : ParentElements)
			{
				if(ProcessTransformElement(InPoseAdapter, InDependentTransformElements, Cast<FTLLRN_RigTransformElement>(const_cast<FTLLRN_RigBaseElement*>(ParentElement))))
				{
					InDependentTransformElements.AddUnique(InTransformElement);
					return true;
				}
			}

			return false;
		}
	};
	
	for(FTLLRN_RigTransformElement* TransformElement : TransformElements)
	{
		Local::ProcessTransformElement(this, DependentTransformElements, TransformElement);
	}
	
	Dependents.Reset();
	Dependents.Reserve(DependentTransformElements.Num() * 3);
	
	for(FTLLRN_RigTransformElement* DependentTransformElement : DependentTransformElements)
	{
		// skip bones - since they are taking care of by BonesToResetToInitial
		if(const FTLLRN_RigBoneElement* BoneElement = Cast<FTLLRN_RigBoneElement>(DependentTransformElement))
		{
			// skip bones if they are not user defined - and if the parent of the bone is not a dependent as well
			if(BoneElement->BoneType == ETLLRN_RigBoneType::Imported &&
				!DependentTransformElements.Contains(BoneElement->ParentElement))
			{
				continue;
			}
		}

		FTLLRN_RigControlElement* DependentControlElement = Cast<FTLLRN_RigControlElement>(DependentTransformElement);
		if(DependentControlElement && DependentControlElement->IsAnimationChannel())
		{
			continue;
		}
		
		Dependents.Emplace(DependentTransformElement->GetKeyAndIndex(), ETLLRN_RigTransformType::CurrentGlobal, ETLLRN_RigTransformStorageType::Pose, &DependentTransformElement->GetDirtyState().Current);
		if(DependentControlElement)
		{
			Dependents.Emplace(DependentTransformElement->GetKeyAndIndex(), ETLLRN_RigTransformType::CurrentGlobal, ETLLRN_RigTransformStorageType::Offset, &DependentControlElement->GetOffsetDirtyState().Current);
			Dependents.Emplace(DependentTransformElement->GetKeyAndIndex(), ETLLRN_RigTransformType::CurrentGlobal, ETLLRN_RigTransformStorageType::Shape, &DependentControlElement->GetShapeDirtyState().Current);
		}
	}

	// 4. relink the storage for the transforms (local or global or both) and dirty states to our local storage
	static constexpr bool bLocalIsPrimary = true;
	UpdateDirtyStates(bLocalIsPrimary);
	for(int32 PoseIndex = 0; PoseIndex < PoseIndexToElementIndex.Num(); PoseIndex++)
	{
		const int32& TransformElementIndex = PoseIndexToElementIndex[PoseIndex];
		if(TransformElementIndex != INDEX_NONE)
		{
			const FTLLRN_RigElementKeyAndIndex& KeyAndIndex = Hierarchy->GetKeyAndIndex(TransformElementIndex);
			RelinkTransformStorage(KeyAndIndex, ETLLRN_RigTransformType::CurrentLocal, ETLLRN_RigTransformStorageType::Pose, &LocalPose[PoseIndex], &LocalPoseIsDirty[PoseIndex]);
			RelinkTransformStorage(KeyAndIndex, ETLLRN_RigTransformType::CurrentGlobal, ETLLRN_RigTransformStorageType::Pose, &GlobalPose[PoseIndex], &GlobalPoseIsDirty[PoseIndex]);
		}
	}
	UpdateDirtyStates(bLocalIsPrimary); // do this again to make sure local is flagged as clean and global as dirty

	// 5. Shrink the storage on the hierarchy now that we've relinked it
	ShrinkHierarchyStorage();
	
	LastTopologyVersion = Hierarchy->GetTopologyVersion();
}

void FAnimNode_TLLRN_ControlRig_PoseAdapter::PreUnlinked(UTLLRN_RigHierarchy* InHierarchy)
{
	UnlinkTransformStorage();
	UpdateHierarchyStorage();
	
	LastTopologyVersion = UINT32_MAX;
	Hierarchy = nullptr;
	FTLLRN_RigHierarchyPoseAdapter::PreUnlinked(InHierarchy);
}

void FAnimNode_TLLRN_ControlRig_PoseAdapter::UnlinkTransformStorage()
{
	check(Hierarchy);

	for(int32 PoseIndex = 0; PoseIndex < PoseIndexToElementIndex.Num(); PoseIndex++)
	{
		const int32& TransformElementIndex = PoseIndexToElementIndex[PoseIndex];
		if(TransformElementIndex != INDEX_NONE)
		{
			const FTLLRN_RigElementKeyAndIndex& KeyAndIndex = Hierarchy->GetKeyAndIndex(TransformElementIndex);
			RestoreTransformStorage(KeyAndIndex, ETLLRN_RigTransformType::CurrentLocal, ETLLRN_RigTransformStorageType::Pose, false);
			RestoreTransformStorage(KeyAndIndex, ETLLRN_RigTransformType::CurrentGlobal, ETLLRN_RigTransformStorageType::Pose, false);
		}
	}

	ElementIndexToPoseIndex.Reset();
	PoseIndexToElementIndex.Reset();
}

void FAnimNode_TLLRN_ControlRig_PoseAdapter::ConvertToLocalPose()
{
	check(ParentPoseIndices.Num() == GlobalPose.Num());
	LocalPose.SetNum(GlobalPose.Num());

	if(Hierarchy)
	{
		for(int32 Index = 0; Index < ParentPoseIndices.Num(); Index++)
		{
			(void)GetLocalTransform(Index);
		}
	}
}
void FAnimNode_TLLRN_ControlRig_PoseAdapter::ConvertToGlobalPose()
{
	check(ParentPoseIndices.Num() == LocalPose.Num());
	GlobalPose.SetNum(LocalPose.Num());

	if(Hierarchy)
	{
		for(int32 Index = 0; Index < ParentPoseIndices.Num(); Index++)
		{
			(void)GetGlobalTransform(Index);
		}
	}
}

const FTransform& FAnimNode_TLLRN_ControlRig_PoseAdapter::GetLocalTransform(int32 InIndex)
{
	check(LocalPose.IsValidIndex(InIndex));
	check(LocalPoseIsDirty.IsValidIndex(InIndex));
	check(ParentPoseIndices.IsValidIndex(InIndex));
	check(RequiresHierarchyForSpaceConversion.IsValidIndex(InIndex));

	check(Hierarchy);

	if(LocalPoseIsDirty[InIndex])
	{
		check(GlobalPoseIsDirty[InIndex] == false);
		if(RequiresHierarchyForSpaceConversion[InIndex] && PoseIndexToElementIndex.IsValidIndex(InIndex))
		{
			LocalPose[InIndex] = Hierarchy->GetLocalTransformByIndex(PoseIndexToElementIndex[InIndex]);
		}
		else
		{
			const int32 ParentIndex = ParentPoseIndices[InIndex];
			if(ParentIndex == INDEX_NONE)
			{
				LocalPose[InIndex] = GetGlobalTransform(InIndex);
			}
			else
			{
				LocalPose[InIndex] = GetGlobalTransform(InIndex).GetRelativeTransform(GetGlobalTransform(ParentIndex));
			}
			LocalPose[InIndex].NormalizeRotation();
		}
		LocalPoseIsDirty[InIndex] = false;
	}
	return LocalPose[InIndex];
}

const FTransform& FAnimNode_TLLRN_ControlRig_PoseAdapter::GetGlobalTransform(int32 InIndex)
{
	check(GlobalPose.IsValidIndex(InIndex));
	check(GlobalPoseIsDirty.IsValidIndex(InIndex));
	check(ParentPoseIndices.IsValidIndex(InIndex));
	check(RequiresHierarchyForSpaceConversion.IsValidIndex(InIndex));

	check(Hierarchy);

	if(GlobalPoseIsDirty[InIndex])
	{
		check(LocalPoseIsDirty[InIndex] == false);
		if(RequiresHierarchyForSpaceConversion[InIndex] && PoseIndexToElementIndex.IsValidIndex(InIndex))
		{
			GlobalPose[InIndex] = Hierarchy->GetGlobalTransformByIndex(PoseIndexToElementIndex[InIndex]);
		}
		else
		{
			const int32 ParentIndex = ParentPoseIndices[InIndex];
			if(ParentIndex == INDEX_NONE)
			{
				GlobalPose[InIndex] = GetLocalTransform(InIndex);
			}
			else
			{
				GlobalPose[InIndex] = GetLocalTransform(InIndex) * GetGlobalTransform(ParentIndex);
			}
			GlobalPose[InIndex].NormalizeRotation();
		}
		GlobalPoseIsDirty[InIndex] = false;
	}
	return GlobalPose[InIndex];
}

void FAnimNode_TLLRN_ControlRig_PoseAdapter::UpdateDirtyStates(const TOptional<bool> InLocalIsPrimary)
{
	bool bLocalIsPrimary = bTransferInLocalSpace;
	if(InLocalIsPrimary.IsSet())
	{
		bLocalIsPrimary = InLocalIsPrimary.GetValue();
	}
	
	if(LocalPoseIsDirty.Num() != LocalPose.Num())
	{
		LocalPoseIsDirty.Reset();
		LocalPoseIsDirty.AddZeroed(LocalPose.Num());
	}
	else if(bLocalIsPrimary)
	{
		FMemory::Memzero(LocalPoseIsDirty.GetData(), LocalPoseIsDirty.GetAllocatedSize());
	}
	if(!bLocalIsPrimary)
	{
		for(bool& Flag : LocalPoseIsDirty)
		{
			Flag = true;
		}
	}

	if(GlobalPoseIsDirty.Num() != GlobalPose.Num())
	{
		GlobalPoseIsDirty.Reset();
		GlobalPoseIsDirty.AddZeroed(GlobalPose.Num());
	}
	else if(!bLocalIsPrimary)
	{
		FMemory::Memzero(GlobalPoseIsDirty.GetData(), GlobalPoseIsDirty.GetAllocatedSize());
	}
	if(bLocalIsPrimary)
	{
		for(bool& Flag : GlobalPoseIsDirty)
		{
			Flag = true;
		}
	}
}

void FAnimNode_TLLRN_ControlRig_PoseAdapter::ComputeDependentTransforms()
{
	check(Hierarchy);

	// ensure to compute all local transforms
	for(const FDependentTransform& Dependent : Dependents)
	{
		if(FTLLRN_RigTransformElement* TransformElement = Hierarchy->Get<FTLLRN_RigTransformElement>(Dependent.KeyAndIndex.Index))
		{
			switch(Dependent.StorageType)
			{
				case ETLLRN_RigTransformStorageType::Pose:
				{
					(void)Hierarchy->GetTransform(TransformElement, MakeLocal(Dependent.TransformType));
					break;
				}
				case ETLLRN_RigTransformStorageType::Offset:
				{
					FTLLRN_RigControlElement* ControlElement = CastChecked<FTLLRN_RigControlElement>(TransformElement);
					(void)Hierarchy->GetControlOffsetTransform(ControlElement, MakeLocal(Dependent.TransformType));
					break;
				}
				case ETLLRN_RigTransformStorageType::Shape:
				{
					FTLLRN_RigControlElement* ControlElement = CastChecked<FTLLRN_RigControlElement>(TransformElement);
					(void)Hierarchy->GetControlShapeTransform(ControlElement, MakeLocal(Dependent.TransformType));
					break;
				}
				default:
				{
					break;
				}
			}
			check(Dependent.DirtyState->Local.Get() == false);
		}
	}
}

void FAnimNode_TLLRN_ControlRig_PoseAdapter::MarkDependentsDirty()
{
	ComputeDependentTransforms();

	check(Hierarchy);

	// mark the global dependent as dirty
	for(const FDependentTransform& Dependent : Dependents)
	{
		check(Dependent.DirtyState->Local.Get() == false);
		Dependent.DirtyState->Global.Set(true);
	}
}

FAnimNode_TLLRN_ControlRigBase::FAnimNode_TLLRN_ControlRigBase()
	: FAnimNode_CustomProperty()
	, bResetInputPoseToInitial(true) 
	, bTransferInputPose(true)
	, bTransferInputCurves(true)
	, bTransferPoseInGlobalSpace(CVarTLLRN_ControlRigEnableAnimNodePerformanceOptimizations->GetInt() == 0) // default to local in optimized mode
	, InputSettings(FTLLRN_ControlRigIOSettings())
	, OutputSettings(FTLLRN_ControlRigIOSettings())
	, bExecute(true)
	, InternalBlendAlpha (1.f)
	, bTLLRN_ControlRigRequiresInitialization(true)
	, bEnablePoseAdapter(CVarTLLRN_ControlRigEnableAnimNodePerformanceOptimizations->GetInt() != 0)
	, LastBonesSerialNumberForCacheBones(0)
{
}

void FAnimNode_TLLRN_ControlRigBase::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FAnimNode_CustomProperty::OnInitializeAnimInstance(InProxy, InAnimInstance);
	
	bEnablePoseAdapter = (CVarTLLRN_ControlRigEnableAnimNodePerformanceOptimizations->GetInt() != 0);

	WeakAnimInstanceObject = TWeakObjectPtr<const UAnimInstance>(InAnimInstance);
	if(bEnablePoseAdapter)
	{
		PoseAdapter = MakeShareable(new FAnimNode_TLLRN_ControlRig_PoseAdapter());
	}

	USkeletalMeshComponent* Component = InAnimInstance->GetOwningComponent();
	UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig();
	if (Component && Component->GetSkeletalMeshAsset() && TLLRN_ControlRig)
	{
#if WITH_EDITORONLY_DATA
		UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(TLLRN_ControlRig->GetClass());
		if (BlueprintClass)
		{
			UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy);
			// node mapping container will be saved on the initialization part
			NodeMappingContainer = Component->GetSkeletalMeshAsset()->GetNodeMappingContainer(Blueprint);
		}
#endif

		// register skeletalmesh component for now
		TLLRN_ControlRig->GetDataSourceRegistry()->RegisterDataSource(UTLLRN_ControlRig::OwnerComponent, InAnimInstance->GetOwningComponent());
		UpdateGetAssetUserDataDelegate(TLLRN_ControlRig);
	}
}

void FAnimNode_TLLRN_ControlRigBase::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FAnimNode_CustomProperty::Initialize_AnyThread(Context);
	Source.Initialize(Context);
}

void FAnimNode_TLLRN_ControlRigBase::GatherDebugData(FNodeDebugData& DebugData)
{
	Source.GatherDebugData(DebugData.BranchFlow(1.f));
}

void FAnimNode_TLLRN_ControlRigBase::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FAnimNode_CustomProperty::Update_AnyThread(Context);
	Source.Update(Context);

	if (bExecute)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
		{
			// @TODO: fix this to be thread-safe
			// Pre-update doesn't work for custom anim instances
			// FAnimNode_TLLRN_ControlRigExternalSource needs this to be called to reset to ref pose
			TLLRN_ControlRig->SetDeltaTime(Context.GetDeltaTime());
		}
	}
}

bool FAnimNode_TLLRN_ControlRigBase::CanExecute()
{
	if(CVarTLLRN_ControlRigDisableExecutionAnimNode->GetInt() != 0)
	{
		return false;
	}

	if(bEnablePoseAdapter && !PoseAdapter.IsValid())
	{
		return false;
	}

	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		return TLLRN_ControlRig->CanExecute(); 
	}

	return false;
}

void FAnimNode_TLLRN_ControlRigBase::UpdateInput(UTLLRN_ControlRig* TLLRN_ControlRig, FPoseContext& InOutput)
{
	SCOPE_CYCLE_COUNTER(STAT_TLLRN_ControlRig_UpdateInput);

	if(!CanExecute())
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
	if(Hierarchy == nullptr)
	{
		return;
	}

#if WITH_EDITOR
	// if we are recording any change - let's clear the undo stack
	if(bExecute && Hierarchy->IsTracingChanges())
	{
		Hierarchy->ResetTransformStack();
	}
#endif

	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if(bEnablePoseAdapter)
	{
		if (PoseAdapter && InputSettings.bUpdatePose)
		{
			if(InputSettings.bUpdatePose)
			{
				// reset transforms here to prevent additive transforms from accumulating to INF
				// we only update transforms from the mesh pose for bones in the current LOD, 
				// so the reset here ensures excluded bones are also reset
				if(!PoseAdapter->BonesToResetToInitial.IsEmpty())
				{
					FTLLRN_RigHierarchyValidityBracket ValidityBracket(Hierarchy);
					{
#if WITH_EDITOR
						// make sure transient controls don't get reset
						UTLLRN_ControlRig::FTransientControlPoseScope PoseScope(TLLRN_ControlRig);
#endif
						for(const int32& BoneElementIndex : PoseAdapter->BonesToResetToInitial)
						{
							if(FTLLRN_RigTransformElement* BoneElement = Hierarchy->Get<FTLLRN_RigBoneElement>(BoneElementIndex))
							{
								const FTransform InitialLocalTransform = Hierarchy->GetTransform(BoneElement, ETLLRN_RigTransformType::InitialLocal);
								BoneElement->GetTransform().Set(ETLLRN_RigTransformType::CurrentLocal, InitialLocalTransform);
								BoneElement->GetDirtyState().MarkClean(ETLLRN_RigTransformType::CurrentLocal);
								BoneElement->GetDirtyState().MarkDirty(ETLLRN_RigTransformType::CurrentGlobal);
							}
						}
					}
				}
				
				if(bTransferInputPose && InOutput.Pose.GetNumBones() == PoseAdapter->LocalPose.Num())
				{
					PoseAdapter->MarkDependentsDirty();
					InOutput.Pose.CopyBonesTo(PoseAdapter->LocalPose);
					PoseAdapter->UpdateDirtyStates(true);
				}
			}
			
			if(InputSettings.bUpdateCurves || OutputSettings.bUpdateCurves)
			{
				Hierarchy->UnsetCurveValues();
				
				const TArray<FTLLRN_RigBaseElement*> HierarchyCurves = Hierarchy->GetCurvesFast();
				if(PoseAdapter->HierarchyCurveLookup.Num() != HierarchyCurves.Num())
				{
					PoseAdapter->HierarchyCurveLookup.Reset();
					PoseAdapter->HierarchyCurveLookup.Reserve(HierarchyCurves.Num());
					for(int32 Index = 0; Index < HierarchyCurves.Num(); Index++)
					{
						PoseAdapter->HierarchyCurveLookup.Add(HierarchyCurves[Index]->GetFName(), Index);
					}
				}

				PoseAdapter->PoseCurveToHierarchyCurve.SetNumUninitialized(InOutput.Curve.Num());

				int32 CurveIndex = 0;
				InOutput.Curve.ForEachElement([this, &CurveIndex, &HierarchyCurves](const UE::Anim::FCurveElement& InCurveElement)
				{
					PoseAdapter->PoseCurveToHierarchyCurve[CurveIndex] = INDEX_NONE;

					// the index stored here is the sub index of the curve (the index of the curve within the list of curves)
					if(const int32* IndexPtr = PoseAdapter->HierarchyCurveLookup.Find(InCurveElement.Name))
					{
						const int32& Index = *IndexPtr;
						FTLLRN_RigCurveElement* HierarchyCurve = CastChecked<FTLLRN_RigCurveElement>(HierarchyCurves[Index]);

						// when setting the curve we need to mark it as "value set", otherwise the copy
						// pose may reset it to unset - thus we'll loose the value that was just copied in.
						HierarchyCurve->Set(InCurveElement.Value, true);
						PoseAdapter->PoseCurveToHierarchyCurve[CurveIndex] = Index;
					}
					CurveIndex++;
				});
			}
		}
	}
	else
	{
		if (InputSettings.bUpdatePose && bTransferInputPose)
		{
			const FBoneContainer& RequiredBones = InOutput.Pose.GetBoneContainer();

			// reset transforms here to prevent additive transforms from accumulating to INF
			// we only update transforms from the mesh pose for bones in the current LOD, 
			// so the reset here ensures excluded bones are also reset
			if(!TLLRN_ControlTLLRN_RigBoneInputMappingByName.IsEmpty() || bResetInputPoseToInitial)
			{
				FTLLRN_RigHierarchyValidityBracket ValidityBracket(Hierarchy);

				{
	#if WITH_EDITOR
					// make sure transient controls don't get reset
					UTLLRN_ControlRig::FTransientControlPoseScope PoseScope(TLLRN_ControlRig);
	#endif 
					Hierarchy->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
				}
			}

			if(bTransferPoseInGlobalSpace || NodeMappingContainer.IsValid())
			{
				// get component pose from control rig
				FCSPose<FCompactPose> MeshPoses;
				// first I need to convert to local pose
				MeshPoses.InitPose(InOutput.Pose);

				if(!TLLRN_ControlTLLRN_RigBoneInputMappingByIndex.IsEmpty())
				{
					for (const TPair<uint16, uint16>& Pair : TLLRN_ControlTLLRN_RigBoneInputMappingByIndex)
					{
						const uint16 TLLRN_ControlRigIndex = Pair.Key;
						const uint16 SkeletonIndex = Pair.Value;
						
						FCompactPoseBoneIndex CompactPoseIndex(SkeletonIndex);
						const FTransform& ComponentTransform = MeshPoses.GetComponentSpaceTransform(CompactPoseIndex);
						Hierarchy->SetGlobalTransformByIndex(TLLRN_ControlRigIndex, ComponentTransform, false);
					}
				}
				else
				{
					for (auto Iter = TLLRN_ControlTLLRN_RigBoneInputMappingByName.CreateConstIterator(); Iter; ++Iter)
					{
						const FName& Name = Iter.Key();
						const uint16 Index = Iter.Value();
						const FTLLRN_RigElementKey Key(Name, ETLLRN_RigElementType::Bone);

						FCompactPoseBoneIndex CompactPoseIndex(Index);

						const FTransform& ComponentTransform = MeshPoses.GetComponentSpaceTransform(CompactPoseIndex);
						if (NodeMappingContainer.IsValid())
						{
							const FTransform& RelativeTransformReverse = NodeMappingContainer->GetSourceToTargetTransform(Name).GetRelativeTransformReverse(ComponentTransform);
							Hierarchy->SetGlobalTransform(Key, RelativeTransformReverse, false);
						}
						else
						{
							Hierarchy->SetGlobalTransform(Key, ComponentTransform, false);
						}
						
					}
				}
			}
			else
			{
				if(!TLLRN_ControlTLLRN_RigBoneInputMappingByIndex.IsEmpty())
				{
					for (const TPair<uint16, uint16>& Pair : TLLRN_ControlTLLRN_RigBoneInputMappingByIndex)
					{
						const uint16 TLLRN_ControlRigIndex = Pair.Key;
						const uint16 SkeletonIndex = Pair.Value;
						
						FCompactPoseBoneIndex CompactPoseIndex(SkeletonIndex);
						const FTransform& LocalTransform = InOutput.Pose[CompactPoseIndex];
						Hierarchy->SetLocalTransformByIndex(TLLRN_ControlRigIndex, LocalTransform, false);
					}
				}
				else
				{
					for (auto Iter = TLLRN_ControlTLLRN_RigBoneInputMappingByName.CreateConstIterator(); Iter; ++Iter)
					{
						const FName& Name = Iter.Key();
						const uint16 SkeletonIndex = Iter.Value();
						const FTLLRN_RigElementKey Key(Name, ETLLRN_RigElementType::Bone);

						FCompactPoseBoneIndex CompactPoseIndex(SkeletonIndex);
						const FTransform& LocalTransform = InOutput.Pose[CompactPoseIndex];
						Hierarchy->SetLocalTransform(Key, LocalTransform, false);
					}
				}
			}
			
		}

		if (InputSettings.bUpdateCurves && bTransferInputCurves)
		{
			Hierarchy->UnsetCurveValues();
			
			InOutput.Curve.ForEachElement([Hierarchy](const UE::Anim::FCurveElement& InCurveElement)
			{
				const FTLLRN_RigElementKey Key(InCurveElement.Name, ETLLRN_RigElementType::Curve);
				Hierarchy->SetCurveValue(Key, InCurveElement.Value);
			});
		}		
	}

#if WITH_EDITOR
	if(bExecute && Hierarchy->IsTracingChanges())
	{
		Hierarchy->StorePoseForTrace(TEXT("FAnimNode_TLLRN_ControlRigBase::UpdateInput"));
	}
#endif
}

void FAnimNode_TLLRN_ControlRigBase::UpdateOutput(UTLLRN_ControlRig* TLLRN_ControlRig, FPoseContext& InOutput)
{
	SCOPE_CYCLE_COUNTER(STAT_TLLRN_ControlRig_UpdateOutput);

	if(!CanExecute())
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
	if(Hierarchy == nullptr)
	{
		return;
	}

	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if(bEnablePoseAdapter)
	{
		if (OutputSettings.bUpdatePose)
		{
			if(PoseAdapter)
			{
				// make sure the local respectively global transforms are all ready
				if(PoseAdapter->bTransferInLocalSpace)
				{
					PoseAdapter->ConvertToLocalPose();
				}
				else
				{
					PoseAdapter->ConvertToGlobalPose();
				}

				// if we transfered in global - let's convert back to local
				if(!PoseAdapter->bTransferInLocalSpace)
				{
					if(PoseAdapter->GlobalPose.Num() == InOutput.Pose.GetNumBones())
					{
						PoseAdapter->ConvertToLocalPose();
					}
				}
				
				InOutput.Pose.CopyBonesFrom(PoseAdapter->LocalPose);
			}
		}

		if(OutputSettings.bUpdateCurves)
		{
			const TArray<int32>& ChangedCurveIndices = Hierarchy->GetChangedCurveIndices();
			if(ChangedCurveIndices.Num() > 0)
			{
				const TArray<FTLLRN_RigBaseElement*> HierarchyCurves = Hierarchy->GetCurvesFast();

				int32 CurveIndex = 0;
				int32 CurvesCopied = 0;
				InOutput.Curve.ForEachElement([this, &CurveIndex, &CurvesCopied, &HierarchyCurves](const UE::Anim::FCurveElement& InCurveElement)
				{
					// the index stored here is the sub index of the curve (the index of the curve within the list of curves)
					const int32& HierarchyIndex = PoseAdapter->PoseCurveToHierarchyCurve[CurveIndex];
					if(HierarchyIndex != INDEX_NONE)
					{
						const FTLLRN_RigCurveElement* HierarchyCurve = CastChecked<FTLLRN_RigCurveElement>(HierarchyCurves[HierarchyIndex]);
						if(HierarchyCurve->IsValueSet())
						{
							const_cast<UE::Anim::FCurveElement*>(&InCurveElement)->Value = HierarchyCurve->Get();
						}
						CurvesCopied++;
					}
					CurveIndex++;
				});

				if(CurvesCopied < HierarchyCurves.Num())
				{
					PoseAdapter->HierarchyCurveCopied.SetNumUninitialized(Hierarchy->Num());
					FMemory::Memzero(PoseAdapter->HierarchyCurveCopied.GetData(), PoseAdapter->HierarchyCurveCopied.GetAllocatedSize());
					
					FBlendedCurve TLLRN_ControlTLLRN_RigCurves;
					TLLRN_ControlTLLRN_RigCurves.Reserve(ChangedCurveIndices.Num());
					for(const int32& ChangedCurveIndex : ChangedCurveIndices)
					{
						if(!PoseAdapter->HierarchyCurveCopied[ChangedCurveIndex])
						{
							if(const FTLLRN_RigCurveElement* HierarchyCurve = Hierarchy->Get<FTLLRN_RigCurveElement>(ChangedCurveIndex))
							{
								if(HierarchyCurve->IsValueSet())
								{
									TLLRN_ControlTLLRN_RigCurves.Add(HierarchyCurve->GetFName(), HierarchyCurve->Get());
								}
							}
							PoseAdapter->HierarchyCurveCopied[ChangedCurveIndex] = true;
						}
					}

					InOutput.Curve.Combine(TLLRN_ControlTLLRN_RigCurves);
				}
			}
		}
	}
	else
	{
		if (OutputSettings.bUpdatePose)
		{
			// copy output of the rig
			const FBoneContainer& RequiredBones = InOutput.Pose.GetBoneContainer();

			TMap<FName, uint16>& NameBasedMapping = TLLRN_ControlTLLRN_RigBoneOutputMappingByName;
			TArray<TPair<uint16, uint16>>& IndexBasedMapping = TLLRN_ControlTLLRN_RigBoneOutputMappingByIndex;

			// if we don't have a different mapping for outputs, use the input mapping
			if(NameBasedMapping.IsEmpty() && IndexBasedMapping.IsEmpty())
			{
				NameBasedMapping = TLLRN_ControlTLLRN_RigBoneInputMappingByName;
				IndexBasedMapping = TLLRN_ControlTLLRN_RigBoneInputMappingByIndex;
			}

			if(bTransferPoseInGlobalSpace || NodeMappingContainer.IsValid())
			{
				// get component pose from control rig
				FCSPose<FCompactPose> MeshPoses;
				MeshPoses.InitPose(InOutput.Pose);

				if(!IndexBasedMapping.IsEmpty())
				{
					for (const TPair<uint16, uint16>& Pair : IndexBasedMapping)
					{
						const uint16 TLLRN_ControlRigIndex = Pair.Key;
						const uint16 SkeletonIndex = Pair.Value;

						FCompactPoseBoneIndex CompactPoseIndex(SkeletonIndex);
						FTransform ComponentTransform = Hierarchy->GetGlobalTransformByIndex(TLLRN_ControlRigIndex);
						MeshPoses.SetComponentSpaceTransform(CompactPoseIndex, ComponentTransform);
					}
				}
				else
				{
					for (auto Iter = NameBasedMapping.CreateConstIterator(); Iter; ++Iter)
					{
						const FName& Name = Iter.Key();
						const uint16 SkeletonIndex = Iter.Value();
						const FTLLRN_RigElementKey Key(Name, ETLLRN_RigElementType::Bone);

						FCompactPoseBoneIndex CompactPoseIndex(SkeletonIndex);
						FTransform ComponentTransform = Hierarchy->GetGlobalTransform(Key);
						if (NodeMappingContainer.IsValid())
						{
							ComponentTransform = NodeMappingContainer->GetSourceToTargetTransform(Name) * ComponentTransform;
						}

						MeshPoses.SetComponentSpaceTransform(CompactPoseIndex, ComponentTransform);
					}
				}

				FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(MeshPoses, InOutput.Pose);
				InOutput.Pose.NormalizeRotations();
			}
			else
			{
				if(!IndexBasedMapping.IsEmpty())
				{
					for (const TPair<uint16, uint16>& Pair : IndexBasedMapping)
					{
						const uint16 TLLRN_ControlRigIndex = Pair.Key;
						const uint16 SkeletonIndex = Pair.Value;

						FCompactPoseBoneIndex CompactPoseIndex(SkeletonIndex);
						FTransform LocalTransform = Hierarchy->GetLocalTransformByIndex(TLLRN_ControlRigIndex);
						InOutput.Pose[CompactPoseIndex] = LocalTransform;
					}
				}
				else
				{
					for (auto Iter = NameBasedMapping.CreateConstIterator(); Iter; ++Iter)
					{
						const FName& Name = Iter.Key();
						const uint16 Index = Iter.Value();
						const FTLLRN_RigElementKey Key(Name, ETLLRN_RigElementType::Bone);

						FCompactPoseBoneIndex CompactPoseIndex(Index);
						FTransform LocalTransform = Hierarchy->GetLocalTransform(Key);
						InOutput.Pose[CompactPoseIndex] = LocalTransform;
					}
				}
			}
		}

		if (OutputSettings.bUpdateCurves)
		{
			FBlendedCurve TLLRN_ControlTLLRN_RigCurves;
			TLLRN_ControlTLLRN_RigCurves.Reserve(Hierarchy->Num(ETLLRN_RigElementType::Curve));
			Hierarchy->ForEach<FTLLRN_RigCurveElement>([&TLLRN_ControlTLLRN_RigCurves](const FTLLRN_RigCurveElement* InElement)
			{
				if(InElement->IsValueSet())
				{
					TLLRN_ControlTLLRN_RigCurves.Add(InElement->GetFName(), InElement->Get());
				}
				return true;
			});

			InOutput.Curve.Combine(TLLRN_ControlTLLRN_RigCurves);
		}
	}

#if WITH_EDITOR
	if(bExecute && Hierarchy->IsTracingChanges())
	{
		Hierarchy->StorePoseForTrace(TEXT("FAnimNode_TLLRN_ControlRigBase::UpdateOutput"));
		Hierarchy->DumpTransformStackToFile();
	}
#endif
}

void FAnimNode_TLLRN_ControlRigBase::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if(bEnablePoseAdapter && !PoseAdapter.IsValid())
	{
		return;
	}

	FPoseContext SourcePose(Output);

	if (Source.GetLinkNode())
	{
		Source.Evaluate(SourcePose);
	}
	else
	{
		// apply refpose
		SourcePose.ResetToRefPose();
	}

	if (CanExecute() && FAnimWeight::IsRelevant(InternalBlendAlpha) && GetTLLRN_ControlRig())
	{
		if (FAnimWeight::IsFullWeight(InternalBlendAlpha))
		{
			ExecuteTLLRN_ControlRig(SourcePose);
			Output = SourcePose;
		}
		else 
		{
			// this blends additively - by weight
			FPoseContext TLLRN_ControlTLLRN_RigPose(SourcePose);
			TLLRN_ControlTLLRN_RigPose = SourcePose;
			ExecuteTLLRN_ControlRig(TLLRN_ControlTLLRN_RigPose);

			FPoseContext AdditivePose(TLLRN_ControlTLLRN_RigPose);
			AdditivePose = TLLRN_ControlTLLRN_RigPose;
			FAnimationRuntime::ConvertPoseToAdditive(AdditivePose.Pose, SourcePose.Pose);
			AdditivePose.Curve.ConvertToAdditive(SourcePose.Curve);
			Output = SourcePose;

			UE::Anim::Attributes::ConvertToAdditive(SourcePose.CustomAttributes, AdditivePose.CustomAttributes);

			FAnimationPoseData BaseAnimationPoseData(Output);
			const FAnimationPoseData AdditiveAnimationPoseData(AdditivePose);
			FAnimationRuntime::AccumulateAdditivePose(BaseAnimationPoseData, AdditiveAnimationPoseData, InternalBlendAlpha, AAT_LocalSpaceBase);
		}
	}
	else // if not relevant, skip to run control rig
		// this may cause issue if we have simulation node in the control rig that accumulates time
	{
		Output = SourcePose;
	}
}

void FAnimNode_TLLRN_ControlRigBase::ExecuteTLLRN_ControlRig(FPoseContext& InOutput)
{
	SCOPE_CYCLE_COUNTER(STAT_TLLRN_ControlRig_Evaluate);

	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		// Before we start modifying the TLLRN_RigHierarchy, we need to lock the rig to avoid
		// corrupted state
		FScopeLock LockRig(&TLLRN_ControlRig->GetEvaluateMutex());
		
		// temporarily give control rig access to the stack allocated attribute container
		// control rig may have rig units that can add/get attributes to/from this container
		UTLLRN_ControlRig::FAnimAttributeContainerPtrScope AttributeScope(TLLRN_ControlRig, InOutput.CustomAttributes);

		UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
		if(Hierarchy == nullptr)
		{
			return;
		}

		if(bEnablePoseAdapter)
		{
			const uint32 TopologyVersion = Hierarchy->GetTopologyVersion();
			if(PoseAdapter && PoseAdapter->LastTopologyVersion != TopologyVersion)
			{
				Hierarchy->UnlinkPoseAdapter();
				UpdateInputOutputMappingIfRequired(TLLRN_ControlRig, InOutput.Pose.GetBoneContainer());
				Hierarchy->LinkPoseAdapter(PoseAdapter);
			}
		}

		// first update input to the system
		UpdateInput(TLLRN_ControlRig, InOutput);
		
		if (bExecute)
		{
			TGuardValue<bool> ResetCurrentTransfromsAfterConstructionGuard(TLLRN_ControlRig->bResetCurrentTransformsAfterConstruction, true);
			
#if WITH_EDITOR
			if(Hierarchy->IsTracingChanges())
			{
				Hierarchy->StorePoseForTrace(TEXT("FAnimNode_TLLRN_ControlRigBase::BeforeEvaluate"));
			}
#endif

			// pick the event to run
			if(EventQueue.IsEmpty())
			{
				if(bClearEventQueueRequired)
				{
					TLLRN_ControlRig->SetEventQueue({FTLLRN_RigUnit_BeginExecution::EventName});
					bClearEventQueueRequired = false;
				}
			}
			else
			{
				TArray<FName> EventNames;
				Algo::Transform(EventQueue, EventNames, [](const FTLLRN_ControlRigAnimNodeEventName& InEventName) 
				{
					return InEventName.EventName;
				});
				TLLRN_ControlRig->SetEventQueue(EventNames);
				bClearEventQueueRequired = true;
			}

			if (TLLRN_ControlRig->IsAdditive())
			{
				TLLRN_ControlRig->ClearPoseBeforeBackwardsSolve();
			}

			// evaluate control rig
			UpdateGetAssetUserDataDelegate(TLLRN_ControlRig);
			TLLRN_ControlRig->Evaluate_AnyThread();

#if ENABLE_ANIM_DEBUG 
			// When Control Rig is at editing time (in CR editor), draw instructions are consumed by TLLRN_ControlRigEditMode, so we need to skip drawing here.
			bool bShowDebug = (CVarAnimNodeTLLRN_ControlRigDebug.GetValueOnAnyThread() == 1 && TLLRN_ControlRig->ExecutionType != ETLLRN_RigExecutionType::Editing);

			if (bShowDebug)
			{ 
				QueueTLLRN_ControlRigDrawInstructions(TLLRN_ControlRig, InOutput.AnimInstanceProxy);
			}
#endif

#if WITH_EDITOR
			if(Hierarchy->IsTracingChanges())
			{
				Hierarchy->StorePoseForTrace(TEXT("FAnimNode_TLLRN_ControlRigBase::AfterEvaluate"));
			}
#endif
		}

		// now update output
		UpdateOutput(TLLRN_ControlRig, InOutput);
	}
}

struct FTLLRN_ControlTLLRN_RigControlScope
{
	FTLLRN_ControlTLLRN_RigControlScope(UTLLRN_ControlRig* InTLLRN_ControlRig)
		: TLLRN_ControlRig(InTLLRN_ControlRig)
	{
		if (TLLRN_ControlRig.IsValid())
		{
			UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
			Hierarchy->ForEach<FTLLRN_RigControlElement>([this, Hierarchy](FTLLRN_RigControlElement* ControlElement) -> bool
			{
				ControlValues.Add(ControlElement->GetKey(), Hierarchy->GetControlValueByIndex(ControlElement->GetIndex()));
				return true; // continue
			});
		}
	}

	~FTLLRN_ControlTLLRN_RigControlScope()
	{
		if (TLLRN_ControlRig.IsValid())
		{
			UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
			for (const TPair<FTLLRN_RigElementKey, FTLLRN_RigControlValue>& Pair: ControlValues)
			{
				Hierarchy->SetControlValue(Pair.Key, Pair.Value);
			}
		}
	}

	TMap<FTLLRN_RigElementKey, FTLLRN_RigControlValue> ControlValues;
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;
};

void FAnimNode_TLLRN_ControlRigBase::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FAnimNode_CustomProperty::CacheBones_AnyThread(Context);
	Source.CacheBones(Context);

	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		// fill up node names
		const FBoneContainer& RequiredBones = Context.AnimInstanceProxy->GetRequiredBones();

		const uint16 BonesSerialNumber = RequiredBones.GetSerialNumber();

		// the construction event may create a set of bones that we can map to. let's run construction now.
		if (bExecute)
		{
			const bool bIsLODChange = !bTLLRN_ControlRigRequiresInitialization && (BonesSerialNumber != LastBonesSerialNumberForCacheBones);
		
			if(TLLRN_ControlRig->IsConstructionModeEnabled() ||
				(TLLRN_ControlRig->IsConstructionRequired() && (bTLLRN_ControlRigRequiresInitialization || bIsLODChange)))
			{
				UpdateGetAssetUserDataDelegate(TLLRN_ControlRig);
				TLLRN_ControlRig->Execute(FTLLRN_RigUnit_PrepareForExecution::EventName);
				bTLLRN_ControlRigRequiresInitialization = false;
			}
		}

		UpdateInputOutputMappingIfRequired(TLLRN_ControlRig, RequiredBones);

		if(bTLLRN_ControlRigRequiresInitialization)
		{
			if(bExecute)
			{
				// re-init only if this is the first run
				// and restore control values
				TLLRN_ControlRig->RequestInit();
				bTLLRN_ControlRigRequiresInitialization = false;
			}
		}
		
		LastBonesSerialNumberForCacheBones = BonesSerialNumber;

		if(bEnablePoseAdapter)
		{
			if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
			{
				Hierarchy->LinkPoseAdapter(PoseAdapter);
			}
		}
	}
}

void FAnimNode_TLLRN_ControlRigBase::UpdateInputOutputMappingIfRequired(UTLLRN_ControlRig* InTLLRN_ControlRig, const FBoneContainer& InRequiredBones)
{
	UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig->GetHierarchy();
	if(Hierarchy == nullptr)
	{
		return;
	}

	if(bEnablePoseAdapter)
	{
		Hierarchy->UnlinkPoseAdapter();

		if(!PoseAdapter.IsValid())
		{
			return;
		}

		PoseAdapter->ParentPoseIndices.Reset();
		PoseAdapter->RequiresHierarchyForSpaceConversion.Reset();
		PoseAdapter->ElementIndexToPoseIndex.Reset();

		const int32 NumBonesInPose = InRequiredBones.GetCompactPoseNumBones();

		PoseAdapter->GlobalPose.Reset();
		PoseAdapter->LocalPose.Reset();
		PoseAdapter->ParentPoseIndices.Reserve(NumBonesInPose);
		PoseAdapter->RequiresHierarchyForSpaceConversion.Reserve(NumBonesInPose);
		PoseAdapter->GlobalPose.AddDefaulted(NumBonesInPose);
		PoseAdapter->LocalPose.AddDefaulted(NumBonesInPose);
		PoseAdapter->HierarchyCurveLookup.Reset();

		PoseAdapter->bTransferInLocalSpace = !(bTransferPoseInGlobalSpace || NodeMappingContainer.IsValid());

		for(int32 Index = 0; Index < NumBonesInPose; Index++)
		{
			PoseAdapter->ParentPoseIndices.Add(INDEX_NONE);

			const FCompactPoseBoneIndex ParentBoneIndex = InRequiredBones.GetParentBoneIndex(FCompactPoseBoneIndex(Index));
			if(ParentBoneIndex.IsValid())
			{
				PoseAdapter->ParentPoseIndices[Index] = ParentBoneIndex.GetInt();
			}
			PoseAdapter->RequiresHierarchyForSpaceConversion.Add(false);
		}
		PoseAdapter->UpdateDirtyStates();

		TArray<int32> MappedBoneElementIndices;
		if(InRequiredBones.IsValid())
		{
			PoseAdapter->ElementIndexToPoseIndex.Reserve(NumBonesInPose);
			PoseAdapter->PoseIndexToElementIndex.Reserve(NumBonesInPose);

			const FReferenceSkeleton* RefSkeleton = &InRequiredBones.GetReferenceSkeleton();
			if(const USkeleton* Skeleton = InRequiredBones.GetSkeletonAsset())
			{
				RefSkeleton = &Skeleton->GetReferenceSkeleton();
			}			

			// @todo: thread-safe? probably not in editor, but it may not be a big issue in editor
			if (NodeMappingContainer.IsValid())
			{
				// get target to source mapping table - this is reversed mapping table
				TMap<FName, FName> TargetToSourceMappingTable;
				NodeMappingContainer->GetTargetToSourceMappingTable(TargetToSourceMappingTable);

				// now fill up node name
				for (uint16 Index = 0; Index < NumBonesInPose; ++Index)
				{
					// get bone name, and find reverse mapping
					const FSkeletonPoseBoneIndex BoneIndex = InRequiredBones.GetSkeletonPoseIndexFromCompactPoseIndex(FCompactPoseBoneIndex(Index));
					if(BoneIndex.IsValid())
					{
						FName TargetNodeName = RefSkeleton->GetBoneName(BoneIndex.GetInt());
						FName* SourceName = TargetToSourceMappingTable.Find(TargetNodeName);
						if (SourceName)
						{
							const int32 ElementIndex = Hierarchy->GetIndex({*SourceName, ETLLRN_RigElementType::Bone});
							if(ElementIndex != INDEX_NONE)
							{
								MappedBoneElementIndices.Add(ElementIndex);
								PoseAdapter->ElementIndexToPoseIndex.Add(static_cast<uint16>(ElementIndex), Index);
								PoseAdapter->PoseIndexToElementIndex.Add(ElementIndex);
								PoseAdapter->LocalPose[Index] = Hierarchy->GetLocalTransform(ElementIndex);
								PoseAdapter->GlobalPose[Index] = Hierarchy->GetGlobalTransform(ElementIndex);
								continue;
							}
						}
					}
					PoseAdapter->PoseIndexToElementIndex.Add(INDEX_NONE);
				}
			}
			else
			{
				TArray<FName> NodeNames;
				TArray<FNodeItem> NodeItems;
				InTLLRN_ControlRig->GetMappableNodeData(NodeNames, NodeItems);

				// even if not mapped, we map only node that exists in the controlrig
				for (uint16 Index = 0; Index < NumBonesInPose; ++Index)
				{
					const FSkeletonPoseBoneIndex BoneIndex = InRequiredBones.GetSkeletonPoseIndexFromCompactPoseIndex(FCompactPoseBoneIndex(Index));
					if(BoneIndex.IsValid())
					{
						const FName& BoneName = RefSkeleton->GetBoneName(BoneIndex.GetInt());
						if (NodeNames.Contains(BoneName))
						{
							const int32 ElementIndex = Hierarchy->GetIndex({BoneName, ETLLRN_RigElementType::Bone});
							if(ElementIndex != INDEX_NONE)
							{
								MappedBoneElementIndices.Add(ElementIndex);
								PoseAdapter->ElementIndexToPoseIndex.Add(static_cast<uint16>(ElementIndex), Index);
								PoseAdapter->PoseIndexToElementIndex.Add(ElementIndex);
								PoseAdapter->LocalPose[Index] = Hierarchy->GetLocalTransform(ElementIndex);
								PoseAdapter->GlobalPose[Index] = Hierarchy->GetGlobalTransform(ElementIndex);
								continue;
							}
						}
					}
					PoseAdapter->PoseIndexToElementIndex.Add(INDEX_NONE);
				}
			}

			// once we know all of the bones we are going to transfer - we can check if any of these bones has a different parenting
			// relationship in the skeleton used in the anim graph vs the hierarchy in the rig. in that case we have to transfer in global
			if(PoseAdapter->bTransferInLocalSpace)
			{
				for(const int32& BoneElementIndex : MappedBoneElementIndices)
				{
					const int32 HierarchyParentIndex = Hierarchy->GetFirstParent(BoneElementIndex);
					const int16 PoseIndex = PoseAdapter->ElementIndexToPoseIndex.FindChecked(BoneElementIndex); 
					const FCompactPoseBoneIndex CompactPoseParentIndex(PoseAdapter->ParentPoseIndices[PoseIndex]);

					FName HierarchyParentName(NAME_None);
					FName PoseParentName(NAME_None);
				
					if(HierarchyParentIndex != INDEX_NONE)
					{
						HierarchyParentName = Hierarchy->Get(HierarchyParentIndex)->GetFName();
					}
					if(CompactPoseParentIndex.IsValid())
					{
						const FSkeletonPoseBoneIndex SkeletonIndex = InRequiredBones.GetSkeletonPoseIndexFromCompactPoseIndex(CompactPoseParentIndex);
						PoseParentName = RefSkeleton->GetBoneName(SkeletonIndex.GetInt());
					}

					if(HierarchyParentName.IsEqual(PoseParentName, ENameCase::CaseSensitive))
					{
						continue;
					}
				
					PoseAdapter->RequiresHierarchyForSpaceConversion[PoseIndex] = true;
					check(PoseAdapter->PoseIndexToElementIndex[PoseIndex] != INDEX_NONE);
					PoseAdapter->bTransferInLocalSpace = false;
				}
			}

			// only reset the full pose if we are not mapping all bones
			const TArray<FTLLRN_RigBaseElement*>& HierarchyBones = Hierarchy->GetBonesFast();
			const bool bMapsAllBones = MappedBoneElementIndices.Num() == HierarchyBones.Num();
			PoseAdapter->BonesToResetToInitial.Reset();
			PoseAdapter->bRequiresResetPoseToInitial = bResetInputPoseToInitial && !bMapsAllBones;

			if(PoseAdapter->bRequiresResetPoseToInitial)
			{
				PoseAdapter->BonesToResetToInitial.Reserve(HierarchyBones.Num() - MappedBoneElementIndices.Num());

				// bone is mapped stores sub indices (bone index within the list of bones)
				TArray<bool> BoneIsMapped;
				BoneIsMapped.AddZeroed(HierarchyBones.Num());
				for(const int32& MappedTransformIndex : MappedBoneElementIndices)
				{
					const FTLLRN_RigBaseElement* MappedElement = Hierarchy->Get(MappedTransformIndex);
					check(MappedElement);
					BoneIsMapped[MappedElement->GetSubIndex()] = true;
				}

				// when we want to know which bones to reset we want to convert back to a global index
				for(int32 UnmappedBoneIndex = 0; UnmappedBoneIndex < BoneIsMapped.Num(); UnmappedBoneIndex++)
				{
					if(!BoneIsMapped[UnmappedBoneIndex])
					{
						PoseAdapter->BonesToResetToInitial.Add(HierarchyBones[UnmappedBoneIndex]->GetIndex());
					}
				}
			}
		}
	}
	else
	{
		TLLRN_ControlTLLRN_RigBoneInputMappingByIndex.Reset();
		TLLRN_ControlTLLRN_RigBoneOutputMappingByIndex.Reset();
		TLLRN_ControlTLLRN_RigCurveMappingByIndex.Reset();
		TLLRN_ControlTLLRN_RigBoneInputMappingByName.Reset();
		TLLRN_ControlTLLRN_RigBoneOutputMappingByName.Reset();
		TLLRN_ControlTLLRN_RigCurveMappingByName.Reset();

		if(InRequiredBones.IsValid())
		{
			const TArray<FBoneIndexType>& RequiredBonesArray = InRequiredBones.GetBoneIndicesArray();
			const int32 NumBones = RequiredBonesArray.Num();

			const FReferenceSkeleton& RefSkeleton = InRequiredBones.GetReferenceSkeleton();

			// @todo: thread-safe? probably not in editor, but it may not be a big issue in editor
			if (NodeMappingContainer.IsValid())
			{
				// get target to source mapping table - this is reversed mapping table
				TMap<FName, FName> TargetToSourceMappingTable;
				NodeMappingContainer->GetTargetToSourceMappingTable(TargetToSourceMappingTable);

				// now fill up node name
				for (uint16 Index = 0; Index < NumBones; ++Index)
				{
					// get bone name, and find reverse mapping
					FName TargetNodeName = RefSkeleton.GetBoneName(RequiredBonesArray[Index]);
					FName* SourceName = TargetToSourceMappingTable.Find(TargetNodeName);
					if (SourceName)
					{
						TLLRN_ControlTLLRN_RigBoneInputMappingByName.Add(*SourceName, Index);
					}
				}
			}
			else
			{
				TArray<FName> NodeNames;
				TArray<FNodeItem> NodeItems;
				InTLLRN_ControlRig->GetMappableNodeData(NodeNames, NodeItems);

				// even if not mapped, we map only node that exists in the controlrig
				for (uint16 Index = 0; Index < NumBones; ++Index)
				{
					const FName& BoneName = RefSkeleton.GetBoneName(RequiredBonesArray[Index]);
					if (NodeNames.Contains(BoneName))
					{
						TLLRN_ControlTLLRN_RigBoneInputMappingByName.Add(BoneName, Index);
					}
				}
			}

			auto UpdatingMappingFromSpecificTransferList = [] (
				TArray<FBoneReference>& InTransferList,
				const TWeakObjectPtr<UNodeMappingContainer>& InMappingContainer,
				const FBoneContainer& InRequiredBones,
				const FReferenceSkeleton& InRefSkeleton,
				const TArray<FBoneIndexType>& InRequiredBonesArray,
				const UTLLRN_ControlRig* InTLLRN_ControlRig,
				TMap<FName, uint16>& OutMapping
			) {
				OutMapping.Reset();
				
				if (InMappingContainer.IsValid())
				{
					// get target to source mapping table - this is reversed mapping table
					TMap<FName, FName> TargetToSourceMappingTable;
					InMappingContainer->GetTargetToSourceMappingTable(TargetToSourceMappingTable);

					for(FBoneReference& InputBoneToTransfer : InTransferList)
					{
						if(!InputBoneToTransfer.Initialize(InRequiredBones))
						{
							continue;
						}
						const FName TargetNodeName = InRefSkeleton.GetBoneName(InputBoneToTransfer.BoneIndex);
						if (const FName* SourceName = TargetToSourceMappingTable.Find(TargetNodeName))
						{
							OutMapping.Add(*SourceName, InputBoneToTransfer.BoneIndex);
						}
					}
				}
				else
				{
					TArray<FName> NodeNames;
					TArray<FNodeItem> NodeItems;
					InTLLRN_ControlRig->GetMappableNodeData(NodeNames, NodeItems);

					for(FBoneReference& InputBoneToTransfer : InTransferList)
					{
						if(!InputBoneToTransfer.Initialize(InRequiredBones))
						{
							continue;
						}
						if (InRequiredBonesArray.IsValidIndex(InputBoneToTransfer.BoneIndex))
						{
							const FName& BoneName = InRefSkeleton.GetBoneName(InRequiredBonesArray[InputBoneToTransfer.BoneIndex]);
							if (NodeNames.Contains(BoneName))
							{
								OutMapping.Add(BoneName, InputBoneToTransfer.BoneIndex);
							}
						}
					}
				}
			};
			
			if(!InputBonesToTransfer.IsEmpty())
			{
				TLLRN_ControlTLLRN_RigBoneOutputMappingByName = TLLRN_ControlTLLRN_RigBoneInputMappingByName;

				UpdatingMappingFromSpecificTransferList(
					InputBonesToTransfer,
					NodeMappingContainer,
					InRequiredBones,
					RefSkeleton,
					RequiredBonesArray,
					InTLLRN_ControlRig,
					TLLRN_ControlTLLRN_RigBoneInputMappingByName);
			}

			if(!OutputBonesToTransfer.IsEmpty())
			{
				UpdatingMappingFromSpecificTransferList(
					OutputBonesToTransfer,
					NodeMappingContainer,
					InRequiredBones,
					RefSkeleton,
					RequiredBonesArray,
					InTLLRN_ControlRig,
					TLLRN_ControlTLLRN_RigBoneOutputMappingByName);
			}

			// check if we can switch the bones to an index based mapping.
			// we can only do that if there is no node mapping container set.
			if(!NodeMappingContainer.IsValid())
			{
				for(int32 InputOutput = 0; InputOutput < 2; InputOutput++)
				{
					bool bIsMappingByIndex = true;
					TMap<FName, uint16>& NameBasedMapping = InputOutput == 0 ? TLLRN_ControlTLLRN_RigBoneInputMappingByName : TLLRN_ControlTLLRN_RigBoneOutputMappingByName;
					if(NameBasedMapping.IsEmpty())
					{
						continue;
					}
					
					TArray<TPair<uint16, uint16>>& IndexBasedMapping = InputOutput == 0 ? TLLRN_ControlTLLRN_RigBoneInputMappingByIndex : TLLRN_ControlTLLRN_RigBoneOutputMappingByIndex;
					
					for (auto Iter = NameBasedMapping.CreateConstIterator(); Iter; ++Iter)
					{
						const uint16 SkeletonIndex = Iter.Value();
						const int32 TLLRN_ControlRigIndex = Hierarchy->GetIndex(FTLLRN_RigElementKey(Iter.Key(), ETLLRN_RigElementType::Bone));
						if(TLLRN_ControlRigIndex != INDEX_NONE)
						{
							IndexBasedMapping.Add(TPair<uint16, uint16>((uint16)TLLRN_ControlRigIndex, SkeletonIndex));
						}
						else
						{
							bIsMappingByIndex = false;
						}
					}

					if(bIsMappingByIndex)
					{
						NameBasedMapping.Reset();
					}
					else
					{
						IndexBasedMapping.Reset();
					}
				}
			}
		}		
	}
}

UClass* FAnimNode_TLLRN_ControlRigBase::GetTargetClass() const
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		return TLLRN_ControlRig->GetClass();
	}

	return nullptr;
}

void FAnimNode_TLLRN_ControlRigBase::QueueTLLRN_ControlRigDrawInstructions(UTLLRN_ControlRig* TLLRN_ControlRig, FAnimInstanceProxy* Proxy) const
{
	ensure(TLLRN_ControlRig);
	ensure(Proxy);

	if (TLLRN_ControlRig && Proxy)
	{
		for (const FRigVMDrawInstruction& Instruction : TLLRN_ControlRig->GetDrawInterface())
		{
			if (!Instruction.IsValid())
			{
				continue;
			}

			FTransform InstructionTransform = Instruction.Transform * Proxy->GetComponentTransform();
			switch (Instruction.PrimitiveType)
			{
				case ERigVMDrawSettings::Points:
				{
					for (const FVector& Point : Instruction.Positions)
					{
						Proxy->AnimDrawDebugPoint(InstructionTransform.TransformPosition(Point), Instruction.Thickness, Instruction.Color.ToFColor(true), false, -1.f, SDPG_Foreground);
					}
					break;
				}
				case ERigVMDrawSettings::Lines:
				{
					const TArray<FVector>& Points = Instruction.Positions;

					for (int32 PointIndex = 0; PointIndex < Points.Num() - 1; PointIndex += 2)
					{
						Proxy->AnimDrawDebugLine(InstructionTransform.TransformPosition(Points[PointIndex]), InstructionTransform.TransformPosition(Points[PointIndex + 1]), Instruction.Color.ToFColor(true), false, -1.f, Instruction.Thickness, SDPG_Foreground);
					}
					break;
				}
				case ERigVMDrawSettings::LineStrip:
				{
					const TArray<FVector>& Points = Instruction.Positions;

					for (int32 PointIndex = 0; PointIndex < Points.Num() - 1; PointIndex++)
					{
						Proxy->AnimDrawDebugLine(InstructionTransform.TransformPosition(Points[PointIndex]), InstructionTransform.TransformPosition(Points[PointIndex + 1]), Instruction.Color.ToFColor(true), false, -1.f, Instruction.Thickness, SDPG_Foreground);
					}
					break;
				}

				case ERigVMDrawSettings::DynamicMesh:
				{
					// TODO: Add support for this if anyone is actually using it. Currently it is only defined and referenced in an unused API, DrawCone in Control Rig.
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}
}

void FAnimNode_TLLRN_ControlRigBase::UpdateGetAssetUserDataDelegate(UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	if (!IsInGameThread())
	{
		return;
	}
	
	if(GetAssetUserData().IsEmpty() || !WeakAnimInstanceObject.IsValid())
	{
		InTLLRN_ControlRig->GetExternalAssetUserDataDelegate.Unbind();
		return;
	}
	
	// due to the re-instancing of the anim nodes we have to set this up for every run
	// since the delegate may go stale quickly. to guard against destroyed anim nodes
	// we'll rely on the anim instance to provide an indication if the memory is still valid. 
	TWeakObjectPtr<const UAnimInstance> LocalWeakAnimInstance = WeakAnimInstanceObject;
	InTLLRN_ControlRig->GetExternalAssetUserDataDelegate = UTLLRN_ControlRig::FGetExternalAssetUserData::CreateLambda([InTLLRN_ControlRig, LocalWeakAnimInstance, this]
	{
		if(LocalWeakAnimInstance.IsValid())
		{
			return this->GetAssetUserData();
		}
		if(IsValid(InTLLRN_ControlRig))
		{
			InTLLRN_ControlRig->GetExternalAssetUserDataDelegate.Unbind();
		}
		return TArray<TObjectPtr<UAssetUserData>>();
	});
}


