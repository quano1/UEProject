// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_IKRig.h"

#include "TLLRN_IKRigObjectVersion.h"
#include "UObject/DevObjectVersion.h"

#define LOCTEXT_NAMESPACE "TLLRN_IKRigModule"

IMPLEMENT_MODULE(FTLLRN_IKRigModule, TLLRN_IKRig)

// Unique IK Rig Object version id
const FGuid FTLLRN_IKRigObjectVersion::GUID(0xF6DFBB78, 0xBB50A0E4, 0x4018B84D, 0x60CBAF23);
// Register custom version with Core
FDevVersionRegistration GRegisterTLLRN_IKRigObjectVersion(FTLLRN_IKRigObjectVersion::GUID, FTLLRN_IKRigObjectVersion::LatestVersion, TEXT("Dev-IKRig"));


void FTLLRN_IKRigModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FTLLRN_IKRigModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
