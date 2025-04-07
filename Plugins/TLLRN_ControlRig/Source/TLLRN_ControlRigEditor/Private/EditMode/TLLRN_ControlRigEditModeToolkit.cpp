// Copyright Epic Games, Inc. All Rights Reserved.

/**
* Control Rig Edit Mode Toolkit
*/
#include "EditMode/TLLRN_ControlRigEditModeToolkit.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "EditorModes.h"
#include "Toolkits/BaseToolkit.h"
#include "EditorModeManager.h"
#include "EditMode/STLLRN_ControlRigEditModeTools.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "Modules/ModuleManager.h"
#include "EditMode/STLLRN_ControlRigBaseListWidget.h"
#include "EditMode/STLLRN_ControlRigTweenWidget.h"
#include "EditMode/STLLRN_ControlRigSnapper.h"
#include "Tools/SMotionTrailOptions.h"
#include "Toolkits/AssetEditorModeUILayer.h"
#include "Widgets/Docking/SDockTab.h"
#include "EditMode/TLLRN_ControlRigEditModeSettings.h"
#include "EditMode/STLLRN_ControlRigDetails.h"
#include "EditMode/STLLRN_ControlRigOutliner.h"
#include "EditMode/STLLRN_ControlTLLRN_RigSpacePicker.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "Sequencer/TLLRN_TLLRN_AnimLayers/STLLRN_TLLRN_AnimLayers.h"

#define LOCTEXT_NAMESPACE "FTLLRN_ControlRigEditModeToolkit"

namespace 
{
	static const FName AnimationName(TEXT("Animation")); 
	const TArray<FName> AnimationPaletteNames = { AnimationName };
}

bool FTLLRN_ControlRigEditModeToolkit::bMotionTrailsTabOpen = false;
bool FTLLRN_ControlRigEditModeToolkit::bTLLRN_AnimLayerTabOpen = false;
bool FTLLRN_ControlRigEditModeToolkit::bPoseTabOpen = false;
bool FTLLRN_ControlRigEditModeToolkit::bSnapperTabOpen = false;
bool FTLLRN_ControlRigEditModeToolkit::bTweenOpen = false;

const FName FTLLRN_ControlRigEditModeToolkit::PoseTabName = FName(TEXT("PoseTab"));
const FName FTLLRN_ControlRigEditModeToolkit::MotionTrailTabName = FName(TEXT("MotionTrailTab"));
const FName FTLLRN_ControlRigEditModeToolkit::SnapperTabName = FName(TEXT("SnapperTab"));
const FName FTLLRN_ControlRigEditModeToolkit::TLLRN_AnimLayerTabName = FName(TEXT("TLLRN_AnimLayerTab"));
const FName FTLLRN_ControlRigEditModeToolkit::TweenOverlayName = FName(TEXT("TweenOverlay"));
const FName FTLLRN_ControlRigEditModeToolkit::OutlinerTabName = FName(TEXT("TLLRN_ControlRigOutlinerTab"));
const FName FTLLRN_ControlRigEditModeToolkit::DetailsTabName = FName(TEXT("TLLRN_ControlRigDetailsTab"));
const FName FTLLRN_ControlRigEditModeToolkit::SpacePickerTabName = FName(TEXT("TLLRN_ControlTLLRN_RigSpacePicker"));

TSharedPtr<STLLRN_ControlRigDetails> FTLLRN_ControlRigEditModeToolkit::Details = nullptr;
TSharedPtr<STLLRN_ControlRigOutliner> FTLLRN_ControlRigEditModeToolkit::Outliner = nullptr;

FTLLRN_ControlRigEditModeToolkit::~FTLLRN_ControlRigEditModeToolkit()
{
	if (ModeTools)
	{
		ModeTools->Cleanup();
	}
}

