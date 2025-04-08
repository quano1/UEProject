// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_MeshModelingToolsEditorOnly.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

#include "DetailsCustomizations/UVUnwrapToolCustomizations.h"
#include "Properties/TLLRN_RecomputeUVsProperties.h"

#define LOCTEXT_NAMESPACE "FTLLRN_MeshModelingToolsEditorOnlyModule"

void FTLLRN_MeshModelingToolsEditorOnlyModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTLLRN_MeshModelingToolsEditorOnlyModule::OnPostEngineInit);

}

void FTLLRN_MeshModelingToolsEditorOnlyModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	// Unregister customizations
	FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");
	if (PropertyEditorModule)
	{
		for (const FName& ClassName : ClassesToUnregisterOnShutdown)
		{
			PropertyEditorModule->UnregisterCustomClassLayout(ClassName);
		}
		for (const FName& PropertyName : PropertiesToUnregisterOnShutdown)
		{
			PropertyEditorModule->UnregisterCustomPropertyTypeLayout(PropertyName);
		}
	}
}

void FTLLRN_MeshModelingToolsEditorOnlyModule::OnPostEngineInit()
{

	// Register details view customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	/// Unwrap
	PropertyModule.RegisterCustomClassLayout("TLLRN_RecomputeUVsToolProperties", FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_RecomputeUVsToolDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_RecomputeUVsToolProperties::StaticClass()->GetFName());


}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTLLRN_MeshModelingToolsEditorOnlyModule, TLLRN_MeshModelingToolsEditorOnly)