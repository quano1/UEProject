// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_UVEditorModeToolkit.h"

#include "Styling/AppStyle.h" //FAppStyle
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "SPrimaryButton.h"
#include "STransformGizmoNumericalUIOverlay.h"
#include "Tools/UEdMode.h"
#include "TLLRN_UVEditorBackgroundPreview.h"
#include "TLLRN_UVEditorCommands.h"
#include "TLLRN_UVEditorMode.h"
#include "TLLRN_UVEditorUXSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "EdModeInteractiveToolsContext.h"
#include "EditorModeManager.h"
#include "TLLRN_UVEditorModeUILayer.h"
#include "TLLRN_UVEditorStyle.h"
#include "AssetEditorModeManager.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FTLLRN_UVEditorModeToolkit"

namespace TLLRN_UVEditorModeToolkitLocals
{
	TAutoConsoleVariable<bool> CVarAddDebugTools(
		TEXT("modeling.TLLRN_UVEditor.AddDebugTools"),
		false,
		TEXT("Enable UV tools that can be useful for debugging (currently just the \"Unset UVs\" tool."));
}

FTLLRN_UVEditorModeToolkit::FTLLRN_UVEditorModeToolkit()
{
	// Construct the panel that we will give in GetInlineContent().
	// This could probably be done in Init() instead, but constructor
	// makes it easy to guarantee that GetInlineContent() will always
	// be ready to work.

	SAssignNew(ToolkitWidget, SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		SAssignNew(ToolWarningArea, STextBlock)
		.AutoWrapText(true)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.15f, 0.15f))) //TODO: This probably needs to not be hardcoded
		.Text(FText::GetEmpty())
		.Visibility(EVisibility::Collapsed)
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		SAssignNew(ModeWarningArea, STextBlock)
		.AutoWrapText(true)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.15f, 0.15f))) //TODO: This probably needs to not be hardcoded
		.Text(FText::GetEmpty())
		.Visibility(EVisibility::Collapsed)
	]
	+ SVerticalBox::Slot()
	[
		SAssignNew(ToolDetailsContainer, SBorder)
		.BorderImage(FAppStyle::GetBrush("NoBorder"))
	];
}

FTLLRN_UVEditorModeToolkit::~FTLLRN_UVEditorModeToolkit()
{
	UEdMode* Mode = GetScriptableEditorMode().Get();
	if (ensure(Mode))
	{
		UEditorInteractiveToolsContext* Context = Mode->GetInteractiveToolsContext();
		if (ensure(Context))
		{
			Context->OnToolNotificationMessage.RemoveAll(this);
			Context->OnToolWarningMessage.RemoveAll(this);
		}
	}
}

