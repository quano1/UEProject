// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorStyle.h"

#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/ToolBarStyle.h"
#include "tll/log.h" // TLL_DEV

FName FTLLRN_UVEditorStyle::StyleName("TLLRN_UVStyle");

FTLLRN_UVEditorStyle::FTLLRN_UVEditorStyle()
	: FSlateStyleSet(StyleName)
{
	// Used FFractureEditorStyle as a model

	const FVector2D IconSize(16.0f, 16.0f);
	const FVector2D ToolbarIconSize(20.0f, 20.0f);
#ifdef TLL_DEV
	SetContentRoot(FPaths::LaunchDir() / TEXT("Plugins/TLLRN_UVEditor/Content/Icons"));
#else
	SetContentRoot(FPaths::EnginePluginsDir() / TEXT("Marketplace/TLLRN_UVEditor/Content/Icons"));
#endif
	SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate") );

	Set("TLLRN_UVEditor.OpenTLLRN_UVEditor",				new IMAGE_BRUSH_SVG("TLLRN_UVEditor", IconSize));

	Set("TLLRN_UVEditor.BeginSelectTool",				new FSlateVectorImageBrush(FPaths::EngineContentDir() / TEXT("Slate/Starship/Common/edit.svg"), ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginLayoutTool",             new IMAGE_BRUSH_SVG("UVLayout", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginParameterizeMeshTool",   new IMAGE_BRUSH_SVG("AutoUnwrap", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginChannelEditTool",        new IMAGE_BRUSH_SVG("AttributeEditor", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginSeamTool",               new IMAGE_BRUSH_SVG("ModelingUVSeamEdit", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginRecomputeUVsTool",       new IMAGE_BRUSH_SVG("GroupUnwrap", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginTransformTool",          new IMAGE_BRUSH_SVG("TransformUVs", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginAlignTool",              new IMAGE_BRUSH_SVG("AlignLeft", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginDistributeTool",         new IMAGE_BRUSH_SVG("DistributeHorizontally", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginTexelDensityTool",       new IMAGE_BRUSH_SVG("TexelDensity", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginBrushSelectTool",        new IMAGE_BRUSH("MeshSelect_40x", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginBrushSelectTool.Small",  new IMAGE_BRUSH("MeshSelect_40x", ToolbarIconSize));
	Set("TLLRN_UVEditor.BeginUVSnapshotTool",		    new IMAGE_BRUSH_SVG("UVSnapshot", ToolbarIconSize));
	
	// Select tool actions
	Set("TLLRN_UVEditor.SewAction", new IMAGE_BRUSH_SVG("UVSew", ToolbarIconSize));
	Set("TLLRN_UVEditor.SplitAction", new IMAGE_BRUSH_SVG("UVCut", ToolbarIconSize));
	Set("TLLRN_UVEditor.MakeIslandAction", new IMAGE_BRUSH_SVG("SelectionIslands", ToolbarIconSize));
	Set("TLLRN_UVEditor.IslandConformalUnwrapAction", new IMAGE_BRUSH_SVG("UVUnwrap", ToolbarIconSize));

	// Top toolbar icons
	Set("TLLRN_UVEditor.ApplyChanges", new CORE_IMAGE_BRUSH_SVG("Starship/Common/Apply", ToolbarIconSize));
	Set("TLLRN_UVEditor.ChannelSettings", new CORE_IMAGE_BRUSH_SVG("Starship/Common/SetDrawUVs", ToolbarIconSize));
	Set("TLLRN_UVEditor.BackgroundSettings", new CORE_IMAGE_BRUSH_SVG("Starship/Common/Sprite", ToolbarIconSize));

	// Viewport icons
	Set("TLLRN_UVEditor.OrbitCamera", new CORE_IMAGE_BRUSH_SVG("Starship/EditorViewport/rotate", ToolbarIconSize));
	Set("TLLRN_UVEditor.FlyCamera", new CORE_IMAGE_BRUSH_SVG("Starship/EditorViewport/camera", ToolbarIconSize));
	Set("TLLRN_UVEditor.FocusCamera", new CORE_IMAGE_BRUSH_SVG("Starship/Actors/snap-view-to-object", ToolbarIconSize));

	Set("TLLRN_UVEditor.VertexSelection", new IMAGE_BRUSH_SVG("SelectionVertices", ToolbarIconSize));
	Set("TLLRN_UVEditor.EdgeSelection", new IMAGE_BRUSH_SVG("SelectionLine", ToolbarIconSize));
	Set("TLLRN_UVEditor.TriangleSelection", new IMAGE_BRUSH_SVG("SelectionTriangle", ToolbarIconSize));
	Set("TLLRN_UVEditor.IslandSelection", new IMAGE_BRUSH_SVG("SelectionIslands", ToolbarIconSize));
	Set("TLLRN_UVEditor.FullMeshSelection", new IMAGE_BRUSH_SVG("SelectionMulti", ToolbarIconSize));

	// Distribute
	Set("TLLRN_UVEditor.DistributeSpaceVertically", new IMAGE_BRUSH_SVG("DistributeVertical", ToolbarIconSize));
	Set("TLLRN_UVEditor.DistributeSpaceHorizontally", new IMAGE_BRUSH_SVG("DistributeHorizontal", ToolbarIconSize));
	Set("TLLRN_UVEditor.DistributeTopEdges", new IMAGE_BRUSH_SVG("DistributeEdgesTop", ToolbarIconSize));
	Set("TLLRN_UVEditor.DistributeBottomEdges", new IMAGE_BRUSH_SVG("DistributeEdgesBottom", ToolbarIconSize));
	Set("TLLRN_UVEditor.DistributeLeftEdges", new IMAGE_BRUSH_SVG("DistributeEdgesLeft", ToolbarIconSize));
	Set("TLLRN_UVEditor.DistributeRightEdges", new IMAGE_BRUSH_SVG("DistributeEdgesRight", ToolbarIconSize));
	Set("TLLRN_UVEditor.DistributeCentersHorizontally", new IMAGE_BRUSH_SVG("DistributeCentersHorizontal", ToolbarIconSize));
	Set("TLLRN_UVEditor.DistributeCentersVertically", new IMAGE_BRUSH_SVG("DistributeCentersVertical", ToolbarIconSize));
	Set("TLLRN_UVEditor.DistributeRemoveOverlap", new IMAGE_BRUSH_SVG("RemoveOverlap", ToolbarIconSize));

	// Align
	Set("TLLRN_UVEditor.AlignDirectionBottomEdges", new IMAGE_BRUSH_SVG("AlignEdgesBottom", ToolbarIconSize));
	Set("TLLRN_UVEditor.AlignDirectionTopEdges", new IMAGE_BRUSH_SVG("AlignEdgesTop", ToolbarIconSize));
	Set("TLLRN_UVEditor.AlignDirectionLeftEdges", new IMAGE_BRUSH_SVG("AlignEdgesLeft", ToolbarIconSize));
	Set("TLLRN_UVEditor.AlignDirectionRightEdges", new IMAGE_BRUSH_SVG("AlignEdgesRight", ToolbarIconSize));
	Set("TLLRN_UVEditor.AlignDirectionCentersHorizontally", new IMAGE_BRUSH_SVG("AlignCentersHorizontal", ToolbarIconSize));
	Set("TLLRN_UVEditor.AlignDirectionCentersVertically", new IMAGE_BRUSH_SVG("AlignCentersVertical", ToolbarIconSize));

	// Style for the toolbar in the SeamTool customization
	{
		// For the selection button toolbar, we want to use something similar to the toolbar we use in the viewport
		Set("SeamTool.ModeToolbar", FAppStyle::Get().GetWidgetStyle<FToolBarStyle>("EditorViewportToolBar"));

		// However, increase the size of the buttons a bit
		FCheckBoxStyle ToggleButtonStart = FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("EditorViewportToolBar.ToggleButton.Start");
		ToggleButtonStart.SetPadding(FMargin(9, 7, 6, 7));
		Set("SeamTool.ModeToolbar.ToggleButton.Start", ToggleButtonStart);

		FCheckBoxStyle ToggleButtonMiddle = FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("EditorViewportToolBar.ToggleButton.Middle");
		ToggleButtonMiddle.SetPadding(FMargin(9, 7, 6, 7));
		Set("SeamTool.ModeToolbar.ToggleButton.Middle", ToggleButtonMiddle);

		FCheckBoxStyle ToggleButtonEnd = FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("EditorViewportToolBar.ToggleButton.End");
		ToggleButtonEnd.SetPadding(FMargin(7, 7, 8, 7));
		Set("SeamTool.ModeToolbar.ToggleButton.End", ToggleButtonEnd);
	}

	FSlateStyleRegistry::RegisterSlateStyle(*this);
}

FTLLRN_UVEditorStyle::~FTLLRN_UVEditorStyle()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*this);
}

FTLLRN_UVEditorStyle& FTLLRN_UVEditorStyle::Get()
{
	static FTLLRN_UVEditorStyle Inst;
	return Inst;
}


