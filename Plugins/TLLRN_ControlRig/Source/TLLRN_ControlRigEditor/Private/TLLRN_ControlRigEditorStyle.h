// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleColors.h"
#include "Brushes/SlateRoundedBoxBrush.h"

#define TTF_FONT(RelativePath, ...) FSlateFontInfo(RootToContentDir(RelativePath, TEXT(".ttf")), __VA_ARGS__)
#define OTF_FONT(RelativePath, ...) FSlateFontInfo(RootToContentDir(RelativePath, TEXT(".otf")), __VA_ARGS__)

class FTLLRN_ControlRigEditorStyle final
	: public FSlateStyleSet
{
	class FContentRootBracket
	{
	public:
		FContentRootBracket(FTLLRN_ControlRigEditorStyle* InStyle, const FString& NewContentRoot)
			: Style(InStyle)
			, PreviousContentRoot(InStyle->GetContentRootDir())
		{
			Style->SetContentRoot(NewContentRoot);
		}

		~FContentRootBracket()
		{
			Style->SetContentRoot(PreviousContentRoot);
		}
	private:
		FTLLRN_ControlRigEditorStyle* Style;
		FString PreviousContentRoot;
	};
	
public:
	FTLLRN_ControlRigEditorStyle()
		: FSlateStyleSet("TLLRN_ControlRigEditorStyle")
		, BoneUserInterfaceColor(0.6f, 0.6f, 0.6f)
		, NullUserInterfaceColor(0.75f, 0.75f, 0.75f)
		, ConnectorUserInterfaceColor(0.0, 112.f/255.f, 224.f/255.f)
		, SocketUserInterfaceColor(0.0, 112.f/255.f, 224.f/255.f)
	{
		const FVector2D Icon10x10(10.0f, 10.0f);
		const FVector2D Icon14x14(14.0f, 14.0f);
		const FVector2D Icon16x16(16.0f, 16.0f);
		const FVector2D Icon20x20(20.0f, 20.0f);
		const FVector2D Icon24x24(24.0f, 24.0f);
		const FVector2D Icon32x32(32.0f, 32.0f);
		const FVector2D Icon40x40(40.0f, 40.0f);
		const FVector2D Icon128x128(128.0f, 128.0f);
		const FString TLLRN_ControlRigPluginContentDir = FPaths::EnginePluginsDir() / TEXT("Animation/TLLRN_ControlRig/Content");
		const FString EngineEditorSlateDir = FPaths::EngineContentDir() / TEXT("Editor/Slate");
		SetContentRoot(TLLRN_ControlRigPluginContentDir);
		SetCoreContentRoot(EngineEditorSlateDir);

		const FSlateColor DefaultForeground(FLinearColor(0.72f, 0.72f, 0.72f, 1.f));

		// Class Icons
		{
			Set("ClassIcon.TLLRN_ControlRigSequence", new IMAGE_BRUSH("Slate/TLLRN_ControlRigSequence_16x", Icon16x16));
			Set("ClassIcon.TLLRN_ControlRigBlueprint", new IMAGE_BRUSH("Slate/TLLRN_ControlRig_16", Icon16x16));
			Set("ClassIcon.TLLRN_ControlTLLRN_RigPose", new IMAGE_BRUSH("Slate/TLLRN_ControlTLLRN_RigPose_16", Icon16x16));
		}

		// Editor Icons
		{
			Set("TLLRN_ControlRig.Editor.TabIcon", new IMAGE_BRUSH_SVG("Slate/TLLRN_ControlRigEditorTabIcon_16x", Icon16x16));
		}

		// Sequencer styles
		{
			Set("TLLRN_ControlRig.ExportAnimSequence", new IMAGE_BRUSH("Slate/ExportAnimSequence_24x", Icon24x24));
			Set("TLLRN_ControlRig.ExportAnimSequence.Small", new IMAGE_BRUSH("Slate/ExportAnimSequence_24x", Icon24x24));
			Set("TLLRN_ControlRig.ReExportAnimSequence", new IMAGE_BRUSH("Slate/ExportAnimSequence_24x", Icon24x24));
			Set("TLLRN_ControlRig.ReExportAnimSequence.Small", new IMAGE_BRUSH("Slate/ExportAnimSequence_24x", Icon24x24));
			Set("TLLRN_ControlRig.ImportFromRigSequence", new IMAGE_BRUSH("Slate/ReImportRigSequence_16x", Icon16x16));
			Set("TLLRN_ControlRig.ImportFromRigSequence.Small", new IMAGE_BRUSH("Slate/ReImportRigSequence_16x", Icon16x16));
			Set("TLLRN_ControlRig.ReImportFromRigSequence", new IMAGE_BRUSH("Slate/ReImportRigSequence_16x", Icon16x16));
			Set("TLLRN_ControlRig.ReImportFromRigSequence.Small", new IMAGE_BRUSH("Slate/ReImportRigSequence_16x", Icon16x16));
		}

		//Tool Styles
		{
			Set("TLLRN_ControlRig.OnlySelectControls", new IMAGE_BRUSH_SVG("Slate/AnimationSelectOnlyTLLRN_ControlRig", Icon16x16));
			Set("TLLRN_ControlRig.SnapperTool", new IMAGE_BRUSH_SVG("Slate/AnimationTLLRN_ControlRigSnapper", Icon16x16));
			Set("TLLRN_ControlRig.PoseTool", new IMAGE_BRUSH_SVG("Slate/AnimationPoses", Icon16x16));
			Set("TLLRN_ControlRig.CreatePose", new IMAGE_BRUSH_SVG("Slate/AnimationCreatePose", Icon16x16));
			Set("TLLRN_ControlRig.TweenTool", new IMAGE_BRUSH_SVG("Slate/AnimationTweens", Icon16x16));
			Set("TLLRN_ControlRig.EditableMotionTrails", new IMAGE_BRUSH_SVG("Slate/EditableMotionTrails", Icon16x16));
			Set("TLLRN_ControlRig.TemporaryPivot", new IMAGE_BRUSH_SVG("Slate/TemporaryPivot", Icon16x16));
			Set("TLLRN_ControlRig.TLLRN_AnimLayerSelected", new IMAGE_BRUSH_SVG("Slate/TLLRN_AnimLayerSelected", Icon16x16));
			Set("TLLRN_ControlRig.FilterTLLRN_AnimLayerSelected", new IMAGE_BRUSH_SVG("Slate/FilterTLLRN_AnimLayerSelected", Icon16x16));
			Set("TLLRN_ControlRig.TLLRN_TLLRN_AnimLayers", new IMAGE_BRUSH_SVG("Slate/TLLRN_TLLRN_AnimLayers", Icon16x16));

		}
		// Control Rig Editor styles
		{
			// tab icons
			Set("TLLRN_RigHierarchy.TabIcon", new IMAGE_BRUSH_SVG("Slate/TLLRN_RigHierarchy", Icon16x16));
			Set("RigValidation.TabIcon", new IMAGE_BRUSH_SVG("Slate/RigValidation", Icon16x16));
			Set("CurveContainer.TabIcon", new IMAGE_BRUSH_SVG("Slate/CurveContainer", Icon16x16));
			Set("HierarchicalProfiler.TabIcon", new IMAGE_BRUSH_SVG("Slate/HierarchicalProfiler", Icon16x16));
			
			{
				FContentRootBracket Bracket(this, EngineEditorSlateDir);
				Set("TLLRN_ControlRig.ConstructionMode", new IMAGE_BRUSH_SVG("Starship/Common/Adjust", Icon40x40));
				Set("TLLRN_ControlRig.ConstructionMode.Small", new IMAGE_BRUSH_SVG("Starship/Common/Adjust", Icon20x20));
				Set("TLLRN_ControlRig.ForwardsSolveEvent", new IMAGE_BRUSH("Icons/diff_next_40x", Icon40x40));
				Set("TLLRN_ControlRig.BackwardsSolveEvent", new IMAGE_BRUSH("Icons/diff_prev_40x", Icon40x40));
				Set("TLLRN_ControlRig.BackwardsAndForwardsSolveEvent", new IMAGE_BRUSH("Icons/Loop_40x", Icon40x40));
			}

			{
				FContentRootBracket Bracket(this, EngineEditorSlateDir);
				// similar style to "LevelViewport.StartingPlayInEditorBorder"
				Set( "TLLRN_ControlRig.Viewport.Border", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(1.0f,1.0f,1.0f,1.0f) ) );
				// similar style to "AnimViewport.Notification.Warning"
				Set( "TLLRN_ControlRig.Viewport.Notification.ChangeShapeTransform", new BOX_BRUSH("Common/RoundedSelection_16x", 4.0f/16.0f, FLinearColor(FColor(169, 0, 148))));
				// similar style to "AnimViewport.Notification.Warning"
				Set( "TLLRN_ControlRig.Viewport.Notification.DirectManipulation", new BOX_BRUSH("Common/RoundedSelection_16x", 4.0f/16.0f, FLinearColor(FColor(0, 112, 224))));
			}
		}

		// Tree styles
		{
			Set("TLLRN_ControlRig.Tree.BoneUser", new IMAGE_BRUSH("Slate/BoneNonWeighted_16x", Icon16x16));
			Set("TLLRN_ControlRig.Tree.BoneImported", new IMAGE_BRUSH("Slate/Bone_16x", Icon16x16));
			Set("TLLRN_ControlRig.Tree.Control", new IMAGE_BRUSH("Slate/TLLRN_RigControlCircle_16x", Icon16x16));
			Set("TLLRN_ControlRig.Tree.ProxyControl", new IMAGE_BRUSH("Slate/ProxyControl1_16x", Icon16x16));
			Set("TLLRN_ControlRig.Tree.Null", new IMAGE_BRUSH("Slate/Null_16x", Icon16x16));
			Set("TLLRN_ControlRig.Tree.RigidBody", new IMAGE_BRUSH("Slate/RigidBody_16x", Icon16x16));
			Set("TLLRN_ControlRig.Tree.Socket_Open", new IMAGE_BRUSH_SVG("Slate/Socket_Open", Icon16x16));
			Set("TLLRN_ControlRig.Tree.Socket_Closed", new IMAGE_BRUSH_SVG("Slate/Socket_Closed", Icon16x16));
			{
				FContentRootBracket Bracket(this, EngineEditorSlateDir);
				Set("TLLRN_ControlRig.Tree.Connector", new IMAGE_BRUSH_SVG("Starship/Common/SetShowSockets", Icon16x16));
			}
		}

		// Font?
		{
			Set("TLLRN_ControlRig.Hierarchy.Menu", TTF_FONT("Fonts/Roboto-Regular", 12));
		}

		// Space picker
		SpacePickerSelectColor = FStyleColors::Select;
		{
			Set("TLLRN_ControlRig.SpacePicker.RoundedRect", new FSlateRoundedBoxBrush(FStyleColors::White, 4.0f, FStyleColors::Transparent, 0.0f));
		}

		// Test Data
		{
			Set("TLLRN_ControlRig.TestData.Record", new IMAGE_BRUSH("Slate/RecordingIndicator", Icon32x32));
		}

		Set("TLLRN_ControlRig.ConnectorPrimary", new IMAGE_BRUSH_SVG("Slate/Connector_Primary", Icon128x128));
		Set("TLLRN_ControlRig.ConnectorSecondary", new IMAGE_BRUSH_SVG("Slate/Connector_Secondary", Icon128x128));
		Set("TLLRN_ControlRig.ConnectorOptional", new IMAGE_BRUSH_SVG("Slate/Connector_Optional", Icon128x128));
		Set("TLLRN_ControlRig.ConnectorWarning", new IMAGE_BRUSH_SVG("Slate/Connector_Warning", Icon128x128));

		// Schematic
		{
			Set("TLLRN_ControlRig.Schematic.ConnectorPrimary", new IMAGE_BRUSH_SVG("Slate/Connector_Primary_Schematic", Icon128x128));
			Set("TLLRN_ControlRig.Schematic.ConnectorSecondary", new IMAGE_BRUSH_SVG("Slate/Connector_Secondary_Schematic", Icon128x128));
			Set("TLLRN_ControlRig.Schematic.ConnectorOptional", new IMAGE_BRUSH_SVG("Slate/Connector_Optional_Schematic", Icon128x128));
			Set("TLLRN_ControlRig.Schematic.ConnectorWarning", new IMAGE_BRUSH_SVG("Slate/Connector_Warning_Schematic", Icon128x128));
			Set("TLLRN_ControlRig.Schematic.Bone", new IMAGE_BRUSH_SVG("Slate/Bone_Schematic", Icon128x128));
			Set("TLLRN_ControlRig.Schematic.Control", new IMAGE_BRUSH_SVG("Slate/Control_Schematic", Icon128x128));
			Set("TLLRN_ControlRig.Schematic.Null", new IMAGE_BRUSH_SVG("Slate/Null_Schematic", Icon128x128));
			Set("TLLRN_ControlRig.Schematic.Link", new IMAGE_BRUSH_SVG("Slate/Link_Schematic", Icon128x128));
		}

		// Constraint Manager Icons
		{
			const FButtonStyle ConstraintOptionButton = FButtonStyle()
				.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Transparent, 4.f))
				.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Hover, 4.f))
				.SetHoveredForeground(FLinearColor::White)
				.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Hover, 4.f))
				.SetPressedForeground(FLinearColor::White)
				.SetPressedPadding(FMargin(0.0, 1.0, 0.0, 0.0));

			const FString EngineSlateDir = FPaths::EngineContentDir() / TEXT("Slate");
			FContentRootBracket Bracket(this, EngineSlateDir);		

			FComboButtonStyle ConstraintComboButton = FComboButtonStyle()
				.SetButtonStyle(ConstraintOptionButton)
				.SetDownArrowImage(IMAGE_BRUSH_SVG("Starship/Common/ellipsis-vertical-narrow", FVector2f(6.f, 15.f)));
			ConstraintComboButton.ButtonStyle = ConstraintOptionButton;
			
			Set("ConstraintManager.ComboButton", ConstraintComboButton);
		}
		
		FSlateStyleRegistry::RegisterSlateStyle(*this);
	}

	static FTLLRN_ControlRigEditorStyle& Get()
	{
		static FTLLRN_ControlRigEditorStyle Inst;
		return Inst;
	}
	
	~FTLLRN_ControlRigEditorStyle()
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*this);
	}

	FSlateColor SpacePickerSelectColor;
	FLinearColor BoneUserInterfaceColor;
	FLinearColor NullUserInterfaceColor;
	FLinearColor ConnectorUserInterfaceColor;
	FLinearColor SocketUserInterfaceColor;
};

#undef TTF_FONT
#undef OTF_FONT
