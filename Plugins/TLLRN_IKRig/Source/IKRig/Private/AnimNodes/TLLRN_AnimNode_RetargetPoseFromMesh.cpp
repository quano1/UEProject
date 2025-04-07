// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNodes/TLLRN_AnimNode_RetargetPoseFromMesh.h"

#include "Animation/AnimCurveUtils.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Retargeter/TLLRN_IKRetargetOps.h"
#include "Retargeter/RetargetOps/TLLRN_CurveRemapOp.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_AnimNode_RetargetPoseFromMesh)


void FTLLRN_AnimNode_RetargetPoseFromMesh::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
    FAnimNode_Base::Initialize_AnyThread(Context);

	// Initial update of the node, so we dont have a frame-delay on setup
	GetEvaluateGraphExposedInputs().Execute(Context);

	// create the processor
	CreateRetargetProcessorIfNeeded(Context.AnimInstanceProxy->GetSkelMeshComponent());
}

void FTLLRN_AnimNode_RetargetPoseFromMesh::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)

	const FBoneContainer& RequiredBones = Context.AnimInstanceProxy->GetRequiredBones();
	if (!RequiredBones.IsValid())
	{
		return;
	}
	
	if (!Context.AnimInstanceProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset())
	{
		return;
	}

	// rebuild mapping
	RequiredToTargetBoneMapping.Reset();

	const FReferenceSkeleton& RefSkeleton = RequiredBones.GetReferenceSkeleton();
	const FReferenceSkeleton& TargetSkeleton = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset()->GetRefSkeleton();
	const TArray<FBoneIndexType>& RequiredBonesArray = RequiredBones.GetBoneIndicesArray();
	for (int32 Index = 0; Index < RequiredBonesArray.Num(); ++Index)
	{
		const FBoneIndexType ReqBoneIndex = RequiredBonesArray[Index]; 
		if (ReqBoneIndex != INDEX_NONE)
		{
			const FName Name = RefSkeleton.GetBoneName(ReqBoneIndex);
			const int32 TargetBoneIndex = TargetSkeleton.FindBoneIndex(Name);
			if (TargetBoneIndex != INDEX_NONE)
			{
				// store require bone to target bone indices
				RequiredToTargetBoneMapping.Emplace(Index, TargetBoneIndex);
			}
		}
	}
}

void FTLLRN_AnimNode_RetargetPoseFromMesh::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	FAnimNode_Base::Update_AnyThread(Context);
    // this introduces a frame of latency in setting the pin-driven source component,
    // but we cannot do the work to extract transforms on a worker thread as it is not thread safe.
    GetEvaluateGraphExposedInputs().Execute(Context);
	DeltaTime += Context.GetDeltaTime();
}

