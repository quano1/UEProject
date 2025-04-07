// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TLLRN_IKRig : ModuleRules
{
	public TLLRN_IKRig(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"AnimationCore",
			});

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"ControlRig",
				"RigVM",
				"Core",
				"PBIK",
				"SkeletonCopier",
			});

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			});
		
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"ApplicationCore",
					"MessageLog",
				});

			PublicIncludePathModuleNames.AddRange(
				new string[]
				{
					"AnimationWidgets",
				});
		}
	}
}
