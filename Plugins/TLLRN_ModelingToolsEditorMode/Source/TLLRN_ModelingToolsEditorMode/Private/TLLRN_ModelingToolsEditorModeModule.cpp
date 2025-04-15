// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModelingToolsEditorModeModule.h"

#include "TLLRN_ModelingToolsActions.h"
#include "TLLRN_ModelingToolsManagerActions.h"
#include "TLLRN_ModelingToolsEditorModeStyle.h"


#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "DetailsCustomizations/TLLRN_ModelingToolPropertyCustomizations.h"
#include "DetailsCustomizations/TLLRN_ModelingToolsBrushSizeCustomization.h"
#include "DetailsCustomizations/TLLRN_MeshVertexSculptToolCustomizations.h"
#include "DetailsCustomizations/TLLRN_BakeMeshAttributeToolCustomizations.h"
#include "DetailsCustomizations/TLLRN_BakeTransformToolCustomizations.h"
#include "DetailsCustomizations/TLLRN_MeshTopologySelectionMechanicCustomization.h"

#include "PropertySets/AxisFilterPropertyType.h"
#include "PropertySets/ColorChannelFilterPropertyType.h"
#include "Selection/MeshTopologySelectionMechanic.h"
#include "MeshVertexSculptTool.h"
#include "BakeMeshAttributeMapsTool.h"
#include "BakeMultiMeshAttributeMapsTool.h"
#include "BakeMeshAttributeVertexTool.h"
#include "BakeTransformTool.h"


#define LOCTEXT_NAMESPACE "FTLLRN_ModelingToolsEditorModeModule"

void FTLLRN_ModelingToolsEditorModeModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTLLRN_ModelingToolsEditorModeModule::OnPostEngineInit);
}

void FTLLRN_ModelingToolsEditorModeModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	FModelingToolActionCommands::UnregisterAllToolActions();
	FTLLRN_ModelingToolsManagerCommands::Unregister();
	FTLLRN_ModelingModeActionCommands::Unregister();

	// Unregister customizations
	FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");
	if (PropertyEditorModule)
	{
		for (FName ClassName : ClassesToUnregisterOnShutdown)
		{
			PropertyEditorModule->UnregisterCustomClassLayout(ClassName);
		}
		for (FName PropertyName : PropertiesToUnregisterOnShutdown)
		{
			PropertyEditorModule->UnregisterCustomPropertyTypeLayout(PropertyName);
		}
	}

	// Unregister slate style overrides
	FTLLRN_ModelingToolsEditorModeStyle::Shutdown();
}


void FTLLRN_ModelingToolsEditorModeModule::OnPostEngineInit()
{
	// Register slate style overrides
	FTLLRN_ModelingToolsEditorModeStyle::Initialize();

	FModelingToolActionCommands::RegisterAllToolActions();
	FTLLRN_ModelingToolsManagerCommands::Register();
	FTLLRN_ModelingModeActionCommands::Register();

	// same as ClassesToUnregisterOnShutdown but for properties, there is none right now
	PropertiesToUnregisterOnShutdown.Reset();
	ClassesToUnregisterOnShutdown.Reset();


	// Register details view customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	/// Sculpt
	PropertyModule.RegisterCustomPropertyTypeLayout("TLLRN_ModelingToolsAxisFilter", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_ModelingToolsAxisFilterCustomization::MakeInstance));
	PropertiesToUnregisterOnShutdown.Add(FModelingToolsAxisFilter::StaticStruct()->GetFName());
	PropertyModule.RegisterCustomPropertyTypeLayout("TLLRN_ModelingToolsColorChannelFilter", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_ModelingToolsColorChannelFilterCustomization::MakeInstance));
	PropertiesToUnregisterOnShutdown.Add(FModelingToolsColorChannelFilter::StaticStruct()->GetFName());
	PropertyModule.RegisterCustomPropertyTypeLayout("BrushToolRadius", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_ModelingToolsBrushSizeCustomization::MakeInstance));
	PropertiesToUnregisterOnShutdown.Add(FBrushToolRadius::StaticStruct()->GetFName());
	PropertyModule.RegisterCustomClassLayout("SculptBrushProperties", FOnGetDetailCustomizationInstance::CreateStatic(&FSculptBrushPropertiesDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(USculptBrushProperties::StaticClass()->GetFName());
	PropertyModule.RegisterCustomClassLayout("VertexBrushSculptProperties", FOnGetDetailCustomizationInstance::CreateStatic(&FVertexBrushSculptPropertiesDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UVertexBrushSculptProperties::StaticClass()->GetFName());
	PropertyModule.RegisterCustomClassLayout("VertexBrushAlphaProperties", FOnGetDetailCustomizationInstance::CreateStatic(&FVertexBrushAlphaPropertiesDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UVertexBrushAlphaProperties::StaticClass()->GetFName());
	/// Bake
	PropertyModule.RegisterCustomClassLayout("BakeMeshAttributeMapsToolProperties", FOnGetDetailCustomizationInstance::CreateStatic(&FBakeMeshAttributeMapsToolDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UBakeMeshAttributeMapsToolProperties::StaticClass()->GetFName());
	PropertyModule.RegisterCustomClassLayout("BakeMultiMeshAttributeMapsToolProperties", FOnGetDetailCustomizationInstance::CreateStatic(&FBakeMultiMeshAttributeMapsToolDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UBakeMultiMeshAttributeMapsToolProperties::StaticClass()->GetFName());
	PropertyModule.RegisterCustomClassLayout("BakeMeshAttributeVertexToolProperties", FOnGetDetailCustomizationInstance::CreateStatic(&FBakeMeshAttributeVertexToolDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UBakeMeshAttributeVertexToolProperties::StaticClass()->GetFName());
	// PolyEd
	PropertyModule.RegisterCustomClassLayout("MeshTopologySelectionMechanicProperties", FOnGetDetailCustomizationInstance::CreateStatic(&FMeshTopologySelectionMechanicPropertiesDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UMeshTopologySelectionMechanicProperties::StaticClass()->GetFName());

	PropertyModule.RegisterCustomClassLayout("BakeTransformToolProperties", FOnGetDetailCustomizationInstance::CreateStatic(&FBakeTransformToolDetails::MakeInstance));
	ClassesToUnregisterOnShutdown.Add(UBakeTransformToolProperties::StaticClass()->GetFName());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTLLRN_ModelingToolsEditorModeModule, TLLRN_ModelingToolsEditorMode)