void FTLLRN_UVEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);	

	UTLLRN_UVEditorMode* TLLRN_UVEditorMode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
	check(TLLRN_UVEditorMode);
	UEditorInteractiveToolsContext* InteractiveToolsContext = TLLRN_UVEditorMode->GetInteractiveToolsContext();
	check(InteractiveToolsContext);

	// Currently, there's no EToolChangeTrackingMode that reverts back to a default tool on undo (if we add that
	// support, the tool manager will need to be aware of the default tool). So, we instead opt to do our own management
	// of tool start transactions. See FTLLRN_UVEditorModeToolkit::OnToolStarted for how we issue the transactions.
	InteractiveToolsContext->ToolManager->ConfigureChangeTrackingMode(EToolChangeTrackingMode::NoChangeTracking);

	// Set up tool message areas
	ClearNotification();
	ClearWarning();
	GetScriptableEditorMode()->GetInteractiveToolsContext()->OnToolNotificationMessage.AddSP(this, &FTLLRN_UVEditorModeToolkit::PostNotification);
	GetScriptableEditorMode()->GetInteractiveToolsContext()->OnToolWarningMessage.AddSP(this, &FTLLRN_UVEditorModeToolkit::PostWarning);

	// Hook up the tool detail panel
	ToolDetailsContainer->SetContent(DetailsView.ToSharedRef());
	
	// Set up the overlay. Largely copied from ModelingToolsEditorModeToolkit.
	// TODO: We could put some of the shared code in some common place.
	SAssignNew(ViewportOverlayWidget, SHorizontalBox)

	+SHorizontalBox::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(0.0f, 0.0f, 0.f, 15.f))
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("EditorViewport.OverlayBrush"))
		.Padding(8.f)
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(SImage)
				.Image_Lambda([this] () { return ActiveToolIcon; })
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(STextBlock)
				.Text(this, &FTLLRN_UVEditorModeToolkit::GetActiveToolDisplayName)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0.0, 0.f, 2.f, 0.f))
			[
				SNew(SPrimaryButton)
				.Text(LOCTEXT("OverlayAccept", "Accept"))
				.ToolTipText(LOCTEXT("OverlayAcceptTooltip", "Accept/Commit the results of the active Tool [Enter]"))
				.OnClicked_Lambda([this]() { 
					GetScriptableEditorMode()->GetInteractiveToolsContext()->EndTool(EToolShutdownType::Accept);
					Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode())->ActivateDefaultTool();
					return FReply::Handled(); 
					})
				.IsEnabled_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext()->CanAcceptActiveTool(); })
				.Visibility_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext()->ActiveToolHasAccept() ? EVisibility::Visible : EVisibility::Collapsed; })
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(2.0, 0.f, 0.f, 0.f))
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "Button")
				.TextStyle( FAppStyle::Get(), "DialogButtonText" )
				.Text(LOCTEXT("OverlayCancel", "Cancel"))
				.ToolTipText(LOCTEXT("OverlayCancelTooltip", "Cancel the active Tool [Esc]"))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([this]() { 
					GetScriptableEditorMode()->GetInteractiveToolsContext()->EndTool(EToolShutdownType::Cancel); 
					Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode())->ActivateDefaultTool();
					return FReply::Handled(); 
					})
				.IsEnabled_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext()->CanCancelActiveTool(); })
				.Visibility_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext()->ActiveToolHasAccept() ? EVisibility::Visible : EVisibility::Collapsed; })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(2.0, 0.f, 0.f, 0.f))
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
				.TextStyle(FAppStyle::Get(), "DialogButtonText")
				.Text(LOCTEXT("OverlayComplete", "Complete"))
				.ToolTipText(LOCTEXT("OverlayCompleteTooltip", "Exit the active Tool [Enter]"))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([this]() { 
					GetScriptableEditorMode()->GetInteractiveToolsContext()->EndTool(EToolShutdownType::Completed);
					Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode())->ActivateDefaultTool();
					return FReply::Handled(); 
					})
				.IsEnabled_Lambda([this]() {
					UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
					return GetScriptableEditorMode()->GetInteractiveToolsContext()->CanCompleteActiveTool();
				})
				.Visibility_Lambda([this]() { return GetScriptableEditorMode()->GetInteractiveToolsContext()->CanCompleteActiveTool() ? EVisibility::Visible : EVisibility::Collapsed; })
			]
		]	
	];

	// Set up the gizmo numerical UI
	GizmoNumericalUIOverlayWidget = SNew(STransformGizmoNumericalUIOverlay)
		.bPositionRelativeToBottom(true)
		.DefaultLeftPadding(15.0f)
		// Unlike the level editor, we don't have an axis indicator that we have to be above, so this value is different
		.DefaultVerticalPadding(15.0f)
		// Enable non-delta mode by giving a default reference transform
		.DefaultLocalReferenceTransform(FTransform())
		// Make our displays show 0-1 units instead of world units.
		.InternalToDisplayFunction([](const FVector3d& WorldPosition)
		{
			FVector2f UV = FTLLRN_UVEditorUXSettings::UnwrapWorldPositionToExternalUV(WorldPosition);
			return FVector3d(UV.X, UV.Y, 0);
		})
		.DisplayToInternalFunction([](const FVector3d& UVPosition)
		{
			return FTLLRN_UVEditorUXSettings::ExternalUVToUnwrapWorldPosition(FVector2f(UVPosition.X, UVPosition.Y));
		})
		.TranslationScrubSensitivity(1 / FTLLRN_UVEditorUXSettings::UVMeshScalingFactor)
		;

	GetToolkitHost()->AddViewportOverlayWidget(GizmoNumericalUIOverlayWidget.ToSharedRef());
}

