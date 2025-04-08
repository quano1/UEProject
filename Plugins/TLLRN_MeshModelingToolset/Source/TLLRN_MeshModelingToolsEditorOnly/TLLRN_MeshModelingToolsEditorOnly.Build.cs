// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TLLRN_MeshModelingToolsEditorOnly : ModuleRules
{
	public TLLRN_MeshModelingToolsEditorOnly(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InteractiveToolsFramework",
				"GeometryCore",
				"TLLRN_MeshModelingTools",
				"TLLRN_ModelingComponents",
				"TLLRN_ModelingOperatorsEditorOnly",
				"TLLRN_ModelingOperators",
				"PropertyEditor",
				"DynamicMesh",
				// ... add other public dependencies that you statically link with here ...
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				// ... add private dependencies that you statically link with here ...	
			}
		);
	}
}