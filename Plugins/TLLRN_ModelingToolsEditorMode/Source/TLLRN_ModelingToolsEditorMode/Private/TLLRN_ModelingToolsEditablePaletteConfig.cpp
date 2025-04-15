// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModelingToolsEditablePaletteConfig.h"

TObjectPtr<UTLLRN_ModelingModeEditableToolPaletteConfig> UTLLRN_ModelingModeEditableToolPaletteConfig::Instance = nullptr;

void UTLLRN_ModelingModeEditableToolPaletteConfig::Initialize()
{
	if(!Instance)
	{
		Instance = NewObject<UTLLRN_ModelingModeEditableToolPaletteConfig>(); 
		Instance->AddToRoot();
	}
}

FEditableToolPaletteSettings* UTLLRN_ModelingModeEditableToolPaletteConfig::GetMutablePaletteConfig(const FName& InstanceName)
{
	if (InstanceName.IsNone())
	{
		return nullptr;
	}

	return &EditableToolPalettes.FindOrAdd(InstanceName);
}

const FEditableToolPaletteSettings* UTLLRN_ModelingModeEditableToolPaletteConfig::GetConstPaletteConfig(const FName& InstanceName)
{
	if (InstanceName.IsNone())
	{
		return nullptr;
	}

	return EditableToolPalettes.Find(InstanceName);
}

void UTLLRN_ModelingModeEditableToolPaletteConfig::SavePaletteConfig(const FName&)
{
	SaveEditorConfig();
}