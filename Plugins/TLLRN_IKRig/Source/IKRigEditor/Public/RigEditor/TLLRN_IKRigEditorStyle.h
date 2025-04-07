// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

class FTLLRN_IKRigEditorStyle	final : public FSlateStyleSet
{
public:
	
	FTLLRN_IKRigEditorStyle() : FSlateStyleSet("TLLRN_IKRigEditorStyle")
	{
		const FVector2D Icon16x16(16.0f, 16.0f);
		const FVector2D Icon20x20(20.0f, 20.0f);
		const FVector2D Icon40x40(40.0f, 40.0f);
		const FVector2D Icon64x64(64.0f, 64.0f);

		SetContentRoot(FPaths::EnginePluginsDir() / TEXT("Animation/IKRig/Content"));
		
		Set("TLLRN_IKRig.Tree.Bone", new IMAGE_BRUSH("Slate/Bone_16x", Icon16x16));
		Set("TLLRN_IKRig.Tree.BoneWithSettings", new IMAGE_BRUSH("Slate/BoneWithSettings_16x", Icon16x16));
		Set("TLLRN_IKRig.Tree.Goal", new IMAGE_BRUSH("Slate/Goal_16x", Icon16x16));
		Set("TLLRN_IKRig.Tree.Effector", new IMAGE_BRUSH("Slate/Effector_16x", Icon16x16));
		Set("TLLRN_IKRig.TabIcon", new IMAGE_BRUSH("Slate/Tab_16x", Icon16x16));
		Set("TLLRN_IKRig.Solver", new IMAGE_BRUSH("Slate/Solver_16x", Icon16x16));
		Set("TLLRN_IKRig.DragSolver", new IMAGE_BRUSH("Slate/DragSolver", FVector2D(6, 15)));
		Set("TLLRN_IKRig.Reset", new IMAGE_BRUSH("Slate/Reset", Icon40x40));
		Set("TLLRN_IKRig.Reset.Small", new IMAGE_BRUSH("Slate/Reset", Icon20x20));

		Set("ClassIcon.TLLRN_IKRigDefinition", new IMAGE_BRUSH_SVG("Slate/IKRig", Icon16x16));
		Set("ClassThumbnail.TLLRN_IKRigDefinition", new IMAGE_BRUSH_SVG("Slate/IKRig_64", Icon64x64));

		Set("TLLRN_IKRig.SolverStack", new IMAGE_BRUSH_SVG("Slate/SolverStack", Icon64x64));
		Set("TLLRN_IKRig.IKRetargeting", new IMAGE_BRUSH_SVG("Slate/IKRetargeting", Icon64x64));
		Set("TLLRN_IKRig.Hierarchy", new IMAGE_BRUSH_SVG("Slate/Hierarchy", Icon64x64));

		Set("TLLRN_IKRig.AssetSettings", new IMAGE_BRUSH_SVG("Slate/AssetSettings", Icon64x64));
		Set("TLLRN_IKRig.AutoRetarget", new IMAGE_BRUSH_SVG("Slate/AutoChainRetarget", Icon64x64));
		Set("TLLRN_IKRig.AutoIK", new IMAGE_BRUSH_SVG("Slate/AutoFullBodyIK", Icon64x64));
		
		FTextBlockStyle NormalText = FAppStyle::GetWidgetStyle<FTextBlockStyle>("SkeletonTree.NormalFont");
		Set( "TLLRN_IKRig.Tree.NormalText", FTextBlockStyle(NormalText));
		Set( "TLLRN_IKRig.Tree.ItalicText", FTextBlockStyle(NormalText).SetFont(DEFAULT_FONT("Italic", 10)));

		SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
		Set( "TLLRN_IKRig.Viewport.Border", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(1.0f,1.0f,1.0f,1.0f) ) );
		
		FSlateStyleRegistry::RegisterSlateStyle(*this);
	}

	static FTLLRN_IKRigEditorStyle& Get()
	{
		static FTLLRN_IKRigEditorStyle Inst;
		return Inst;
	}
	
	~FTLLRN_IKRigEditorStyle()
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*this);
	}
};