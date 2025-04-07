// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class TLLRN_IKRigDeveloper : ModuleRules
    {
        public TLLRN_IKRigDeveloper(ReadOnlyTargetRules Target) : base(Target)
        {
	        PrivateDependencyModuleNames.AddRange(
                new string[]
                {
					"Core",
					"CoreUObject",
					"Engine",
					"TLLRN_IKRig",
                }
            );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                }
            );

            if (Target.bBuildEditor == true)
            {
                PrivateDependencyModuleNames.AddRange(
                    new string[]
                    {
                        "UnrealEd",
                        "AnimGraph",
                        "BlueprintGraph",
                        "Slate",
                        "SlateCore",
                        "Persona"
                    }
                );
            }
        }
    }
}