void FTLLRN_ControlRigEditModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	SAssignNew(ModeTools, STLLRN_ControlRigEditModeTools, SharedThis(this), EditMode);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;

	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bSearchInitialKeyFocus = false;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = true;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	FModeToolkit::Init(InitToolkitHost);
}

void FTLLRN_ControlRigEditModeToolkit::GetToolPaletteNames(TArray<FName>& InPaletteName) const
{
	InPaletteName = AnimationPaletteNames;
}

FText FTLLRN_ControlRigEditModeToolkit::GetToolPaletteDisplayName(FName PaletteName) const
{
	if (PaletteName == AnimationName)
	{
		FText::FromName(AnimationName);
	}
	return FText();
}

void FTLLRN_ControlRigEditModeToolkit::BuildToolPalette(FName PaletteName, class FToolBarBuilder& ToolBarBuilder)
{
	if (PaletteName == AnimationName)
	{
		ModeTools->CustomizeToolBarPalette(ToolBarBuilder);
	}
}

void FTLLRN_ControlRigEditModeToolkit::OnToolPaletteChanged(FName PaletteName)
{

}

void FTLLRN_ControlRigEditModeToolkit::TryInvokeToolkitUI(const FName InName)
{
	TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();

	if (InName == MotionTrailTabName)
	{
		FTabId TabID(MotionTrailTabName);
		ModeUILayerPtr->GetTabManager()->TryInvokeTab(TabID, false /*bIsActive*/);
	}
	else if (InName == TLLRN_AnimLayerTabName)
	{
		FTabId TabID(TLLRN_AnimLayerTabName);
		ModeUILayerPtr->GetTabManager()->TryInvokeTab(TabID, false /*bIsActive*/);
	}
	else if (InName == PoseTabName)
	{
		FTabId TabID(PoseTabName);
		ModeUILayerPtr->GetTabManager()->TryInvokeTab(TabID, false /*bIsActive*/);
	}
	else if (InName == SnapperTabName)
	{
		FTabId TabID(SnapperTabName);
		ModeUILayerPtr->GetTabManager()->TryInvokeTab(TabID, false /*bIsActive*/);
	}
	else if (InName == OutlinerTabName)
	{
		ModeUILayerPtr->GetTabManager()->TryInvokeTab(UAssetEditorUISubsystem::TopRightTabID);
	}
	else if (InName == SpacePickerTabName)
	{
		ModeUILayerPtr->GetTabManager()->TryInvokeTab(UAssetEditorUISubsystem::BottomLeftTabID);
	}
	else if (InName == DetailsTabName)
	{
		ModeUILayerPtr->GetTabManager()->TryInvokeTab(UAssetEditorUISubsystem::BottomRightTabID);
	}
	else if (InName == TweenOverlayName)
	{
		if(TweenWidgetParent)
		{ 
			RemoveAndDestroyTweenOverlay();
		}
		else
		{
			CreateAndShowTweenOverlay();
		}
	}
}

bool FTLLRN_ControlRigEditModeToolkit::IsToolkitUIActive(const FName InName) const
{
	if (InName == TweenOverlayName)
	{
		return TweenWidgetParent.IsValid();
	}

	TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
	return ModeUILayerPtr->GetTabManager()->FindExistingLiveTab(FTabId(InName)).IsValid();
}

FText FTLLRN_ControlRigEditModeToolkit::GetActiveToolDisplayName() const
{
	return ModeTools->GetActiveToolName();
}

FText FTLLRN_ControlRigEditModeToolkit::GetActiveToolMessage() const
{

	return ModeTools->GetActiveToolMessage();
}

TSharedRef<SDockTab> SpawnPoseTab(const FSpawnTabArgs& Args, TWeakPtr<FTLLRN_ControlRigEditModeToolkit> SharedToolkit)
{
	return SNew(SDockTab)
		[
			SNew(STLLRN_ControlRigBaseListWidget)
			.InSharedToolkit(SharedToolkit)
		];
}

TSharedRef<SDockTab> SpawnSnapperTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		[
			SNew(STLLRN_ControlRigSnapper)
		];
}

