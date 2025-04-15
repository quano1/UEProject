// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModelingToolsEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "TLLRN_ModelingToolsEditorMode.h"
#include "Framework/Commands/UICommandList.h"
#include "TLLRN_ModelingToolsManagerActions.h"
#include "TLLRN_ModelingToolsEditorModeSettings.h"
#include "TLLRN_ModelingToolsEditorModeStyle.h"

#include "TLLRN_ModelingSelectionInteraction.h"
#include "Selection/GeometrySelectionManager.h"

#include "ModelingComponentsSettings.h"
#include "PropertySets/CreateMeshObjectTypeProperties.h"

#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "STransformGizmoNumericalUIOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SComboButton.h"

// for showing toast notifications
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

#include "ToolMenus.h"
// to toggle component instance selection

#define LOCTEXT_NAMESPACE "FTLLRN_ModelingToolsEditorModeToolkit"


namespace UELocal
{

void MakeSubMenu_QuickSettings(FMenuBuilder& MenuBuilder)
{
	const FUIAction OpenTLLRN_ModelingModeProjectSettings(
		FExecuteAction::CreateLambda([]
		{
			if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
			{
				SettingsModule->ShowViewer("Project", "Plugins", "TLLRN_ModelingMode");
			}
		}), FCanExecuteAction(), FIsActionChecked());
	MenuBuilder.AddMenuEntry(LOCTEXT("TLLRN_ModelingModeProjectSettings", "Modeling Mode (Project)"), 
		LOCTEXT("TLLRN_ModelingModeProjectSettings_Tooltip", "Jump to the Project Settings for Modeling Mode. Project Settings are Project-specific."),
		FSlateIcon(), OpenTLLRN_ModelingModeProjectSettings, NAME_None, EUserInterfaceActionType::Button);

	const FUIAction OpenTLLRN_ModelingModeEditorSettings(
		FExecuteAction::CreateLambda([]
		{
			if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
			{
				SettingsModule->ShowViewer("Editor", "Plugins", "TLLRN_ModelingMode");
			}
		}), FCanExecuteAction(), FIsActionChecked());
	MenuBuilder.AddMenuEntry(LOCTEXT("TLLRN_ModelingModeEditorSettings", "Modeling Mode (Editor)"), 
		LOCTEXT("TLLRN_ModelingModeEditorSettings_Tooltip", "Jump to the Editor Settings for Modeling Mode. Editor Settings apply across all Projects."),
		FSlateIcon(), OpenTLLRN_ModelingModeEditorSettings, NAME_None, EUserInterfaceActionType::Button);

	const FUIAction OpenTLLRN_ModelingToolsProjectSettings(
		FExecuteAction::CreateLambda([]
		{
			if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
			{
				SettingsModule->ShowViewer("Project", "Plugins", "Modeling Mode Tools");
			}
		}), FCanExecuteAction(), FIsActionChecked());
	MenuBuilder.AddMenuEntry(LOCTEXT("TLLRN_ModelingToolsProjectSettings", "Modeling Tools (Project)"), 
		LOCTEXT("TLLRN_ModelingToolsProjectSettings_Tooltip", "Jump to the Project Settings for Modeling Tools. Project Settings are Project-specific."),
		FSlateIcon(), OpenTLLRN_ModelingToolsProjectSettings, NAME_None, EUserInterfaceActionType::Button);

}

void MakeSubMenu_GizmoVisibilityMode(FTLLRN_ModelingToolsEditorModeToolkit* Toolkit, FMenuBuilder& MenuBuilder)
{
	UTLLRN_ModelingToolsEditorModeSettings* Settings = GetMutableDefault<UTLLRN_ModelingToolsEditorModeSettings>();
	UEditorInteractiveToolsContext* Context = Toolkit->GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode);

