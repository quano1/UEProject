// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TLLRN_MeshModelingTools : ModuleRules
{
	public TLLRN_MeshModelingTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InputCore",
				"InteractiveToolsFramework",
				"GeometryCore",
				"GeometryFramework",
				"GeometryAlgorithms",
				"DynamicMesh",
				"MeshConversion",
				"MeshDescription",
				"StaticMeshDescription",
				"TLLRN_ModelingComponents",
				"TLLRN_ModelingOperators",
				// ... add other public dependencies that you statically link with here ...
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				// ... add private dependencies that you statically link with here ...
			}
		);

		bDisableAutoRTFMInstrumentation = true;
	}
}