TSharedRef<SDockTab> SpawnMotionTrailTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		[
			SNew(SMotionTrailOptions)
		];
}

TSharedRef<SDockTab> SpawnTLLRN_AnimLayerTab(const FSpawnTabArgs& Args, FTLLRN_ControlRigEditMode* InEditorMode)
{
	return SNew(SDockTab)
		[
			SNew(STLLRN_TLLRN_AnimLayers, *InEditorMode)
		];
}

TSharedRef<SDockTab> SpawnOutlinerTab(const FSpawnTabArgs& Args, FTLLRN_ControlRigEditMode* InEditorMode)
{
	return SNew(SDockTab)
		[
			SAssignNew(FTLLRN_ControlRigEditModeToolkit::Outliner, STLLRN_ControlRigOutliner, *InEditorMode)
		];
}

TSharedRef<SDockTab> SpawnSpacePickerTab(const FSpawnTabArgs& Args, FTLLRN_ControlRigEditMode* InEditorMode)
{
	return SNew(SDockTab)
		[
			SNew(STLLRN_ControlTLLRN_RigSpacePicker, *InEditorMode)
		];
}

TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args, FTLLRN_ControlRigEditMode* InEditorMode)
{
	return SNew(SDockTab)
		[
			SAssignNew(FTLLRN_ControlRigEditModeToolkit::Details,STLLRN_ControlRigDetails, *InEditorMode)
		];
}


void FTLLRN_ControlRigEditModeToolkit::CreateAndShowTweenOverlay()
{
	FVector2D NewTweenWidgetLocation = GetDefault<UTLLRN_ControlRigEditModeSettings>()->LastInViewportTweenWidgetLocation;

	if (NewTweenWidgetLocation.IsZero())
	{
		const FVector2D ActiveViewportSize = GetToolkitHost()->GetActiveViewportSize();
		NewTweenWidgetLocation.X = ActiveViewportSize.X / 2.0f;
		NewTweenWidgetLocation.Y = FMath::Max(ActiveViewportSize.Y - 100.0f, 0);
	}
	
	UpdateTweenWidgetLocation(NewTweenWidgetLocation);

	SAssignNew(TweenWidgetParent, SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Left)
		.Padding(TAttribute<FMargin>(this, &FTLLRN_ControlRigEditModeToolkit::GetTweenWidgetPadding))
		[
			SAssignNew(TweenWidget, STLLRN_ControlRigTweenWidget)
			.InOwningToolkit(SharedThis(this))
			.InOwningEditMode(SharedThis(&EditMode))
		];

	TryShowTweenOverlay();
}

void FTLLRN_ControlRigEditModeToolkit::GetToNextActiveSlider()
{
	if (TweenWidgetParent && TweenWidget)
	{
		TweenWidget->GetToNextActiveSlider();
	}
}
bool FTLLRN_ControlRigEditModeToolkit::CanChangeAnimSliderTool() const
{
	return (TweenWidgetParent && TweenWidget);
}

void FTLLRN_ControlRigEditModeToolkit::DragAnimSliderTool(double Val)
{
	if (TweenWidgetParent && TweenWidget)
	{
		TweenWidget->DragAnimSliderTool(Val);
	}
}
void FTLLRN_ControlRigEditModeToolkit::ResetAnimSlider()
{
	if (TweenWidgetParent && TweenWidget)
	{
		TweenWidget->ResetAnimSlider();
	}
}

void FTLLRN_ControlRigEditModeToolkit::StartAnimSliderTool()
{
	if (TweenWidgetParent && TweenWidget)
	{
		TweenWidget->StartAnimSliderTool();
	}
}
void FTLLRN_ControlRigEditModeToolkit::TryShowTweenOverlay()
{
	if (TweenWidgetParent)
	{
		GetToolkitHost()->AddViewportOverlayWidget(TweenWidgetParent.ToSharedRef());
	}
}

