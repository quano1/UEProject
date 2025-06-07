// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MyProject55EditorTarget : TargetRules
{
	public MyProject55EditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("MyProject55");
		// PublicDefinitions.Add("_CRT_HAS_CXX17=0");
	}
}
