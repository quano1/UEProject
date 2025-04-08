// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModelingComponentsSettings.h"
#include "TLLRN_ModelingObjectsCreationAPI.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ModelingComponentsSettings)


#define LOCTEXT_NAMESPACE "TLLRN_ModelingComponentsSettings"

#if WITH_EDITOR
FText UTLLRN_ModelingComponentsSettings::GetSectionText() const 
{ 
	return LOCTEXT("TLLRN_ModelingComponentsProjectSettingsName", "Modeling Mode Tools"); 
}

FText UTLLRN_ModelingComponentsSettings::GetSectionDescription() const
{
	return LOCTEXT("TLLRN_ModelingComponentsProjectSettingsDescription", "Configure Tool-level Settings for the Modeling Tools Editor Mode plugin");
}

FText UTLLRN_ModelingComponentsEditorSettings::GetSectionText() const 
{ 
	return LOCTEXT("TLLRN_ModelingComponentsEditorSettingsName", "Modeling Mode Tools"); 
}

FText UTLLRN_ModelingComponentsEditorSettings::GetSectionDescription() const
{
	return LOCTEXT("TLLRN_ModelingComponentsEditorSettingsDescription", "Configure Tool-level Settings for the Modeling Tools Editor Mode plugin");
}
#endif


void UTLLRN_ModelingComponentsSettings::ApplyDefaultsToTLLRN_CreateMeshObjectParams(FTLLRN_CreateMeshObjectParams& Params)
{
	const UTLLRN_ModelingComponentsSettings* Settings = GetDefault<UTLLRN_ModelingComponentsSettings>();
	if (Settings)
	{
		Params.bEnableCollision = Settings->bEnableCollision;
		Params.CollisionMode = Settings->CollisionMode;
		Params.bGenerateLightmapUVs = Settings->bGenerateLightmapUVs;
		Params.bEnableRaytracingSupport = Settings->bEnableRayTracing;
	}
}

#undef LOCTEXT_NAMESPACE