void FTLLRN_ControlRigEditModeToolkit::RemoveAndDestroyTweenOverlay()
{
	TryRemoveTweenOverlay();
	if (TweenWidgetParent)
	{
		TweenWidgetParent.Reset();
		TweenWidget.Reset();
	}
}

void FTLLRN_ControlRigEditModeToolkit::TryRemoveTweenOverlay()
{
	if (IsHosted() && TweenWidgetParent)
	{
		if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
		{
			if (TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule->GetFirstLevelEditor())
			{
				for (TSharedPtr<SLevelViewport> LevelViewport : LevelEditor->GetViewports())
				{
					if (LevelViewport.IsValid())
					{
						if (TweenWidgetParent)
						{
							LevelViewport->RemoveOverlayWidget(TweenWidgetParent.ToSharedRef());
						}
					}
				}
			}
		}
	}
}

void FTLLRN_ControlRigEditModeToolkit::UpdateTweenWidgetLocation(const FVector2D InLocation)
{
	const FVector2D ActiveViewportSize = GetToolkitHost()->GetActiveViewportSize();
	FVector2D ScreenPos = InLocation;

	const float EdgeFactor = 0.97f;
	const float MinX = ActiveViewportSize.X * (1 - EdgeFactor);
	const float MinY = ActiveViewportSize.Y * (1 - EdgeFactor);
	const float MaxX = ActiveViewportSize.X * EdgeFactor;
	const float MaxY = ActiveViewportSize.Y * EdgeFactor;
	const bool bOutside = ScreenPos.X < MinX || ScreenPos.X > MaxX || ScreenPos.Y < MinY || ScreenPos.Y > MaxY;
	if (bOutside)
	{
		// reset the location if it was placed out of bounds
		ScreenPos.X = ActiveViewportSize.X / 2.0f;
		ScreenPos.Y = FMath::Max(ActiveViewportSize.Y - 100.0f, 0);
	}
	InViewportTweenWidgetLocation = ScreenPos;
	UTLLRN_ControlRigEditModeSettings* TLLRN_ControlRigEditModeSettings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
	TLLRN_ControlRigEditModeSettings->LastInViewportTweenWidgetLocation = ScreenPos;
	TLLRN_ControlRigEditModeSettings->SaveConfig();
}

FMargin FTLLRN_ControlRigEditModeToolkit::GetTweenWidgetPadding() const
{
	return FMargin(InViewportTweenWidgetLocation.X, InViewportTweenWidgetLocation.Y, 0, 0);
}