void FTLLRN_UVEditorModeToolkit::InitializeAfterModeSetup()
{
	if (bFirstInitializeAfterModeSetup)
	{
		if (ensure(GizmoNumericalUIOverlayWidget.IsValid()))
		{
			// This has to happen after the gizmo context object is registered
			GizmoNumericalUIOverlayWidget->BindToGizmoContextObject(GetScriptableEditorMode()->GetInteractiveToolsContext());
		}
		bFirstInitializeAfterModeSetup = false;
	}
	
}

FName FTLLRN_UVEditorModeToolkit::GetToolkitFName() const
{
	return FName("TLLRN_UVEditorMode");
}

FText FTLLRN_UVEditorModeToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("TLLRN_UVEditorModeToolkit", "DisplayName", "TLLRN_UVEditorMode");
}

TSharedRef<SWidget> FTLLRN_UVEditorModeToolkit::CreateChannelMenu()
{
	UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr);

	// For each asset, create a submenu labeled with its name
	const TArray<FString>& AssetNames = Mode->GetAssetNames();
	for (int32 AssetID = 0; AssetID < AssetNames.Num(); ++AssetID)
	{
		MenuBuilder.AddSubMenu(
			FText::AsCultureInvariant(AssetNames[AssetID]), // label
			FText(), // tooltip
			FNewMenuDelegate::CreateWeakLambda(Mode, [this, Mode, AssetID](FMenuBuilder& SubMenuBuilder)
		{

			// Inside each submenu, create a button for each channel
			int32 NumChannels = Mode->GetNumUVChannels(AssetID);
			for (int32 Channel = 0; Channel < NumChannels; ++Channel)
			{
				SubMenuBuilder.AddMenuEntry(
					FText::Format(LOCTEXT("ChannelLabel", "UV Channel {0}"), Channel), // label
					FText(), // tooltip
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateWeakLambda(Mode, [Mode, AssetID, Channel]()
						{
							Mode->RequestUVChannelChange(AssetID, Channel);

							// A bit of a hack to force the menu to close if the checkbox is clicked (which usually doesn't
							// close the menu)
							FSlateApplication::Get().DismissAllMenus();
						}), 
						FCanExecuteAction::CreateWeakLambda(Mode, []() { return true; }),
						FIsActionChecked::CreateWeakLambda(Mode, [Mode, AssetID, Channel]()
						{
							return Mode->GetDisplayedChannel(AssetID) == Channel;
						})),
					NAME_None,
					EUserInterfaceActionType::RadioButton);
			}
		}));
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FTLLRN_UVEditorModeToolkit::CreateBackgroundSettingsWidget()
{
	UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
	return CreateDisplaySettingsWidget(Mode->GetBackgroundSettingsObject());
}

TSharedRef<SWidget> FTLLRN_UVEditorModeToolkit::CreateDistortionVisualsSettingsWidget()
{
	UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
	return CreateDisplaySettingsWidget(Mode->GetDistortionVisualsSettingsObject());
}

TSharedRef<SWidget> FTLLRN_UVEditorModeToolkit::CreateGridSettingsWidget()
{
	UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
	return CreateDisplaySettingsWidget(Mode->GetGridSettingsObject());
}

TSharedRef<SWidget> FTLLRN_UVEditorModeToolkit::CreateUnwrappedUXSettingsWidget()
{
	UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
	return CreateDisplaySettingsWidget(Mode->GetUnwrappedUXSettingsObject());
}

TSharedRef<SWidget> FTLLRN_UVEditorModeToolkit::CreateLivePreviewUXSettingsWidget()
{
	UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
	return CreateDisplaySettingsWidget(Mode->GetLivePreviewUXSettingsObject());
}

TSharedRef<SWidget> FTLLRN_UVEditorModeToolkit::CreateUDIMSettingsWidget()
{
	UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
	return CreateDisplaySettingsWidget(Mode->GetUDIMSettingsObject());
}

