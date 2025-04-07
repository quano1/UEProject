// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class TLLRN_ControlRig : ModuleRules
    {
        public TLLRN_ControlRig(ReadOnlyTargetRules Target) : base(Target)
        {
			NumIncludedBytesPerUnityCPPOverride = 688128; // best unity size found from using UBT ProfileUnitySizes mode

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "AnimGraphRuntime",
                    "MovieScene",
                    "MovieSceneTracks",
                    "PropertyPath",
					"TimeManagement",
					"DeveloperSettings"
				}
            );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "AnimationCore",
                    "LevelSequence",
                    "RigVM",
                    "RHI",
                    "Constraints"
                }
            );

            if (Target.bBuildEditor == true)
            {
                PublicDependencyModuleNames.AddRange(
				    new string[]
					{
						"Slate",
						"SlateCore",
						"RigVMDeveloper",
                        "AnimGraph",
                        "Json",
                        "Serialization",
                        "JsonUtilities",
                        "AnimationBlueprintLibrary",
                    }
                );

                PrivateDependencyModuleNames.AddRange(
                    new string[]
                    {
						"EditorFramework",
                        "UnrealEd",
                        "BlueprintGraph",
                        "PropertyEditor",
                    }
                );

                PrivateIncludePathModuleNames.Add("TLLRN_ControlRigEditor");
                DynamicallyLoadedModuleNames.Add("TLLRN_ControlRigEditor");
            }
        }
    }
}
