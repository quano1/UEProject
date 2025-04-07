// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyleMacros.h"

class FTLLRN_IKRetargetEditorStyle final	: public FSlateStyleSet
{
public:
	
	FTLLRN_IKRetargetEditorStyle() : FSlateStyleSet("TLLRN_IKRetargetEditorStyle")
	{
		const FVector2D Icon16x16(16.0f, 16.0f);
		const FVector2D Icon64x64(64.0f, 64.0f);
		
		// SetContentRoot(FPaths::EnginePluginsDir() / TEXT("Animation/IKRig/Content"));
		SetContentRoot(FPaths::LaunchDir() / TEXT("Plugins/TLLRN_IKRig/Content"));
		
		Set("TLLRN_IKRetarget.Tree.Bone", new IMAGE_BRUSH("Slate/Bone_16x", Icon16x16));
		Set("ClassIcon.TLLRN_IKRetargeter", new IMAGE_BRUSH_SVG("Slate/IKRigRetargeter", Icon16x16));
		Set("ClassThumbnail.TLLRN_IKRetargeter", new IMAGE_BRUSH_SVG("Slate/IKRigRetargeter_64", Icon64x64));
		
		Set("TLLRN_IKRetarget.AssetSettings", new IMAGE_BRUSH_SVG("Slate/AssetSettings", Icon64x64));
		Set("TLLRN_IKRetarget.GlobalSettings", new IMAGE_BRUSH_SVG("Slate/GlobalSettings", Icon64x64));
		Set("TLLRN_IKRetarget.RootSettings", new IMAGE_BRUSH_SVG("Slate/RootSettings", Icon64x64));
		Set("TLLRN_IKRetarget.PostSettings", new IMAGE_BRUSH_SVG("Slate/PostSettings", Icon64x64));
		Set("TLLRN_IKRetarget.ChainMapping", new IMAGE_BRUSH_SVG("Slate/ChainMapping", Icon64x64));

		Set("TLLRN_IKRetarget.RunRetargeter", new IMAGE_BRUSH_SVG("Slate/RunRetargeter", Icon64x64));
		Set("TLLRN_IKRetarget.EditRetargetPose", new IMAGE_BRUSH_SVG("Slate/EditRetargetPose", Icon64x64));
		Set("TLLRN_IKRetarget.ShowRetargetPose", new IMAGE_BRUSH_SVG("Slate/ShowRetargetPose", Icon64x64));
		Set("TLLRN_IKRetarget.AutoAlign", new IMAGE_BRUSH_SVG("Slate/AutoRetargetPose", Icon16x16));

		SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
		Set( "TLLRN_IKRetarget.Viewport.Border", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(1.0f,1.0f,1.0f,1.0f) ) );
		
		FSlateStyleRegistry::RegisterSlateStyle(*this);
	}

	static FTLLRN_IKRetargetEditorStyle& Get()
	{
		static FTLLRN_IKRetargetEditorStyle Inst;
		return Inst;
	}
	
	~FTLLRN_IKRetargetEditorStyle()
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*this);
	}
};
