// Copyright Epic Games, Inc. All Rights Reserved.

#include "RetargetEditor/TLLRN_SIKRetargetPoseEditor.h"

#include "RetargetEditor/TLLRN_IKRetargetCommands.h"
#include "RetargetEditor/TLLRN_IKRetargetEditor.h"
#include "RetargetEditor/TLLRN_IKRetargetEditorController.h"

#include "ToolMenus.h"
#include "Widgets/Layout/SSeparator.h"

#define LOCTEXT_NAMESPACE "TLLRN_SIKRetargetPoseEditor"

void TLLRN_SIKRetargetPoseEditor::Construct(
	const FArguments& InArgs,
	TSharedRef<FTLLRN_IKRetargetEditorController> InEditorController)
{
	EditorController = InEditorController;

	// the editor controller
	FTLLRN_IKRetargetEditorController* Controller = EditorController.Pin().Get();

	// the commands for the menus
	TSharedPtr<FUICommandList> Commands = Controller->Editor.Pin()->GetToolkitCommands();
	
	ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.Padding(2.0f)
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SHorizontalBox)
			
			// pose selection label
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4,0)
			[
				SNew(STextBlock).Text(LOCTEXT("CurrentPose", "Current Retarget Pose:"))
			]
						
			// pose selection combobox
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(PoseListComboBox, SComboBox<TSharedPtr<FName>>)
				.OptionsSource(&PoseNames)
				.OnGenerateWidget_Lambda([](TSharedPtr<FName> InItem)
				{
					return SNew(STextBlock).Text(FText::FromName(*InItem.Get()));
				})
				.OnSelectionChanged(Controller, &FTLLRN_IKRetargetEditorController::OnPoseSelected)
				[
					SNew(STextBlock).Text(Controller, &FTLLRN_IKRetargetEditorController::GetCurrentPoseName)
				]
			]

			// pose blending slider
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SSpinBox<float>)
				.Font(FAppStyle::Get().GetFontStyle("PropertyWindow.NormalFont"))
				.MinDesiredWidth(100)
				.MinValue(0.0f)
				.MaxValue(1.0f)
				.Value(Controller, &FTLLRN_IKRetargetEditorController::GetRetargetPoseAmount)
				.OnValueChanged(Controller, &FTLLRN_IKRetargetEditorController::SetRetargetPoseAmount)
			]
		]

		// pose editing toolbar
		+SVerticalBox::Slot()
		.Padding(2.0f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Center)
			[
				MakeToolbar(Commands)
			]
		]
	];
}

void TLLRN_SIKRetargetPoseEditor::Refresh()
{
	// get the retarget poses from the editor controller
	const FTLLRN_IKRetargetEditorController* Controller = EditorController.Pin().Get();
	const TMap<FName, FTLLRN_IKRetargetPose>& RetargetPoses = Controller->AssetController->GetRetargetPoses(Controller->GetSourceOrTarget());

	// fill list of pose names
	PoseNames.Reset();
	for (const TTuple<FName, FTLLRN_IKRetargetPose>& Pose : RetargetPoses)
	{
		PoseNames.Add(MakeShareable(new FName(Pose.Key)));
	}
}

TSharedRef<SWidget>  TLLRN_SIKRetargetPoseEditor::MakeToolbar(TSharedPtr<FUICommandList> Commands)
{
	FToolBarBuilder ToolbarBuilder(Commands, FMultiBoxCustomization::None);
	
	ToolbarBuilder.BeginSection("Edit Current Pose");

	ToolbarBuilder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateSP(this, &TLLRN_SIKRetargetPoseEditor::GenerateResetMenuContent, Commands),
		LOCTEXT("ResetPose_Label", "Reset"),
		LOCTEXT("ResetPoseToolTip_Label", "Reset bones to reference pose."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));

	ToolbarBuilder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateSP(this, &TLLRN_SIKRetargetPoseEditor::GenerateEditMenuContent, Commands),
		LOCTEXT("AutoAlign_Label", "Auto Align"),
		LOCTEXT("AutoAlignTip_Label", "Automatically aligns bones on source skeleton to target (or vice versa)."),
		FSlateIcon(FTLLRN_IKRetargetEditorStyle::Get().GetStyleSetName(),"TLLRN_IKRetarget.TLLRN_AutoAlign"));

	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Create Poses");

	ToolbarBuilder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateSP(this, &TLLRN_SIKRetargetPoseEditor::GenerateNewMenuContent, Commands),
		LOCTEXT("CreatePose_Label", "Create"),
		TAttribute<FText>(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));

	ToolbarBuilder.AddToolBarButton(
		FTLLRN_IKRetargetCommands::Get().DeleteRetargetPose,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FAppStyle::Get().GetStyleSetName(),"Icons.Delete"));

	ToolbarBuilder.AddToolBarButton(
		FTLLRN_IKRetargetCommands::Get().RenameRetargetPose,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FAppStyle::Get().GetStyleSetName(),"Icons.Settings"));

	ToolbarBuilder.EndSection();

	return ToolbarBuilder.MakeWidget();
}

TSharedRef<SWidget> TLLRN_SIKRetargetPoseEditor::GenerateResetMenuContent(TSharedPtr<FUICommandList> Commands)
{
	FMenuBuilder MenuBuilder(true, Commands);
	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().ResetSelectedBones);
	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().ResetSelectedAndChildrenBones);
	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().ResetAllBones);
	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> TLLRN_SIKRetargetPoseEditor::GenerateEditMenuContent(TSharedPtr<FUICommandList> Commands)
{
	FMenuBuilder MenuBuilder(true, Commands);

	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().AutoAlignAllBones);
	MenuBuilder.AddSeparator();
	
	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().AlignSelected);
	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().AlignSelectedAndChildren);
	MenuBuilder.AddSeparator();
	
	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().AlignSelectedUsingMesh);
	MenuBuilder.AddSeparator();
	
	MenuBuilder.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().SnapCharacterToGround);

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> TLLRN_SIKRetargetPoseEditor::GenerateNewMenuContent(TSharedPtr<FUICommandList> Commands)
{
	const FName ParentEditorName = EditorController.Pin()->Editor.Pin()->GetToolMenuName();
	const FName MenuName = FName(ParentEditorName.ToString() + TEXT(".CreateMenu"));
	UToolMenu* ToolMenu = UToolMenus::Get()->ExtendMenu(MenuName);

	FToolMenuSection& CreateSection = ToolMenu->AddSection("Create", LOCTEXT("CreatePoseOperations", "Create New Retarget Pose"));
	CreateSection.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().NewRetargetPose);
	CreateSection.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().DuplicateRetargetPose);
	
	FToolMenuSection& ImportSection = ToolMenu->AddSection("Import",LOCTEXT("ImportPoseOperations", "Import Retarget Pose"));
	ImportSection.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().ImportRetargetPose);
	ImportSection.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().ImportRetargetPoseFromAnim);

	FToolMenuSection& ExportSection = ToolMenu->AddSection("Export",LOCTEXT("ExportPoseOperations", "Export Retarget Pose"));
	ExportSection.AddMenuEntry(FTLLRN_IKRetargetCommands::Get().ExportRetargetPose);

	return UToolMenus::Get()->GenerateWidget(MenuName, FToolMenuContext(Commands));
}

#undef LOCTEXT_NAMESPACE
