// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TLLRN_ModelingComponents : ModuleRules
{
	public TLLRN_ModelingComponents(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"PhysicsCore",
				"InteractiveToolsFramework",
				"MeshDescription",
				"StaticMeshDescription",
				"GeometryCore",
				"GeometryFramework",
				"GeometryAlgorithms",
				"DynamicMesh",
				"MeshConversion",
				"TLLRN_ModelingOperators",
				"DeveloperSettings",
				// ... add other public dependencies that you statically link with here ...
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Chaos",
				"Engine",
				"RenderCore",
				"RHI",
				"ImageWriteQueue",
				"SkeletalMeshDescription", // FSkeletalMeshAttributes::DefaultSkinWeightProfileName
				"SlateCore",
				"ImageCore",
				"PlanarCut",
				"GeometryCollectionEngine",
				"MeshConversionEngineTypes"
				// ... add private dependencies that you statically link with here ...
			}
		);
		
		if (Target.bCompileAgainstEditor) // #if WITH_EDITOR
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"Slate"
				});
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}