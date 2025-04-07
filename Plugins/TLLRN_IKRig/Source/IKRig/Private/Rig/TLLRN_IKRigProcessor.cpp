// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rig/TLLRN_IKRigProcessor.h"
#include "Rig/TLLRN_IKRigDefinition.h"
#include "Rig/Solvers/TLLRN_IKRigSolver.h"

#include "Engine/SkeletalMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRigProcessor)

#define LOCTEXT_NAMESPACE "TLLRN_IKRigProcessor"

UTLLRN_IKRigProcessor::UTLLRN_IKRigProcessor()
{
	const FName LogName = FName("IKRig_",GetUniqueID());
	Log.SetLogTarget(LogName, LOCTEXT("IKRigLogName", "IK Rig Log"));
}

void UTLLRN_IKRigProcessor::Initialize(
	const UTLLRN_IKRigDefinition* InRigAsset,
	const USkeletalMesh* SkeletalMesh,
	const TArray<FName>& ExcludedGoals)
{
	// we instantiate UObjects here which MUST be done on game thread...
	check(IsInGameThread());
	check(InRigAsset);
	
	bInitialized = false;

	// bail out if we've already tried initializing with this exact version of the rig asset
	if (bTriedToInitialize)
	{
		return; // don't keep spamming
	}

	// ok, lets try to initialize
	bTriedToInitialize = true;

	// copy skeleton data from the actual skeleton we want to run on
	Skeleton.SetInputSkeleton(SkeletalMesh, InRigAsset->GetSkeleton().ExcludedBones);
	
	if (InRigAsset->GetSkeleton().BoneNames.IsEmpty())
	{
		Log.LogError(FText::Format(
			LOCTEXT("NoSkeleton", "Trying to initialize IK Rig, '{0}' that has no skeleton."),
			FText::FromString(InRigAsset->GetName())));
		return;
	}

	if (!UTLLRN_IKRigProcessor::IsIKRigCompatibleWithSkeleton(InRigAsset, SkeletalMesh, &Log))
	{
		Log.LogError(FText::Format(
			LOCTEXT("SkeletonMissingRequiredBones", "Trying to initialize IKRig, '{0}' with a Skeleton that is missing required bones. See prior warnings."),
			FText::FromString(InRigAsset->GetName())));
		return;
	}
	
	// initialize goals based on source asset
	GoalContainer.Empty();
	const TArray<UTLLRN_IKRigEffectorGoal*>& EffectorGoals = InRigAsset->GetGoalArray();
	for (const UTLLRN_IKRigEffectorGoal* EffectorGoal : EffectorGoals)
	{
		// skip excluded goals
		if (ExcludedGoals.Contains(EffectorGoal->GoalName))
		{
			continue;
		}
		
		// add a copy of the Goal to the container
		GoalContainer.SetIKGoal(EffectorGoal);
	}
	
	// initialize goal bones from asset
	GoalBones.Reset();
	for (const UTLLRN_IKRigEffectorGoal* EffectorGoal : EffectorGoals)
	{
		// skip excluded goals
		if (ExcludedGoals.Contains(EffectorGoal->GoalName))
		{
			continue;
		}
		
		FTLLRN_GoalBone NewGoalBone;
		NewGoalBone.BoneName = EffectorGoal->BoneName;
		NewGoalBone.BoneIndex = Skeleton.GetBoneIndexFromName(EffectorGoal->BoneName);

		// validate that the skeleton we are trying to solve this goal on contains the bone the goal expects
		if (NewGoalBone.BoneIndex == INDEX_NONE)
		{
			Log.LogError(FText::Format(
				LOCTEXT("MissingGoalBone", "IK Rig, '{0}' has a Goal, '{1}' that references an unknown bone, '{2}'. Cannot evaluate."),
				FText::FromString(InRigAsset->GetName()), FText::FromName(EffectorGoal->GoalName), FText::FromName(EffectorGoal->BoneName) ));
			return;
		}

		// validate that there is not already a different goal, with the same name, that is using a different bone
		// (all goals with the same name must reference the same bone within a single IK Rig)
		if (const FTLLRN_GoalBone* Bone = GoalBones.Find(EffectorGoal->GoalName))
		{
			if (Bone->BoneName != NewGoalBone.BoneName)
			{
				Log.LogError(FText::Format(
				LOCTEXT("DuplicateGoal", "IK Rig, '{0}' has a Goal, '{1}' that references different bones in different solvers, '{2}' and '{3}'. Cannot evaluate."),
                FText::FromString(InRigAsset->GetName()),
                FText::FromName(EffectorGoal->GoalName),
                FText::FromName(Bone->BoneName), 
                FText::FromName(NewGoalBone.BoneName)));
				return;
			}
		}

		// warn if goal is not connected to any solvers
		bool bIsGoalConnectedToAnySolver = false;
		const TArray<UTLLRN_IKRigSolver*>& AssetSolvers = InRigAsset->GetSolverArray();
		for (const UTLLRN_IKRigSolver* TLLRN_IKRigSolver : AssetSolvers)
		{
			if (TLLRN_IKRigSolver->IsGoalConnected(EffectorGoal->GoalName))
			{
				bIsGoalConnectedToAnySolver = true;
			}
		}
		if (!bIsGoalConnectedToAnySolver)
		{
			Log.LogWarning(FText::Format(
				LOCTEXT("DisconnectedGoal", "IK Rig, '{0}' has a Goal, '{1}' that is not connected to any solvers. It will have no effect."),
				FText::FromString(InRigAsset->GetName()), FText::FromName(EffectorGoal->GoalName)));
		}
		
		GoalBones.Add(EffectorGoal->GoalName, NewGoalBone);
	}

	// create copies of all the solvers in the IK rig
	const TArray<UTLLRN_IKRigSolver*>& AssetSolvers = InRigAsset->GetSolverArray();
	Solvers.Reset(AssetSolvers.Num());
	int32 SolverIndex = 0;
	for (const UTLLRN_IKRigSolver* TLLRN_IKRigSolver : AssetSolvers)
	{
		if (!TLLRN_IKRigSolver)
		{
			// this can happen if asset references deleted IK Solver type which should only happen during development (if at all)
			Log.LogWarning(FText::Format(
				LOCTEXT("UnknownSolver", "IK Rig, '{0}' has null/unknown solver in it. Please remove it."),
				FText::FromString(InRigAsset->GetName())));
			continue;
		}

		// create duplicate solver instance with unique name
		const FName BaseName = FName(*(TLLRN_IKRigSolver->GetName() + "_SolverInstance_"));
		const FName UniqueName = MakeUniqueObjectName(this, TLLRN_IKRigSolver->GetClass(), BaseName, EUniqueObjectNameOptions::GloballyUnique);
		UTLLRN_IKRigSolver* Solver = DuplicateObject(TLLRN_IKRigSolver, this, UniqueName);

		// remove excluded goals from the solver
		for (const FName& ExcludedGoal : ExcludedGoals)
		{
			Solver->RemoveGoal(ExcludedGoal);
		}

		// initialize it and store in the processor
		Solver->Initialize(Skeleton);
		Solvers.Add(Solver);
	}

	// validate retarget chains
	const TArray<FTLLRN_BoneChain>& Chains = InRigAsset->GetRetargetChains();
	TArray<int32> OutBoneIndices;
	for (const FTLLRN_BoneChain& Chain : Chains)
	{
		if (!Skeleton.ValidateChainAndGetBones(Chain, OutBoneIndices))
		{
			Log.LogWarning(FText::Format(
				LOCTEXT("InvalidRetargetChain", "Invalid Retarget Chain: '{0}'. End bone is not a child of the start bone in Skeletal Mesh, '{1}'."),
				FText::FromString(Chain.ChainName.ToString()), FText::FromString(SkeletalMesh->GetName())));
		}
	}

	Log.LogInfo(FText::Format(
				LOCTEXT("SuccessfulInit", "IK Rig, '{0}' ready to run on {1}."),
				FText::FromString(InRigAsset->GetName()), FText::FromString(SkeletalMesh->GetName())));
	
	bInitialized = true;
}

