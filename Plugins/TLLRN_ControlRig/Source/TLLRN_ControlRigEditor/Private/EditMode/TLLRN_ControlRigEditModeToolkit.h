// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "EditorModes.h"
#include "Toolkits/BaseToolkit.h"
#include "EditorModeManager.h"
#include "EditMode/STLLRN_ControlRigEditModeTools.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"

class STLLRN_ControlRigTweenWidget;
class STLLRN_ControlRigDetails;
class STLLRN_ControlRigOutliner;

class FTLLRN_ControlRigEditModeToolkit : public FModeToolkit
{
public:
	friend class STLLRN_ControlRigTweenWidget;

	FTLLRN_ControlRigEditModeToolkit(FTLLRN_ControlRigEditMode& InEditMode)
		: EditMode(InEditMode)
	{
		
	}

	~FTLLRN_ControlRigEditModeToolkit();

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override { return FName("AnimationMode"); }
	virtual FText GetBaseToolkitName() const override { return NSLOCTEXT("AnimationModeToolkit", "DisplayName", "Animation"); }
	virtual class FEdMode* GetEditorMode() const override { return &EditMode; }
	virtual TSharedPtr<class SWidget> GetInlineContent() const override { return ModeTools; }
	virtual bool ProcessCommandBindings(const FKeyEvent& InKeyEvent) const override
	{
		if (EditMode.GetCommandBindings() && EditMode.GetCommandBindings()->ProcessCommandBindings(InKeyEvent))
		{
			return true;
		}
		return false;
	}
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;

	/** Mode Toolbar Palettes **/
	virtual void GetToolPaletteNames(TArray<FName>& InPaletteName) const override;
	virtual FText GetToolPaletteDisplayName(FName PaletteName) const override;
	virtual void BuildToolPalette(FName PaletteName, class FToolBarBuilder& ToolbarBuilder) override;

	/** Modes Panel Header Information **/
	virtual FText GetActiveToolDisplayName() const override;
	virtual FText GetActiveToolMessage() const override;
	virtual void OnToolPaletteChanged(FName PaletteName) override;

	void TryInvokeToolkitUI(const FName InName);
	bool IsToolkitUIActive(const FName InName) const;

	/** For updating tween values via hot keys*/
	void GetToNextActiveSlider();
	bool CanChangeAnimSliderTool() const;
	void DragAnimSliderTool(double Val);
	void ResetAnimSlider();
	void StartAnimSliderTool();
public:
	static const FName PoseTabName;
	static const FName MotionTrailTabName;
	static const FName TLLRN_AnimLayerTabName;
	static const FName TweenOverlayName;
	static const FName SnapperTabName;
	static const FName DetailsTabName;
	static const FName OutlinerTabName;
	static const FName SpacePickerTabName;

	static TSharedPtr<STLLRN_ControlRigDetails> Details;
	static TSharedPtr<STLLRN_ControlRigOutliner> Outliner;

protected:

	void CreateAndShowTweenOverlay();
	void TryShowTweenOverlay();
	void RemoveAndDestroyTweenOverlay();
	void TryRemoveTweenOverlay();

	void UpdateTweenWidgetLocation(const FVector2D InLocation);

	FMargin GetTweenWidgetPadding() const;

	/* FModeToolkit Interface */
	virtual void RequestModeUITabs() override;
	virtual void InvokeUI() override;

	//this also saves the layout
	void UnregisterAndRemoveFloatingTabs();

	static bool bMotionTrailsTabOpen;
	static bool bTLLRN_AnimLayerTabOpen;
	static bool bPoseTabOpen;
	static bool bSnapperTabOpen;
	static bool bTweenOpen;
private:
	/** The edit mode we are bound to */
	FTLLRN_ControlRigEditMode& EditMode;
	TSharedPtr<SWidget> TweenWidgetParent;
	TSharedPtr<STLLRN_ControlRigTweenWidget> TweenWidget;

	FVector2D InViewportTweenWidgetLocation;
	/** The tools widget */
	TSharedPtr<STLLRN_ControlRigEditModeTools> ModeTools;

};