TSharedRef<SWidget> FTLLRN_UVEditorModeToolkit::GetToolDisplaySettingsWidget()
{
	UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
	UObject* SettingsObject = Mode->GetToolDisplaySettingsObject();
	if (SettingsObject)
	{
		return CreateDisplaySettingsWidget(SettingsObject);
	}
	else
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("NoBorder"))
			.Padding(0);
	}
}

void FTLLRN_UVEditorModeToolkit::MakeGizmoNumericalUISubMenu(FMenuBuilder& MenuBuilder)
{
	if (GizmoNumericalUIOverlayWidget.IsValid())
	{
		GizmoNumericalUIOverlayWidget->MakeNumericalUISubMenu(MenuBuilder);
	}
}

TSharedRef<SWidget> FTLLRN_UVEditorModeToolkit::CreateDisplaySettingsWidget(UObject* SettingsObject) const
{
	TSharedRef<SBorder> GridDetailsContainer =
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("NoBorder"));

	TSharedRef<SWidget> Widget = SNew(SBorder)
		.HAlign(HAlign_Fill)
		.Padding(4)
		[
			SNew(SBox)
			.MinDesiredWidth(500)
		[
			GridDetailsContainer
		]
		];

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs GridDetailsViewArgs;
	GridDetailsViewArgs.bAllowSearch = false;
	GridDetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	GridDetailsViewArgs.bHideSelectionTip = true;
	GridDetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
	GridDetailsViewArgs.bShowOptions = false;
	GridDetailsViewArgs.bAllowMultipleTopLevelObjects = false;

	TSharedRef<IDetailsView> GridDetailsView = PropertyEditorModule.CreateDetailView(GridDetailsViewArgs);
	GridDetailsView->SetObject(SettingsObject);
	GridDetailsContainer->SetContent(GridDetailsView);

	return Widget;

}


void FTLLRN_UVEditorModeToolkit::UpdateActiveToolProperties()
{
	UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left);
	if (CurTool != nullptr)
	{
		DetailsView->SetObjects(CurTool->GetToolProperties(true));
	}
}

void FTLLRN_UVEditorModeToolkit::InvalidateCachedDetailPanelState(UObject* ChangedObject)
{
	DetailsView->InvalidateCachedState();
}

void FTLLRN_UVEditorModeToolkit::OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	FModeToolkit::OnToolStarted(Manager, Tool);

	UTLLRN_UVEditorMode* Mode = Cast<UTLLRN_UVEditorMode>(GetScriptableEditorMode());
	UInteractiveTool* CurTool = Mode->GetToolManager()->GetActiveTool(EToolSide::Left);
	CurTool->OnPropertySetsModified.AddSP(this, &FTLLRN_UVEditorModeToolkit::UpdateActiveToolProperties);
	CurTool->OnPropertyModifiedDirectlyByTool.AddSP(this, &FTLLRN_UVEditorModeToolkit::InvalidateCachedDetailPanelState);

	ActiveToolName = Tool->GetToolInfo().ToolDisplayName;

	FString ActiveToolIdentifier = GetScriptableEditorMode()->GetToolManager()->GetActiveToolName(EToolSide::Mouse);
	ActiveToolIdentifier.InsertAt(0, ".");
	FName ActiveToolIconName = ISlateStyle::Join(FTLLRN_UVEditorCommands::Get().GetContextName(), TCHAR_TO_ANSI(*ActiveToolIdentifier));
	ActiveToolIcon = FTLLRN_UVEditorStyle::Get().GetOptionalBrush(ActiveToolIconName);

	if (!Mode->IsDefaultToolActive())
	{
		// Add the accept/cancel overlay only if the tool is not the default tool.
		GetToolkitHost()->AddViewportOverlayWidget(ViewportOverlayWidget.ToSharedRef());
	}
}