bool UTLLRN_IKRigProcessor::IsIKRigCompatibleWithSkeleton(
	const UTLLRN_IKRigDefinition* InRigAsset,
	const FTLLRN_IKRigInputSkeleton& InputSkeleton,
	const FTLLRN_IKRigLogger* Log)
{
	// first we validate that all the required bones are in the input skeleton...
	
	TSet<FName> RequiredBones;
	const TArray<UTLLRN_IKRigSolver*>& AssetSolvers = InRigAsset->GetSolverArray();
	for (const UTLLRN_IKRigSolver* Solver : AssetSolvers)
	{
		const FName RootBone = Solver->GetRootBone();
		if (RootBone != NAME_None)
		{
			RequiredBones.Add(RootBone);
		}

		Solver->GetBonesWithSettings(RequiredBones);
	}

	const TArray<UTLLRN_IKRigEffectorGoal*>& Goals = InRigAsset->GetGoalArray();
	for (const UTLLRN_IKRigEffectorGoal* Goal : Goals)
	{
		RequiredBones.Add(Goal->BoneName);
	}

	bool bAllRequiredBonesFound = true;
	for (const FName& RequiredBone : RequiredBones)
	{
		if (!InputSkeleton.BoneNames.Contains(RequiredBone))
		{
			if (Log)
			{
				Log->LogError(FText::Format(
			LOCTEXT("MissingBone", "IK Rig, '{0}' is missing a required bone, '{1}' in the Skeletal Mesh."),
				FText::FromString(InRigAsset->GetName()),
				FText::FromName(RequiredBone)));
			}
			
			bAllRequiredBonesFound = false;
		}
	}

	if (!bAllRequiredBonesFound)
	{
		return false;
	}

	// now we validate that hierarchy matches for all required bones...
	bool bAllParentsValid = true;
	const FTLLRN_IKRigSkeleton& AssetSkeleton = InRigAsset->GetSkeleton();
	for (const FName& RequiredBone : RequiredBones)
	{
		const int32 InputBoneIndex = InputSkeleton.BoneNames.Find(RequiredBone);
		const int32 AssetBoneIndex = AssetSkeleton.BoneNames.Find(RequiredBone);

		// we shouldn't get this far otherwise due to early return above...
		check(InputBoneIndex != INDEX_NONE && AssetBoneIndex != INDEX_NONE)

		// validate that input skeleton hierarchy is as expected
		const int32 AssetParentIndex = AssetSkeleton.ParentIndices[AssetBoneIndex];
		if (AssetSkeleton.BoneNames.IsValidIndex(AssetParentIndex)) // root bone has no parent
		{
			const FName& AssetParentName = AssetSkeleton.BoneNames[AssetParentIndex];
			const int32 InputParentIndex = InputSkeleton.ParentIndices[InputBoneIndex];
			if (!InputSkeleton.BoneNames.IsValidIndex(InputParentIndex))
			{
				bAllParentsValid = false;

				if (Log)
				{
					Log->LogError(FText::Format(
					LOCTEXT("InvalidParent", "IK Rig is running on a skeleton with a required bone, '{0}', that expected to have a valid parent. The expected parent was, '{1}'."),
					FText::FromName(RequiredBone),
					FText::FromName(AssetParentName)));
				}
				
				continue;
			}
			
			const FName& InputParentName = InputSkeleton.BoneNames[InputParentIndex];
			if (AssetParentName != InputParentName)
			{
				if (Log)
				{
					// we only warn about this, because it may be nice not to have the exact same hierarchy
					Log->LogWarning(FText::Format(
					LOCTEXT("UnexpectedParent", "IK Rig is running on a skeleton with a required bone, '{0}', that has a different parent '{1}'. The expected parent was, '{2}'."),
					FText::FromName(RequiredBone),
					FText::FromName(InputParentName),
					FText::FromName(AssetParentName)));
				}
				
				continue;
			}
		}
	}

	return bAllParentsValid;
}

