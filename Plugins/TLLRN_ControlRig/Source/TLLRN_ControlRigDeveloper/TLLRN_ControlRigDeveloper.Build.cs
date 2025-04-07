// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class TLLRN_ControlRigDeveloper : ModuleRules
    {
        public TLLRN_ControlRigDeveloper(ReadOnlyTargetRules Target) : base(Target)
        {
			// Copying some these from TLLRN_ControlRig.build.cs, our deps are likely leaner
			// and therefore these could be pruned if needed:
			PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "AnimGraphRuntime",
                    "TLLRN_ControlRig",
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "KismetCompiler",
                    "MovieScene",
                    "MovieSceneTracks",
                    "PropertyPath",
                    "Slate",
                    "SlateCore",
                    "InputCore",
                    "TimeManagement",
					"EditorWidgets",
					"MessageLog",
                    "RigVM",
					"ToolWidgets",
                }
            );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "AnimationCore",
                    "VisualGraphUtils",
                }
            );

            if (Target.bBuildEditor == true)
            {
                PrivateDependencyModuleNames.AddRange(
                    new string[]
                    {
						"EditorFramework",
                        "UnrealEd",
						"Kismet",
                        "AnimGraph",
                        "BlueprintGraph",
                        "PropertyEditor",
                        "RigVMDeveloper",
						"GraphEditor",
						"ApplicationCore",
						"RigVMEditor",
					}
                );

                PrivateIncludePathModuleNames.Add("TLLRN_ControlRigEditor");
                DynamicallyLoadedModuleNames.Add("TLLRN_ControlRigEditor");
            }
        }
    }
}
