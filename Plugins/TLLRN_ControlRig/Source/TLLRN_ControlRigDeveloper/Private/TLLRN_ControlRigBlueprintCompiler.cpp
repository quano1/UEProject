// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigBlueprintCompiler.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Kismet2/KismetReinstanceUtilities.h"

bool FTLLRN_ControlRigBlueprintCompiler::CanCompile(const UBlueprint* Blueprint)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if (Blueprint && Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(UTLLRN_ControlRig::StaticClass()))
	{
		return true;
	}

	return false;
}

void FTLLRN_ControlRigBlueprintCompiler::Compile(UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& Results)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FTLLRN_ControlRigBlueprintCompilerContext Compiler(Blueprint, Results, CompileOptions);
	Compiler.Compile();
}

void FTLLRN_ControlRigBlueprintCompilerContext::SpawnNewClass(const FString& NewClassName)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	NewRigVMBlueprintGeneratedClass = FindObject<UTLLRN_ControlRigBlueprintGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);

	if (NewRigVMBlueprintGeneratedClass == nullptr)
	{
		NewRigVMBlueprintGeneratedClass = NewObject<UTLLRN_ControlRigBlueprintGeneratedClass>(Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		// Already existed, but wasn't linked in the Blueprint yet due to load ordering issues
		FBlueprintCompileReinstancer::Create(NewRigVMBlueprintGeneratedClass);
	}
	NewClass = NewRigVMBlueprintGeneratedClass;
}

void FTLLRN_ControlRigBlueprintCompilerContext::CopyTermDefaultsToDefaultObject(UObject* DefaultObject)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FRigVMBlueprintCompilerContext::CopyTermDefaultsToDefaultObject(DefaultObject);

	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(Blueprint);
	if (TLLRN_ControlRigBlueprint)
	{
		// here, CDO is initialized from BP,
		// and in UTLLRN_ControlRig::InitializeFromCDO,
		// other Control Rig Instances are then initialized From the CDO 
		UTLLRN_ControlRig* TLLRN_ControlRig = CastChecked<UTLLRN_ControlRig>(DefaultObject);
		
		// copy hierarchy
		{
			TGuardValue<bool> Guard(TLLRN_ControlRig->GetHierarchy()->GetSuspendNotificationsFlag(), false);
			TLLRN_ControlRig->GetHierarchy()->CopyHierarchy(TLLRN_ControlRigBlueprint->Hierarchy);
			TLLRN_ControlRig->GetHierarchy()->ResetPoseToInitial(ETLLRN_RigElementType::All);

#if WITH_EDITOR
			// link CDO hierarchy to BP's hierarchy
			// so that whenever BP's hierarchy is modified, CDO's hierarchy is kept in sync
			// Other instances of Control Rig can make use of CDO to reset to the initial state
			TLLRN_ControlRigBlueprint->Hierarchy->ClearListeningHierarchy();
			TLLRN_ControlRigBlueprint->Hierarchy->RegisterListeningHierarchy(TLLRN_ControlRig->GetHierarchy());
#endif
		}

		// notify clients that the hierarchy has changed
		TLLRN_ControlRig->GetHierarchy()->Notify(ETLLRN_RigHierarchyNotification::HierarchyReset, nullptr);

		TLLRN_ControlRig->DrawContainer = TLLRN_ControlRigBlueprint->DrawContainer;
		TLLRN_ControlRig->Influences = TLLRN_ControlRigBlueprint->Influences;
	}
}