void UTLLRN_IKRigProcessor::SetInputPoseGlobal(const TArray<FTransform>& InGlobalBoneTransforms) 
{
	check(bInitialized);
	check(InGlobalBoneTransforms.Num() == Skeleton.CurrentPoseGlobal.Num());
	Skeleton.CurrentPoseGlobal = InGlobalBoneTransforms;
	Skeleton.UpdateAllLocalTransformFromGlobal();
}

void UTLLRN_IKRigProcessor::SetInputPoseToRefPose()
{
	check(bInitialized);
	Skeleton.CurrentPoseGlobal = Skeleton.RefPoseGlobal;
	Skeleton.UpdateAllLocalTransformFromGlobal();
}

void UTLLRN_IKRigProcessor::SetIKGoal(const FTLLRN_IKRigGoal& InGoal)
{
	check(bInitialized);
	GoalContainer.SetIKGoal(InGoal);
}

void UTLLRN_IKRigProcessor::SetIKGoal(const UTLLRN_IKRigEffectorGoal* InGoal)
{
	check(bInitialized);
	GoalContainer.SetIKGoal(InGoal);
}

void UTLLRN_IKRigProcessor::Solve(const FTransform& ComponentToWorld)
{
	check(bInitialized);
	
	// convert goals into component space and blend towards input pose by alpha
	ResolveFinalGoalTransforms(ComponentToWorld);

	// run all the solvers
	for (UTLLRN_IKRigSolver* Solver : Solvers)
	{
		#if WITH_EDITOR
		if (Solver->IsEnabled())
		{
			Solver->Solve(Skeleton, GoalContainer);
		}
		#else
		Solver->Solve(Skeleton, GoalContainer);
		#endif
	}

	// make sure rotations are normalized coming out
	Skeleton.NormalizeRotations(Skeleton.CurrentPoseGlobal);
}

