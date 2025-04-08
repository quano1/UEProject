// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_MeshModelingTools.h"

#define LOCTEXT_NAMESPACE "FTLLRN_MeshModelingToolsModule"

void FTLLRN_MeshModelingToolsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FTLLRN_MeshModelingToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTLLRN_MeshModelingToolsModule, TLLRN_MeshModelingTools)