void FTLLRN_AnimNode_RetargetPoseFromMesh::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)

	SCOPE_CYCLE_COUNTER(STAT_IKRetarget);

	if (!(TLLRN_IKRetargeterAsset && Processor && SourceMeshComponent.IsValid() && IsLODEnabled(Output.AnimInstanceProxy)))
	{
		Output.ResetToRefPose();
		return;
	}

	// it's possible in editor to have anim instances initialized before PreUpdate() is called
	// which results in trying to run the retargeter without an source pose to copy from
	const bool bSourcePoseCopied = !SourceMeshComponentSpaceBoneTransforms.IsEmpty();

	// ensure processor was initialized with the currently used assets (source/target meshes and retarget asset)
	// if processor is not ready this tick, it will be next tick as this state will trigger re-initialization
	const TObjectPtr<USkeletalMesh> SourceMesh = SourceMeshComponent->GetSkeletalMeshAsset();
	const TObjectPtr<USkeletalMesh> TargetMesh = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset();
	const bool bIsProcessorReady = Processor->WasInitializedWithTheseAssets(SourceMesh, TargetMesh, TLLRN_IKRetargeterAsset);

	// if not ready to run, skip retarget and output the ref pose
	if (!(bIsProcessorReady && bSourcePoseCopied))
	{
		Output.ResetToRefPose();
		return;
	}

	#if WITH_EDITOR
	if (GIsEditor)
	{
		// live preview source asset settings in the retarget, editor only
		// NOTE: this copies goal targets as well, but these are overwritten by IK chain goals
		Processor->CopyIKRigSettingsFromAsset();
	}
	#endif

	// LOD off the IK pass
	bool bForceIKOff = LODThresholdForIK != INDEX_NONE && Output.AnimInstanceProxy->GetLODLevel() > LODThresholdForIK;
	FTLLRN_RetargetProfile RetargetProfileToUse = GetMergedRetargetProfile(bForceIKOff);

	// run the retargeter
	const TArray<FTransform>& RetargetedPose = Processor->RunRetargeter(
		SourceMeshComponentSpaceBoneTransforms,
		SpeedValuesFromCurves,
		DeltaTime,
		RetargetProfileToUse);
	DeltaTime = 0.0f;
	
	// convert pose to local space and apply to output
	FCSPose<FCompactPose> ComponentPose;
	ComponentPose.InitPose(Output.Pose);
	const FCompactPose& CompactPose = ComponentPose.GetPose();
	for (const TPair<int32, int32>& Pair : RequiredToTargetBoneMapping)
	{
		const FCompactPoseBoneIndex CompactBoneIndex(Pair.Key);
		if (CompactPose.IsValidIndex(CompactBoneIndex))
		{
			const int32 TargetBoneIndex = Pair.Value;
			ComponentPose.SetComponentSpaceTransform(CompactBoneIndex, RetargetedPose[TargetBoneIndex]);
		}
	}

	// convert to local space
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(ComponentPose, Output.Pose);

	// once converted back to local space, we copy scale values back
	// (retargeter strips scale values and deals with translation only in component space)
	const TArray<FTransform>& RefPose = TargetMesh->GetRefSkeleton().GetRefBonePose();
	for (const TPair<int32, int32>& Pair : RequiredToTargetBoneMapping)
	{
		const FCompactPoseBoneIndex CompactBoneIndex(Pair.Key);
		if (Output.Pose.IsValidIndex(CompactBoneIndex))
		{
			Output.Pose[CompactBoneIndex].SetScale3D(RefPose[Pair.Value].GetScale3D());
		}
	}

	// copy and/or remap curves from the source to the target skeletal mesh
	CopyAndRemapCurvesFromSourceToTarget(Output.Curve);
}

void FTLLRN_AnimNode_RetargetPoseFromMesh::PreUpdate(const UAnimInstance* InAnimInstance)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(PreUpdate)
	
	if (!TLLRN_IKRetargeterAsset)
	{
		return;
	}
	
	if (!Processor)
	{
		Processor = NewObject<UTLLRN_IKRetargetProcessor>(InAnimInstance->GetOwningComponent());	
	}

	const TObjectPtr<USkeletalMeshComponent> TargetMeshComponent = InAnimInstance->GetSkelMeshComponent();
	if (EnsureProcessorIsInitialized(TargetMeshComponent))
	{
		CopyBoneTransformsFromSource(TargetMeshComponent);

		if(bCopyCurves)
		{
			if (const UAnimInstance* SourceAnimInstance = SourceMeshComponent->GetAnimInstance())
			{
				// Potential optimization/tradeoff: If we stored the curve results on the mesh component in non-editor scenarios, this would be
				// much faster (but take more memory). As it is, we need to translate the map stored on the anim instance.
				const TMap<FName, float>& AnimCurveList = SourceAnimInstance->GetAnimationCurveList(EAnimCurveType::AttributeCurve);
				UE::Anim::FCurveUtils::BuildUnsorted(SourceCurves, AnimCurveList);
			}
			else
			{
				SourceCurves.Empty();
			}
			
			UpdateSpeedValuesFromCurves();
		}
	}
}

void FTLLRN_AnimNode_RetargetPoseFromMesh::CreateRetargetProcessorIfNeeded(UObject* Outer)
{
	if (!Processor && IsInGameThread())
	{
		Processor = NewObject<UTLLRN_IKRetargetProcessor>(Outer);	
	}
}

