// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorToolsEditorOnlyModule.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

#include "DetailsCustomizations/UVTransformToolCustomizations.h"
#include "DetailsCustomizations/TLLRN_UVEditorSeamToolCustomizations.h"

#include "TLLRN_UVEditorTransformTool.h"
#include "Operators/TLLRN_UVEditorUVTransformOp.h"
#include "TLLRN_UVEditorSeamTool.h"

#define LOCTEXT_NAMESPACE "FTLLRN_UVEditorToolsEditorOnlyModule"

void FTLLRN_UVEditorToolsEditorOnlyModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTLLRN_UVEditorToolsEditorOnlyModule::OnPostEngineInit);

}

void FTLLRN_UVEditorToolsEditorOnlyModule::ShutdownModule()
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

void FTLLRN_UVEditorToolsEditorOnlyModule::OnPostEngineInit()
{

	// Register details view customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Transform
	PropertyModule.RegisterCustomClassLayout(UTLLRN_UVEditorUVTransformProperties::StaticClass()->GetFName(),
		                                     FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_UVEditorUVTransformToolDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_UVEditorUVTransformProperties::StaticClass()->GetFName());

	// Quick Transform
	PropertyModule.RegisterCustomClassLayout(UTLLRN_UVEditorUVQuickTransformProperties::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_UVEditorUVQuickTransformToolDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_UVEditorUVQuickTransformProperties::StaticClass()->GetFName());

	// Align
	PropertyModule.RegisterCustomClassLayout(UTLLRN_UVEditorUVAlignProperties::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_UVEditorUVAlignToolDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_UVEditorUVAlignProperties::StaticClass()->GetFName());

	// Distribute
	PropertyModule.RegisterCustomClassLayout(UTLLRN_UVEditorUVDistributeProperties::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_UVEditorUVDistributeToolDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_UVEditorUVDistributeProperties::StaticClass()->GetFName());

	// Seam Tool
	PropertyModule.RegisterCustomClassLayout(UTLLRN_UVEditorSeamToolProperties::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_UVEditorSeamToolPropertiesDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UTLLRN_UVEditorSeamToolProperties::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTLLRN_UVEditorToolsEditorOnlyModule, TLLRN_UVEditorToolsEditorOnly)