	// toggle for Combined/Separate Gizmo Mode
	const FUIAction GizmoMode_Combined(
		FExecuteAction::CreateLambda([Settings, Context]
		{
			Settings->bRespectLevelEditorGizmoMode = ! Settings->bRespectLevelEditorGizmoMode;
			Context->SetForceCombinedGizmoMode(Settings->bRespectLevelEditorGizmoMode == false);
			Settings->SaveConfig();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([Settings]()
		{
			return Settings->bRespectLevelEditorGizmoMode == false;
		}));
	MenuBuilder.AddMenuEntry(LOCTEXT("GizmoMode_Combined", "Combined Gizmo"), 
		LOCTEXT("GizmoMode_Combined_Tooltip", "Ignore Level Editor Gizmo Mode and always use a Combined Transform Gizmo in Modeling Tools"),
		FSlateIcon(), GizmoMode_Combined, NAME_None, EUserInterfaceActionType::ToggleButton);

	// toggle for Absolute Grid Snapping mode in World Coordinates
	const FUIAction GizmoMode_AbsoluteWorldSnap(
		FExecuteAction::CreateLambda([Settings, Context]
		{
			Context->SetAbsoluteWorldSnappingEnabled( ! Context->GetAbsoluteWorldSnappingEnabled() );
			Settings->bEnableAbsoluteWorldSnapping = Context->GetAbsoluteWorldSnappingEnabled();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([Context]()
		{
			return Context->GetAbsoluteWorldSnappingEnabled();
		}));
	MenuBuilder.AddMenuEntry(LOCTEXT("GizmoMode_AbsoluteGridSnap", "Absolute Grid Snapping"), 
		LOCTEXT("GizmoMode_AbsoluteGridSnap_Tooltip", "Snap translation/rotation to absolute grid coordinates in the world coordinate system, instead of relative to the initial position"),
		FSlateIcon(), GizmoMode_AbsoluteWorldSnap, NAME_None, EUserInterfaceActionType::ToggleButton);
	

	MenuBuilder.AddSubMenu(
		LOCTEXT("TransformPanelSubMenu", "Transform Panel"), LOCTEXT("TransformPanelSubMenu_ToolTip", "Configure the Gizmo Transform Panel."),
		FNewMenuDelegate::CreateLambda([Toolkit](FMenuBuilder& SubMenuBuilder) {
			TSharedPtr<STransformGizmoNumericalUIOverlay> NumericalUI = Toolkit->GetGizmoNumericalUIOverlayWidget();
			if (ensure(NumericalUI.IsValid()))
			{
				NumericalUI->MakeNumericalUISubMenu(SubMenuBuilder);
			}
		}));
}



void MakeSubMenu_SelectionSupport(FTLLRN_ModelingToolsEditorModeToolkit* Toolkit, FMenuBuilder& MenuBuilder)
{
	UTLLRN_ModelingToolsEditorModeSettings* Settings = GetMutableDefault<UTLLRN_ModelingToolsEditorModeSettings>();

	// toggle for Combined/Separate Gizmo Mode
	const FUIAction EnableSelectionsToggle(
		FExecuteAction::CreateLambda([Toolkit, Settings]
		{
			Settings->bEnablePersistentSelections = ! Settings->bEnablePersistentSelections;
			Settings->SaveConfig();
			Toolkit->NotifySelectionSystemEnabledStateModified();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([Settings]()
		{
			return Settings->bEnablePersistentSelections;
		}));
	MenuBuilder.AddMenuEntry(LOCTEXT("MeshElementSelectionToggle", "Mesh Element Selection"), 
		LOCTEXT("MeshElementSelectionToggle_Tooltip", "Enable support for in-viewport Mesh Element Selection in Modeling Mode"),
		FSlateIcon(), EnableSelectionsToggle, NAME_None, EUserInterfaceActionType::ToggleButton);
}




void MakeSubMenu_ModeToggles(FMenuBuilder& MenuBuilder)
{
	// add toggle for Instance Selection cvar
	const FUIAction ToggleInstancesEverywhereAction(
		FExecuteAction::CreateLambda([]
		{
			IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("TypedElements.EnableViewportSMInstanceSelection")); 
			int32 CurValue = CVar->GetInt();
			CVar->Set(CurValue == 0 ? 1 : 0);
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([]()
		{
			IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("TypedElements.EnableViewportSMInstanceSelection"));
			return (CVar->GetInt() == 1);
		}));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ToggleInstancesSelection", "Instance Selection"), 
		LOCTEXT("ToggleInstancesSelection_Tooltip", "Enable/Disable support for direct selection of InstancedStaticMeshComponent Instances (via TypedElements.EnableViewportInstanceSelection cvar)"),
		FSlateIcon(), ToggleInstancesEverywhereAction, NAME_None, EUserInterfaceActionType::ToggleButton);

	// add toggle for Volume Snapping cvar
	const FUIAction ToggleVolumeSnappingAction(
		FExecuteAction::CreateLambda([]
		{
			IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("modeling.EnableVolumeSnapping")); 
			bool CurValue = CVar->GetBool();
			CVar->Set(CurValue ? false : true);
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([]()
		{
			IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("modeling.EnableVolumeSnapping"));
			return (CVar->GetBool());
		}));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ToggleVolumeSnapping", "Volume Snapping"), 
		LOCTEXT("ToggleVolumeSnapping_Tooltip", "Enable Vertex/Face Snapping and Ray-hits against Volumes in the Level. Note that if your level contains many overlapping large volumes, some Modeling functionality may not work correctly"),
		FSlateIcon(), ToggleVolumeSnappingAction, NAME_None, EUserInterfaceActionType::ToggleButton);


}


void MakeSubMenu_DefaultMeshObjectPhysicsSettings(FMenuBuilder& MenuBuilder)
{
	UModelingComponentsSettings* Settings = GetMutableDefault<UModelingComponentsSettings>();

	const FUIAction ToggleEnableCollisionAction = FUIAction(
		FExecuteAction::CreateLambda([Settings]
		{
			Settings->bEnableCollision = !Settings->bEnableCollision;
			Settings->SaveConfig();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([Settings]()
		{
			return Settings->bEnableCollision;
		}));

	MenuBuilder.AddMenuEntry(LOCTEXT("DefaultMeshObjectEnableCollision", "Enable Collision"), TAttribute<FText>(), FSlateIcon(), ToggleEnableCollisionAction, NAME_None, EUserInterfaceActionType::ToggleButton);

	MenuBuilder.BeginSection("CollisionTypeSection", LOCTEXT("CollisionTypeSection", "Collision Mode"));

	auto MakeCollisionTypeAction = [Settings](ECollisionTraceFlag FlagType)
	{
		return FUIAction(
			FExecuteAction::CreateLambda([Settings, FlagType]
			{
				Settings->CollisionMode = FlagType;
				Settings->SaveConfig();
			}),
			FCanExecuteAction::CreateLambda([Settings]() { return Settings->bEnableCollision; }),
			FIsActionChecked::CreateLambda([Settings, FlagType]()
			{
				return Settings->CollisionMode == FlagType;
			}));
	};
	const FUIAction CollisionMode_Default = MakeCollisionTypeAction(ECollisionTraceFlag::CTF_UseDefault);
	MenuBuilder.AddMenuEntry(LOCTEXT("DefaultMeshObject_CollisionDefault", "Default"), LOCTEXT("DefaultMeshObject_CollisionDefault_Tooltip", "Configure the new Mesh Object to have the Project Default collision settings (CTF_UseDefault)"),
		FSlateIcon(), CollisionMode_Default, NAME_None, EUserInterfaceActionType::ToggleButton);
	const FUIAction CollisionMode_Both = MakeCollisionTypeAction(ECollisionTraceFlag::CTF_UseSimpleAndComplex);
	MenuBuilder.AddMenuEntry(LOCTEXT("DefaultMeshObject_CollisionBoth", "Simple And Complex"), LOCTEXT("DefaultMeshObject_CollisionBoth_Tooltip", "Configure the new Mesh Object to have both Simple and Complex collision (CTF_UseSimpleAndComplex)"),
		FSlateIcon(), CollisionMode_Both, NAME_None, EUserInterfaceActionType::ToggleButton);
	const FUIAction CollisionMode_NoComplex = MakeCollisionTypeAction(ECollisionTraceFlag::CTF_UseSimpleAsComplex);
	MenuBuilder.AddMenuEntry(LOCTEXT("DefaultMeshObject_CollisionNoComplex", "Simple Only"), LOCTEXT("DefaultMeshObject_CollisionNoComplex_Tooltip", "Configure the new Mesh Object to have only Simple collision (CTF_UseSimpleAsComplex)"),
		FSlateIcon(), CollisionMode_NoComplex, NAME_None, EUserInterfaceActionType::ToggleButton);
	const FUIAction CollisionMode_ComplexOnly = MakeCollisionTypeAction(ECollisionTraceFlag::CTF_UseComplexAsSimple);
	MenuBuilder.AddMenuEntry(LOCTEXT("DefaultMeshObject_CollisionComplexOnly", "Complex Only"), LOCTEXT("DefaultMeshObject_CollisionComplexOnly_Tooltip", "Configure the new Mesh Object to have only Complex collision (CTF_UseComplexAsSimple)"),
		FSlateIcon(), CollisionMode_ComplexOnly, NAME_None, EUserInterfaceActionType::ToggleButton);

	MenuBuilder.EndSection();
}


void MakeSubMenu_DefaultMeshObjectType(FMenuBuilder& MenuBuilder)
{
	UTLLRN_ModelingToolsEditorModeSettings* Settings = GetMutableDefault<UTLLRN_ModelingToolsEditorModeSettings>();

	MenuBuilder.BeginSection("DefaultObjectTypeSection", LOCTEXT("DefaultObjectTypeSection", "Default Object Type"));

	auto ShowMeshObjectDefaultChangeToast = []()
	{
		FNotificationInfo Info(LOCTEXT("ChangeDefaultMeshObjectTypeToast", 
			"Changing the Default Mesh Object Type will take effect the next time the Editor is started"));
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	};


	MenuBuilder.AddMenuEntry(
		LOCTEXT("DefaultMeshObjectType_StaticMesh", "Static Mesh"), 
		TAttribute<FText>(), 
		FSlateIcon(), 
		FUIAction(
			FExecuteAction::CreateLambda([Settings, ShowMeshObjectDefaultChangeToast]
			{
				Settings->DefaultMeshObjectType = ETLLRN_ModelingModeDefaultMeshObjectType::StaticMeshAsset;
				UCreateMeshObjectTypeProperties::DefaultObjectTypeIdentifier = UCreateMeshObjectTypeProperties::StaticMeshIdentifier;
				Settings->SaveConfig();
				ShowMeshObjectDefaultChangeToast();
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([Settings]()
			{
				return Settings->DefaultMeshObjectType == ETLLRN_ModelingModeDefaultMeshObjectType::StaticMeshAsset;
			})),
		NAME_None, EUserInterfaceActionType::ToggleButton);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DefaultMeshObjectType_DynamicMesh", "Dynamic Mesh Actor"),
		TAttribute<FText>(), 
		FSlateIcon(), 
		FUIAction(
			FExecuteAction::CreateLambda([Settings, ShowMeshObjectDefaultChangeToast]
			{
				Settings->DefaultMeshObjectType = ETLLRN_ModelingModeDefaultMeshObjectType::DynamicMeshActor;
				UCreateMeshObjectTypeProperties::DefaultObjectTypeIdentifier = UCreateMeshObjectTypeProperties::DynamicMeshActorIdentifier;
				Settings->SaveConfig();
				ShowMeshObjectDefaultChangeToast();
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([Settings]()
			{
				return Settings->DefaultMeshObjectType == ETLLRN_ModelingModeDefaultMeshObjectType::DynamicMeshActor;
			})),
		NAME_None, EUserInterfaceActionType::ToggleButton );

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DefaultMeshObjectType_Volume", "Gameplay Volume"),
		TAttribute<FText>(), 
		FSlateIcon(), 
		FUIAction(
			FExecuteAction::CreateLambda([Settings, ShowMeshObjectDefaultChangeToast]
			{
				Settings->DefaultMeshObjectType = ETLLRN_ModelingModeDefaultMeshObjectType::VolumeActor;
				UCreateMeshObjectTypeProperties::DefaultObjectTypeIdentifier = UCreateMeshObjectTypeProperties::VolumeIdentifier;
				Settings->SaveConfig();
				ShowMeshObjectDefaultChangeToast();
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([Settings]()
			{
				return Settings->DefaultMeshObjectType == ETLLRN_ModelingModeDefaultMeshObjectType::VolumeActor;
			})),
		NAME_None, EUserInterfaceActionType::ToggleButton);


	MenuBuilder.EndSection();

	MenuBuilder.AddMenuSeparator();

	MakeSubMenu_DefaultMeshObjectPhysicsSettings(MenuBuilder);
}

void MakeSubMenu_Selection_DragMode(FTLLRN_ModelingToolsEditorModeToolkit* Toolkit, FMenuBuilder& MenuBuilder)
{
	UTLLRN_ModelingSelectionInteraction* SelectionInteraction = Cast<UTLLRN_ModelingToolsEditorMode>(Toolkit->GetScriptableEditorMode())->GetSelectionInteraction();

	auto MakeDragModeOptionAction = [SelectionInteraction](ETLLRN_ModelingSelectionInteraction_DragMode DragMode)
	{
		return FUIAction(
			FExecuteAction::CreateLambda([SelectionInteraction, DragMode]
			{
				SelectionInteraction->SetActiveDragMode(DragMode);
				UTLLRN_ModelingToolsModeCustomizationSettings* ModelingEditorSettings = GetMutableDefault<UTLLRN_ModelingToolsModeCustomizationSettings>();
				ModelingEditorSettings->LastMeshSelectionDragMode = static_cast<int>(DragMode);
				ModelingEditorSettings->SaveConfig();
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([SelectionInteraction, DragMode]()
			{
				return SelectionInteraction->GetActiveDragMode() == DragMode;
			}));
	};

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Selection_DragInput_None", "None"), 
		LOCTEXT("Selection_DragInput_None_Tooltip", "No drag input"),
		FSlateIcon(), MakeDragModeOptionAction(ETLLRN_ModelingSelectionInteraction_DragMode::NoDragInteraction), NAME_None, EUserInterfaceActionType::ToggleButton);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Selection_DragInput_Path", "Path"), 
		LOCTEXT("Selection_DragInput_Path_Tooltip", "Path Drag Input"),
		FSlateIcon(), MakeDragModeOptionAction(ETLLRN_ModelingSelectionInteraction_DragMode::PathInteraction), NAME_None, EUserInterfaceActionType::ToggleButton);

	// marquee mode is not functional yet, so this is disabled
	//MenuBuilder.AddMenuEntry(
	//	LOCTEXT("Selection_DragInput_Marquee", "Rectangle"), 
	//	LOCTEXT("Selection_DragInput_Marquee_Tooltip", "Rectangle Marquee"),
	//	FSlateIcon(), MakeDragModeOptionAction(ETLLRN_ModelingSelectionInteraction_DragMode::RectangleMarqueeInteraction), NAME_None, EUserInterfaceActionType::ToggleButton);

}


void MakeSubMenu_Selection_MeshType(FTLLRN_ModelingToolsEditorModeToolkit* Toolkit, FMenuBuilder& MenuBuilder)
{
	UTLLRN_ModelingToolsEditorMode* TLLRN_ModelingMode = Cast<UTLLRN_ModelingToolsEditorMode>(Toolkit->GetScriptableEditorMode());
	FUIAction ToggleVolumesAction(
		FExecuteAction::CreateLambda([TLLRN_ModelingMode]
		{
			TLLRN_ModelingMode->bEnableVolumeElementSelection = ! TLLRN_ModelingMode->bEnableVolumeElementSelection;

			UTLLRN_ModelingToolsModeCustomizationSettings* ModelingEditorSettings = GetMutableDefault<UTLLRN_ModelingToolsModeCustomizationSettings>();
			ModelingEditorSettings->bLastMeshSelectionVolumeToggle = TLLRN_ModelingMode->bEnableVolumeElementSelection;
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([TLLRN_ModelingMode]()
		{
			return TLLRN_ModelingMode->bEnableVolumeElementSelection;
		}));
	FUIAction ToggleStaticMeshesAction(
		FExecuteAction::CreateLambda([TLLRN_ModelingMode]
		{
			TLLRN_ModelingMode->bEnableStaticMeshElementSelection = ! TLLRN_ModelingMode->bEnableStaticMeshElementSelection;

			UTLLRN_ModelingToolsModeCustomizationSettings* ModelingEditorSettings = GetMutableDefault<UTLLRN_ModelingToolsModeCustomizationSettings>();
			ModelingEditorSettings->bLastMeshSelectionStaticMeshToggle = TLLRN_ModelingMode->bEnableStaticMeshElementSelection;
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([TLLRN_ModelingMode]()
		{
			return TLLRN_ModelingMode->bEnableStaticMeshElementSelection;
		}));


	MenuBuilder.AddMenuEntry(
		LOCTEXT("Selection_MeshTypes_Volumes", "Volumes"), 
		LOCTEXT("Selection_MeshTypes_Volumes_Tooltip", "Toggle whether Volume mesh elements can be selected in the Viewport"),
		FSlateIcon(), ToggleVolumesAction, NAME_None, EUserInterfaceActionType::ToggleButton);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Selection_MeshTypes_StaticMesh", "Static Meshes"), 
		LOCTEXT("Selection_MeshTypes_StaticMesh_Tooltip", "Toggle whether Static Mesh mesh elements can be selected in the Viewport"),
		FSlateIcon(), ToggleStaticMeshesAction, NAME_None, EUserInterfaceActionType::ToggleButton);

}
	
void MakeSubMenu_Selection_LocalFrameMode(FTLLRN_ModelingToolsEditorModeToolkit* Toolkit, FMenuBuilder& MenuBuilder)
{
		
	UTLLRN_ModelingSelectionInteraction* SelectionInteraction = Cast<UTLLRN_ModelingToolsEditorMode>(Toolkit->GetScriptableEditorMode())->GetSelectionInteraction();
	auto ToggleFromGeometryAction = [SelectionInteraction](ETLLRN_ModelingSelectionInteraction_LocalFrameMode LocalFrameMode)
	{
		return FUIAction(
			FExecuteAction::CreateLambda([SelectionInteraction, LocalFrameMode]
			{
				SelectionInteraction->SetLocalFrameMode(LocalFrameMode);
				UTLLRN_ModelingToolsModeCustomizationSettings* ModelingEditorSettings = GetMutableDefault<UTLLRN_ModelingToolsModeCustomizationSettings>();
				ModelingEditorSettings->LastMeshSelectionLocalFrameMode = static_cast<int>(LocalFrameMode);
				ModelingEditorSettings->SaveConfig();
			}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([SelectionInteraction, LocalFrameMode]()
			{
				return (SelectionInteraction->GetLocalFrameMode() == LocalFrameMode);
			}));
	};
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Selection_LocalFrameMode_FromGeometry", "From Geometry"), 
		LOCTEXT("Selection_LocalFrameMode_FromGeometry_Tooltip", "Gizmo Orientation Based on Selected Geometry"),
		FSlateIcon(), ToggleFromGeometryAction(ETLLRN_ModelingSelectionInteraction_LocalFrameMode::FromGeometry), NAME_None, EUserInterfaceActionType::ToggleButton);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Selection_LocalFrameMode_FromObject", "From Object"), 
		LOCTEXT("Selection_LocalFrameMode_FromObject_Tooltip", "Gizmo Orientation Based on Object"),
		FSlateIcon(), ToggleFromGeometryAction(ETLLRN_ModelingSelectionInteraction_LocalFrameMode::FromObject), NAME_None, EUserInterfaceActionType::ToggleButton);
}

TSharedRef<SWidget> MakeMenu_SelectionConfigSettings(FTLLRN_ModelingToolsEditorModeToolkit* Toolkit)
{
	FMenuBuilder MenuBuilder(true, TSharedPtr<FUICommandList>());

	MenuBuilder.BeginSection("Section_DragMode", LOCTEXT("Section_DragMode", "Drag Mode"));
	MakeSubMenu_Selection_DragMode(Toolkit, MenuBuilder);
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Section_MeshTypes", LOCTEXT("Section_MeshTypes", "Selectable Mesh Types"));
	MakeSubMenu_Selection_MeshType(Toolkit, MenuBuilder);
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Section_LocalFrameMode", LOCTEXT("Section_LocalFrameMode", "Local Frame Mode"));
	MakeSubMenu_Selection_LocalFrameMode(Toolkit, MenuBuilder);
	MenuBuilder.EndSection();

	TSharedRef<SWidget> MenuWidget = MenuBuilder.MakeWidget();
	return MenuWidget;
}


TSharedRef<SWidget> MakeMenu_SelectionEdits(FTLLRN_ModelingToolsEditorModeToolkit* Toolkit)
{
	FMenuBuilder MenuBuilder(true, Toolkit->GetToolkitCommands());

	MenuBuilder.BeginSection("Section_SelectionEdits", LOCTEXT("Section_SelectionEdits", "Selection Edits"));

	const FTLLRN_ModelingToolsManagerCommands& Commands = FTLLRN_ModelingToolsManagerCommands::Get();

	MenuBuilder.AddMenuEntry(Commands.BeginSelectionAction_SelectAll);
	MenuBuilder.AddMenuEntry(Commands.BeginSelectionAction_ExpandToConnected);
	MenuBuilder.AddMenuEntry(Commands.BeginSelectionAction_Invert);
	MenuBuilder.AddMenuEntry(Commands.BeginSelectionAction_InvertConnected);
	MenuBuilder.AddMenuEntry(Commands.BeginSelectionAction_Expand);
	MenuBuilder.AddMenuEntry(Commands.BeginSelectionAction_Contract);

	TSharedRef<SWidget> MenuWidget = MenuBuilder.MakeWidget();
	return MenuWidget;
}


} // end namespace UELocal


TSharedRef<SWidget> FTLLRN_ModelingToolsEditorModeToolkit::MakeMenu_TLLRN_ModelingModeConfigSettings()
{
	using namespace UELocal;

	FMenuBuilder MenuBuilder(true, TSharedPtr<FUICommandList>());

	MenuBuilder.BeginSection("Section_Gizmo", LOCTEXT("Section_Gizmo", "Gizmo"));
	MakeSubMenu_GizmoVisibilityMode(this, MenuBuilder);
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Section_SelectionSupport", LOCTEXT("Section_SelectionSupport", "Mesh Element Selection"));
	MakeSubMenu_SelectionSupport(this, MenuBuilder);
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Section_MeshObjects", LOCTEXT("Section_MeshObjects", "New Mesh Objects"));
	MenuBuilder.AddSubMenu(
		LOCTEXT("MeshObjectTypeSubMenu", "New Mesh Settings"), LOCTEXT("MeshObjectTypeSubMenu_ToolTip", "Configure default settings for new Mesh Object Types"),
		FNewMenuDelegate::CreateLambda([=](FMenuBuilder& SubMenuBuilder) {
			MakeSubMenu_DefaultMeshObjectType(SubMenuBuilder);
		}));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Section_ModeToggles", LOCTEXT("Section_ModeToggles", "Mode Settings"));
	MakeSubMenu_ModeToggles(MenuBuilder);
	MenuBuilder.EndSection();

	// only show settings UI quick access in non-Restrictive mode
	const UTLLRN_ModelingToolsEditorModeSettings* Settings = GetDefault<UTLLRN_ModelingToolsEditorModeSettings>();
	if (!Settings->InRestrictiveMode())
	{
		MenuBuilder.BeginSection("Section_Settings", LOCTEXT("Section_Settings", "Quick Settings"));
		MenuBuilder.AddSubMenu(
			LOCTEXT("QuickSettingsSubMenu", "Jump To Settings"), LOCTEXT("QuickSettingsSubMenu_ToolTip", "Jump to sections of the Settings dialogs relevant to Modeling Mode"),
			FNewMenuDelegate::CreateLambda([=](FMenuBuilder& SubMenuBuilder) {
				MakeSubMenu_QuickSettings(SubMenuBuilder);
				}));
		MenuBuilder.EndSection();
	}

	TSharedRef<SWidget> MenuWidget = MenuBuilder.MakeWidget();
	return MenuWidget;
}

void FTLLRN_ModelingToolsEditorModeToolkit::ExtendSecondaryModeToolbar(UToolMenu *InModeToolbarMenu)
{
	return;		// disable for now

	/*
	 * Sanity check because the toolbar extension should happen after the ModeUILayer setup which is currently manual
	 * for standalone asset editors
	 */
	if( !ModeUILayer.IsValid() )
	{
		return;
	}

	UTLLRN_ModelingToolsEditorModeSettings* TLLRN_ModelingModeSettings = GetMutableDefault<UTLLRN_ModelingToolsEditorModeSettings>();
	const bool bEnableSelectionUI = TLLRN_ModelingModeSettings && TLLRN_ModelingModeSettings->GetMeshSelectionsEnabled();
	if ( !bEnableSelectionUI )
	{
		return;
	}
	
	FToolMenuSection& Section = InModeToolbarMenu->FindOrAddSection("SelectionPalette");

	/*
	 * NOTE: Any commands used in the SecondaryModeToolbar need to be present in the command list that lives in the
	 * ModeUILayer. Currently this is done automatically by parenting said command list to the toolkit's command list
	 */
	const FTLLRN_ModelingToolsManagerCommands& Commands = FTLLRN_ModelingToolsManagerCommands::Get();

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.MeshSelectionModeAction_NoSelection));

	Section.AddSeparator(NAME_None);

	TSharedRef<SWidget> TrianglesTextWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f, 8.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TriangleSelectionText", "Triangles"))
		];

	Section.AddEntry(FToolMenuEntry::InitWidget("TrianglesTextWidget", TrianglesTextWidget, FText()));
	
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.MeshSelectionModeAction_MeshTriangles));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.MeshSelectionModeAction_MeshEdges));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.MeshSelectionModeAction_MeshVertices));

	Section.AddSeparator(NAME_None);

	TSharedRef<SWidget> PolyGroupsTextWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f, 8.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PolyGroupsSelectionText", "PolyGroups"))
		];

	Section.AddEntry(FToolMenuEntry::InitWidget("PolyGroupsTextWidget", PolyGroupsTextWidget, FText()));
	
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.MeshSelectionModeAction_GroupFaces));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.MeshSelectionModeAction_GroupEdges));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.MeshSelectionModeAction_GroupCorners));

	Section.AddSeparator(NAME_None);

	TSharedRef<SWidget> SelectionEditTextWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f, 8.0f, 0.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectionEditText", "Selection Edit"))
		];

	Section.AddEntry(FToolMenuEntry::InitWidget("SelectionEditTextWidget", SelectionEditTextWidget, FText()));

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.BeginSelectionAction_SelectAll));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.BeginSelectionAction_Invert));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.BeginSelectionAction_ExpandToConnected));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.BeginSelectionAction_InvertConnected));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.BeginSelectionAction_Expand));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(Commands.BeginSelectionAction_Contract));
	
	Section.AddSeparator(NAME_None);

	TSharedRef<SWidget> DefaultSettingsWidget = SNew(SComboButton)
		.HasDownArrow(false)
		.MenuPlacement(EMenuPlacement::MenuPlacement_MenuRight)
		.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
		.OnGetMenuContent(FOnGetContent::CreateLambda([this]()
		{
			return UELocal::MakeMenu_SelectionConfigSettings(this);
		}))
		.ContentPadding(FMargin(3.0f, 1.0f))
		.ButtonContent()
		[
			SNew(SImage)
			.Image(FTLLRN_ModelingToolsEditorModeStyle::Get()->GetBrush("TLLRN_ModelingMode.DefaultSettings"))
			.ColorAndOpacity(FSlateColor::UseForeground())
		];

	Section.AddEntry(FToolMenuEntry::InitWidget("DefaultSettings", DefaultSettingsWidget, FText()));
	
	Section.AddSeparator(NAME_None);

}