void FTLLRN_AnimNode_RetargetPoseFromMesh::CopyAndRemapCurvesFromSourceToTarget(FBlendedCurve& OutputCurves) const
{
	if (!bCopyCurves)
	{
		return;
	}
	
	if (!(Processor && Processor->IsInitialized()))
	{
		return;
	}

	// copy curves over to same name on target (if it exists)
	OutputCurves.CopyFrom(SourceCurves);
	
	// now remap curves that have different names using the TLLRN_CurveRemapOp data (if there are any)
	FBlendedCurve RemapedCurves;
	const TArray<TObjectPtr<UTLLRN_RetargetOpBase>>& RetargetOps = Processor->GetRetargetOps();
	for (UTLLRN_RetargetOpBase* RetargetOp : RetargetOps)
	{
		UTLLRN_CurveRemapOp* TLLRN_CurveRemapOp = Cast<UTLLRN_CurveRemapOp>(RetargetOp);
		if (!TLLRN_CurveRemapOp)
		{
			continue;
		}

		if (!TLLRN_CurveRemapOp->bIsEnabled)
		{
			continue;
		}
		
		for (const FTLLRN_CurveRemapPair& CurveToRemap : TLLRN_CurveRemapOp->CurvesToRemap)
		{
			bool bOutIsValid = false;
			const float SourceValue = SourceCurves.Get(CurveToRemap.SourceCurve, bOutIsValid);
			if (bOutIsValid)
			{
				RemapedCurves.Add(CurveToRemap.TargetCurve, SourceValue);
			}
		}
	}
	OutputCurves.Combine(RemapedCurves);
}

void FTLLRN_AnimNode_RetargetPoseFromMesh::UpdateSpeedValuesFromCurves()
{
	SpeedValuesFromCurves.Reset();
	
	if (!TLLRN_IKRetargeterAsset)
	{
		return;
	}

	const TArray<TObjectPtr<UTLLRN_RetargetChainSettings>>& ChainSettings = TLLRN_IKRetargeterAsset->GetAllChainSettings();
	for (const TObjectPtr<UTLLRN_RetargetChainSettings>& ChainSetting : ChainSettings)
	{
		FName CurveName = ChainSetting->Settings.SpeedPlanting.SpeedCurveName;
		if (CurveName == NAME_None)
		{
			continue;
		}

		// value will be negative if the curve was not found (retargeter ignores negative speeds)
		bool bCurveValid = false;
		float CurveValue = SourceCurves.Get(CurveName, bCurveValid, -1.0f);
		if (bCurveValid)
		{
			SpeedValuesFromCurves.Add(CurveName, CurveValue);
		}
	}
}

UTLLRN_IKRetargetProcessor* FTLLRN_AnimNode_RetargetPoseFromMesh::GetRetargetProcessor() const
{
	return Processor;
}

bool FTLLRN_AnimNode_RetargetPoseFromMesh::EnsureProcessorIsInitialized(const TObjectPtr<USkeletalMeshComponent> TargetMeshComponent)
{
	// has user supplied a retargeter asset?
	if (!TLLRN_IKRetargeterAsset)
	{
		return false;
	}
	
	// if user hasn't explicitly connected a source mesh, optionally use the parent mesh component (if there is one) 
	if (!SourceMeshComponent.IsValid() && bUseAttachedParent)
	{
		// Walk up the attachment chain until we find a skeletal mesh component
		USkeletalMeshComponent* ParentComponent = nullptr;
		for (USceneComponent* AttachParentComp = TargetMeshComponent->GetAttachParent(); AttachParentComp != nullptr; AttachParentComp = AttachParentComp->GetAttachParent())
		{
			ParentComponent = Cast<USkeletalMeshComponent>(AttachParentComp);
			if (ParentComponent)
			{
				break;
			}
		}

		if (ParentComponent)
		{
			SourceMeshComponent = ParentComponent;
		}
	}
	
	// has a source mesh been plugged in or found?
	if (!SourceMeshComponent.IsValid())
	{
		return false; // can't do anything if we don't have a source mesh component
	}

	// check that both a source and target mesh exist
	const TObjectPtr<USkeletalMesh> SourceMesh = SourceMeshComponent->GetSkeletalMeshAsset();
	const TObjectPtr<USkeletalMesh> TargetMesh = TargetMeshComponent->GetSkeletalMeshAsset();
	if (!SourceMesh || !TargetMesh)
	{
		return false; // cannot initialize if components are missing skeletal mesh references
	}
	
	// try initializing the processor
	if (!Processor->WasInitializedWithTheseAssets(SourceMesh, TargetMesh, TLLRN_IKRetargeterAsset))
	{
		// initialize retarget processor with source and target skeletal meshes
		// (asset is passed in as outer UObject for new UTLLRN_IKRigProcessor)
		bool bForceIKOff = false;
		FTLLRN_RetargetProfile RetargetProfileToUse = GetMergedRetargetProfile(bForceIKOff);
		Processor->Initialize(SourceMesh, TargetMesh, TLLRN_IKRetargeterAsset, RetargetProfileToUse);
	}

	return Processor->IsInitialized();
}