void UTLLRN_IKRigProcessor::CopyOutputGlobalPoseToArray(TArray<FTransform>& OutputPoseGlobal) const
{
	OutputPoseGlobal = Skeleton.CurrentPoseGlobal;
}

void UTLLRN_IKRigProcessor::Reset()
{
	Solvers.Reset();
	GoalContainer.Empty();
	GoalBones.Reset();
	Skeleton.Reset();
	SetNeedsInitialized();
}

void UTLLRN_IKRigProcessor::SetNeedsInitialized()
{
	bInitialized = false;
	bTriedToInitialize = false;
};

void UTLLRN_IKRigProcessor::CopyAllInputsFromSourceAssetAtRuntime(const UTLLRN_IKRigDefinition* SourceAsset)
{
	check(SourceAsset)
	
	if (!bInitialized)
	{
		return;
	}
	
	// copy goal settings
	const TArray<UTLLRN_IKRigEffectorGoal*>& AssetGoals =  SourceAsset->GetGoalArray();
	for (const UTLLRN_IKRigEffectorGoal* AssetGoal : AssetGoals)
	{
		SetIKGoal(AssetGoal);
	}

	// copy solver settings
	const TArray<UTLLRN_IKRigSolver*>& AssetSolvers = SourceAsset->GetSolverArray();
	check(Solvers.Num() == AssetSolvers.Num()); // if number of solvers has been changed, processor should have been reinitialized
	for (int32 SolverIndex=0; SolverIndex<Solvers.Num(); ++SolverIndex)
	{
		Solvers[SolverIndex]->SetEnabled(AssetSolvers[SolverIndex]->IsEnabled());
		Solvers[SolverIndex]->UpdateSolverSettings(AssetSolvers[SolverIndex]);
	}
}

const FTLLRN_IKRigGoalContainer& UTLLRN_IKRigProcessor::GetGoalContainer() const
{
	check(bInitialized);
	return GoalContainer;
}

const FTLLRN_GoalBone* UTLLRN_IKRigProcessor::GetGoalBone(const FName& GoalName) const
{
	return GoalBones.Find(GoalName);
}

FTLLRN_IKRigSkeleton& UTLLRN_IKRigProcessor::GetSkeletonWriteable()
{
	return Skeleton;
}