void FTLLRN_ModelingToolsEditorModeToolkit::MakeSelectionPaletteOverlayWidget()
{
	FVerticalToolBarBuilder ToolbarBuilder(
		GetToolkitCommands(),
		FMultiBoxCustomization::None,
		TSharedPtr<FExtender>(), /*InForceSmallIcons=*/ true);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);
	ToolbarBuilder.SetStyle(FTLLRN_ModelingToolsEditorModeStyle::Get().Get(), "SelectionToolBar");

	const FTLLRN_ModelingToolsManagerCommands& Commands = FTLLRN_ModelingToolsManagerCommands::Get();

	ToolbarBuilder.AddToolBarButton(Commands.MeshSelectionModeAction_NoSelection);
	ToolbarBuilder.AddWidget(SNew(SSpacer).Size(FVector2D(1, 1)));
	ToolbarBuilder.AddToolBarButton(Commands.MeshSelectionModeAction_MeshTriangles);
	ToolbarBuilder.AddToolBarButton(Commands.MeshSelectionModeAction_MeshEdges);
	ToolbarBuilder.AddToolBarButton(Commands.MeshSelectionModeAction_MeshVertices);
	ToolbarBuilder.AddWidget(SNew(SSpacer).Size(FVector2D(1, 1)));
	ToolbarBuilder.AddToolBarButton(Commands.MeshSelectionModeAction_GroupFaces);
	ToolbarBuilder.AddToolBarButton(Commands.MeshSelectionModeAction_GroupEdges);
	ToolbarBuilder.AddToolBarButton(Commands.MeshSelectionModeAction_GroupCorners);
	ToolbarBuilder.AddWidget(SNew(SSpacer).Size(FVector2D(1, 8)));


	ToolbarBuilder.AddWidget(
		SNew(SComboButton)
		.HasDownArrow(false)
		.MenuPlacement(EMenuPlacement::MenuPlacement_MenuRight)
		.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
		.IsEnabled_Lambda([this]() { return IsInActiveTool() == false; })
		.OnGetMenuContent(FOnGetContent::CreateLambda([this]()
			{
				return UELocal::MakeMenu_SelectionEdits(this);
			}))
		.ContentPadding(FMargin(1.0f, 1.0f))
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FTLLRN_ModelingToolsEditorModeStyle::Get()->GetBrush("TLLRN_ModelingModeSelection.Edits_Right"))
				]);

	ToolbarBuilder.AddWidget(
		SNew(SComboButton)
		.HasDownArrow(false)
		.MenuPlacement(EMenuPlacement::MenuPlacement_MenuRight)
		.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
		.IsEnabled_Lambda([this]() { return IsInActiveTool() == false; })
		.OnGetMenuContent(FOnGetContent::CreateLambda([this]()
			{
				return UELocal::MakeMenu_SelectionConfigSettings(this);
			}))
		.ContentPadding(FMargin(1.0f, 1.0f))
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FTLLRN_ModelingToolsEditorModeStyle::Get()->GetBrush("TLLRN_ModelingMode.DefaultSettings"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]);

	ToolbarBuilder.AddWidget(SNew(SSpacer).Size(FVector2D(1, 16)));

	//
	// locking toggle button is implemented using 3 separate buttons because we cannot 
	// control enabled/disabled or background color for a toolbar button
	//

	FUIAction UnlockTargetAction(
		FExecuteAction::CreateLambda([this] { GetMeshSelectionManager()->SetCurrentTargetsLockState(false); }),
		FCanExecuteAction::CreateLambda([this] { return GetMeshSelectionManager()->GetAnyCurrentTargetsLocked() && IsInActiveTool() == false; }),
		FIsActionChecked::CreateLambda([] { return false; }),
		FIsActionButtonVisible::CreateLambda([this] {
			return GetMeshSelectionManager()->GetMeshTopologyMode() != UGeometrySelectionManager::EMeshTopologyMode::None
			&& GetMeshSelectionManager()->GetAnyCurrentTargetsLockable()
			&& GetMeshSelectionManager()->GetAnyCurrentTargetsLocked();
			})
	);
	FUIAction LockTargetAction(
		FExecuteAction::CreateLambda([this] { GetMeshSelectionManager()->SetCurrentTargetsLockState(true); }),
		FCanExecuteAction::CreateLambda([this] { return !GetMeshSelectionManager()->GetAnyCurrentTargetsLocked() && IsInActiveTool() == false; }),
		FIsActionChecked::CreateLambda([]() { return false; }),
		FIsActionButtonVisible::CreateLambda([this] {
			return GetMeshSelectionManager()->GetMeshTopologyMode() != UGeometrySelectionManager::EMeshTopologyMode::None
			&& GetMeshSelectionManager()->GetAnyCurrentTargetsLockable()
			&& (GetMeshSelectionManager()->GetAnyCurrentTargetsLocked() == false);
			})
	);
	FUIAction DisabledLockTargetAction(
		FExecuteAction::CreateLambda([] {}),
		FCanExecuteAction::CreateLambda([] { return false; }),
		FIsActionChecked::CreateLambda([]() { return false; }),
		FIsActionButtonVisible::CreateLambda([this] {
			return GetMeshSelectionManager()->GetMeshTopologyMode() == UGeometrySelectionManager::EMeshTopologyMode::None
			|| GetMeshSelectionManager()->GetAnyCurrentTargetsLockable() == false;
			})
	);

	ToolbarBuilder.BeginStyleOverride(FName("SelectionToolBar.RedButton"));
	ToolbarBuilder.AddToolBarButton(UnlockTargetAction, NAME_None,
		LOCTEXT("Selection_UnlockTarget", "Unlock"),
		LOCTEXT("Selection_UnlockTarget_Tooltip", "Click to Unlock the Selected Object and allow Mesh Selections"),
		FSlateIcon(FTLLRN_ModelingToolsEditorModeStyle::GetStyleSetName(), "SelectionToolBarIcons.LockedTarget"));
	ToolbarBuilder.EndStyleOverride();
	ToolbarBuilder.BeginStyleOverride(FName("SelectionToolBar.GreenButton"));
	ToolbarBuilder.AddToolBarButton(LockTargetAction, NAME_None,
		LOCTEXT("Selection_LockTarget", "Lock"),
		LOCTEXT("Selection_LockTarget_Tooltip", "Click to Lock the Selected Object"),
		FSlateIcon(FTLLRN_ModelingToolsEditorModeStyle::GetStyleSetName(), "SelectionToolBarIcons.UnlockedTarget"));
	ToolbarBuilder.EndStyleOverride();
	ToolbarBuilder.AddToolBarButton(DisabledLockTargetAction, NAME_None,
		LOCTEXT("Selection_LockTargetDisabled", "(No Locking)"),
		LOCTEXT("Selection_LockTargetDisabled_Tooltip", "No Active Selection Targets are Lockable"),
		FSlateIcon(FTLLRN_ModelingToolsEditorModeStyle::GetStyleSetName(), "SelectionToolBarIcons.UnlockedTarget"));


	TSharedRef<SWidget> Toolbar = ToolbarBuilder.MakeWidget();

	SAssignNew(SelectionPaletteOverlayWidget, SVerticalBox)
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FMargin(2.0f, 0.0f, 0.f, 0.f))
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("EditorViewport.OverlayBrush"))
			.Padding(FMargin(3.0f, 6.0f, 3.f, 6.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0, 0.f, 0.f, 0.f))
				[
					Toolbar
				]
			]
		];

	SelectionPaletteOverlayWidget->SetVisibility( TAttribute<EVisibility>::CreateLambda([this]()
	{
		if ( UTLLRN_ModelingToolsEditorMode* TLLRN_ModelingMode = Cast<UTLLRN_ModelingToolsEditorMode>(GetScriptableEditorMode()) )
		{
			return TLLRN_ModelingMode->GetMeshElementSelectionSystemEnabled() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		}
		return EVisibility::Collapsed;
	}) );


}




#undef LOCTEXT_NAMESPACE
