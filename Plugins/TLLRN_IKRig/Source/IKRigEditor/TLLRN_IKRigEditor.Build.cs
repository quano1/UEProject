// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
    public class TLLRN_IKRigEditor : ModuleRules
    {
        public TLLRN_IKRigEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
                new string[]
                {
	                "Core",
	                "AdvancedPreviewScene",
                }
            );

			PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "InputCore",
                    "EditorFramework",
                    "UnrealEd",
                    "ToolMenus",
                    "CoreUObject",
                    "Engine",
                    "Slate",
                    "SlateCore",
                    "AssetTools",
                    "EditorWidgets",
                    "Kismet",
                    "KismetWidgets",
                    "Persona",
                    "SkeletonEditor",
                    "ContentBrowser",
                    "AnimationBlueprintLibrary",
                    "MessageLog",

                    "PropertyEditor",
					"BlueprintGraph",
					"AnimGraph",
					"AnimGraphRuntime",
					
					"TLLRN_IKRig",
					"TLLRN_IKRigDeveloper",
					"ToolWidgets",
					"AnimationCore",
                    "AnimationWidgets",
                    "ApplicationCore",
					"MeshDescription",
					"SkeletalMeshDescription"
                }
            );
        }
    }
}
