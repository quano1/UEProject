// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TLLRN_SkeletalMeshModelingTools : ModuleRules
{
	public TLLRN_SkeletalMeshModelingTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ContentBrowser",
				"Core",
				"CoreUObject",
				"EditorFramework",
				"EditorInteractiveToolsFramework",
				"Engine",
				"InputCore",
				"InteractiveToolsFramework",
				"MeshModelingTools",
				"MeshModelingToolsEditorOnly",
				"MeshModelingToolsEditorOnlyExp",
				"MeshModelingToolsExp",
				"ModelingComponentsEditorOnly",
				"ModelingToolsEditorMode",
				"SkeletalMeshEditor",
				"SkeletalMeshUtilitiesCommon",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"ToolWidgets",
				"UnrealEd",
				"StatusBar",
				"PropertyEditor",
				"AnimationCore",
				"AnimationWidgets",
				"ApplicationCore",
				"WidgetRegistration",
				"SkeletalMeshModifiers",
				"Persona"
			}
		);

		ShortName = "SkelMeshModTools";
	}
}
