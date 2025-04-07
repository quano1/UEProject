// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlTLLRN_RigModule.h"

#include "TLLRN_ControlRigGizmoActor.h"
#include "TransformableRegistry.h"
#include "Modules/ModuleManager.h"
#include "ILevelSequenceModule.h"
#include "TLLRN_ControlRigObjectVersion.h"
#include "Constraints/TLLRN_ControlTLLRN_RigTransformableHandle.h"
#include "UObject/DevObjectVersion.h"
#include "TLLRN_ControlRig.h"

// Unique Control Rig Object version id
const FGuid FTLLRN_ControlRigObjectVersion::GUID(0xA7820CFB, 0x20A74359, 0x8C542C14, 0x9623CF50);
// Register TLLRN_ControlRig custom version with Core
FDevVersionRegistration GRegisterTLLRN_ControlRigObjectVersion(FTLLRN_ControlRigObjectVersion::GUID, FTLLRN_ControlRigObjectVersion::LatestVersion, TEXT("Dev-TLLRN_ControlRig"));

void FTLLRN_ControlTLLRN_RigModule::StartupModule()
{
	ManipulatorMaterial = LoadObject<UMaterial>(nullptr, TEXT("/TLLRN_ControlRig/M_Manip.M_Manip"));

	RegisterTransformableCustomization();
}

void FTLLRN_ControlTLLRN_RigModule::ShutdownModule()
{
}

void FTLLRN_ControlTLLRN_RigModule::RegisterTransformableCustomization() const
{
	// load the module to trigger default objects registration
	static const FName ConstraintsModuleName(TEXT("Constraints"));
	FModuleManager::Get().LoadModule(ConstraintsModuleName);
	
	// register UTLLRN_ControlRig and ATLLRN_ControlRigShapeActor
	auto CreateControlHandle = [](UObject* InObject, const FName& InControlName)->UTransformableHandle*
	{
		if (const UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InObject))
		{
			return TLLRN_ControlRig->CreateTLLRN_TransformableControlHandle(InControlName);
		}
		return nullptr;
	};

	auto GetControlHash = [](const UObject* InObject, const FName& InControlName)->uint32
	{
		if (const UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InObject))
		{
			return UTLLRN_TransformableControlHandle::ComputeHash(TLLRN_ControlRig, InControlName);
		}
		return 0;
	};
	
	auto CreateControlHandleFromActor = [CreateControlHandle](UObject* InObject, const FName&)->UTransformableHandle*
	{
		if (const ATLLRN_ControlRigShapeActor* ControlActor = Cast<ATLLRN_ControlRigShapeActor>(InObject))
		{
			return CreateControlHandle(ControlActor->TLLRN_ControlRig.Get(), ControlActor->ControlName);
		}
		return nullptr;
	};

	auto GetControlHashFromActor = [GetControlHash](const UObject* InObject, const FName&)->uint32
	{
		if (const ATLLRN_ControlRigShapeActor* ControlActor = Cast<ATLLRN_ControlRigShapeActor>(InObject))
		{
			return GetControlHash(ControlActor->TLLRN_ControlRig.Get(), ControlActor->ControlName);
		}
		return 0;
	};

	// Register UTLLRN_TransformableControlHandle and has function to handle control thru the transform constraints system
	// as ATLLRN_ControlRigShapeActor is only available if the TLLRN_ControlRig plugin is loaded.
	FTransformableRegistry& Registry = FTransformableRegistry::Get();
	Registry.Register(ATLLRN_ControlRigShapeActor::StaticClass(), CreateControlHandleFromActor, GetControlHashFromActor);
	Registry.Register(UTLLRN_ControlRig::StaticClass(), CreateControlHandle, GetControlHash);
}

IMPLEMENT_MODULE(FTLLRN_ControlTLLRN_RigModule, TLLRN_ControlRig)