void FTLLRN_UVEditorModeToolkit::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	FModeToolkit::OnToolEnded(Manager, Tool);

	ActiveToolName = FText::GetEmpty();
	ClearNotification();
	ClearWarning();

	if (IsHosted())
	{
		GetToolkitHost()->RemoveViewportOverlayWidget(ViewportOverlayWidget.ToSharedRef());
	}

	UInteractiveTool* CurTool = GetScriptableEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left);
	if (CurTool)
	{
		CurTool->OnPropertySetsModified.RemoveAll(this);
		CurTool->OnPropertyModifiedDirectlyByTool.RemoveAll(this);
	}
}


// Place tool category names here, for creating the tool palette below
static const FName ToolsTabName(TEXT("Tools"));

const TArray<FName> FTLLRN_UVEditorModeToolkit::PaletteNames_Standard = { ToolsTabName };

void FTLLRN_UVEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames = PaletteNames_Standard;
}

FText FTLLRN_UVEditorModeToolkit::GetToolPaletteDisplayName(FName Palette) const
{
	return FText::FromName(Palette);
}

void FTLLRN_UVEditorModeToolkit::BuildToolPalette(FName PaletteIndex, class FToolBarBuilder& ToolbarBuilder)
{
	using namespace TLLRN_UVEditorModeToolkitLocals;

	const FTLLRN_UVEditorCommands& Commands = FTLLRN_UVEditorCommands::Get();

	if (PaletteIndex == ToolsTabName)
	{
		ToolbarBuilder.AddToolBarButton(Commands.SewAction);
		ToolbarBuilder.AddToolBarButton(Commands.SplitAction);
		ToolbarBuilder.AddToolBarButton(Commands.MakeIslandAction);
		if (CVarAddDebugTools.GetValueOnGameThread())
		{
			ToolbarBuilder.AddToolBarButton(Commands.TLLRN_UnsetUVsAction);
		}

		ToolbarBuilder.AddToolBarButton(Commands.BeginLayoutTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginTransformTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginAlignTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginDistributeTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginTexelDensityTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginChannelEditTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginSeamTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginParameterizeMeshTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginRecomputeUVsTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginBrushSelectTool);
		ToolbarBuilder.AddToolBarButton(Commands.BeginUVSnapshotTool);
	}
}

void FTLLRN_UVEditorModeToolkit::PostNotification(const FText& Message)
{
	ClearNotification();

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		ActiveToolMessageHandle = GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PushStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), Message);
	}
}

void FTLLRN_UVEditorModeToolkit::ClearNotification()
{
	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PopStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), ActiveToolMessageHandle);
	}
	ActiveToolMessageHandle.Reset();
}

void FTLLRN_UVEditorModeToolkit::PostWarning(const FText& Message)
{
	if (Message.IsEmpty())
	{
		ClearWarning();
	}
	else
	{
		ToolWarningArea->SetText(Message);
		ToolWarningArea->SetVisibility(EVisibility::Visible);
	}
}

void FTLLRN_UVEditorModeToolkit::ClearWarning()
{
	ToolWarningArea->SetText(FText());
	ToolWarningArea->SetVisibility(EVisibility::Collapsed);
}

void FTLLRN_UVEditorModeToolkit::PostModeWarning(const FText& Message)
{
	if (Message.IsEmpty())
	{
		ClearModeWarning();
	}
	else
	{
		ModeWarningArea->SetText(Message);
		ModeWarningArea->SetVisibility(EVisibility::Visible);
	}
}

void FTLLRN_UVEditorModeToolkit::ClearModeWarning()
{
	ModeWarningArea->SetText(FText());
	ModeWarningArea->SetVisibility(EVisibility::Collapsed);
}

void FTLLRN_UVEditorModeToolkit::UpdatePIEWarnings()
{
	if (bShowPIEWarning)
	{
		PostModeWarning(LOCTEXT("ModelingModeToolkitPIEWarning", "UV Editor functionality is limited during Play in Editor sessions. End the current Play in Editor session to continue using the editor."));
	}
	else
	{
		ClearModeWarning();
	}
}

void FTLLRN_UVEditorModeToolkit::EnableShowPIEWarning(bool bEnable)
{
	if (bShowPIEWarning != bEnable)
	{
		bShowPIEWarning = bEnable;
		UpdatePIEWarnings();
	}
}

#undef LOCTEXT_NAMESPACE