const FTLLRN_IKRigSkeleton& UTLLRN_IKRigProcessor::GetSkeleton() const
{
	return Skeleton;
}

void UTLLRN_IKRigProcessor::ResolveFinalGoalTransforms(const FTransform& WorldToComponent)
{
	for (FTLLRN_IKRigGoal& Goal : GoalContainer.Goals)
	{
		if (!GoalBones.Contains(Goal.Name))
		{
			// user is changing goals after initialization
			// not necessarily a bad thing, but new goal names won't work until re-init
			continue;
		}

		const FTLLRN_GoalBone& GoalBone = GoalBones[Goal.Name];
		const FTransform& InputPoseBoneTransform = Skeleton.CurrentPoseGlobal[GoalBone.BoneIndex];

		FVector ComponentSpaceGoalPosition = Goal.Position;
		FQuat ComponentSpaceGoalRotation = Goal.Rotation.Quaternion();

		// FIXME find a way to cache SourceBoneIndex to avoid calling Skeleton.GetBoneIndexFromName here
		// we may use FTLLRN_GoalBone::OptSourceIndex for this
		int SourceBoneIndex = INDEX_NONE;
		if (Goal.TransformSource == ETLLRN_IKRigGoalTransformSource::Bone && Goal.SourceBone.BoneName != NAME_None)
		{
			SourceBoneIndex = Skeleton.GetBoneIndexFromName(Goal.SourceBone.BoneName);
		}

		if (SourceBoneIndex != INDEX_NONE)
		{
			ComponentSpaceGoalPosition = Skeleton.CurrentPoseGlobal[SourceBoneIndex].GetLocation();
		}
		else
		{
			// put goal POSITION in Component Space
			switch (Goal.PositionSpace)
			{
			case ETLLRN_IKRigGoalSpace::Additive:
				// add position offset to bone position
				ComponentSpaceGoalPosition = Skeleton.CurrentPoseGlobal[GoalBone.BoneIndex].GetLocation() + Goal.Position;
				break;
			case ETLLRN_IKRigGoalSpace::Component:
				// was already supplied in Component Space
				break;
			case ETLLRN_IKRigGoalSpace::World:
				// convert from World Space to Component Space
				ComponentSpaceGoalPosition = WorldToComponent.TransformPosition(Goal.Position);
				break;
			default:
				checkNoEntry();
				break;
			}
		}
		
		// put goal ROTATION in Component Space
		if (SourceBoneIndex != INDEX_NONE)
		{
			ComponentSpaceGoalRotation = Skeleton.CurrentPoseGlobal[SourceBoneIndex].GetRotation();
		}
		else
		{
			switch (Goal.RotationSpace)
			{
			case ETLLRN_IKRigGoalSpace::Additive:
				// add rotation offset to bone rotation
				ComponentSpaceGoalRotation = Goal.Rotation.Quaternion() * Skeleton.CurrentPoseGlobal[GoalBone.BoneIndex].GetRotation();
				break;
			case ETLLRN_IKRigGoalSpace::Component:
				// was already supplied in Component Space
				break;
			case ETLLRN_IKRigGoalSpace::World:
				// convert from World Space to Component Space
				ComponentSpaceGoalRotation = WorldToComponent.TransformRotation(Goal.Rotation.Quaternion());
				break;
			default:
				checkNoEntry();
				break;
			}
		}

		// blend by alpha from the input pose, to the supplied goal transform
		// when Alpha is 0, the goal transform matches the bone transform at the input pose.
		// when Alpha is 1, the goal transform is left fully intact
		Goal.FinalBlendedPosition = FMath::Lerp(
            InputPoseBoneTransform.GetTranslation(),
            ComponentSpaceGoalPosition,
            Goal.PositionAlpha);
		
		Goal.FinalBlendedRotation = FQuat::FastLerp(
            InputPoseBoneTransform.GetRotation(),
            ComponentSpaceGoalRotation,
            Goal.RotationAlpha);
	}
}

#undef LOCTEXT_NAMESPACE