void FTLLRN_ControlRigEditModeToolkit::RequestModeUITabs()
{
	FModeToolkit::RequestModeUITabs();
	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		TSharedRef<FWorkspaceItem> MenuGroup = ModeUILayerPtr->GetModeMenuCategory().ToSharedRef();

		FMinorTabConfig DetailTabInfo;
		DetailTabInfo.OnSpawnTab = FOnSpawnTab::CreateStatic(&SpawnDetailsTab, &EditMode);
		DetailTabInfo.TabLabel = LOCTEXT("TLLRN_ControlRigDetailTab", "Anim Details");
		DetailTabInfo.TabTooltip = LOCTEXT("TLLRN_ControlRigDetailTabTooltip", "Show Details For Selected Controls.");
		ModeUILayerPtr->SetModePanelInfo(UAssetEditorUISubsystem::BottomRightTabID, DetailTabInfo);

		FMinorTabConfig OutlinerTabInfo;
		OutlinerTabInfo.OnSpawnTab = FOnSpawnTab::CreateStatic(&SpawnOutlinerTab, &EditMode);
		OutlinerTabInfo.TabLabel = LOCTEXT("AnimationOutlinerTab", "Anim Outliner");
		OutlinerTabInfo.TabTooltip = LOCTEXT("AnimationOutlinerTabTooltip", "TLLRN Control Rig Controls");
		ModeUILayerPtr->SetModePanelInfo(UAssetEditorUISubsystem::TopRightTabID, OutlinerTabInfo);
		/* doesn't work as expected
		FMinorTabConfig SpawnSpacePickerTabInfo;
		SpawnSpacePickerTabInfo.OnSpawnTab = FOnSpawnTab::CreateStatic(&SpawnSpacePickerTab, &EditMode);
		SpawnSpacePickerTabInfo.TabLabel = LOCTEXT("TLLRN_ControlTLLRN_RigSpacePickerTab", "TLLRN Control Rig Space Picker");
		SpawnSpacePickerTabInfo.TabTooltip = LOCTEXT("TLLRN_ControlTLLRN_RigSpacePickerTabTooltip", "TLLRN Control Rig Space Picker");
		ModeUILayerPtr->SetModePanelInfo(UAssetEditorUISubsystem::TopLeftTabID, SpawnSpacePickerTabInfo);
		*/

		ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(SnapperTabName);
		ModeUILayerPtr->GetTabManager()->RegisterTabSpawner(SnapperTabName, FOnSpawnTab::CreateStatic(&SpawnSnapperTab))
			.SetDisplayName(LOCTEXT("TLLRN_ControlRigSnapperTab", "TLLRN Control Rig Snapper"))
			.SetTooltipText(LOCTEXT("TLLRN_ControlRigSnapperTabTooltip", "Snap child objects to a parent object over a set of frames."))
			.SetGroup(MenuGroup)
			.SetIcon(FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.SnapperTool")));
		ModeUILayerPtr->GetTabManager()->RegisterDefaultTabWindowSize(SnapperTabName, FVector2D(300, 325));

		ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(PoseTabName);

		TWeakPtr<FTLLRN_ControlRigEditModeToolkit> WeakToolkit = SharedThis(this);
		ModeUILayerPtr->GetTabManager()->RegisterTabSpawner(PoseTabName, FOnSpawnTab::CreateLambda([WeakToolkit](const FSpawnTabArgs& Args)
		{
			return SpawnPoseTab(Args, WeakToolkit);
		}))
			.SetDisplayName(LOCTEXT("TLLRN_ControlTLLRN_RigPoseTab", "TLLRN Control Rig Pose"))
			.SetTooltipText(LOCTEXT("TLLRN_ControlTLLRN_RigPoseTabTooltip", "Show Poses."))
			.SetGroup(MenuGroup)
			.SetIcon(FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.PoseTool")));
		ModeUILayerPtr->GetTabManager()->RegisterDefaultTabWindowSize(PoseTabName, FVector2D(675, 625));

		ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(MotionTrailTabName);
		ModeUILayerPtr->GetTabManager()->RegisterTabSpawner(MotionTrailTabName, FOnSpawnTab::CreateStatic(&SpawnMotionTrailTab))
			.SetDisplayName(LOCTEXT("MotionTrailTab", "Motion Trail"))
			.SetTooltipText(LOCTEXT("MotionTrailTabTooltip", "Display motion trails for animated objects."))
			.SetGroup(MenuGroup)
			.SetIcon(FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.EditableMotionTrails")));
		ModeUILayerPtr->GetTabManager()->RegisterDefaultTabWindowSize(MotionTrailTabName, FVector2D(425, 575));

		ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(TLLRN_AnimLayerTabName);
		ModeUILayerPtr->GetTabManager()->RegisterTabSpawner(TLLRN_AnimLayerTabName, FOnSpawnTab::CreateStatic(&SpawnTLLRN_AnimLayerTab,&EditMode))
			.SetDisplayName(LOCTEXT("TLLRN_AnimLayerTab", "Anim Layers"))
			.SetTooltipText(LOCTEXT("AnimationLayerTabTooltip", "Animation layers"))
			.SetGroup(MenuGroup)
			.SetIcon(FSlateIcon(TEXT("TLLRN_ControlRigEditorStyle"), TEXT("TLLRN_ControlRig.TLLRN_TLLRN_AnimLayers")));
		ModeUILayerPtr->GetTabManager()->RegisterDefaultTabWindowSize(TLLRN_AnimLayerTabName, FVector2D(425, 200));



		ModeUILayer.Pin()->ToolkitHostShutdownUI().BindSP(this, &FTLLRN_ControlRigEditModeToolkit::UnregisterAndRemoveFloatingTabs);


	}
};

void FTLLRN_ControlRigEditModeToolkit::InvokeUI()
{
	FModeToolkit::InvokeUI();

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		ModeUILayerPtr->GetTabManager()->TryInvokeTab(UAssetEditorUISubsystem::TopRightTabID);
		// doesn't work as expected todo ModeUILayerPtr->GetTabManager()->TryInvokeTab(UAssetEditorUISubsystem::TopLeftTabID);
		ModeUILayerPtr->GetTabManager()->TryInvokeTab(UAssetEditorUISubsystem::BottomRightTabID);
		if (bTweenOpen)
		{
			CreateAndShowTweenOverlay();
		}
		if (bMotionTrailsTabOpen)
		{
			TryInvokeToolkitUI(MotionTrailTabName);
		}
		if (bTLLRN_AnimLayerTabOpen)
		{
			TryInvokeToolkitUI(TLLRN_AnimLayerTabName);
		}
		if (bSnapperTabOpen)
		{
			TryInvokeToolkitUI(SnapperTabName);
		}
		if (bPoseTabOpen)
		{
			TryInvokeToolkitUI(PoseTabName);
		}
	}	
}

void FTLLRN_ControlRigEditModeToolkit::UnregisterAndRemoveFloatingTabs()
{
	if (FSlateApplication::IsInitialized())
	{
		if (TweenWidgetParent)
		{
			bTweenOpen = true;
		}
		else
		{
			bTweenOpen = false;
		}
		RemoveAndDestroyTweenOverlay();
		if (ModeUILayer.IsValid())
		{
			TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();

			TSharedPtr<SDockTab> MotionTrailTab = ModeUILayerPtr->GetTabManager()->FindExistingLiveTab(FTabId(MotionTrailTabName));
			if (MotionTrailTab)
			{
				bMotionTrailsTabOpen = true;
				MotionTrailTab->RequestCloseTab();
			}
			else
			{
				bMotionTrailsTabOpen = false;
			}
			ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(MotionTrailTabName);

			TSharedPtr<SDockTab> TLLRN_AnimLayerTab = ModeUILayerPtr->GetTabManager()->FindExistingLiveTab(FTabId(TLLRN_AnimLayerTabName));
			if (TLLRN_AnimLayerTab)
			{
				bTLLRN_AnimLayerTabOpen = true;
				TLLRN_AnimLayerTab->RequestCloseTab();
			}
			else
			{
				bTLLRN_AnimLayerTabOpen = false;
			}
			ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(TLLRN_AnimLayerTabName);

			TSharedPtr<SDockTab> SnapperTab = ModeUILayerPtr->GetTabManager()->FindExistingLiveTab(FTabId(SnapperTabName));
			if (SnapperTab)
			{
				bSnapperTabOpen = true;
				SnapperTab->RequestCloseTab();
			}
			else
			{
				bSnapperTabOpen = false;
			}
			ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(SnapperTabName);
		
			TSharedPtr<SDockTab> PoseTab = ModeUILayerPtr->GetTabManager()->FindExistingLiveTab(FTabId(PoseTabName));
			if (PoseTab)
			{
				bPoseTabOpen = true;
				PoseTab->RequestCloseTab();
			}
			else
			{
				bPoseTabOpen = false;
			}
			ModeUILayerPtr->GetTabManager()->UnregisterTabSpawner(PoseTabName);
		}
	}
}

#undef LOCTEXT_NAMESPACE
