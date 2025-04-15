// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModelingToolsEditorModeSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ModelingToolsEditorModeSettings)


#define LOCTEXT_NAMESPACE "TLLRN_ModelingToolsEditorModeSettings"


FText UTLLRN_ModelingToolsEditorModeSettings::GetSectionText() const 
{ 
	return LOCTEXT("TLLRN_ModelingModeSettingsName", "Modeling Mode"); 
}

FText UTLLRN_ModelingToolsEditorModeSettings::GetSectionDescription() const
{
	return LOCTEXT("TLLRN_ModelingModeSettingsDescription", "Configure the Modeling Tools Editor Mode plugin");
}


FText UTLLRN_ModelingToolsModeCustomizationSettings::GetSectionText() const 
{ 
	return LOCTEXT("TLLRN_ModelingModeSettingsName", "Modeling Mode"); 
}

FText UTLLRN_ModelingToolsModeCustomizationSettings::GetSectionDescription() const
{
	return LOCTEXT("TLLRN_ModelingModeSettingsDescription", "Configure the Modeling Tools Editor Mode plugin");
}


#undef LOCTEXT_NAMESPACE
