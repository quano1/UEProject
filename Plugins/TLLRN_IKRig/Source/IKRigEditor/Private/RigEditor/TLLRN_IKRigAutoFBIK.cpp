// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigAutoFBIK.h"

#include "TLLRN_IKRigEditor.h"
#include "Rig/Solvers/TLLRN_IKRig_FBIKSolver.h"
#include "RigEditor/TLLRN_IKRigAutoCharacterizer.h"
#include "RigEditor/TLLRN_IKRigController.h"
#include "Templates/SubclassOf.h"

#define LOCTEXT_NAMESPACE "TLLRN_AutoFBIK"

void FTLLRN_AutoFBIKCreator::CreateFBIKSetup(const UTLLRN_IKRigController& TLLRN_IKRigController, FTLLRN_AutoFBIKResults& Results) const
{
	// ensure we have a mesh to operate on
	USkeletalMesh* Mesh = TLLRN_IKRigController.GetSkeletalMesh();
	if (!Mesh)
	{
		Results.Outcome = EAutoFBIKResult::MissingMesh;
		return;
	}

	// auto generate a retarget definition
	FTLLRN_AutoCharacterizeResults CharacterizeResults;
	TLLRN_IKRigController.AutoGenerateRetargetDefinition(CharacterizeResults);
	if (!CharacterizeResults.bUsedTemplate)
	{
		Results.Outcome = EAutoFBIKResult::UnknownSkeletonType;
		return;
	}
	
	// create all the goals in the template
	TArray<FName> GoalNames;
	const TArray<FTLLRN_BoneChain>& ExpectedChains = CharacterizeResults.AutoRetargetDefinition.RetargetDefinition.BoneChains;
	for (const FTLLRN_BoneChain& ExpectedChain : ExpectedChains)
	{
		if (ExpectedChain.IKGoalName == NAME_None)
		{
			continue;
		}
		
		const FTLLRN_BoneChain* Chain = TLLRN_IKRigController.GetRetargetChainByName(ExpectedChain.ChainName);
		FName GoalName = (Chain && Chain->IKGoalName != NAME_None) ? Chain->IKGoalName : ExpectedChain.IKGoalName;
		const UTLLRN_IKRigEffectorGoal* ChainGoal = TLLRN_IKRigController.GetGoal(GoalName);
		if (!ChainGoal)
		{
			// create new goal
			GoalName = TLLRN_IKRigController.AddNewGoal(GoalName, ExpectedChain.EndBone.BoneName);
			if (Chain)
			{
				TLLRN_IKRigController.SetRetargetChainGoal(Chain->ChainName, GoalName);
			}
			else
			{
				UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Auto FBIK created goal for a limb, but it did not have a retarget chain. %s"), *ExpectedChain.ChainName.ToString());
			}
		}

		GoalNames.Add(GoalName);
	}

	// create IK solver and attach all the goals to it
	const int32 SolverIndex = TLLRN_IKRigController.AddSolver(UTLLRN_IKRigFBIKSolver::StaticClass());
	for (const FName& GoalName : GoalNames)
	{
		TLLRN_IKRigController.ConnectGoalToSolver(GoalName, SolverIndex);	
	}

	// set the root of the solver
	const bool bSetRoot = TLLRN_IKRigController.SetRootBone(CharacterizeResults.AutoRetargetDefinition.RetargetDefinition.RootBone, SolverIndex);
	if (!bSetRoot)
	{
		Results.Outcome = EAutoFBIKResult::MissingRootBone;
		return;
	}

	// update solver settings for retargeting
	UTLLRN_IKRigFBIKSolver* Solver = CastChecked<UTLLRN_IKRigFBIKSolver>(TLLRN_IKRigController.GetSolverAtIndex(SolverIndex));
	// set the root behavior to "free", allows pelvis motion only when needed to reach goals
	Solver->RootBehavior = EPBIKRootBehavior::Free;
	// removing pull chain alpha on all goals "calms" the motion down, especially when retargeting arms
	Solver->PullChainAlpha = 0.0f;

	// assign bone settings from template
	const FTLLRN_AutoCharacterizer& Characterizer = TLLRN_IKRigController.GetAutoCharacterizer();
	const FTLLRN_TemplateHierarchy* TemplateHierarchy = Characterizer.GetKnownTemplateHierarchy(CharacterizeResults.BestTemplateName);
	if (!ensureAlways(TemplateHierarchy))
	{
		return;
	}
	const FTLLRN_AbstractHierarchy MeshAbstractHierarchy(Mesh);
	const TArray<FTLLRN_BoneSettingsForIK>& AllBoneSettings = TemplateHierarchy->AutoRetargetDefinition.BoneSettingsForIK.GetBoneSettings();
	for (const FTLLRN_BoneSettingsForIK& BoneSettings : AllBoneSettings)
	{
		// templates use "clean" names, free from prefixes, so we need to resolve this onto the actual skeletal mesh being setup
		const int32 BoneIndex = MeshAbstractHierarchy.GetBoneIndex(BoneSettings.BoneToApplyTo, ETLLRN_CleanOrFullName::Clean);
		if (BoneIndex == INDEX_NONE)
		{
			UE_LOG(LogTLLRN_IKRigEditor, Warning, TEXT("Auto FBIK using a template with settings for a bone that is not in this skeletal mesh: %s"), *BoneSettings.BoneToApplyTo.ToString());
			continue;
		}
		const FName BoneFullName = MeshAbstractHierarchy.GetBoneName(BoneIndex, ETLLRN_CleanOrFullName::Full);
		
		// optionally exclude bones
		if (BoneSettings.bExcluded)
		{
			TLLRN_IKRigController.SetBoneExcluded(BoneFullName, true /*bExclude*/);
		}

		// get the bone settings for this bone for the FBIK solver (or add one if there is none)
		auto GetOrAddBoneSettings = [SolverIndex, &TLLRN_IKRigController, BoneFullName]() -> UTLLRN_IKRig_FBIKBoneSettings*
		{
			if (!TLLRN_IKRigController.GetBoneSettings(BoneFullName, SolverIndex))
			{
				TLLRN_IKRigController.AddBoneSetting(BoneFullName, SolverIndex);
			}
			
			return CastChecked<UTLLRN_IKRig_FBIKBoneSettings>(TLLRN_IKRigController.GetBoneSettings(BoneFullName, SolverIndex));
		};

		// apply rotational stiffness
		if (BoneSettings.RotationStiffness > 0.f)
		{
			UTLLRN_IKRig_FBIKBoneSettings* Settings = GetOrAddBoneSettings();
			Settings->RotationStiffness = FMath::Clamp(BoneSettings.RotationStiffness, 0.f, 1.0f);
		}
		
		// apply preferred angles
		if (BoneSettings.PreferredAxis != ETLLRN_PreferredAxis::None)
		{
			UTLLRN_IKRig_FBIKBoneSettings* Settings = GetOrAddBoneSettings();
			Settings->bUsePreferredAngles = true;
			Settings->PreferredAngles = BoneSettings.GetPreferredAxisAsAngles();
		}
		
		// apply locked axes
		if (BoneSettings.bIsHinge)
		{
			UTLLRN_IKRig_FBIKBoneSettings* Settings = GetOrAddBoneSettings();
			BoneSettings.LockNonPreferredAxes(Settings->X, Settings->Y, Settings->Z);	
		}
	}
}

#undef LOCTEXT_NAMESPACE
