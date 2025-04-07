// Copyright Epic Games, Inc. All Rights Reserved.

#include "STLLRN_UVEditor3DViewportToolBar.h"

#include "EditorViewportCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SEditorViewportToolBarMenu.h"
#include "SEditorViewportViewMenu.h"
#include "STLLRN_UVEditor3DViewport.h"
#include "Styling/AppStyle.h"
#include "TLLRN_UVEditorCommands.h"
#include "TLLRN_UVEditorStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "STLLRN_UVEditor3DViewportToolBar"

void STLLRN_UVEditor3DViewportToolBar::Construct(const FArguments& InArgs, TSharedPtr<class STLLRN_UVEditor3DViewport> InTLLRN_UVEditor3DViewport)
{
	TLLRN_UVEditor3DViewportPtr = InTLLRN_UVEditor3DViewport;
	CommandList = InArgs._CommandList;

	const FMargin ToolbarSlotPadding(4.0f, 1.0f);
	TSharedPtr<SHorizontalBox> MainBoxPtr;

	ChildSlot
	[
		SNew( SBorder )
		.BorderImage(FAppStyle::Get().GetBrush("EditorViewportToolBar.Background"))
		.Cursor(EMouseCursor::Default)
		[
			SAssignNew( MainBoxPtr, SHorizontalBox )
		]
	];

	MainBoxPtr->AddSlot()
		.Padding(ToolbarSlotPadding)
		.HAlign(HAlign_Left)
		[
			MakeDisplayToolBar(InArgs._Extenders)
		];

	MainBoxPtr->AddSlot()
		.Padding(ToolbarSlotPadding)
		.HAlign(HAlign_Right)
		[
			MakeToolBar(InArgs._Extenders)
		];

	SViewportToolBar::Construct(SViewportToolBar::FArguments());
}

TSharedRef<SWidget> STLLRN_UVEditor3DViewportToolBar::MakeDisplayToolBar(const TSharedPtr<FExtender> InExtenders)
{
	TSharedRef<SEditorViewport> ViewportRef = StaticCastSharedPtr<SEditorViewport>(TLLRN_UVEditor3DViewportPtr.Pin()).ToSharedRef();

	return SNew(SEditorViewportViewMenu, ViewportRef, SharedThis(this))
		.Cursor(EMouseCursor::Default)
		.MenuExtenders(InExtenders);
}

TSharedRef<SWidget> STLLRN_UVEditor3DViewportToolBar::MakeToolBar(const TSharedPtr<FExtender> InExtenders)
{
	// The following is modeled after portions of STransformViewportToolBar, which gets 
	// used in SCommonEditorViewportToolbarBase.

	// The buttons are hooked up to actual functions via command bindings in STLLRN_UVEditor3DViewport::BindCommands(),
	// and the toolbar gets built in STLLRN_UVEditor3DViewport::MakeViewportToolbar().

	FSlimHorizontalToolBarBuilder ToolbarBuilder(CommandList, FMultiBoxCustomization::None, InExtenders);

	//// Use a custom style
	FName ToolBarStyle = "EditorViewportToolBar";
	ToolbarBuilder.SetStyle(&FAppStyle::Get(), ToolBarStyle);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);

	ToolbarBuilder.BeginSection("OrbitFlyToggle");
	{
		ToolbarBuilder.BeginBlockGroup();
	    
		// TODO: Right now we're (sort-of) hardcoding the icons in here. We should have a style set for the uv
		// editor that sets the correct icons for these.

		// Orbit Camera
		static FName OrbitCameraName = FName(TEXT("OrbitCamera"));
		ToolbarBuilder.AddToolBarButton(FTLLRN_UVEditorCommands::Get().EnableOrbitCamera, NAME_None, TAttribute<FText>(), TAttribute<FText>(), 
			TAttribute<FSlateIcon>(FSlateIcon(FTLLRN_UVEditorStyle::Get().GetStyleSetName(), "TLLRN_UVEditor.OrbitCamera")), OrbitCameraName);

		// Fly Camera
		static FName FlyCameraName = FName(TEXT("FlyCamera"));
		ToolbarBuilder.AddToolBarButton(FTLLRN_UVEditorCommands::Get().EnableFlyCamera, NAME_None, TAttribute<FText>(), TAttribute<FText>(),
			TAttribute<FSlateIcon>(FSlateIcon(FTLLRN_UVEditorStyle::Get().GetStyleSetName(), "TLLRN_UVEditor.FlyCamera")), FlyCameraName);

		ToolbarBuilder.EndBlockGroup();

		ToolbarBuilder.BeginBlockGroup();

		// Focus Camera
		static FName FocusCameraName = FName(TEXT("FocusCamera"));
		ToolbarBuilder.AddToolBarButton(FTLLRN_UVEditorCommands::Get().SetFocusCamera, NAME_None, TAttribute<FText>(), TAttribute<FText>(),
			TAttribute<FSlateIcon>(FSlateIcon(FTLLRN_UVEditorStyle::Get().GetStyleSetName(), "TLLRN_UVEditor.FocusCamera")), FocusCameraName);

		ToolbarBuilder.EndBlockGroup();
	}

	ToolbarBuilder.EndSection();

	return ToolbarBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