void FTLLRN_AnimNode_RetargetPoseFromMesh::CopyBoneTransformsFromSource(USkeletalMeshComponent* TargetMeshComponent)
{
	// get the mesh component to use as the source
	const TObjectPtr<USkeletalMeshComponent> ComponentToCopyFrom =  GetComponentToCopyPoseFrom();

	// this should not happen as we're guaranteed to be initialized at this stage
	// but just in case component is lost after initialization, we avoid a crash
	if (!ComponentToCopyFrom)
	{
		return; 
	}
	
	// skip copying pose when component is no longer ticking
	if (!ComponentToCopyFrom->IsRegistered())
	{
		return; 
	}
	
	const bool bUROInSync =
		ComponentToCopyFrom->ShouldUseUpdateRateOptimizations() &&
		ComponentToCopyFrom->AnimUpdateRateParams != nullptr &&
		SourceMeshComponent->AnimUpdateRateParams == TargetMeshComponent->AnimUpdateRateParams;
	const bool bUsingExternalInterpolation = ComponentToCopyFrom->IsUsingExternalInterpolation();
	const TArray<FTransform>& CachedComponentSpaceTransforms = ComponentToCopyFrom->GetCachedComponentSpaceTransforms();
	const bool bArraySizesMatch = CachedComponentSpaceTransforms.Num() == ComponentToCopyFrom->GetComponentSpaceTransforms().Num();

	// copy source array from the appropriate location
	SourceMeshComponentSpaceBoneTransforms.Reset();
	if ((bUROInSync || bUsingExternalInterpolation) && bArraySizesMatch)
	{
		SourceMeshComponentSpaceBoneTransforms.Append(CachedComponentSpaceTransforms); // copy from source's cache
	}
	else
	{
		SourceMeshComponentSpaceBoneTransforms.Append(ComponentToCopyFrom->GetComponentSpaceTransforms()); // copy directly
	}
	
	// strip all scale out of the pose values, the translation of a component-space pose has incorporated scale values
	for (FTransform& Transform : SourceMeshComponentSpaceBoneTransforms)
	{
		Transform.SetScale3D(FVector::OneVector);
	}
}

TObjectPtr<USkeletalMeshComponent> FTLLRN_AnimNode_RetargetPoseFromMesh::GetComponentToCopyPoseFrom() const
{
	// if our source is running under leader-pose, then get bone data from there
	if (SourceMeshComponent.IsValid())
	{
		if(USkeletalMeshComponent* LeaderPoseComponent = Cast<USkeletalMeshComponent>(SourceMeshComponent->LeaderPoseComponent.Get()))
		{
			return LeaderPoseComponent;
		}
	}
	
	return SourceMeshComponent.Get();
}

FTLLRN_RetargetProfile FTLLRN_AnimNode_RetargetPoseFromMesh::GetMergedRetargetProfile(bool bForceIKOff) const
{
	// collect settings to retarget with starting with asset settings and overriding with custom profile
	FTLLRN_RetargetProfile Profile;
	TLLRN_IKRetargeterAsset->FillProfileWithAssetSettings(Profile);
	// load custom profile plugged into the anim node
	Profile.MergeWithOtherProfile(CustomRetargetProfile);
	// force all IK off (skips IK solve)
	if (bForceIKOff)
	{
		Profile.GlobalSettings.bEnableIK = false;
	}

	return Profile;
}

