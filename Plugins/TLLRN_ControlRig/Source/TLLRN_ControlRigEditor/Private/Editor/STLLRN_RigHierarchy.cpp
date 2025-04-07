// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/STLLRN_RigHierarchy.h"
#include "Widgets/Input/SComboButton.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintVariableNodeSpawner.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "K2Node_VariableGet.h"
#include "RigVMBlueprintUtils.h"
#include "TLLRN_ControlTLLRN_RigHierarchyCommands.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "Graph/TLLRN_ControlRigGraphNode.h"
#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "TLLRN_ModularRig.h"
#include "GraphEditorModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AnimationRuntime.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor/EditorEngine.h"
#include "HelperUtil.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "TLLRN_ControlRig.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformTime.h"
#include "Dialogs/Dialogs.h"
#include "IPersonaToolkit.h"
#include "SKismetInspector.h"
#include "Types/WidgetActiveTimerDelegate.h"
#include "Dialog/SCustomDialog.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "ToolMenus.h"
#include "Editor/TLLRN_ControlRigContextMenuContext.h"
#include "Editor/STLLRN_RigSpacePickerWidget.h"
#include "Settings/TLLRN_ControlRigSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Styling/AppStyle.h"
#include "TLLRN_ControlRigSkeletalMeshComponent.h"
#include "TLLRN_ModularRigRuleManager.h"
#include "Sequencer/TLLRN_ControlRigLayerInstance.h"
#include "Algo/MinElement.h"
#include "Algo/MaxElement.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"
#include "Preferences/PersonaOptions.h"
#include "Editor/STLLRN_ModularRigModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(STLLRN_RigHierarchy)

#define LOCTEXT_NAMESPACE "STLLRN_RigHierarchy"

///////////////////////////////////////////////////////////

const FName STLLRN_RigHierarchy::ContextMenuName = TEXT("TLLRN_ControlRigEditor.TLLRN_RigHierarchy.ContextMenu");
const FName STLLRN_RigHierarchy::DragDropMenuName = TEXT("TLLRN_ControlRigEditor.TLLRN_RigHierarchy.DragDropMenu");

STLLRN_RigHierarchy::~STLLRN_RigHierarchy()
{
	const FTLLRN_ControlRigEditor* Editor = TLLRN_ControlRigEditor.IsValid() ? TLLRN_ControlRigEditor.Pin().Get() : nullptr;
	OnEditorClose(Editor, TLLRN_ControlRigBlueprint.Get());
}

void STLLRN_RigHierarchy::Construct(const FArguments& InArgs, TSharedRef<FTLLRN_ControlRigEditor> InTLLRN_ControlRigEditor)
{
	TLLRN_ControlRigEditor = InTLLRN_ControlRigEditor;

	TLLRN_ControlRigBlueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint();

	TLLRN_ControlRigBlueprint->Hierarchy->OnModified().AddRaw(this, &STLLRN_RigHierarchy::OnHierarchyModified);
	TLLRN_ControlRigBlueprint->OnRefreshEditor().AddRaw(this, &STLLRN_RigHierarchy::HandleRefreshEditorFromBlueprint);
	TLLRN_ControlRigBlueprint->OnSetObjectBeingDebugged().AddRaw(this, &STLLRN_RigHierarchy::HandleSetObjectBeingDebugged);

	if(UTLLRN_ModularTLLRN_RigController* TLLRN_ModularTLLRN_RigController = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
	{
		TLLRN_ModularTLLRN_RigController->OnModified().AddSP(this, &STLLRN_RigHierarchy::OnTLLRN_ModularRigModified);
	}

	// for deleting, renaming, dragging
	CommandList = MakeShared<FUICommandList>();

	UEditorEngine* Editor = Cast<UEditorEngine>(GEngine);
	if (Editor != nullptr)
	{
		Editor->RegisterForUndo(this);
	}

	BindCommands();

	// setup all delegates for the rig hierarchy widget
	FRigTreeDelegates Delegates;
	Delegates.OnGetHierarchy = FOnGetRigTreeHierarchy::CreateSP(this, &STLLRN_RigHierarchy::GetHierarchyForTreeView);
	Delegates.OnGetDisplaySettings = FOnGetRigTreeDisplaySettings::CreateSP(this, &STLLRN_RigHierarchy::GetDisplaySettings);
	Delegates.OnRenameElement = FOnRigTreeRenameElement::CreateSP(this, &STLLRN_RigHierarchy::HandleRenameElement);
	Delegates.OnVerifyElementNameChanged = FOnRigTreeVerifyElementNameChanged::CreateSP(this, &STLLRN_RigHierarchy::HandleVerifyNameChanged);
	Delegates.OnSelectionChanged = FOnRigTreeSelectionChanged::CreateSP(this, &STLLRN_RigHierarchy::OnSelectionChanged);
	Delegates.OnContextMenuOpening = FOnContextMenuOpening::CreateSP(this, &STLLRN_RigHierarchy::CreateContextMenuWidget);
	Delegates.OnMouseButtonClick = FOnRigTreeMouseButtonClick::CreateSP(this, &STLLRN_RigHierarchy::OnItemClicked);
	Delegates.OnMouseButtonDoubleClick = FOnRigTreeMouseButtonClick::CreateSP(this, &STLLRN_RigHierarchy::OnItemDoubleClicked);
	Delegates.OnSetExpansionRecursive = FOnRigTreeSetExpansionRecursive::CreateSP(this, &STLLRN_RigHierarchy::OnSetExpansionRecursive);
	Delegates.OnCanAcceptDrop = FOnRigTreeCanAcceptDrop::CreateSP(this, &STLLRN_RigHierarchy::OnCanAcceptDrop);
	Delegates.OnAcceptDrop = FOnRigTreeAcceptDrop::CreateSP(this, &STLLRN_RigHierarchy::OnAcceptDrop);
	Delegates.OnDragDetected = FOnDragDetected::CreateSP(this, &STLLRN_RigHierarchy::OnDragDetected);
	Delegates.OnGetResolvedKey = FOnRigTreeGetResolvedKey::CreateSP(this, &STLLRN_RigHierarchy::OnGetResolvedKey);
	Delegates.OnRequestDetailsInspection = FOnRigTreeRequestDetailsInspection::CreateSP(this, &STLLRN_RigHierarchy::OnRequestDetailsInspection);
	Delegates.OnRigTreeElementKeyTagDragDetected = FOnRigTreeRequestDetailsInspection::CreateSP(this, &STLLRN_RigHierarchy::OnElementKeyTagDragDetected);
	Delegates.OnRigTreeGetItemToolTip = FOnRigTreeItemGetToolTip::CreateSP(this, &STLLRN_RigHierarchy::OnGetItemTooltip);

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.Padding(0.0f)
		[
			SNew(SBorder)
			.Padding(0.0f)
			.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
			.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Top)
				[
					SNew(SHorizontalBox)
					.Visibility(this, &STLLRN_RigHierarchy::IsToolbarVisible)

					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.MaxWidth(180.0f)
					.Padding(3.0f, 1.0f)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
						.ForegroundColor(FLinearColor::White)
						.OnClicked(FOnClicked::CreateSP(this, &STLLRN_RigHierarchy::OnImportSkeletonClicked))
						.Text(this, &STLLRN_RigHierarchy::GetImportHierarchyText)
						.IsEnabled(this, &STLLRN_RigHierarchy::IsImportHierarchyEnabled)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Top)
				[
					SNew(SHorizontalBox)
					.Visibility(this, &STLLRN_RigHierarchy::IsSearchbarVisible)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 2.0f, 0.0f)
					[
						SNew(SComboButton)
						.Visibility(EVisibility::Visible)
						.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButtonWithIcon"))
						.ForegroundColor(FSlateColor::UseStyle())
						.ContentPadding(0.0f)
						.OnGetMenuContent(this, &STLLRN_RigHierarchy::CreateFilterMenu)
						.ButtonContent()
						[
							SNew(SImage)
							.Image(FAppStyle::Get().GetBrush("Icons.Filter"))
							.ColorAndOpacity(FSlateColor::UseForeground())
						 ]
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(3.0f, 1.0f)
					[
						SAssignNew(FilterBox, SSearchBox)
						.OnTextChanged(this, &STLLRN_RigHierarchy::OnFilterTextChanged)
					]
				]
			]
		]
		+SVerticalBox::Slot()
		.Padding(0.0f, 0.0f)
		[
			SNew(SBorder)
			.Padding(0.0f)
			.ShowEffectWhenDisabled(false)
			[
				SNew(SBorder)
				.Padding(2.0f)
				.BorderImage(FAppStyle::GetBrush("SCSEditor.TreePanel"))
				[
					SAssignNew(TreeView, STLLRN_RigHierarchyTreeView)
					.RigTreeDelegates(Delegates)
					.AutoScrollEnabled(true)
				]
			]
		]

		/*
		+ SVerticalBox::Slot()
		.Padding(0.0f, 0.0f)
		.FillHeight(0.1f)
		[
			SNew(SBorder)
			.Padding(2.0f)
			.BorderImage(FAppStyle::GetBrush("SCSEditor.TreePanel"))
			[
			SNew(SSpacer)
			]
		]
		*/
	];

	bIsChangingTLLRN_RigHierarchy = false;
	LastHierarchyHash = INDEX_NONE;
	bIsConstructionEventRunning = false;
	
	RefreshTreeView();

	if (TLLRN_ControlRigEditor.IsValid())
	{
		TLLRN_ControlRigEditor.Pin()->GetKeyDownDelegate().BindLambda([&](const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)->FReply {
			return OnKeyDown(MyGeometry, InKeyEvent);
		});
		TLLRN_ControlRigEditor.Pin()->OnGetViewportContextMenu().BindSP(this, &STLLRN_RigHierarchy::GetContextMenu);
		TLLRN_ControlRigEditor.Pin()->OnViewportContextMenuCommands().BindSP(this, &STLLRN_RigHierarchy::GetContextMenuCommands);
		TLLRN_ControlRigEditor.Pin()->OnEditorClosed().AddSP(this, &STLLRN_RigHierarchy::OnEditorClose);
		TLLRN_ControlRigEditor.Pin()->OnRequestNavigateToConnectorWarning().AddSP(this, &STLLRN_RigHierarchy::OnNavigateToFirstConnectorWarning);
	}
	
	CreateContextMenu();
	CreateDragDropMenu();

	// after opening the editor the debugged rig won't exist yet. we'll have to wait for a tick so
	// that we have a valid rig to listen to.
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateLambda([this](double, float) {
		if(TLLRN_ControlRigBlueprint.IsValid())
		{
			(void)HandleSetObjectBeingDebugged(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
		}
		return EActiveTimerReturnType::Stop;
	}));
}

void STLLRN_RigHierarchy::OnEditorClose(const FRigVMEditor* InEditor, URigVMBlueprint* InBlueprint)
{
	if (InEditor)
	{
		FTLLRN_ControlRigEditor* Editor = (FTLLRN_ControlRigEditor*)InEditor;  
		Editor->GetKeyDownDelegate().Unbind();
		Editor->OnGetViewportContextMenu().Unbind();
		Editor->OnViewportContextMenuCommands().Unbind();
	}

	if (UTLLRN_ControlRigBlueprint* BP = Cast<UTLLRN_ControlRigBlueprint>(InBlueprint))
	{
		BP->Hierarchy->OnModified().RemoveAll(this);
		InBlueprint->OnRefreshEditor().RemoveAll(this);
		InBlueprint->OnSetObjectBeingDebugged().RemoveAll(this);

		if(UTLLRN_ModularTLLRN_RigController* TLLRN_ModularTLLRN_RigController = BP->GetTLLRN_ModularTLLRN_RigController())
		{
			TLLRN_ModularTLLRN_RigController->OnModified().RemoveAll(this);
		}
	}
	
	TLLRN_ControlRigEditor.Reset();
	TLLRN_ControlRigBlueprint.Reset();
}

void STLLRN_RigHierarchy::BindCommands()
{
	// create new command
	const FTLLRN_ControlTLLRN_RigHierarchyCommands& Commands = FTLLRN_ControlTLLRN_RigHierarchyCommands::Get();

	CommandList->MapAction(Commands.AddBoneItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleNewItem, ETLLRN_RigElementType::Bone, false),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanAddElement, ETLLRN_RigElementType::Bone));

	CommandList->MapAction(Commands.AddControlItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleNewItem, ETLLRN_RigElementType::Control, false),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanAddElement, ETLLRN_RigElementType::Control));

	CommandList->MapAction(Commands.AddAnimationChannelItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleNewItem, ETLLRN_RigElementType::Control, true),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanAddAnimationChannel));

	CommandList->MapAction(Commands.AddNullItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleNewItem, ETLLRN_RigElementType::Null, false),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanAddElement, ETLLRN_RigElementType::Null));

	CommandList->MapAction(Commands.AddConnectorItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleNewItem, ETLLRN_RigElementType::Connector, false),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanAddElement, ETLLRN_RigElementType::Connector));

	CommandList->MapAction(Commands.AddSocketItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleNewItem, ETLLRN_RigElementType::Socket, false),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanAddElement, ETLLRN_RigElementType::Socket));

	CommandList->MapAction(Commands.FindReferencesOfItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleFindReferencesOfItem),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanFindReferencesOfItem));

	CommandList->MapAction(Commands.DuplicateItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleDuplicateItem),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanDuplicateItem));

	CommandList->MapAction(Commands.MirrorItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleMirrorItem),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanDuplicateItem));

	CommandList->MapAction(Commands.DeleteItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleDeleteItem),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanDeleteItem));

	CommandList->MapAction(Commands.RenameItem,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleRenameItem),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanRenameItem));

	CommandList->MapAction(Commands.CopyItems,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleCopyItems),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanCopyOrPasteItems));

	CommandList->MapAction(Commands.PasteItems,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandlePasteItems),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanPasteItems));

	CommandList->MapAction(Commands.PasteLocalTransforms,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandlePasteLocalTransforms),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanCopyOrPasteItems));

	CommandList->MapAction(Commands.PasteGlobalTransforms,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandlePasteGlobalTransforms),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::CanCopyOrPasteItems));

	CommandList->MapAction(
		Commands.ResetTransform,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleResetTransform, true),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::IsMultiSelected, true));

	CommandList->MapAction(
		Commands.ResetAllTransforms,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleResetTransform, false));

	CommandList->MapAction(
		Commands.SetInitialTransformFromClosestBone,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleSetInitialTransformFromClosestBone),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::IsControlOrNullSelected, false));

	CommandList->MapAction(
		Commands.SetInitialTransformFromCurrentTransform,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleSetInitialTransformFromCurrentTransform),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::IsMultiSelected, false));

	CommandList->MapAction(
		Commands.SetShapeTransformFromCurrent,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleSetShapeTransformFromCurrent),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::IsControlSelected, false));

	CommandList->MapAction(
		Commands.FrameSelection,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleFrameSelection),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::IsMultiSelected, true));

	CommandList->MapAction(
		Commands.ControlBoneTransform,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleControlBoneOrSpaceTransform),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::IsSingleBoneSelected, false),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &STLLRN_RigHierarchy::IsSingleBoneSelected, false)
		);

	CommandList->MapAction(
		Commands.Unparent,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleUnparent),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::IsMultiSelected, false));

	CommandList->MapAction(
		Commands.FilteringFlattensHierarchy,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bFlattenHierarchyOnFilter = !DisplaySettings.bFlattenHierarchyOnFilter; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bFlattenHierarchyOnFilter; }));

	CommandList->MapAction(
		Commands.HideParentsWhenFiltering,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bHideParentsOnFilter = !DisplaySettings.bHideParentsOnFilter; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bHideParentsOnFilter; }));

	CommandList->MapAction(
		Commands.ShowShortNames,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bUseShortName = !DisplaySettings.bUseShortName; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bUseShortName; }));

	CommandList->MapAction(
		Commands.ShowImportedBones,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bShowImportedBones = !DisplaySettings.bShowImportedBones; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bShowImportedBones; }));
	
	CommandList->MapAction(
		Commands.ShowBones,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bShowBones = !DisplaySettings.bShowBones; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bShowBones; }));

	CommandList->MapAction(
		Commands.ShowControls,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bShowControls = !DisplaySettings.bShowControls; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bShowControls; }));
	
	CommandList->MapAction(
		Commands.ShowNulls,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bShowNulls = !DisplaySettings.bShowNulls; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bShowNulls; }));

	CommandList->MapAction(
		Commands.ShowPhysics,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bShowPhysics = !DisplaySettings.bShowPhysics; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bShowPhysics; }));

	CommandList->MapAction(
		Commands.ShowReferences,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bShowReferences = !DisplaySettings.bShowReferences; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bShowReferences; }));

	CommandList->MapAction(
		Commands.ShowSockets,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bShowSockets = !DisplaySettings.bShowSockets; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bShowSockets; }));

	CommandList->MapAction(
		Commands.ToggleControlShapeTransformEdit,
		FExecuteAction::CreateLambda([this]()
		{
			TLLRN_ControlRigEditor.Pin()->GetEditMode()->ToggleControlShapeTransformEdit();
		}));

	CommandList->MapAction(
		Commands.SpaceSwitching,
		FExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::HandleTestSpaceSwitching),
		FCanExecuteAction::CreateSP(this, &STLLRN_RigHierarchy::IsControlSelected, true));

	CommandList->MapAction(
		Commands.ShowIconColors,
		FExecuteAction::CreateLambda([this]() { DisplaySettings.bShowIconColors = !DisplaySettings.bShowIconColors; RefreshTreeView(); }),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([this]() { return DisplaySettings.bShowIconColors; }));
}

FReply STLLRN_RigHierarchy::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList.IsValid() && CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply STLLRN_RigHierarchy::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
	if(Reply.IsEventHandled())
	{
		return Reply;
	}

	if(MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		if(const TSharedPtr<FRigTreeElement>* ItemPtr = TreeView->FindItemAtPosition(MouseEvent.GetScreenSpacePosition()))
		{
			if(const TSharedPtr<FRigTreeElement>& Item = *ItemPtr)
			{
				if (UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
				{
					TArray<FTLLRN_RigElementKey> KeysToSelect = {Item->Key};
					KeysToSelect.Append(Hierarchy->GetChildren(Item->Key, true));

					UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
					check(Controller);
		
					Controller->SetSelection(KeysToSelect);
				}
			}
		}
	}

	return FReply::Unhandled();
}

EVisibility STLLRN_RigHierarchy::IsToolbarVisible() const
{
	if (UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		if (Hierarchy->Num(ETLLRN_RigElementType::Bone) > 0)
		{
			return EVisibility::Collapsed;
		}
	}
	return EVisibility::Visible;
}

EVisibility STLLRN_RigHierarchy::IsSearchbarVisible() const
{
	if (UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		if ((Hierarchy->Num(ETLLRN_RigElementType::Bone) +
			Hierarchy->Num(ETLLRN_RigElementType::Null) +
			Hierarchy->Num(ETLLRN_RigElementType::Control) +
			Hierarchy->Num(ETLLRN_RigElementType::Connector) +
			Hierarchy->Num(ETLLRN_RigElementType::Socket)) > 0)
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

FReply STLLRN_RigHierarchy::OnImportSkeletonClicked()
{
	FTLLRN_RigHierarchyImportSettings Settings;
	TSharedPtr<FStructOnScope> StructToDisplay = MakeShareable(new FStructOnScope(FTLLRN_RigHierarchyImportSettings::StaticStruct(), (uint8*)&Settings));

	TSharedRef<SKismetInspector> KismetInspector = SNew(SKismetInspector);
	KismetInspector->ShowSingleStruct(StructToDisplay);

	SGenericDialogWidget::FArguments DialogArguments;
	DialogArguments.OnOkPressed_Lambda([&Settings, this] ()
	{
		if (Settings.Mesh != nullptr)
		{
			if(TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule())
			{
				//TLLRN_ControlRigBlueprint->SetPreviewMesh(Settings.Mesh);
				UpdateMesh(Settings.Mesh, true);
			}
			else
			{
				ImportHierarchy(FAssetData(Settings.Mesh));
			}
		}
	});
	
	SGenericDialogWidget::OpenDialog(LOCTEXT("TLLRN_ControlTLLRN_RigHierarchyImport", "Import Hierarchy"), KismetInspector, DialogArguments, true);

	return FReply::Handled();
}

FText STLLRN_RigHierarchy::GetImportHierarchyText() const
{
	if(TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule())
	{
		return LOCTEXT("SetPreviewMesh", "Set Preview Mesh");
	}
	return LOCTEXT("ImportHierarchy", "Import Hierarchy");
}

bool STLLRN_RigHierarchy::IsImportHierarchyEnabled() const
{
	// for now we'll enable this always
	return true;
}

void STLLRN_RigHierarchy::OnFilterTextChanged(const FText& SearchText)
{
	DisplaySettings.FilterText = SearchText;
	RefreshTreeView();
}

void STLLRN_RigHierarchy::RefreshTreeView(bool bRebuildContent)
{
	const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if(Hierarchy)
	{
		// is the rig currently running
		if(Hierarchy->HasExecuteContext())
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady([this, bRebuildContent]()
			{
				RefreshTreeView(bRebuildContent);
			}, TStatId(), NULL, ENamedThreads::GameThread);
		}
	}
	
	bool bDummySuspensionFlag = false;
	bool* SuspensionFlagPtr = &bDummySuspensionFlag;
	if (TLLRN_ControlRigEditor.IsValid())
	{
		SuspensionFlagPtr = &TLLRN_ControlRigEditor.Pin()->GetSuspendDetailsPanelRefreshFlag();
	}
	TGuardValue<bool> SuspendDetailsPanelRefreshGuard(*SuspensionFlagPtr, true);
	TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);

	TreeView->RefreshTreeView(bRebuildContent);
}

TArray<FTLLRN_RigElementKey> STLLRN_RigHierarchy::GetSelectedKeys() const
{
	TArray<TSharedPtr<FRigTreeElement>> SelectedItems = TreeView->GetSelectedItems();
	
	TArray<FTLLRN_RigElementKey> SelectedKeys;
	for (const TSharedPtr<FRigTreeElement>& SelectedItem : SelectedItems)
	{
		if(SelectedItem->Key.IsValid())
		{
			SelectedKeys.AddUnique(SelectedItem->Key);
		}
	}

	return SelectedKeys;
}

void STLLRN_RigHierarchy::OnSelectionChanged(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo)
{
	if (bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}

	TreeView->ClearHighlightedItems();

	// an element to use for the control rig editor's detail panel
	FTLLRN_RigElementKey LastSelectedElement;

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	if (Hierarchy)
	{
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);
		
		TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);

		// flag to guard during selection changes.
		// in case there's no editor we'll use the local variable.
		bool bDummySuspensionFlag = false;
		bool* SuspensionFlagPtr = &bDummySuspensionFlag;
		if (TLLRN_ControlRigEditor.IsValid())
		{
			SuspensionFlagPtr = &TLLRN_ControlRigEditor.Pin()->GetSuspendDetailsPanelRefreshFlag();
		}

		TGuardValue<bool> SuspendDetailsPanelRefreshGuard(*SuspensionFlagPtr, true);
		
		const TArray<FTLLRN_RigElementKey> NewSelection = GetSelectedKeys();
		if(!Controller->SetSelection(NewSelection, true))
		{
			return;
		}

		if (NewSelection.Num() > 0)
		{
			if (TLLRN_ControlRigEditor.IsValid())
			{
				if (TLLRN_ControlRigEditor.Pin()->GetEventQueueComboValue() == 1)
				{
					HandleControlBoneOrSpaceTransform();
				}
			}

			LastSelectedElement = NewSelection.Last();
		}
	}

	if (TLLRN_ControlRigEditor.IsValid())
	{
		if(LastSelectedElement.IsValid())
		{
			TLLRN_ControlRigEditor.Pin()->SetDetailViewForTLLRN_RigElements();
		}
		else
		{
			TLLRN_ControlRigEditor.Pin()->ClearDetailObject();
		}
	}
}

void STLLRN_RigHierarchy::OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	if(!TLLRN_ControlRigBlueprint.IsValid())
	{
		return;
	}
	
	if (TLLRN_ControlRigBlueprint->bSuspendAllNotifications)
	{
		return;
	}

	if (bIsChangingTLLRN_RigHierarchy || bIsConstructionEventRunning)
	{
		return;
	}

	if(InElement)
	{
		if(InElement->IsTypeOf(ETLLRN_RigElementType::Curve))
		{
			return;
		}
	}

	switch(InNotif)
	{
		case ETLLRN_RigHierarchyNotification::ElementAdded:
		{
			if(InElement)
			{
				if(TreeView->AddElement(InElement))
				{
					RefreshTreeView(false);
				}
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementRemoved:
		{
			if(InElement)
			{
				if(TreeView->RemoveElement(InElement->GetKey()))
				{
					RefreshTreeView(false);
				}
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::ParentChanged:
		{
			check(InHierarchy);
			if(InElement)
			{
				const FTLLRN_RigElementKey ParentKey = InHierarchy->GetFirstParent(InElement->GetKey());
				if(TreeView->ReparentElement(InElement->GetKey(), ParentKey))
				{
					RefreshTreeView(false);
				}
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::ParentWeightsChanged:
		{
			if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
			{
				if(InElement)
				{
					TArray<FTLLRN_RigElementWeight> ParentWeights = Hierarchy->GetParentWeightArray(InElement->GetKey());
					if(ParentWeights.Num() > 0)
					{
						TArray<FTLLRN_RigElementKey> ParentKeys = Hierarchy->GetParents(InElement->GetKey());
						check(ParentKeys.Num() == ParentWeights.Num());
						for(int32 ParentIndex=0;ParentIndex<ParentKeys.Num();ParentIndex++)
						{
							if(ParentWeights[ParentIndex].IsAlmostZero())
							{
								continue;
							}

							if(TreeView->ReparentElement(InElement->GetKey(), ParentKeys[ParentIndex]))
							{
								RefreshTreeView(false);
							}
							break;
						}
					}
				}
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementRenamed:
		case ETLLRN_RigHierarchyNotification::ElementReordered:
		case ETLLRN_RigHierarchyNotification::HierarchyReset:
		{
			RefreshTreeView();
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementSelected:
		case ETLLRN_RigHierarchyNotification::ElementDeselected:
		{
			if(InElement)
			{
				const bool bSelected = (InNotif == ETLLRN_RigHierarchyNotification::ElementSelected); 
					
				for (int32 RootIndex = 0; RootIndex < TreeView->RootElements.Num(); ++RootIndex)
				{
					TSharedPtr<FRigTreeElement> Found = TreeView->FindElement(InElement->GetKey(), TreeView->RootElements[RootIndex]);
					if (Found.IsValid())
					{
						TreeView->SetItemSelection(Found, bSelected, ESelectInfo::OnNavigation);

						if(GetDefault<UPersonaOptions>()->bExpandTreeOnSelection && bSelected)
						{
							HandleFrameSelection();
						}

						if (TLLRN_ControlRigEditor.IsValid() && !GIsTransacting)
						{
							if (TLLRN_ControlRigEditor.Pin()->GetEventQueueComboValue() == 1)
							{
								TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
								HandleControlBoneOrSpaceTransform();
							}
						}
					}
				}
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::ControlSettingChanged:
		case ETLLRN_RigHierarchyNotification::ConnectorSettingChanged:
		case ETLLRN_RigHierarchyNotification::SocketColorChanged:
		{
			// update color and other settings of the item
			if(InElement && (
				(InElement->GetType() == ETLLRN_RigElementType::Control) ||
				(InElement->GetType() == ETLLRN_RigElementType::Connector) ||
				(InElement->GetType() == ETLLRN_RigElementType::Socket)))
			{
				for (int32 RootIndex = 0; RootIndex < TreeView->RootElements.Num(); ++RootIndex)
				{
					TSharedPtr<FRigTreeElement> TreeElement = TreeView->FindElement(InElement->GetKey(), TreeView->RootElements[RootIndex]);
					if (TreeElement.IsValid())
					{
						const FRigTreeDisplaySettings& Settings = TreeView->GetRigTreeDelegates().GetDisplaySettings();
						TreeElement->RefreshDisplaySettings(InHierarchy, Settings);
					}
				}
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

void STLLRN_RigHierarchy::OnHierarchyModified_AnyThread(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	if(bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}
	
	if(!TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		return;
	}

	if(InHierarchy != TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy())
	{
		return;
	}

	if(bIsConstructionEventRunning)
	{
		return;
	}

	if(IsInGameThread())
	{
		OnHierarchyModified(InNotif, InHierarchy, InElement);
	}
	else
	{
		FTLLRN_RigElementKey Key;
		if(InElement)
		{
			Key = InElement->GetKey();
		}

		TWeakObjectPtr<UTLLRN_RigHierarchy> WeakHierarchy = InHierarchy;

		FFunctionGraphTask::CreateAndDispatchWhenReady([this, InNotif, WeakHierarchy, Key]()
        {
            if(!WeakHierarchy.IsValid())
            {
                return;
            }
            if (const FTLLRN_RigBaseElement* Element = WeakHierarchy.Get()->Find(Key))
            {
	            OnHierarchyModified(InNotif, WeakHierarchy.Get(), Element);
            }
			
        }, TStatId(), NULL, ENamedThreads::GameThread);
	}
}

void STLLRN_RigHierarchy::OnTLLRN_ModularRigModified(ETLLRN_ModularRigNotification InNotif, const FTLLRN_RigModuleReference* InModule)
{
	if(!TLLRN_ControlRigBlueprint.IsValid())
	{
		return;
	}

	switch(InNotif)
	{
		case ETLLRN_ModularRigNotification::ModuleSelected:
		case ETLLRN_ModularRigNotification::ModuleDeselected:
		{
			const bool bSelected = InNotif == ETLLRN_ModularRigNotification::ModuleSelected;
			if(TLLRN_ControlRigEditor.IsValid())
			{
				if(UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRig())
				{
					if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
					{
						const FString ModulePath = InModule->GetPath();
						TArray<FTLLRN_RigElementKey> Keys = Hierarchy->GetAllKeys();
						Keys = Keys.FilterByPredicate([Hierarchy, ModulePath](const FTLLRN_RigElementKey& InKey)
						{
							return ModulePath == Hierarchy->GetModulePath(InKey);
						});

						bool bScrollIntoView = true;
						for(const FTLLRN_RigElementKey& Key : Keys)
						{
							if(const TSharedPtr<FRigTreeElement> TreeElement = TreeView->FindElement(Key))
							{
								TreeView->SetItemHighlighted(TreeElement, bSelected);
								if(bScrollIntoView)
								{
									TreeView->RequestScrollIntoView(TreeElement);
									bScrollIntoView = false;
								}
							}
						}
					}
				}
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

void STLLRN_RigHierarchy::HandleRefreshEditorFromBlueprint(URigVMBlueprint* InBlueprint)
{
	if (bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}
	RefreshTreeView();
}

void STLLRN_RigHierarchy::HandleSetObjectBeingDebugged(UObject* InObject)
{
	if(TLLRN_ControlRigBeingDebuggedPtr.Get() == InObject)
	{
		return;
	}

	if(TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		if(UTLLRN_ControlRig* TLLRN_ControlRigBeingDebugged = TLLRN_ControlRigBeingDebuggedPtr.Get())
		{
			if(!URigVMHost::IsGarbageOrDestroyed(TLLRN_ControlRigBeingDebugged))
			{
				TLLRN_ControlRigBeingDebugged->GetHierarchy()->OnModified().RemoveAll(this);
			}
		}
	}

	TLLRN_ControlRigBeingDebuggedPtr.Reset();
	
	if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InObject))
	{
		TLLRN_ControlRigBeingDebuggedPtr = TLLRN_ControlRig;
		if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
		{
			Hierarchy->OnModified().RemoveAll(this);
			Hierarchy->OnModified().AddSP(this, &STLLRN_RigHierarchy::OnHierarchyModified_AnyThread);
		}
		TLLRN_ControlRig->OnPreConstructionForUI_AnyThread().RemoveAll(this);
		TLLRN_ControlRig->OnPreConstructionForUI_AnyThread().AddSP(this, &STLLRN_RigHierarchy::OnPreConstruction_AnyThread);
		TLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
		TLLRN_ControlRig->OnPostConstruction_AnyThread().AddSP(this, &STLLRN_RigHierarchy::OnPostConstruction_AnyThread);
		LastHierarchyHash = INDEX_NONE;
	}

	RefreshTreeView();
}

void STLLRN_RigHierarchy::OnPreConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName)
{
	if(InRig != TLLRN_ControlRigBeingDebuggedPtr.Get())
	{
		return;
	}
	bIsConstructionEventRunning = true;
	SelectionBeforeConstruction = InRig->GetHierarchy()->GetSelectedKeys();
}

void STLLRN_RigHierarchy::OnPostConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName)
{
	if(InRig != TLLRN_ControlRigBeingDebuggedPtr.Get())
	{
		return;
	}

	bIsConstructionEventRunning = false;

	const UTLLRN_RigHierarchy* Hierarchy = InRig->GetHierarchy();
	const int32 HierarchyHash = HashCombine(
		Hierarchy->GetTopologyHash(false),
		InRig->ElementKeyRedirector.GetHash());

	if(LastHierarchyHash != HierarchyHash)
	{
		LastHierarchyHash = HierarchyHash;
		
		auto Task = [this]()
		{
			TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);

			RefreshTreeView(true);

			TreeView->ClearSelection();
			if(!SelectionBeforeConstruction.IsEmpty())
			{
				for (int32 RootIndex = 0; RootIndex < TreeView->RootElements.Num(); ++RootIndex)
				{
					for(const FTLLRN_RigElementKey& Key : SelectionBeforeConstruction)
					{
						TSharedPtr<FRigTreeElement> Found = TreeView->FindElement(Key, TreeView->RootElements[RootIndex]);
						if (Found.IsValid())
						{
							TreeView->SetItemSelection(Found, true, ESelectInfo::OnNavigation);
						}
					}
				}
			}
		};
				
		if(IsInGameThread())
		{
			Task();
		}
		else
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady([Task]()
			{
				Task();
			}, TStatId(), NULL, ENamedThreads::GameThread);
		}
	}
}

void STLLRN_RigHierarchy::OnNavigateToFirstConnectorWarning()
{
	if(TLLRN_ControlRigEditor.IsValid())
	{
		if(UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRig())
		{
			FTLLRN_RigElementKey ConnectorKey;
			if(!TLLRN_ControlRig->AllConnectorsAreResolved(nullptr, &ConnectorKey))
			{
				if(ConnectorKey.IsValid())
				{
					if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
					{
						if(UTLLRN_RigHierarchyController* HierarchyController = Hierarchy->GetController())
						{
							{
								const FTLLRN_RigHierarchyRedirectorGuard RedirectorGuard(TLLRN_ControlRig);
								HierarchyController->SetSelection({ConnectorKey}, false);
							}
							HandleFrameSelection();
						}
					}
				}
			}
		}
	}
}

void STLLRN_RigHierarchy::ClearDetailPanel() const
{
	if(TLLRN_ControlRigEditor.IsValid())
	{
		TLLRN_ControlRigEditor.Pin()->ClearDetailObject();
	}
}

TSharedRef< SWidget > STLLRN_RigHierarchy::CreateFilterMenu()
{
	const FTLLRN_ControlTLLRN_RigHierarchyCommands& Actions = FTLLRN_ControlTLLRN_RigHierarchyCommands::Get();

	const bool CloseAfterSelection = true;
	FMenuBuilder MenuBuilder(CloseAfterSelection, CommandList);

	MenuBuilder.BeginSection("FilterOptions", LOCTEXT("OptionsMenuHeading", "Options"));
	{
		MenuBuilder.AddMenuEntry(Actions.FilteringFlattensHierarchy);
		MenuBuilder.AddMenuEntry(Actions.HideParentsWhenFiltering);
		MenuBuilder.AddMenuEntry(Actions.ShowShortNames);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("FilterBones", LOCTEXT("BonesMenuHeading", "Bones"));
	{
		MenuBuilder.AddMenuEntry(Actions.ShowImportedBones);
		MenuBuilder.AddMenuEntry(Actions.ShowBones);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("FilterControls", LOCTEXT("ControlsMenuHeading", "Controls"));
	{
		MenuBuilder.AddMenuEntry(Actions.ShowControls);
		MenuBuilder.AddMenuEntry(Actions.ShowNulls);
		MenuBuilder.AddMenuEntry(Actions.ShowIconColors);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedPtr< SWidget > STLLRN_RigHierarchy::CreateContextMenuWidget()
{
	UToolMenus* ToolMenus = UToolMenus::Get();

	if (UToolMenu* Menu = GetContextMenu())
	{
		return ToolMenus->GenerateWidget(Menu);
	}
	
	return SNullWidget::NullWidget;
}

void STLLRN_RigHierarchy::OnItemClicked(TSharedPtr<FRigTreeElement> InItem)
{
	UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
	check(Hierarchy);

	if (Hierarchy->IsSelected(InItem->Key))
	{
		if (TLLRN_ControlRigEditor.IsValid())
		{
			TLLRN_ControlRigEditor.Pin()->SetDetailViewForTLLRN_RigElements();
		}

		if (InItem->Key.Type == ETLLRN_RigElementType::Bone)
		{
			if(FTLLRN_RigBoneElement* BoneElement = Hierarchy->Find<FTLLRN_RigBoneElement>(InItem->Key))
			{
				if (BoneElement->BoneType == ETLLRN_RigBoneType::Imported)
				{
					return;
				}
			}
		}

		uint32 CurrentCycles = FPlatformTime::Cycles();
		double SecondsPassed = double(CurrentCycles - TreeView->LastClickCycles) * FPlatformTime::GetSecondsPerCycle();
		if (SecondsPassed > 0.5f)
		{
			RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateLambda([this](double, float) {
				HandleRenameItem();
				return EActiveTimerReturnType::Stop;
			}));
		}

		TreeView->LastClickCycles = CurrentCycles;
	}
}

void STLLRN_RigHierarchy::OnItemDoubleClicked(TSharedPtr<FRigTreeElement> InItem)
{
	if (TreeView->IsItemExpanded(InItem))
	{
		TreeView->SetExpansionRecursive(InItem, false, false);
	}
	else
	{
		TreeView->SetExpansionRecursive(InItem, false, true);
	}
}

void STLLRN_RigHierarchy::OnSetExpansionRecursive(TSharedPtr<FRigTreeElement> InItem, bool bShouldBeExpanded)
{
	TreeView->SetExpansionRecursive(InItem, false, bShouldBeExpanded);
}

TOptional<FText> STLLRN_RigHierarchy::OnGetItemTooltip(const FTLLRN_RigElementKey& InKey) const
{
	if(!DragRigResolveResults.IsEmpty() && FSlateApplication::Get().IsDragDropping())
	{
		FString Message;
		for(const TPair<FTLLRN_RigElementKey, FTLLRN_ModularRigResolveResult>& DragRigResolveResult : DragRigResolveResults)
		{
			if(DragRigResolveResult.Key != InKey)
			{
				if(!DragRigResolveResult.Value.ContainsMatch(InKey, &Message))
				{
					if(!Message.IsEmpty())
					{
						return FText::FromString(Message);
					}
				}
			}
		}
	}
	return TOptional<FText>();
}

void STLLRN_RigHierarchy::CreateDragDropMenu()
{
	static bool bCreatedMenu = false;
	if(bCreatedMenu)
	{
		return;
	}
	bCreatedMenu = true;

	const FName MenuName = DragDropMenuName;
	UToolMenus* ToolMenus = UToolMenus::Get();

	if (!ensure(ToolMenus))
	{
		return;
	}

	if (UToolMenu* Menu = ToolMenus->ExtendMenu(MenuName))
	{
		Menu->AddDynamicSection(NAME_None, FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu)
			{
				UToolMenus* ToolMenus = UToolMenus::Get();
				UTLLRN_ControlRigContextMenuContext* MainContext = InMenu->FindContext<UTLLRN_ControlRigContextMenuContext>();
				
				if (STLLRN_RigHierarchy* TLLRN_RigHierarchyPanel = MainContext->GetTLLRN_RigHierarchyPanel())
				{
					FToolMenuEntry ParentEntry = FToolMenuEntry::InitMenuEntry(
                        TEXT("Parent"),
                        LOCTEXT("DragDropMenu_Parent", "Parent"),
                        LOCTEXT("DragDropMenu_Parent_ToolTip", "Parent Selected Items to the Target Item"),
                        FSlateIcon(),
                        FToolMenuExecuteAction::CreateSP(TLLRN_RigHierarchyPanel, &STLLRN_RigHierarchy::HandleParent)
                    );
		
                    ParentEntry.InsertPosition.Position = EToolMenuInsertType::First;
                    InMenu->AddMenuEntry(NAME_None, ParentEntry);
		
                    UToolMenu* AlignMenu = InMenu->AddSubMenu(
                        ToolMenus->CurrentOwner(),
                        NAME_None,
                        TEXT("Align"),
                        LOCTEXT("DragDropMenu_Align", "Align"),
                        LOCTEXT("DragDropMenu_Align_ToolTip", "Align Selected Items' Transforms to Target Item's Transform")
                    );
		
                    if (FToolMenuSection* DefaultSection = InMenu->FindSection(NAME_None))
                    {
                        if (FToolMenuEntry* AlignMenuEntry = DefaultSection->FindEntry(TEXT("Align")))
                        {
                            AlignMenuEntry->InsertPosition.Name = ParentEntry.Name;
                            AlignMenuEntry->InsertPosition.Position = EToolMenuInsertType::After;
                        }
                    }
		
                    FToolMenuEntry AlignAllEntry = FToolMenuEntry::InitMenuEntry(
                        TEXT("All"),
                        LOCTEXT("DragDropMenu_Align_All", "All"),
                        LOCTEXT("DragDropMenu_Align_All_ToolTip", "Align Selected Items' Transforms to Target Item's Transform"),
                        FSlateIcon(),
                        FToolMenuExecuteAction::CreateSP(TLLRN_RigHierarchyPanel, &STLLRN_RigHierarchy::HandleAlign)
                    );
                    AlignAllEntry.InsertPosition.Position = EToolMenuInsertType::First;
		
                    AlignMenu->AddMenuEntry(NAME_None, AlignAllEntry);	
				}
			})
		);
	}
}

UToolMenu* STLLRN_RigHierarchy::GetDragDropMenu(const TArray<FTLLRN_RigElementKey>& DraggedKeys, FTLLRN_RigElementKey TargetKey)
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!ensure(ToolMenus))
	{
		return nullptr;
	}
	
	const FName MenuName = DragDropMenuName;
	UTLLRN_ControlRigContextMenuContext* MenuContext = NewObject<UTLLRN_ControlRigContextMenuContext>();
	FTLLRN_ControlRigMenuSpecificContext MenuSpecificContext;
	MenuSpecificContext.TLLRN_RigHierarchyDragAndDropContext = FTLLRN_ControlRigTLLRN_RigHierarchyDragAndDropContext(DraggedKeys, TargetKey);
	MenuSpecificContext.TLLRN_RigHierarchyPanel = SharedThis(this);
	MenuContext->Init(TLLRN_ControlRigEditor, MenuSpecificContext);
	
	UToolMenu* Menu = ToolMenus->GenerateMenu(MenuName, FToolMenuContext(MenuContext));

	return Menu;
}

void STLLRN_RigHierarchy::CreateContextMenu()
{
	static bool bCreatedMenu = false;
	if(bCreatedMenu)
	{
		return;
	}
	bCreatedMenu = true;
	
	const FName MenuName = ContextMenuName;

	UToolMenus* ToolMenus = UToolMenus::Get();
	
	if (!ensure(ToolMenus))
	{
		return;
	}

	if (UToolMenu* Menu = ToolMenus->ExtendMenu(MenuName))
	{
		Menu->AddDynamicSection(NAME_None, FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu)
			{
				UTLLRN_ControlRigContextMenuContext* MainContext = InMenu->FindContext<UTLLRN_ControlRigContextMenuContext>();
				
				if (STLLRN_RigHierarchy* TLLRN_RigHierarchyPanel = MainContext->GetTLLRN_RigHierarchyPanel())
				{
					const FTLLRN_ControlTLLRN_RigHierarchyCommands& Commands = FTLLRN_ControlTLLRN_RigHierarchyCommands::Get(); 
				
					FToolMenuSection& ElementsSection = InMenu->AddSection(TEXT("Elements"), LOCTEXT("ElementsHeader", "Elements"));
					ElementsSection.AddSubMenu(TEXT("New"), LOCTEXT("New", "New"), LOCTEXT("New_ToolTip", "Create New Elements"),
						FNewToolMenuDelegate::CreateLambda([Commands, TLLRN_RigHierarchyPanel](UToolMenu* InSubMenu)
						{
							FToolMenuSection& DefaultSection = InSubMenu->AddSection(NAME_None);
							FTLLRN_RigElementKey SelectedKey;
							TArray<TSharedPtr<FRigTreeElement>> SelectedItems = TLLRN_RigHierarchyPanel->TreeView->GetSelectedItems();
							if (SelectedItems.Num() > 0)
							{
								SelectedKey = SelectedItems[0]->Key;
							}
							
							if (!SelectedKey || SelectedKey.Type == ETLLRN_RigElementType::Bone || SelectedKey.Type == ETLLRN_RigElementType::Connector)
							{
								DefaultSection.AddMenuEntry(Commands.AddBoneItem);
							}
							DefaultSection.AddMenuEntry(Commands.AddControlItem);
							if(SelectedKey.Type == ETLLRN_RigElementType::Control)
							{
								DefaultSection.AddMenuEntry(Commands.AddAnimationChannelItem);
							}
							DefaultSection.AddMenuEntry(Commands.AddNullItem);

							if(CVarTLLRN_ControlTLLRN_RigHierarchyEnableModules.GetValueOnAnyThread())
							{
								DefaultSection.AddMenuEntry(Commands.AddConnectorItem);
								DefaultSection.AddMenuEntry(Commands.AddSocketItem);
							}
						})
					);
					
					ElementsSection.AddMenuEntry(Commands.DeleteItem);
					ElementsSection.AddMenuEntry(Commands.DuplicateItem);
					ElementsSection.AddMenuEntry(Commands.FindReferencesOfItem);
					ElementsSection.AddMenuEntry(Commands.RenameItem);
					ElementsSection.AddMenuEntry(Commands.MirrorItem);

					if(TLLRN_RigHierarchyPanel->IsProceduralElementSelected() && TLLRN_RigHierarchyPanel->TLLRN_ControlRigBlueprint.IsValid())
					{
						ElementsSection.AddMenuEntry(
							"SelectSpawnerNode",
							LOCTEXT("SelectSpawnerNode", "Select Spawner Node"),
							LOCTEXT("SelectSpawnerNode_Tooltip", "Selects the node that spawn / added this element."),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([TLLRN_RigHierarchyPanel]() {
								if(!TLLRN_RigHierarchyPanel->TLLRN_ControlRigBlueprint.IsValid())
								{
									return;
								}
								const UTLLRN_ControlRigBlueprint* CurrentTLLRN_ControlRigBlueprint = TLLRN_RigHierarchyPanel->TLLRN_ControlRigBlueprint.Get();
								const TArray<const FTLLRN_RigBaseElement*> Elements = TLLRN_RigHierarchyPanel->GetHierarchy()->GetSelectedElements();
								for(const FTLLRN_RigBaseElement* Element : Elements)
								{
									if(Element->IsProcedural())
									{
										const int32 InstructionIndex = Element->GetCreatedAtInstructionIndex();
										if(const UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(CurrentTLLRN_ControlRigBlueprint->GetObjectBeingDebugged()))
										{
											if(TLLRN_ControlRig->VM)
											{
												if(URigVMNode* Node = Cast<URigVMNode>(TLLRN_ControlRig->VM->GetByteCode().GetSubjectForInstruction(InstructionIndex)))
												{
													if(URigVMController* Controller = CurrentTLLRN_ControlRigBlueprint->GetController(Node->GetGraph()))
													{
														Controller->SelectNode(Node);
														(void)Controller->RequestJumpToHyperlinkDelegate.ExecuteIfBound(Node);
													}
												}
											}
										}
									}
								}
							})
						));
					}

					if (TLLRN_RigHierarchyPanel->IsSingleBoneSelected(false) || TLLRN_RigHierarchyPanel->IsControlSelected(false))
					{
						FToolMenuSection& InteractionSection = InMenu->AddSection(TEXT("Interaction"), LOCTEXT("InteractionHeader", "Interaction"));
						if(TLLRN_RigHierarchyPanel->IsSingleBoneSelected(false))
						{
							InteractionSection.AddMenuEntry(Commands.ControlBoneTransform);
						}
						else if(TLLRN_RigHierarchyPanel->IsControlSelected(false))
						{
							InteractionSection.AddMenuEntry(Commands.SpaceSwitching);
						}
					}

					FToolMenuSection& CopyPasteSection = InMenu->AddSection(TEXT("Copy&Paste"), LOCTEXT("Copy&PasteHeader", "Copy & Paste"));
					CopyPasteSection.AddMenuEntry(Commands.CopyItems);
					CopyPasteSection.AddMenuEntry(Commands.PasteItems);
					CopyPasteSection.AddMenuEntry(Commands.PasteLocalTransforms);
					CopyPasteSection.AddMenuEntry(Commands.PasteGlobalTransforms);
					
					FToolMenuSection& TransformsSection = InMenu->AddSection(TEXT("Transforms"), LOCTEXT("TransformsHeader", "Transforms"));
					TransformsSection.AddMenuEntry(Commands.ResetTransform);
					TransformsSection.AddMenuEntry(Commands.ResetAllTransforms);

					{
						static const FString InitialKeyword = TEXT("Initial");
						static const FString OffsetKeyword = TEXT("Offset");
						static const FString InitialOffsetKeyword = TEXT("Initial / Offset");
						
						const FString* Keyword = &InitialKeyword;
						TArray<ETLLRN_RigElementType> SelectedTypes;;
						TArray<TSharedPtr<FRigTreeElement>> SelectedItems = TLLRN_RigHierarchyPanel->TreeView->GetSelectedItems();
						for(const TSharedPtr<FRigTreeElement>& SelectedItem : SelectedItems)
						{
							if(SelectedItem.IsValid())
							{
								SelectedTypes.AddUnique(SelectedItem->Key.Type);
							}
						}
						if(SelectedTypes.Contains(ETLLRN_RigElementType::Control))
						{
							// since it is unique this means it is only controls
							if(SelectedTypes.Num() == 1)
							{
								Keyword = &OffsetKeyword;
							}
							else
							{
								Keyword = &InitialOffsetKeyword;
							}
						}

						const FText FromCurrentLabel = FText::Format(LOCTEXT("SetTransformFromCurrentTransform", "Set {0} Transform from Current"), FText::FromString(*Keyword));
						const FText FromClosestBoneLabel = FText::Format(LOCTEXT("SetTransformFromClosestBone", "Set {0} Transform from Closest Bone"), FText::FromString(*Keyword));
						TransformsSection.AddMenuEntry(Commands.SetInitialTransformFromCurrentTransform, FromCurrentLabel);
						TransformsSection.AddMenuEntry(Commands.SetInitialTransformFromClosestBone, FromClosestBoneLabel);
					}
					
					TransformsSection.AddMenuEntry(Commands.SetShapeTransformFromCurrent);
					TransformsSection.AddMenuEntry(Commands.Unparent);

					FToolMenuSection& AssetsSection = InMenu->AddSection(TEXT("Assets"), LOCTEXT("AssetsHeader", "Assets"));
					AssetsSection.AddSubMenu(TEXT("Import"), LOCTEXT("ImportSubMenu", "Import"),
						LOCTEXT("ImportSubMenu_ToolTip", "Import hierarchy to the current rig. This only imports non-existing node. For example, if there is hand_r, it won't import hand_r. If you want to reimport whole new hiearchy, delete all nodes, and use import hierarchy."),
						FNewMenuDelegate::CreateSP(TLLRN_RigHierarchyPanel, &STLLRN_RigHierarchy::CreateImportMenu)
					);
					
					AssetsSection.AddSubMenu(TEXT("Refresh"), LOCTEXT("RefreshSubMenu", "Refresh"),
						LOCTEXT("RefreshSubMenu_ToolTip", "Refresh the existing initial transform from the selected mesh. This only updates if the node is found."),
						FNewMenuDelegate::CreateSP(TLLRN_RigHierarchyPanel, &STLLRN_RigHierarchy::CreateRefreshMenu)
					);	

					AssetsSection.AddSubMenu(TEXT("ResetCurves"), LOCTEXT("ResetCurvesSubMenu", "Reset Curves"),
						LOCTEXT("ResetCurvesSubMenu_ToolTip", "Reset all curves in this rig asset to the selected mesh, Useful when if you add more morphs to the mesh but control rig does not update."),
						FNewMenuDelegate::CreateSP(TLLRN_RigHierarchyPanel, &STLLRN_RigHierarchy::CreateResetCurvesMenu)
					);				
				}
			})
		);
	}
}

UToolMenu* STLLRN_RigHierarchy::GetContextMenu()
{
	const FName MenuName = ContextMenuName;
	UToolMenus* ToolMenus = UToolMenus::Get();

	if(!ensure(ToolMenus))
	{
		return nullptr;
	}

	// individual entries in this menu can access members of this context, particularly useful for editor scripting
	UTLLRN_ControlRigContextMenuContext* ContextMenuContext = NewObject<UTLLRN_ControlRigContextMenuContext>();
	FTLLRN_ControlRigMenuSpecificContext MenuSpecificContext;
	MenuSpecificContext.TLLRN_RigHierarchyPanel = SharedThis(this);
	ContextMenuContext->Init(TLLRN_ControlRigEditor, MenuSpecificContext);

	FToolMenuContext MenuContext(CommandList);
	MenuContext.AddObject(ContextMenuContext);

	UToolMenu* Menu = ToolMenus->GenerateMenu(MenuName, MenuContext);

	return Menu;
}

TSharedPtr<FUICommandList> STLLRN_RigHierarchy::GetContextMenuCommands() const
{
	return CommandList;
}

void STLLRN_RigHierarchy::CreateRefreshMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddWidget(
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3)
		[
			SNew(STextBlock)
			.Font(FAppStyle::GetFontStyle("TLLRN_ControlRig.Hierarchy.Menu"))
			.Text(LOCTEXT("RefreshMesh_Title", "Select Mesh"))
			.ToolTipText(LOCTEXT("RefreshMesh_Tooltip", "Select Mesh to refresh transform from... It will refresh init transform from selected mesh. This doesn't change hierarchy. If you want to reimport hierarchy, please delete all nodes, and use import hierarchy."))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(USkeletalMesh::StaticClass())
			.OnObjectChanged(this, &STLLRN_RigHierarchy::RefreshHierarchy, false)
		]
		,
		FText()
	);
}

void STLLRN_RigHierarchy::CreateResetCurvesMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddWidget(
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3)
		[
			SNew(STextBlock)
			.Font(FAppStyle::GetFontStyle("TLLRN_ControlRig.Hierarchy.Menu"))
			.Text(LOCTEXT("ResetMesh_Title", "Select Mesh"))
			.ToolTipText(LOCTEXT("ResetMesh_Tooltip", "Select mesh to reset curves to."))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(USkeletalMesh::StaticClass())
			.OnObjectChanged(this, &STLLRN_RigHierarchy::RefreshHierarchy, true)
		]
		,
		FText()
	);
}

void STLLRN_RigHierarchy::UpdateMesh(USkeletalMesh* InMesh, const bool bImport) const
{
	if (!InMesh || !TLLRN_ControlRigBlueprint.IsValid() || !TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	const bool bUpdateMesh = bImport ? TLLRN_ControlRigBlueprint->GetPreviewMesh() == nullptr : true;
	if (!bUpdateMesh)
	{
		return;
	}

	const TSharedPtr<FTLLRN_ControlRigEditor> EditorSharedPtr = TLLRN_ControlRigEditor.Pin();
	EditorSharedPtr->GetPersonaToolkit()->SetPreviewMesh(InMesh, true);

	UTLLRN_ControlRigSkeletalMeshComponent* Component = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(EditorSharedPtr->GetPersonaToolkit()->GetPreviewMeshComponent());
	if (bImport)
	{
		Component->InitAnim(true);
	}

	UAnimInstance* AnimInstance = Component->GetAnimInstance();
	if (UTLLRN_ControlRigLayerInstance* TLLRN_ControlRigLayerInstance = Cast<UTLLRN_ControlRigLayerInstance>(AnimInstance))
	{
		EditorSharedPtr->PreviewInstance = Cast<UAnimPreviewInstance>(TLLRN_ControlRigLayerInstance->GetSourceAnimInstance());
	}
	else
	{
		EditorSharedPtr->PreviewInstance = Cast<UAnimPreviewInstance>(AnimInstance);
	}

	EditorSharedPtr->Compile();
}

void STLLRN_RigHierarchy::RefreshHierarchy(const FAssetData& InAssetData, bool bOnlyResetCurves)
{
	if (bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}
	TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);

	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	FTLLRN_ControlRigEditor* StrongEditor = TLLRN_ControlRigEditor.Pin().Get();
	StrongEditor->ClearDetailObject();

	UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(InAssetData.GetAsset());
	if (Mesh && Hierarchy)
	{
		TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);

		FScopedTransaction Transaction(LOCTEXT("HierarchyRefresh", "Refresh Transform"));

		// don't select bone if we are in construction mode.
		// we do this to avoid the editmode / viewport shapes to refresh recursively,
		// which can add an extreme slowdown depending on the number of bones (n^(n-1))
		bool bSelectBones = true;
		if (UTLLRN_ControlRig* CurrentRig = StrongEditor->GetTLLRN_ControlRig())
		{
			bSelectBones = !CurrentRig->IsConstructionModeEnabled();
		}

		const FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();

		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);

		if(bOnlyResetCurves)
		{
			TArray<FTLLRN_RigElementKey> CurveKeys = Hierarchy->GetAllKeys(false, ETLLRN_RigElementType::Curve);
			for(const FTLLRN_RigElementKey& CurveKey : CurveKeys)
			{
				Controller->RemoveElement(CurveKey, true, true);
			}			
			Controller->ImportCurvesFromSkeletalMesh(Mesh, NAME_None, false, true, true);
		}
		else
		{
			Controller->ImportBones(Mesh->GetSkeleton(), NAME_None, true, true, bSelectBones, true, true);
			Controller->ImportCurvesFromSkeletalMesh(Mesh, NAME_None, false, true, true);
		}
	}

	TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
	StrongEditor->OnHierarchyChanged();
	TLLRN_ControlRigBlueprint->BroadcastRefreshEditor();
	RefreshTreeView();
	FSlateApplication::Get().DismissAllMenus();

	static constexpr bool bImport = false;
	UpdateMesh(Mesh, bImport);
}

void STLLRN_RigHierarchy::CreateImportMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddWidget(
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3)
		[
			SNew(STextBlock)
			.Font(FAppStyle::GetFontStyle("TLLRN_ControlRig.Hierarchy.Menu"))
			.Text(LOCTEXT("ImportMesh_Title", "Select Mesh"))
			.ToolTipText(LOCTEXT("ImportMesh_Tooltip", "Select Mesh to import hierarchy from... It will only import if the node doesn't exist in the current hierarchy."))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(USkeletalMesh::StaticClass())
			.OnObjectChanged(this, &STLLRN_RigHierarchy::ImportHierarchy)
		]
		,
		FText()
	);
}

FTLLRN_RigElementKey STLLRN_RigHierarchy::OnGetResolvedKey(const FTLLRN_RigElementKey& InKey)
{
	if (const UTLLRN_ControlRigBlueprint* Blueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint())
	{
		const FTLLRN_RigElementKey ResolvedKey = Blueprint->TLLRN_ModularRigModel.Connections.FindTargetFromConnector(InKey);
		if(ResolvedKey.IsValid())
		{
			return ResolvedKey;
		}
	}
	return InKey;
}

void STLLRN_RigHierarchy::OnRequestDetailsInspection(const FTLLRN_RigElementKey& InKey)
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}
	TLLRN_ControlRigEditor.Pin()->SetDetailViewForTLLRN_RigElements({InKey});
}

void STLLRN_RigHierarchy::ImportHierarchy(const FAssetData& InAssetData)
{
	if (bIsChangingTLLRN_RigHierarchy)
	{
		return;
	}

	USkeletalMesh* Mesh = Cast<USkeletalMesh>(InAssetData.GetAsset());
	if (!Mesh)
	{
		return;
	}
	
	TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);

	if (!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}
	
	const TSharedPtr<FTLLRN_ControlRigEditor> EditorSharedPtr = TLLRN_ControlRigEditor.Pin();
	if (UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy())
	{
		// filter out meshes that don't contain a skeleton
		if(Mesh->GetSkeleton() == nullptr)
		{
			FNotificationInfo Info(LOCTEXT("SkeletalMeshHasNoSkeleton", "Chosen Skeletal Mesh has no assigned skeleton. This needs to fixed before the mesh can be used for a Control Rig."));
			Info.bUseSuccessFailIcons = true;
			Info.Image = FAppStyle::GetBrush(TEXT("MessageLog.Warning"));
			Info.bFireAndForget = true;
			Info.bUseThrobber = true;
			Info.FadeOutDuration = 2.f;
			Info.ExpireDuration = 8.f;;
			TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
			if (NotificationPtr)
			{
				NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);
			}
			return;
		}
		
		EditorSharedPtr->ClearDetailObject();
		
		TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);

		FScopedTransaction Transaction(LOCTEXT("HierarchyImport", "Import Hierarchy"));

		// don't select bone if we are in construction mode.
		// we do this to avoid the editmode / viewport shapes to refresh recursively,
		// which can add an extreme slowdown depending on the number of bones (n^(n-1))
		bool bSelectBones = true;
		bool bIsTLLRN_ModularRig = false;
		if (const UTLLRN_ControlRig* CurrentRig = EditorSharedPtr->GetTLLRN_ControlRig())
		{
			bSelectBones = !CurrentRig->IsConstructionModeEnabled();
			bIsTLLRN_ModularRig = CurrentRig->IsTLLRN_ModularRig();
		}

		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);

		const TArray<FTLLRN_RigElementKey> ImportedBones = Controller->ImportBones(Mesh->GetSkeleton(), NAME_None, false, false, bSelectBones, true, true);
		Controller->ImportCurvesFromSkeletalMesh(Mesh, NAME_None, false, true);

		TLLRN_ControlRigBlueprint->SourceHierarchyImport = Mesh->GetSkeleton();
		TLLRN_ControlRigBlueprint->SourceCurveImport = Mesh->GetSkeleton();

		if (ImportedBones.Num() > 0)
		{
			EditorSharedPtr->GetEditMode()->FrameItems(ImportedBones);
		}
	}

	TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
	EditorSharedPtr->OnHierarchyChanged();
	TLLRN_ControlRigBlueprint->BroadcastRefreshEditor();
	RefreshTreeView();
	FSlateApplication::Get().DismissAllMenus();

	static constexpr bool bImport = true;
	UpdateMesh(Mesh, bImport);
}

bool STLLRN_RigHierarchy::IsMultiSelected(bool bIncludeProcedural) const
{
	if(GetSelectedKeys().Num() > 0)
	{
		if(!bIncludeProcedural && IsProceduralElementSelected())
		{
			return false;
		}
		return true;
	}
	return false;
}

bool STLLRN_RigHierarchy::IsSingleSelected(bool bIncludeProcedural) const
{
	if(GetSelectedKeys().Num() == 1)
	{
		if(!bIncludeProcedural && IsProceduralElementSelected())
		{
			return false;
		}
		return true;
	}
	return false;
}

bool STLLRN_RigHierarchy::IsSingleBoneSelected(bool bIncludeProcedural) const
{
	if(!IsSingleSelected(bIncludeProcedural))
	{
		return false;
	}
	return GetSelectedKeys()[0].Type == ETLLRN_RigElementType::Bone;
}

bool STLLRN_RigHierarchy::IsSingleNullSelected(bool bIncludeProcedural) const
{
	if(!IsSingleSelected(bIncludeProcedural))
	{
		return false;
	}
	return GetSelectedKeys()[0].Type == ETLLRN_RigElementType::Null;
}

bool STLLRN_RigHierarchy::IsControlSelected(bool bIncludeProcedural) const
{
	if(!bIncludeProcedural && IsProceduralElementSelected())
	{
		return false;
	}
	
	TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
	for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
	{
		if (SelectedKey.Type == ETLLRN_RigElementType::Control)
		{
			return true;
		}
	}
	return false;
}

bool STLLRN_RigHierarchy::IsControlOrNullSelected(bool bIncludeProcedural) const
{
	if(!bIncludeProcedural && IsProceduralElementSelected())
	{
		return false;
	}

	TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
	for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
	{
		if (SelectedKey.Type == ETLLRN_RigElementType::Control)
		{
			return true;
		}
		if (SelectedKey.Type == ETLLRN_RigElementType::Null)
		{
			return true;
		}
	}
	return false;
}

bool STLLRN_RigHierarchy::IsProceduralElementSelected() const
{
	TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
	if (SelectedKeys.IsEmpty())
	{
		return false;
	}
	
	for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
	{
		if(!GetHierarchy()->IsProcedural(SelectedKey))
		{
			return false;
		}
	}
	return true;
}

bool STLLRN_RigHierarchy::IsNonProceduralElementSelected() const
{
	TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
	if (SelectedKeys.IsEmpty())
	{
		return false;
	}
	
	for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
	{
		if(GetHierarchy()->IsProcedural(SelectedKey))
		{
			return false;
		}
	}
	return true;
}

bool STLLRN_RigHierarchy::CanAddElement(const ETLLRN_RigElementType ElementType) const
{
	if (ElementType == ETLLRN_RigElementType::Connector)
	{
		return TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule();
	}
	if (ElementType == ETLLRN_RigElementType::Socket)
	{
		return TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule() ||
			TLLRN_ControlRigBlueprint->IsTLLRN_ModularRig();
	}
	return !TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule();
}

bool STLLRN_RigHierarchy::CanAddAnimationChannel() const
{
	if (!IsControlSelected(false))
	{
		return false;
	}

	return !TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule();
}

void STLLRN_RigHierarchy::HandleDeleteItem()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	if(!CanDeleteItem())
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();
 	if (Hierarchy)
 	{
		TArray<FTLLRN_RigElementKey> RemovedItems;

		ClearDetailPanel();
		FScopedTransaction Transaction(LOCTEXT("HierarchyTreeDeleteSelected", "Delete selected items from hierarchy"));

		// clear detail view display
		TLLRN_ControlRigEditor.Pin()->ClearDetailObject();

		bool bConfirmedByUser = false;
		bool bDeleteImportedBones = false;

 		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
 		check(Controller);

 		TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();

 		if (TLLRN_ControlRigBlueprint.IsValid() && TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule())
 		{
 			SelectedKeys.RemoveAll([Hierarchy, Controller](const FTLLRN_RigElementKey& Selected)
			{
				if (const FTLLRN_RigBaseElement* Element = Hierarchy->Find(Selected))
				{
				   if (const FTLLRN_RigConnectorElement* Connector = Cast<FTLLRN_RigConnectorElement>(Element))
				   {
					   if (Connector->IsPrimary())
					   {
						   static constexpr TCHAR Format[] = TEXT("Cannot delete primary connector: %s");
						   Controller->ReportAndNotifyErrorf(Format, *Connector->GetName());
						   return true;
					   }
				   }
			   }
				return false;
			});
 		}

 		// clear selection early here to make sure TLLRN_ControlRigEditMode can react to this deletion
 		// it cannot react to it during Controller->RemoveElement() later because bSuspendAllNotifications is true
 		Controller->ClearSelection();

 		FTLLRN_RigHierarchyInteractionBracket InteractionBracket(Hierarchy);
 		
		for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
		{
			TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
			TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);

			if (SelectedKey.Type == ETLLRN_RigElementType::Bone)
			{
				if (FTLLRN_RigBoneElement* BoneElement = Hierarchy->Find<FTLLRN_RigBoneElement>(SelectedKey))
				{
					if (BoneElement->BoneType == ETLLRN_RigBoneType::Imported && BoneElement->ParentElement != nullptr)
					{
						if (!bConfirmedByUser)
						{
							FText ConfirmDelete = LOCTEXT("ConfirmDeleteBoneHierarchy", "Deleting imported(white) bones can cause issues with animation - are you sure ?");

							FSuppressableWarningDialog::FSetupInfo Info(ConfirmDelete, LOCTEXT("DeleteImportedBone", "Delete Imported Bone"), "DeleteImportedBoneHierarchy_Warning");
							Info.ConfirmText = LOCTEXT("DeleteImportedBoneHierarchy_Yes", "Yes");
							Info.CancelText = LOCTEXT("DeleteImportedBoneHierarchy_No", "No");

							FSuppressableWarningDialog DeleteImportedBonesInHierarchy(Info);
							bDeleteImportedBones = DeleteImportedBonesInHierarchy.ShowModal() != FSuppressableWarningDialog::Cancel;
							bConfirmedByUser = true;
						}

						if (!bDeleteImportedBones)
						{
							break;
						}
					}
				}
			}

			Controller->RemoveElement(SelectedKey, true, true);
			RemovedItems.Add(SelectedKey);
		}
	}

	TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
	TLLRN_ControlRigEditor.Pin()->OnHierarchyChanged();
	RefreshTreeView();
	FSlateApplication::Get().DismissAllMenus();
}

bool STLLRN_RigHierarchy::CanDeleteItem() const
{
	return IsMultiSelected(false);
}

/** Create Item */
void STLLRN_RigHierarchy::HandleNewItem(ETLLRN_RigElementType InElementType, bool bIsAnimationChannel)
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	FTLLRN_RigElementKey NewItemKey;
	UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();
	UTLLRN_RigHierarchy* DebugHierarchy = GetHierarchy();
	if (Hierarchy)
	{
		// unselect current selected item
		ClearDetailPanel();

		const bool bAllowMultipleItems =
			InElementType == ETLLRN_RigElementType::Socket ||
			InElementType == ETLLRN_RigElementType::Null ||
			(InElementType == ETLLRN_RigElementType::Control && !bIsAnimationChannel);

		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);

		FScopedTransaction Transaction(LOCTEXT("HierarchyTreeAdded", "Add new item to hierarchy"));

		TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
		if (SelectedKeys.Num() > 1 && !bAllowMultipleItems)
		{
			SelectedKeys = {SelectedKeys[0]};
		}
		else if(SelectedKeys.IsEmpty())
		{
			SelectedKeys = {FTLLRN_RigElementKey()};
		}

		TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> SelectedToCreated;
		for(const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
		{
			FTLLRN_RigElementKey ParentKey;
			FTransform ParentTransform = FTransform::Identity;

			if(SelectedKey.IsValid())
			{
				ParentKey = SelectedKey;
				// Use the transform of the debugged hierarchy rather than the default hierarchy
				ParentTransform = DebugHierarchy->GetGlobalTransform(ParentKey);
			}

			// use bone's name as prefix if creating a control
			FString NewNameTemplate;
			if(ParentKey.IsValid() && ParentKey.Type == ETLLRN_RigElementType::Bone)
			{
				NewNameTemplate = ParentKey.Name.ToString();

				if(InElementType == ETLLRN_RigElementType::Control)
				{
					static const FString CtrlSuffix(TEXT("_ctrl"));
					NewNameTemplate += CtrlSuffix;
				}
				else if(InElementType == ETLLRN_RigElementType::Null)
				{
					static const FString NullSuffix(TEXT("_null"));
					NewNameTemplate += NullSuffix;
				}
				else if(InElementType == ETLLRN_RigElementType::Socket)
				{
					static const FString SocketSuffix(TEXT("_socket"));
					NewNameTemplate += SocketSuffix;
				}
				else
				{
					NewNameTemplate.Reset();
				}
			}

			if(NewNameTemplate.IsEmpty())
			{
				NewNameTemplate = FString::Printf(TEXT("New%s"), *StaticEnum<ETLLRN_RigElementType>()->GetNameStringByValue((int64)InElementType));

				if(bIsAnimationChannel)
				{
					static const FString NewAnimationChannel = TEXT("Channel");
					NewNameTemplate = NewAnimationChannel;
				}
			}
			
			const FName NewElementName = CreateUniqueName(*NewNameTemplate, InElementType);
			{
				TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
				switch (InElementType)
				{
					case ETLLRN_RigElementType::Bone:
					{
						NewItemKey = Controller->AddBone(NewElementName, ParentKey, ParentTransform, true, ETLLRN_RigBoneType::User, true, true);
						break;
					}
					case ETLLRN_RigElementType::Control:
					{
						FTLLRN_RigControlSettings Settings;

						if(bIsAnimationChannel)
						{
							Settings.AnimationType = ETLLRN_RigControlAnimationType::AnimationChannel;
							Settings.ControlType = ETLLRN_RigControlType::Float;
							Settings.MinimumValue = FTLLRN_RigControlValue::Make<float>(0.f);
							Settings.MaximumValue = FTLLRN_RigControlValue::Make<float>(1.f);
							Settings.DisplayName = Hierarchy->GetSafeNewDisplayName(ParentKey, FTLLRN_RigName(NewNameTemplate));

							NewItemKey = Controller->AddAnimationChannel(NewElementName, ParentKey, Settings, true, true);
						}
						else
						{
							Settings.ControlType = ETLLRN_RigControlType::EulerTransform;
							FEulerTransform Identity = FEulerTransform::Identity;
							FTLLRN_RigControlValue ValueToSet = FTLLRN_RigControlValue::Make<FEulerTransform>(Identity);
							Settings.MinimumValue = ValueToSet;
							Settings.MaximumValue = ValueToSet;

							FTLLRN_RigElementKey NewParentKey;
							FTransform OffsetTransform = ParentTransform;
							if (FTLLRN_RigElementKey* CreatedParentKey = SelectedToCreated.Find(Hierarchy->GetDefaultParent(ParentKey)))
							{
								NewParentKey = *CreatedParentKey;
								OffsetTransform = ParentTransform.GetRelativeTransform(Hierarchy->GetGlobalTransform(NewParentKey, true));
							}

							NewItemKey = Controller->AddControl(NewElementName, NewParentKey, Settings, Settings.GetIdentityValue(), OffsetTransform, FTransform::Identity, true, true);

							SelectedToCreated.Add(SelectedKey, NewItemKey);
						}
						break;
					}
					case ETLLRN_RigElementType::Null:
					{
						NewItemKey = Controller->AddNull(NewElementName, ParentKey, ParentTransform, true, true, true);
						break;
					}
					case ETLLRN_RigElementType::Connector:
					{
						FString FailureReason;
						if(!TLLRN_ControlRigBlueprint->CanTurnIntoTLLRN_ControlTLLRN_RigModule(false, &FailureReason))
						{
							if(TLLRN_ControlRigBlueprint->Hierarchy->Num(ETLLRN_RigElementType::Connector) == 0)
							{
								if(!TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule())
								{
									static constexpr TCHAR Format[] = TEXT("Connector cannot be created: %s");
									UE_LOG(LogTLLRN_ControlRig, Warning, Format, *FailureReason);
									FNotificationInfo Info(FText::FromString(FString::Printf(Format, *FailureReason)));
									Info.bUseSuccessFailIcons = true;
									Info.Image = FAppStyle::GetBrush(TEXT("MessageLog.Warning"));
									Info.bFireAndForget = true;
									Info.bUseThrobber = true;
									Info.FadeOutDuration = 2.f;
									Info.ExpireDuration = 8.f;;
									TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
									if (NotificationPtr)
									{
										NotificationPtr->SetCompletionState(SNotificationItem::CS_Fail);
									}
									return;
								}
							}
						}

						const TArray<FTLLRN_RigConnectorElement*> Connectors = Hierarchy->GetConnectors(false);
						const bool bIsPrimary = !Connectors.ContainsByPredicate([](const FTLLRN_RigConnectorElement* Connector) { return Connector->IsPrimary(); });
						
						FTLLRN_RigConnectorSettings Settings;
						Settings.Type = bIsPrimary ? ETLLRN_ConnectorType::Primary : ETLLRN_ConnectorType::Secondary;
						if(!bIsPrimary)
						{
							Settings.Rules.Reset();
							Settings.AddRule(FTLLRN_RigChildOfPrimaryConnectionRule());
							Settings.bOptional = true;
						}
						NewItemKey = Controller->AddConnector(NewElementName, Settings, true);
						(void)ResolveConnector(NewItemKey, ParentKey);
						break;
					}
					case ETLLRN_RigElementType::Socket:
					{
						NewItemKey = Controller->AddSocket(NewElementName, ParentKey, ParentTransform, true, FTLLRN_RigSocketElement::SocketDefaultColor, FString(), true, true);
						break;
					}
					default:
					{
						return;
					}
				}
			}
		}
	}

	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		TLLRN_ControlRigBlueprint->BroadcastRefreshEditor();
	}
	
	if (Hierarchy && NewItemKey.IsValid())
	{
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);

		Controller->ClearSelection();
		Controller->SelectElement(NewItemKey);
	}

	FSlateApplication::Get().DismissAllMenus();
	RefreshTreeView();
}

bool STLLRN_RigHierarchy::CanFindReferencesOfItem() const
{
	return !GetSelectedKeys().IsEmpty();
}

void STLLRN_RigHierarchy::HandleFindReferencesOfItem()
{
	if(!TLLRN_ControlRigEditor.IsValid() || GetSelectedKeys().IsEmpty())
	{
		return;
	}

	TLLRN_ControlRigEditor.Pin()->FindReferencesOfItem(GetSelectedKeys()[0]);
}

/** Check whether we can deleting the selected item(s) */
bool STLLRN_RigHierarchy::CanDuplicateItem() const
{
	if (!IsMultiSelected(false))
	{
		return false;
	}

	if (TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule())
	{
		bool bAnyNonConnector = GetSelectedKeys().ContainsByPredicate([](const FTLLRN_RigElementKey& Key)
		{
			return Key.Type != ETLLRN_RigElementType::Connector;
		});
		
		return !bAnyNonConnector;
	}

	return true;
}

/** Duplicate Item */
void STLLRN_RigHierarchy::HandleDuplicateItem()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();
	if (Hierarchy)
	{
		ClearDetailPanel();
		{
			TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
			TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);

			FScopedTransaction Transaction(LOCTEXT("HierarchyTreeDuplicateSelected", "Duplicate selected items from hierarchy"));

			UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
			check(Controller);

			const TArray<FTLLRN_RigElementKey> KeysToDuplicate = GetSelectedKeys();
			Controller->DuplicateElements(KeysToDuplicate, true, true, true);
		}

		TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
	}

	FSlateApplication::Get().DismissAllMenus();
	TLLRN_ControlRigEditor.Pin()->OnHierarchyChanged();
	{
		TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
		TLLRN_ControlRigBlueprint->BroadcastRefreshEditor();
	}
	RefreshTreeView();
}

/** Mirror Item */
void STLLRN_RigHierarchy::HandleMirrorItem()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}
	
	UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();
	if (Hierarchy)
	{
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);
		
		FRigVMMirrorSettings Settings;
		TSharedPtr<FStructOnScope> StructToDisplay = MakeShareable(new FStructOnScope(FRigVMMirrorSettings::StaticStruct(), (uint8*)&Settings));

		TSharedRef<SKismetInspector> KismetInspector = SNew(SKismetInspector);
		KismetInspector->ShowSingleStruct(StructToDisplay);

		TSharedRef<SCustomDialog> MirrorDialog = SNew(SCustomDialog)
			.Title(FText(LOCTEXT("TLLRN_ControlTLLRN_RigHierarchyMirror", "Mirror Selected Rig Elements")))
			.Content()
			[
				KismetInspector
			]
			.Buttons({
				SCustomDialog::FButton(LOCTEXT("OK", "OK")),
				SCustomDialog::FButton(LOCTEXT("Cancel", "Cancel"))
		});

		if (MirrorDialog->ShowModal() == 0)
		{
			ClearDetailPanel();
			{
				TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
				TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);

				FScopedTransaction Transaction(LOCTEXT("HierarchyTreeMirrorSelected", "Mirror selected items from hierarchy"));

				const TArray<FTLLRN_RigElementKey> KeysToMirror = GetSelectedKeys();
				const TArray<FTLLRN_RigElementKey> KeysToDuplicate = GetSelectedKeys();
				Controller->MirrorElements(KeysToDuplicate, Settings, true, true, true);
			}
			TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
		}
	}

	FSlateApplication::Get().DismissAllMenus();
	TLLRN_ControlRigEditor.Pin()->OnHierarchyChanged();
	RefreshTreeView();
}

/** Check whether we can deleting the selected item(s) */
bool STLLRN_RigHierarchy::CanRenameItem() const
{
	if(IsSingleSelected(false))
	{
		const FTLLRN_RigElementKey Key = GetSelectedKeys()[0];
		if(Key.Type == ETLLRN_RigElementType::Physics ||
			Key.Type == ETLLRN_RigElementType::Reference)
		{
			return false;
		}
		if(Key.Type == ETLLRN_RigElementType::Control)
		{
			if(UTLLRN_RigHierarchy* DebuggedHierarchy = GetHierarchy())
			{
				if(FTLLRN_RigControlElement* ControlElement = DebuggedHierarchy->Find<FTLLRN_RigControlElement>(Key))
				{
					if(ControlElement->Settings.bIsTransientControl)
					{
						return false;
					}
				}
			}
		}
		return true;
	}
	return false;
}

/** Delete Item */
void STLLRN_RigHierarchy::HandleRenameItem()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	if (!CanRenameItem())
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();
	if (Hierarchy)
	{
		FScopedTransaction Transaction(LOCTEXT("HierarchyTreeRenameSelected", "Rename selected item from hierarchy"));

		TArray<TSharedPtr<FRigTreeElement>> SelectedItems = TreeView->GetSelectedItems();
		if (SelectedItems.Num() == 1)
		{
			if (SelectedItems[0]->Key.Type == ETLLRN_RigElementType::Bone)
			{
				if(FTLLRN_RigBoneElement* BoneElement = Hierarchy->Find<FTLLRN_RigBoneElement>(SelectedItems[0]->Key))
				{
					if (BoneElement->BoneType == ETLLRN_RigBoneType::Imported)
					{
						FText ConfirmRename = LOCTEXT("RenameDeleteBoneHierarchy", "Renaming imported(white) bones can cause issues with animation - are you sure ?");

						FSuppressableWarningDialog::FSetupInfo Info(ConfirmRename, LOCTEXT("RenameImportedBone", "Rename Imported Bone"), "RenameImportedBoneHierarchy_Warning");
						Info.ConfirmText = LOCTEXT("RenameImportedBoneHierarchy_Yes", "Yes");
						Info.CancelText = LOCTEXT("RenameImportedBoneHierarchy_No", "No");

						FSuppressableWarningDialog RenameImportedBonesInHierarchy(Info);
						if (RenameImportedBonesInHierarchy.ShowModal() == FSuppressableWarningDialog::Cancel)
						{
							return;
						}
					}
				}
			}
			SelectedItems[0]->RequestRename();
		}
	}
}

bool STLLRN_RigHierarchy::CanPasteItems() const
{
	return true;
}	

bool STLLRN_RigHierarchy::CanCopyOrPasteItems() const
{
	return TreeView->GetNumItemsSelected() > 0;
}

void STLLRN_RigHierarchy::HandleCopyItems()
{
	if (UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);

		const TArray<FTLLRN_RigElementKey> Selection = GetHierarchy()->GetSelectedKeys();
		const FString Content = Controller->ExportToText(Selection);
		FPlatformApplicationMisc::ClipboardCopy(*Content);
	}
}

void STLLRN_RigHierarchy::HandlePasteItems()
{
	if (UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy())
	{
		TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
		TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);

		FString Content;
		FPlatformApplicationMisc::ClipboardPaste(Content);

		FScopedTransaction Transaction(LOCTEXT("HierarchyTreePastedTLLRN_RigElements", "Pasted rig elements."));

		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);

		ETLLRN_RigElementType AllowedTypes = TLLRN_ControlRigBlueprint->IsTLLRN_ControlTLLRN_RigModule() ? ETLLRN_RigElementType::Connector : ETLLRN_RigElementType::All;
		Controller->ImportFromText(Content, AllowedTypes, false, true, true, true);
	}

	//TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
	TLLRN_ControlRigEditor.Pin()->OnHierarchyChanged();
	{
		TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
		TLLRN_ControlRigBlueprint->BroadcastRefreshEditor();
	}
	RefreshTreeView();
}

class STLLRN_RigHierarchyPasteTransformsErrorPipe : public FOutputDevice
{
public:

	int32 NumErrors;

	STLLRN_RigHierarchyPasteTransformsErrorPipe()
		: FOutputDevice()
		, NumErrors(0)
	{
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Error importing transforms to Hierarchy: %s"), V);
		NumErrors++;
	}
};

void STLLRN_RigHierarchy::HandlePasteLocalTransforms()
{
	HandlePasteTransforms(ETLLRN_RigTransformType::CurrentLocal, true);
}

void STLLRN_RigHierarchy::HandlePasteGlobalTransforms()
{
	HandlePasteTransforms(ETLLRN_RigTransformType::CurrentGlobal, false);
}

void STLLRN_RigHierarchy::HandlePasteTransforms(ETLLRN_RigTransformType::Type InTransformType, bool bAffectChildren)
{
	if (UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy())
	{
		FString Content;
		FPlatformApplicationMisc::ClipboardPaste(Content);

		FScopedTransaction Transaction(LOCTEXT("HierarchyTreePastedTransform", "Pasted transforms."));

		STLLRN_RigHierarchyPasteTransformsErrorPipe ErrorPipe;
		FTLLRN_RigHierarchyCopyPasteContent Data;
		FTLLRN_RigHierarchyCopyPasteContent::StaticStruct()->ImportText(*Content, &Data, nullptr, EPropertyPortFlags::PPF_None, &ErrorPipe, FTLLRN_RigHierarchyCopyPasteContent::StaticStruct()->GetName(), true);
		if(ErrorPipe.NumErrors > 0)
		{
			return;
		}
		
		UTLLRN_RigHierarchy* DebuggedHierarchy = GetHierarchy();

		const TArray<FTLLRN_RigElementKey> CurrentSelection = Hierarchy->GetSelectedKeys();
		const int32 Count = FMath::Min<int32>(CurrentSelection.Num(), Data.Elements.Num());
		for(int32 Index = 0; Index < Count; Index++)
		{
			const FTLLRN_RigHierarchyCopyPasteContentPerElement& PerElementData = Data.Elements[Index];
			const FTransform Transform =  PerElementData.Poses[(int32)InTransformType];

			if(FTLLRN_RigTransformElement* TransformElement = Hierarchy->Find<FTLLRN_RigTransformElement>(CurrentSelection[Index]))
			{
				Hierarchy->SetTransform(TransformElement, Transform, InTransformType, bAffectChildren, true, false, true);
			}
			if(FTLLRN_RigBoneElement* BoneElement = Hierarchy->Find<FTLLRN_RigBoneElement>(CurrentSelection[Index]))
			{
				Hierarchy->SetTransform(BoneElement, Transform, ETLLRN_RigTransformType::MakeInitial(InTransformType), bAffectChildren, true, false, true);
			}
			
            if(DebuggedHierarchy && DebuggedHierarchy != Hierarchy)
            {
            	if(FTLLRN_RigTransformElement* TransformElement = DebuggedHierarchy->Find<FTLLRN_RigTransformElement>(CurrentSelection[Index]))
            	{
            		DebuggedHierarchy->SetTransform(TransformElement, Transform, InTransformType, bAffectChildren, true);
            	}
            	if(FTLLRN_RigBoneElement* BoneElement = DebuggedHierarchy->Find<FTLLRN_RigBoneElement>(CurrentSelection[Index]))
            	{
            		DebuggedHierarchy->SetTransform(BoneElement, Transform, ETLLRN_RigTransformType::MakeInitial(InTransformType), bAffectChildren, true);
            	}
            }
		}
	}
}

UTLLRN_RigHierarchy* STLLRN_RigHierarchy::GetHierarchy() const
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		if (UTLLRN_ControlRig* DebuggedRig = TLLRN_ControlRigBeingDebuggedPtr.Get())
		{
			return DebuggedRig->GetHierarchy();
		}
	}
	if (TLLRN_ControlRigEditor.IsValid())
	{
		if (UTLLRN_ControlRig* CurrentRig = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRig())
		{
			return CurrentRig->GetHierarchy();
		}
	}
	return GetDefaultHierarchy();
}

UTLLRN_RigHierarchy* STLLRN_RigHierarchy::GetDefaultHierarchy() const
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		return TLLRN_ControlRigBlueprint->Hierarchy;
	}
	return nullptr;
}


FName STLLRN_RigHierarchy::CreateUniqueName(const FName& InBaseName, ETLLRN_RigElementType InElementType) const
{
	return GetHierarchy()->GetSafeNewName(InBaseName, InElementType);
}

void STLLRN_RigHierarchy::PostRedo(bool bSuccess) 
{
	if (bSuccess)
	{
		RefreshTreeView();
	}
}

void STLLRN_RigHierarchy::PostUndo(bool bSuccess) 
{
	if (bSuccess)
	{
		RefreshTreeView();
	}
}

FReply STLLRN_RigHierarchy::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TArray<FTLLRN_RigElementKey> DraggedElements = GetSelectedKeys();
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && DraggedElements.Num() > 0)
	{
		if (TLLRN_ControlRigEditor.IsValid())
		{
			UpdateConnectorMatchesOnDrag(DraggedElements);
			
			TSharedRef<FTLLRN_RigElementHierarchyDragDropOp> DragDropOp = FTLLRN_RigElementHierarchyDragDropOp::New(MoveTemp(DraggedElements));
			DragDropOp->OnPerformDropToGraph.BindSP(TLLRN_ControlRigEditor.Pin().Get(), &FTLLRN_ControlRigEditor::OnGraphNodeDropToPerform);
			return FReply::Handled().BeginDragDrop(DragDropOp);
		}
	}

	return FReply::Unhandled();
}

TOptional<EItemDropZone> STLLRN_RigHierarchy::OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FRigTreeElement> TargetItem)
{
	const TOptional<EItemDropZone> InvalidDropZone;
	TOptional<EItemDropZone> ReturnDropZone;

	const TSharedPtr<FTLLRN_RigElementHierarchyDragDropOp> RigDragDropOp = DragDropEvent.GetOperationAs<FTLLRN_RigElementHierarchyDragDropOp>();
	if (RigDragDropOp.IsValid())
	{
		FTLLRN_RigElementKey TargetKey;
		if (const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
		{
			switch(DropZone)
			{
				case EItemDropZone::AboveItem:
				case EItemDropZone::BelowItem:
				{
					TargetKey = Hierarchy->GetFirstParent(TargetItem->Key);
					break;
				}
				case EItemDropZone::OntoItem:
				{
					TargetKey = TargetItem->Key;
					break;
				}
			}

			for (const FTLLRN_RigElementKey& DraggedKey : RigDragDropOp->GetElements())
			{
				if(Hierarchy->IsProcedural(DraggedKey) && !RigDragDropOp->IsDraggingSingleConnector())
				{
					return InvalidDropZone;
				}

				// re-parenting directly onto an item
				if (DraggedKey == TargetKey)
				{
					return InvalidDropZone;
				}

				if(RigDragDropOp->IsDraggingSingleConnector() || RigDragDropOp->IsDraggingSingleSocket())
				{
					if(const FTLLRN_ModularRigResolveResult* ResolveResult = DragRigResolveResults.Find(DraggedKey))
					{
						if(!ResolveResult->ContainsMatch(TargetKey))
						{
							return InvalidDropZone;
						}
					}
					if(DropZone != EItemDropZone::OntoItem)
					{
						return InvalidDropZone;
					}
				}
				else if(DropZone == EItemDropZone::OntoItem)
				{
					if(Hierarchy->IsParentedTo(TargetKey, DraggedKey))
					{
						return InvalidDropZone;
					}
				}
			}
		}

		// don't allow dragging onto procedural items (except for connectors + sockets)
		if(TargetKey.IsValid() && !GetDefaultHierarchy()->Contains(TargetKey) &&
			!(RigDragDropOp->IsDraggingSingleConnector() || RigDragDropOp->IsDraggingSingleSocket()))
		{
			return InvalidDropZone;
		}

		switch (TargetKey.Type)
		{
			case ETLLRN_RigElementType::Bone:
			{
				// bones can parent anything
				ReturnDropZone = DropZone;
				break;
			}
			case ETLLRN_RigElementType::Control:
			case ETLLRN_RigElementType::Null:
			case ETLLRN_RigElementType::Physics:
			case ETLLRN_RigElementType::Reference:
			{
				for (const FTLLRN_RigElementKey& DraggedKey : RigDragDropOp->GetElements())
				{
					switch (DraggedKey.Type)
					{
						case ETLLRN_RigElementType::Control:
						case ETLLRN_RigElementType::Null:
						case ETLLRN_RigElementType::Physics:
						case ETLLRN_RigElementType::Reference:
						case ETLLRN_RigElementType::Connector:
						case ETLLRN_RigElementType::Socket:
						{
							break;
						}
						default:
						{
							return InvalidDropZone;
						}
					}
				}
				ReturnDropZone = DropZone;
				break;
			}
			case ETLLRN_RigElementType::Connector:
			{
				// anything can be parented under a connector
				ReturnDropZone = DropZone;
				break;
			}
			case ETLLRN_RigElementType::Socket:
			{
				// Only connectors can be parented under a socket
				if (RigDragDropOp->IsDraggingSingleConnector())
				{
					ReturnDropZone = DropZone;
				}
				else
				{
					return InvalidDropZone;
				}
				break;
			}
			default:
			{
				ReturnDropZone = DropZone;
				break;
			}
		}
	}

	const TSharedPtr<FTLLRN_RigHierarchyTagDragDropOp> TagDragDropOp = DragDropEvent.GetOperationAs<FTLLRN_RigHierarchyTagDragDropOp>();
	if(TagDragDropOp.IsValid())
	{
		if(DropZone != EItemDropZone::OntoItem)
		{
			return InvalidDropZone;
		}

		if (const UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
		{
			FTLLRN_RigElementKey DraggedKey;
			FTLLRN_RigElementKey::StaticStruct()->ImportText(*TagDragDropOp->GetIdentifier(), &DraggedKey, nullptr, EPropertyPortFlags::PPF_None, nullptr, FTLLRN_RigElementKey::StaticStruct()->GetName(), true);

			if(Hierarchy->Contains(DraggedKey) && TargetItem.IsValid())
			{
				if(DraggedKey.Type == ETLLRN_RigElementType::Connector)
				{
					if(const FTLLRN_ModularRigResolveResult* ResolveResult = DragRigResolveResults.Find(DraggedKey))
					{
						if(!ResolveResult->ContainsMatch(TargetItem->Key))
						{
							return InvalidDropZone;
						}
					}
				}
				ReturnDropZone = DropZone;
			}
			else if(!TargetItem.IsValid())
			{
				ReturnDropZone = DropZone;
			}
		}
	}
	
	const TSharedPtr<FTLLRN_ModularTLLRN_RigModuleDragDropOp> ModuleDropOp = DragDropEvent.GetOperationAs<FTLLRN_ModularTLLRN_RigModuleDragDropOp>();
	if (ModuleDropOp.IsValid() && TargetItem.IsValid())
	{
		if(DropZone != EItemDropZone::OntoItem)
		{
			return InvalidDropZone;
		}

		const UTLLRN_ModularRig* TLLRN_ControlRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
		if (!TLLRN_ControlRig)
		{
			return InvalidDropZone;
		}

		const FTLLRN_RigElementKey TargetKey = TargetItem->Key;
		const TArray<FTLLRN_RigElementKey> DraggedKeys = FTLLRN_ControlRigSchematicModel::GetElementKeysFromDragDropEvent(*ModuleDropOp.Get(), TLLRN_ControlRig);
		for(const FTLLRN_RigElementKey& DraggedKey : DraggedKeys)
		{
			if(DraggedKey.Type != ETLLRN_RigElementType::Connector)
			{
				continue;
			}
			
			if(!DragRigResolveResults.Contains(DraggedKey))
			{
				UpdateConnectorMatchesOnDrag({DraggedKey});
			}
			
			const FTLLRN_ModularRigResolveResult& ResolveResult = DragRigResolveResults.FindChecked(DraggedKey);
			if(ResolveResult.ContainsMatch(TargetKey))
			{
				return DropZone;
			}
		}

		return InvalidDropZone;
	}

	TSharedPtr<FAssetDragDropOp> AssetDragDropOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (AssetDragDropOp.IsValid())
	{
		for (const FAssetData& AssetData : AssetDragDropOp->GetAssets())
		{
			static const UEnum* ControlTypeEnum = StaticEnum<ETLLRN_ControlRigType>();
			const FString TLLRN_ControlRigTypeStr = AssetData.GetTagValueRef<FString>(TEXT("TLLRN_ControlRigType"));
			if (TLLRN_ControlRigTypeStr.IsEmpty())
			{
				return InvalidDropZone;
			}

			const ETLLRN_ControlRigType TLLRN_ControlRigType = (ETLLRN_ControlRigType)(ControlTypeEnum->GetValueByName(*TLLRN_ControlRigTypeStr));
			if (TLLRN_ControlRigType != ETLLRN_ControlRigType::TLLRN_RigModule)
			{
				return InvalidDropZone;
			}

			if(UTLLRN_ControlRigBlueprint* AssetBlueprint = Cast<UTLLRN_ControlRigBlueprint>(AssetData.GetAsset()))
			{
				if (UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
				{
					FTLLRN_RigModuleConnector* PrimaryConnector = nullptr;
					for (FTLLRN_RigModuleConnector& Connector : AssetBlueprint->TLLRN_RigModuleSettings.ExposedConnectors)
					{
						if (Connector.IsPrimary())
						{
							PrimaryConnector = &Connector;
							break;
						}
					}
					if (!PrimaryConnector)
					{
						return InvalidDropZone;
					}

					FTLLRN_RigElementKey TargetKey;
					if (TargetItem.IsValid())
					{
						TargetKey = TargetItem->Key;
					}

					/*
					FText ErrorMessage;
					if (Controller->CanConnectConnectorToElement(*PrimaryConnector, TargetKey, ErrorMessage))
					{
						ReturnDropZone = DropZone;
					}
					else
					{
						return InvalidDropZone;
					}
					*/
					ReturnDropZone = DropZone;
				}
			}
		}
	}

	return ReturnDropZone;
}

FReply STLLRN_RigHierarchy::OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FRigTreeElement> TargetItem)
{
	bool bSummonDragDropMenu = DragDropEvent.GetModifierKeys().IsAltDown() && DragDropEvent.GetModifierKeys().IsShiftDown(); 
	bool bMatchTransforms = DragDropEvent.GetModifierKeys().IsAltDown();
	bool bReparentItems = !bMatchTransforms;
	UpdateConnectorMatchesOnDrag({});

	TSharedPtr<FTLLRN_RigElementHierarchyDragDropOp> RigDragDropOp = DragDropEvent.GetOperationAs<FTLLRN_RigElementHierarchyDragDropOp>();
	if (RigDragDropOp.IsValid())
	{
		if (bSummonDragDropMenu)
		{
			const FVector2D& SummonLocation = DragDropEvent.GetScreenSpacePosition();

			// Get the context menu content. If NULL, don't open a menu.
			UToolMenu* DragDropMenu = GetDragDropMenu(RigDragDropOp->GetElements(), TargetItem->Key);
			const TSharedPtr<SWidget> MenuContent = UToolMenus::Get()->GenerateWidget(DragDropMenu);

			if (MenuContent.IsValid())
			{
				const FWidgetPath WidgetPath = DragDropEvent.GetEventPath() != nullptr ? *DragDropEvent.GetEventPath() : FWidgetPath();
				FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuContent.ToSharedRef(), SummonLocation, FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
			}
			
			return FReply::Handled();
		}
		else
		{
			const UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();

			FTLLRN_RigElementKey TargetKey;
			int32 LocalIndex = INDEX_NONE;


			if (TargetItem.IsValid())
			{
				switch(DropZone)
				{
					case EItemDropZone::AboveItem:
					{
						TargetKey = Hierarchy->GetFirstParent(TargetItem->Key);
						LocalIndex = Hierarchy->GetLocalIndex(TargetItem->Key);
						break;
					}
					case EItemDropZone::BelowItem:
					{
						TargetKey = Hierarchy->GetFirstParent(TargetItem->Key);
						LocalIndex = Hierarchy->GetLocalIndex(TargetItem->Key) + 1;
						break;
					}
					case EItemDropZone::OntoItem:
					{
						TargetKey = TargetItem->Key;
						break;
					}
				}
			}

			if(RigDragDropOp->IsDraggingSingleConnector())
			{
				return ResolveConnector(RigDragDropOp->GetElements()[0], TargetKey);
			}
			
			return ReparentOrMatchTransform(RigDragDropOp->GetElements(), TargetKey, bReparentItems, LocalIndex);			
		}

	}

	const TSharedPtr<FTLLRN_RigHierarchyTagDragDropOp> TagDragDropOp = DragDropEvent.GetOperationAs<FTLLRN_RigHierarchyTagDragDropOp>();
	if(TagDragDropOp.IsValid())
	{
		FTLLRN_RigElementKey DraggedKey;
		FTLLRN_RigElementKey::StaticStruct()->ImportText(*TagDragDropOp->GetIdentifier(), &DraggedKey, nullptr, EPropertyPortFlags::PPF_None, nullptr, FTLLRN_RigElementKey::StaticStruct()->GetName(), true);
		if(TargetItem.IsValid())
		{
			return ResolveConnector(DraggedKey, TargetItem->Key);
		}
		return ResolveConnector(DraggedKey, FTLLRN_RigElementKey());
	}

	const TSharedPtr<FTLLRN_ModularTLLRN_RigModuleDragDropOp> ModuleDropOp = DragDropEvent.GetOperationAs<FTLLRN_ModularTLLRN_RigModuleDragDropOp>();
	if (ModuleDropOp.IsValid() && TargetItem.IsValid())
	{
		UTLLRN_ModularRig* TLLRN_ControlRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
		if (!TLLRN_ControlRig)
		{
			return FReply::Handled();
		}

		const FTLLRN_RigElementKey TargetKey = TargetItem->Key;
		const TArray<FTLLRN_RigElementKey> DraggedKeys = FTLLRN_ControlRigSchematicModel::GetElementKeysFromDragDropEvent(*ModuleDropOp.Get(), TLLRN_ControlRig);

		bool bSuccess = false;
		for(const FTLLRN_RigElementKey& DraggedKey : DraggedKeys)
		{
			const FReply Reply = ResolveConnector(DraggedKey, TargetKey);
			if(Reply.IsEventHandled())
			{
				bSuccess = true;
			}
		}
		return bSuccess ? FReply::Handled() : FReply::Unhandled();
	}
	
	TSharedPtr<FAssetDragDropOp> AssetDragDropOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (AssetDragDropOp.IsValid())
	{
		for (const FAssetData& AssetData : AssetDragDropOp->GetAssets())
		{
			UClass* AssetClass = AssetData.GetClass();
			if (!AssetClass->IsChildOf(UTLLRN_ControlRigBlueprint::StaticClass()))
			{
				continue;
			}

			if(UTLLRN_ControlRigBlueprint* AssetBlueprint = Cast<UTLLRN_ControlRigBlueprint>(AssetData.GetAsset()))
			{
				if (UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
				{
					static const FString ParentPath = FString();
					const FTLLRN_RigName ModuleName = Controller->GetSafeNewName(ParentPath, FTLLRN_RigName(AssetBlueprint->TLLRN_RigModuleSettings.Identifier.Name));
					const FString ModulePath = Controller->AddModule(ModuleName, AssetBlueprint->GetTLLRN_ControlRigClass(), ParentPath);
					if(TargetItem.IsValid() && !ModulePath.IsEmpty())
					{
						FTLLRN_RigElementKey PrimaryConnectorKey;
						TArray<FTLLRN_RigConnectorElement*> Connectors = GetHierarchy()->GetElementsOfType<FTLLRN_RigConnectorElement>();
						for (FTLLRN_RigConnectorElement* Connector : Connectors)
						{
							if (Connector->IsPrimary())
							{
								FString Path, Name;
								(void)UTLLRN_RigHierarchy::SplitNameSpace(Connector->GetName(), &Path, &Name);
								if (Path == ModulePath)
								{
									PrimaryConnectorKey = Connector->GetKey();
									break;
								}
							}
						}
						return ResolveConnector(PrimaryConnectorKey, TargetItem->Key);
					}
				}
				return FReply::Handled();
			}
		}
	}

	return FReply::Unhandled();
}

void STLLRN_RigHierarchy::OnElementKeyTagDragDetected(const FTLLRN_RigElementKey& InDraggedTag)
{
	UpdateConnectorMatchesOnDrag({InDraggedTag});
}

void STLLRN_RigHierarchy::UpdateConnectorMatchesOnDrag(const TArray<FTLLRN_RigElementKey>& InDraggedKeys)
{
	DragRigResolveResults.Reset();

	// fade in all items
	for(const TPair<FTLLRN_RigElementKey, TSharedPtr<FRigTreeElement>>& Pair : TreeView->ElementMap)
	{
		Pair.Value->bFadedOutDuringDragDrop = false;
	}
	
	if(TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		if(const UTLLRN_ModularRig* TLLRN_ControlRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigBeingDebuggedPtr.Get()))
		{
			if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
			{
				if(const UTLLRN_ModularRigRuleManager* RuleManager = Hierarchy->GetRuleManager())
				{
					for(const FTLLRN_RigElementKey& DraggedElement : InDraggedKeys)
					{
						if(DraggedElement.Type == ETLLRN_RigElementType::Connector)
						{
							if(const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(DraggedElement))
							{
								const FName NameSpace = Hierarchy->GetNameSpaceFName(Connector->GetKey());
								if(!NameSpace.IsNone())
								{
									if(const FTLLRN_RigModuleInstance* Module = TLLRN_ControlRig->FindModule(NameSpace.ToString()))
									{
										const FTLLRN_ModularRigResolveResult ResolveResult = RuleManager->FindMatches(Connector, Module, TLLRN_ControlRig->ElementKeyRedirector); 
										DragRigResolveResults.Add(DraggedElement, ResolveResult);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// fade out anything that's on an excluded list
	for(const TPair<FTLLRN_RigElementKey, FTLLRN_ModularRigResolveResult>& Pair: DragRigResolveResults)
	{
		for(const FTLLRN_RigElementResolveResult& ExcludedElement : Pair.Value.GetExcluded())
		{
			if(const TSharedPtr<FRigTreeElement>* TreeElementPtr = TreeView->ElementMap.Find(ExcludedElement.GetKey()))
			{
				TreeElementPtr->Get()->bFadedOutDuringDragDrop = true;
			}
		}
	}
}

FName STLLRN_RigHierarchy::HandleRenameElement(const FTLLRN_RigElementKey& OldKey, const FString& NewName)
{
	ClearDetailPanel();

	// make sure there is no duplicate
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		FScopedTransaction Transaction(LOCTEXT("HierarchyRename", "Rename Hierarchy Element"));

		UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);

		FTLLRN_RigName SanitizedName(NewName);
		Hierarchy->SanitizeName(SanitizedName);
		FName ResultingName = NAME_None;

		bool bUseDisplayName = false;
		if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(OldKey))
		{
			if(ControlElement->IsAnimationChannel())
			{
				bUseDisplayName = true;
			}
		}

		if(bUseDisplayName)
		{
			ResultingName = Controller->SetDisplayName(OldKey, SanitizedName, true, true, true);
		}
		else
		{
			ResultingName = Controller->RenameElement(OldKey, SanitizedName, true, true, false).Name;
		}
		TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
		return ResultingName;
	}

	return NAME_None;
}

bool STLLRN_RigHierarchy::HandleVerifyNameChanged(const FTLLRN_RigElementKey& OldKey, const FString& NewName, FText& OutErrorMessage)
{
	bool bIsAnimationChannel = false;
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
		if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(OldKey))
		{
			if(ControlElement->IsAnimationChannel())
			{
				bIsAnimationChannel = true;

				if(ControlElement->GetDisplayName().ToString() == NewName)
				{
					return true;
				}
			}
		}
	}

	if(!bIsAnimationChannel)
	{
		if (OldKey.Name.ToString() == NewName)
		{
			return true;
		}
	}

	if (NewName.IsEmpty())
	{
		OutErrorMessage = FText::FromString(TEXT("Name is empty."));
		return false;
	}

	// make sure there is no duplicate
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();

		if(bIsAnimationChannel)
		{
			if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(OldKey))
			{
				if(const FTLLRN_RigBaseElement* ParentElement = Hierarchy->GetFirstParent(ControlElement))
				{
					FString OutErrorString;
					if (!Hierarchy->IsDisplayNameAvailable(ParentElement->GetKey(), FTLLRN_RigName(NewName), &OutErrorString))
					{
						OutErrorMessage = FText::FromString(OutErrorString);
						return false;
					}
				}
			}
		}
		else
		{
			FString OutErrorString;
			if (!Hierarchy->IsNameAvailable(FTLLRN_RigName(NewName), OldKey.Type, &OutErrorString))
			{
				OutErrorMessage = FText::FromString(OutErrorString);
				return false;
			}
		}
	}
	return true;
}

FReply STLLRN_RigHierarchy::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// only allow drops onto empty space of the widget (when there's no target item under the mouse)
	const TSharedPtr<FRigTreeElement>* ItemAtMouse = TreeView->FindItemAtPosition(DragDropEvent.GetScreenSpacePosition());
	if (ItemAtMouse && ItemAtMouse->IsValid())
	{
		return FReply::Unhandled();
	}
	
	return OnAcceptDrop(DragDropEvent, EItemDropZone::OntoItem, nullptr);
}

void STLLRN_RigHierarchy::HandleResetTransform(bool bSelectionOnly)
{
	if ((IsMultiSelected(true) || !bSelectionOnly) && TLLRN_ControlRigEditor.IsValid())
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint())
		{
			if (UTLLRN_RigHierarchy* DebuggedHierarchy = GetHierarchy())
			{
				FScopedTransaction Transaction(LOCTEXT("HierarchyResetTransforms", "Reset Transforms"));

				TArray<FTLLRN_RigElementKey> KeysToReset = GetSelectedKeys();
				if (!bSelectionOnly)
				{
					KeysToReset = DebuggedHierarchy->GetAllKeys(true, ETLLRN_RigElementType::Control);

					// Bone Transforms can also be modified, support reset for them as well
					KeysToReset.Append(DebuggedHierarchy->GetAllKeys(true, ETLLRN_RigElementType::Bone));
				}

				for (FTLLRN_RigElementKey Key : KeysToReset)
				{
					const FTransform InitialTransform = GetHierarchy()->GetInitialLocalTransform(Key);
					GetHierarchy()->SetLocalTransform(Key, InitialTransform, false, true, true, true);
					DebuggedHierarchy->SetLocalTransform(Key, InitialTransform, false, true, true);

					if (Key.Type == ETLLRN_RigElementType::Bone)
					{
						Blueprint->RemoveTransientControl(Key);
						TLLRN_ControlRigEditor.Pin()->RemoveBoneModification(Key.Name); 
					}
				}
			}
		}
	}
}

void STLLRN_RigHierarchy::HandleSetInitialTransformFromCurrentTransform()
{
	if (IsMultiSelected(false))
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint())
		{
			if (UTLLRN_RigHierarchy* DebuggedHierarchy = GetHierarchy())
			{
				FScopedTransaction Transaction(LOCTEXT("HierarchySetInitialTransforms", "Set Initial Transforms"));

				TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
				TMap<FTLLRN_RigElementKey, FTransform> GlobalTransforms;
				TMap<FTLLRN_RigElementKey, FTransform> ParentGlobalTransforms;

				for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
				{
					GlobalTransforms.FindOrAdd(SelectedKey) = DebuggedHierarchy->GetGlobalTransform(SelectedKey);
					ParentGlobalTransforms.FindOrAdd(SelectedKey) = DebuggedHierarchy->GetParentTransform(SelectedKey);
				}

				UTLLRN_RigHierarchy* DefaultHierarchy = GetDefaultHierarchy();
				
				for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
				{
					FTransform GlobalTransform = GlobalTransforms[SelectedKey];
					FTransform LocalTransform = GlobalTransform.GetRelativeTransform(ParentGlobalTransforms[SelectedKey]);

					if (SelectedKey.Type == ETLLRN_RigElementType::Control)
					{
						if(FTLLRN_RigControlElement* ControlElement = DebuggedHierarchy->Find<FTLLRN_RigControlElement>(SelectedKey))
						{
							DebuggedHierarchy->SetControlOffsetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::InitialLocal, true, true, false, true);
							DebuggedHierarchy->SetControlOffsetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, true, false, true);
							DebuggedHierarchy->SetTransform(ControlElement, FTransform::Identity, ETLLRN_RigTransformType::InitialLocal, true, true, false, true);
							DebuggedHierarchy->SetTransform(ControlElement, FTransform::Identity, ETLLRN_RigTransformType::CurrentLocal, true, true, false, true);
						}

						if(DefaultHierarchy)
						{
							if(FTLLRN_RigControlElement* ControlElement = DefaultHierarchy->Find<FTLLRN_RigControlElement>(SelectedKey))
							{
								DefaultHierarchy->SetControlOffsetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::InitialLocal, true, true);
								DefaultHierarchy->SetControlOffsetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, true);
								DefaultHierarchy->SetTransform(ControlElement, FTransform::Identity, ETLLRN_RigTransformType::InitialLocal, true, true);
								DefaultHierarchy->SetTransform(ControlElement, FTransform::Identity, ETLLRN_RigTransformType::CurrentLocal, true, true);
							}
						}
					}
					else if (SelectedKey.Type == ETLLRN_RigElementType::Null ||
						SelectedKey.Type == ETLLRN_RigElementType::Bone ||
						SelectedKey.Type == ETLLRN_RigElementType::Connector)
					{
						FTransform InitialTransform = LocalTransform;
						if (TLLRN_ControlRigEditor.Pin()->PreviewInstance)
						{
							if (FAnimNode_ModifyBone* ModifyBone = TLLRN_ControlRigEditor.Pin()->PreviewInstance->FindModifiedBone(SelectedKey.Name))
							{
								InitialTransform.SetTranslation(ModifyBone->Translation);
								InitialTransform.SetRotation(FQuat(ModifyBone->Rotation));
								InitialTransform.SetScale3D(ModifyBone->Scale);
							}
						}

						if(FTLLRN_RigTransformElement* TransformElement = DebuggedHierarchy->Find<FTLLRN_RigTransformElement>(SelectedKey))
						{
							DebuggedHierarchy->SetTransform(TransformElement, LocalTransform, ETLLRN_RigTransformType::InitialLocal, true, true, false, true);
							DebuggedHierarchy->SetTransform(TransformElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, true, false, true);
						}

						if(DefaultHierarchy)
						{
							if(FTLLRN_RigTransformElement* TransformElement = DefaultHierarchy->Find<FTLLRN_RigTransformElement>(SelectedKey))
							{
								DefaultHierarchy->SetTransform(TransformElement, LocalTransform, ETLLRN_RigTransformType::InitialLocal, true, true);
								DefaultHierarchy->SetTransform(TransformElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, true);
							}
						}
					}
				}
			}
		}
	}
}

void STLLRN_RigHierarchy::HandleFrameSelection()
{
	TArray<TSharedPtr<FRigTreeElement>> SelectedItems = TreeView->GetSelectedItems();
	for (TSharedPtr<FRigTreeElement> SelectedItem : SelectedItems)
	{
		TreeView->SetExpansionRecursive(SelectedItem, true, true);
	}

	if (SelectedItems.Num() > 0)
	{
		TreeView->RequestScrollIntoView(SelectedItems.Last());
	}
}

void STLLRN_RigHierarchy::HandleControlBoneOrSpaceTransform()
{
	UTLLRN_ControlRigBlueprint* Blueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint();
	if (Blueprint == nullptr)
	{
		return;
	}

	UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(Blueprint->GetObjectBeingDebugged());
	if(DebuggedTLLRN_ControlRig == nullptr)
	{
		return;
	}

	TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
	if (SelectedKeys.Num() == 1)
	{
		if (SelectedKeys[0].Type == ETLLRN_RigElementType::Bone ||
			SelectedKeys[0].Type == ETLLRN_RigElementType::Null ||
			SelectedKeys[0].Type == ETLLRN_RigElementType::Connector)
		{
			if(!DebuggedTLLRN_ControlRig->GetHierarchy()->IsProcedural(SelectedKeys[0]))
			{
				Blueprint->AddTransientControl(SelectedKeys[0]);
			}
		}
	}
}

void STLLRN_RigHierarchy::HandleUnparent()
{
	UTLLRN_ControlRigBlueprint* Blueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint();
	if (Blueprint == nullptr)
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("HierarchyTreeUnparentSelected", "Unparent selected items from hierarchy"));

	bool bUnparentImportedBones = false;
	bool bConfirmedByUser = false;

	TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
	TMap<FTLLRN_RigElementKey, FTransform> InitialTransforms;
	TMap<FTLLRN_RigElementKey, FTransform> GlobalTransforms;

	for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
	{
		UTLLRN_RigHierarchy* Hierarchy = GetHierarchy();
		InitialTransforms.Add(SelectedKey, Hierarchy->GetInitialGlobalTransform(SelectedKey));
		GlobalTransforms.Add(SelectedKey, Hierarchy->GetGlobalTransform(SelectedKey));
	}

	for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
	{
		TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
		TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);

		UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();
		check(Hierarchy);

		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		check(Controller);

		const FTransform& InitialTransform = InitialTransforms.FindChecked(SelectedKey);
		const FTransform& GlobalTransform = GlobalTransforms.FindChecked(SelectedKey);

		switch (SelectedKey.Type)
		{
			case ETLLRN_RigElementType::Bone:
			{
				
				bool bIsImportedBone = false;
				if(FTLLRN_RigBoneElement* BoneElement = Hierarchy->Find<FTLLRN_RigBoneElement>(SelectedKey))
				{
					bIsImportedBone = BoneElement->BoneType == ETLLRN_RigBoneType::Imported;
				}
					
				if (bIsImportedBone && !bConfirmedByUser)
				{
					FText ConfirmUnparent = LOCTEXT("ConfirmUnparentBoneHierarchy", "Unparenting imported(white) bones can cause issues with animation - are you sure ?");

					FSuppressableWarningDialog::FSetupInfo Info(ConfirmUnparent, LOCTEXT("UnparentImportedBone", "Unparent Imported Bone"), "UnparentImportedBoneHierarchy_Warning");
					Info.ConfirmText = LOCTEXT("UnparentImportedBoneHierarchy_Yes", "Yes");
					Info.CancelText = LOCTEXT("UnparentImportedBoneHierarchy_No", "No");

					FSuppressableWarningDialog UnparentImportedBonesInHierarchy(Info);
					bUnparentImportedBones = UnparentImportedBonesInHierarchy.ShowModal() != FSuppressableWarningDialog::Cancel;
					bConfirmedByUser = true;
				}

				if (bUnparentImportedBones || !bIsImportedBone)
				{
					Controller->RemoveAllParents(SelectedKey, true, true, true);
				}
				break;
			}
			case ETLLRN_RigElementType::Null:
			case ETLLRN_RigElementType::Control:
			case ETLLRN_RigElementType::Connector:
			{
				Controller->RemoveAllParents(SelectedKey, true, true, true);
				break;
			}
			default:
			{
				break;
			}
		}

		Hierarchy->SetInitialGlobalTransform(SelectedKey, InitialTransform, true, true);
		Hierarchy->SetGlobalTransform(SelectedKey, GlobalTransform, false, true, true);
	}

	TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();
	TLLRN_ControlRigEditor.Pin()->OnHierarchyChanged();
	RefreshTreeView();

	if (UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy())
	{
		Hierarchy->GetController()->SetSelection(SelectedKeys);
	}
	
	FSlateApplication::Get().DismissAllMenus();
}

bool STLLRN_RigHierarchy::FindClosestBone(const FVector& Point, FName& OutTLLRN_RigElementName, FTransform& OutGlobalTransform) const
{
	if (UTLLRN_RigHierarchy* DebuggedHierarchy = GetHierarchy())
	{
		float NearestDistance = BIG_NUMBER;

		DebuggedHierarchy->ForEach<FTLLRN_RigBoneElement>([&] (FTLLRN_RigBoneElement* Element) -> bool
		{
			const FTransform CurTransform = DebuggedHierarchy->GetTransform(Element, ETLLRN_RigTransformType::CurrentGlobal);
            const float CurDistance = FVector::Distance(CurTransform.GetLocation(), Point);
            if (CurDistance < NearestDistance)
            {
                NearestDistance = CurDistance;
                OutGlobalTransform = CurTransform;
                OutTLLRN_RigElementName = Element->GetFName();
            }
            return true;
		});

		return (OutTLLRN_RigElementName != NAME_None);
	}
	return false;
}

void STLLRN_RigHierarchy::HandleTestSpaceSwitching()
{
	if (FTLLRN_ControlRigEditorEditMode* EditMode = TLLRN_ControlRigEditor.Pin()->GetEditMode())
	{
		/// toooo centralize code here
		EditMode->OpenSpacePickerWidget();
	}
}

void STLLRN_RigHierarchy::HandleParent(const FToolMenuContext& Context)
{
	if (UTLLRN_ControlRigContextMenuContext* MenuContext = Cast<UTLLRN_ControlRigContextMenuContext>(Context.FindByClass(UTLLRN_ControlRigContextMenuContext::StaticClass())))
	{
		const FTLLRN_ControlRigTLLRN_RigHierarchyDragAndDropContext DragAndDropContext = MenuContext->GetTLLRN_RigHierarchyDragAndDropContext();
		ReparentOrMatchTransform(DragAndDropContext.DraggedElementKeys, DragAndDropContext.TargetElementKey, true);
	}
}

void STLLRN_RigHierarchy::HandleAlign(const FToolMenuContext& Context)
{
	if (UTLLRN_ControlRigContextMenuContext* MenuContext = Cast<UTLLRN_ControlRigContextMenuContext>(Context.FindByClass(UTLLRN_ControlRigContextMenuContext::StaticClass())))
	{
		const FTLLRN_ControlRigTLLRN_RigHierarchyDragAndDropContext DragAndDropContext = MenuContext->GetTLLRN_RigHierarchyDragAndDropContext();
		ReparentOrMatchTransform(DragAndDropContext.DraggedElementKeys, DragAndDropContext.TargetElementKey, false);
	}
}

FReply STLLRN_RigHierarchy::ReparentOrMatchTransform(const TArray<FTLLRN_RigElementKey>& DraggedKeys, FTLLRN_RigElementKey TargetKey, bool bReparentItems, int32 LocalIndex)
{
	bool bMatchTransforms = !bReparentItems;

	UTLLRN_RigHierarchy* DebuggedHierarchy = GetHierarchy();
	UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();

	const TArray<FTLLRN_RigElementKey> SelectedKeys = (Hierarchy) ? Hierarchy->GetSelectedKeys() : TArray<FTLLRN_RigElementKey>();

	if (Hierarchy && TLLRN_ControlRigBlueprint.IsValid())
	{
		UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true);
		if(Controller == nullptr)
		{
			return FReply::Unhandled();
		}

		TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
		TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBlueprint->bSuspendAllNotifications, true);
		FScopedTransaction Transaction(LOCTEXT("HierarchyDragAndDrop", "Drag & Drop"));
		FTLLRN_RigHierarchyInteractionBracket InteractionBracket(Hierarchy);

		FTransform TargetGlobalTransform = DebuggedHierarchy->GetGlobalTransform(TargetKey);

		for (const FTLLRN_RigElementKey& DraggedKey : DraggedKeys)
		{
			if (DraggedKey == TargetKey)
			{
				return FReply::Unhandled();
			}

			if (bReparentItems)
			{
				if(Hierarchy->IsParentedTo(TargetKey, DraggedKey))
				{
					if(LocalIndex == INDEX_NONE)
					{
						return FReply::Unhandled();
					}
				}
			}

			if (DraggedKey.Type == ETLLRN_RigElementType::Bone)
			{
				if(FTLLRN_RigBoneElement* BoneElement = Hierarchy->Find<FTLLRN_RigBoneElement>(DraggedKey))
				{
					if (BoneElement->BoneType == ETLLRN_RigBoneType::Imported && BoneElement->ParentElement != nullptr)
					{
						FText ConfirmText = bMatchTransforms ?
							LOCTEXT("ConfirmMatchTransform", "Matching transforms of imported(white) bones can cause issues with animation - are you sure ?") :
							LOCTEXT("ConfirmReparentBoneHierarchy", "Reparenting imported(white) bones can cause issues with animation - are you sure ?");

						FText TitleText = bMatchTransforms ?
							LOCTEXT("MatchTransformImportedBone", "Match Transform on Imported Bone") :
							LOCTEXT("ReparentImportedBone", "Reparent Imported Bone");

						FSuppressableWarningDialog::FSetupInfo Info(ConfirmText, TitleText, "STLLRN_RigHierarchy_Warning");
						Info.ConfirmText = LOCTEXT("STLLRN_RigHierarchy_Warning_Yes", "Yes");
						Info.CancelText = LOCTEXT("STLLRN_RigHierarchy_Warning_No", "No");

						FSuppressableWarningDialog ChangeImportedBonesInHierarchy(Info);
						if (ChangeImportedBonesInHierarchy.ShowModal() == FSuppressableWarningDialog::Cancel)
						{
							return FReply::Unhandled();
						}
					}
				}
			}
		}

		for (const FTLLRN_RigElementKey& DraggedKey : DraggedKeys)
		{
			if (bMatchTransforms)
			{
				if (DraggedKey.Type == ETLLRN_RigElementType::Control)
				{
					int32 ControlIndex = DebuggedHierarchy->GetIndex(DraggedKey);
					if (ControlIndex == INDEX_NONE)
					{
						continue;
					}

					FTransform ParentTransform = DebuggedHierarchy->GetParentTransformByIndex(ControlIndex, false);
					FTransform OffsetTransform = TargetGlobalTransform.GetRelativeTransform(ParentTransform);

					Hierarchy->SetControlOffsetTransformByIndex(ControlIndex, OffsetTransform, ETLLRN_RigTransformType::InitialLocal, true, true, true);
					Hierarchy->SetControlOffsetTransformByIndex(ControlIndex, OffsetTransform, ETLLRN_RigTransformType::CurrentLocal, true, true, true);
					Hierarchy->SetLocalTransform(DraggedKey, FTransform::Identity, true, true, true, true);
					Hierarchy->SetInitialLocalTransform(DraggedKey, FTransform::Identity, true, true, true);
					DebuggedHierarchy->SetControlOffsetTransformByIndex(ControlIndex, OffsetTransform, ETLLRN_RigTransformType::InitialLocal, true, true);
					DebuggedHierarchy->SetControlOffsetTransformByIndex(ControlIndex, OffsetTransform, ETLLRN_RigTransformType::CurrentLocal, true, true);
					DebuggedHierarchy->SetLocalTransform(DraggedKey, FTransform::Identity, true, true, true);
					DebuggedHierarchy->SetInitialLocalTransform(DraggedKey, FTransform::Identity, true, true);
				}
				else
				{
					Hierarchy->SetInitialGlobalTransform(DraggedKey, TargetGlobalTransform, true, true);
					Hierarchy->SetGlobalTransform(DraggedKey, TargetGlobalTransform, false, true, true);
					DebuggedHierarchy->SetInitialGlobalTransform(DraggedKey, TargetGlobalTransform, true, true);
					DebuggedHierarchy->SetGlobalTransform(DraggedKey, TargetGlobalTransform, false, true, true);
				}
				continue;
			}

			FTLLRN_RigElementKey ParentKey = TargetKey;

			const FTransform InitialGlobalTransform = DebuggedHierarchy->GetInitialGlobalTransform(DraggedKey);
			const FTransform CurrentGlobalTransform = DebuggedHierarchy->GetGlobalTransform(DraggedKey);
			const FTransform InitialLocalTransform = DebuggedHierarchy->GetInitialLocalTransform(DraggedKey);
			const FTransform CurrentLocalTransform = DebuggedHierarchy->GetLocalTransform(DraggedKey);
			const FTransform CurrentGlobalOffsetTransform = DebuggedHierarchy->GetGlobalControlOffsetTransform(DraggedKey, false);

			bool bElementWasReparented = false;
			if(ParentKey.IsValid() && (Hierarchy->GetFirstParent(DraggedKey) != ParentKey))
			{
				bElementWasReparented = Controller->SetParent(DraggedKey, ParentKey, true, true, true);
			}
			else if(!ParentKey.IsValid() && (Hierarchy->GetNumberOfParents(DraggedKey) > 0))
			{
				bElementWasReparented = Controller->RemoveAllParents(DraggedKey, true, true, true);
			}

			if(LocalIndex != INDEX_NONE)
			{
				Controller->ReorderElement(DraggedKey, LocalIndex, true, true);
			}

			if(bElementWasReparented)
			{
				if (DraggedKey.Type == ETLLRN_RigElementType::Control)
				{
					int32 ControlIndex = DebuggedHierarchy->GetIndex(DraggedKey);
					if (ControlIndex == INDEX_NONE)
					{
						continue;
					}

					const FTransform GlobalParentTransform = DebuggedHierarchy->GetGlobalTransform(ParentKey, false);
					const FTransform LocalOffsetTransform = CurrentGlobalOffsetTransform.GetRelativeTransform(GlobalParentTransform);

					Hierarchy->SetControlOffsetTransformByIndex(ControlIndex, LocalOffsetTransform, ETLLRN_RigTransformType::InitialLocal, true, true, true);
					Hierarchy->SetControlOffsetTransformByIndex(ControlIndex, LocalOffsetTransform, ETLLRN_RigTransformType::CurrentLocal, true, true, true);
					Hierarchy->SetLocalTransform(DraggedKey, CurrentLocalTransform, true, true, true, true);
					Hierarchy->SetInitialLocalTransform(DraggedKey, InitialLocalTransform, true, true, true);
					DebuggedHierarchy->SetControlOffsetTransformByIndex(ControlIndex, LocalOffsetTransform, ETLLRN_RigTransformType::InitialLocal, true, true);
					DebuggedHierarchy->SetControlOffsetTransformByIndex(ControlIndex, LocalOffsetTransform, ETLLRN_RigTransformType::CurrentLocal, true, true);
					DebuggedHierarchy->SetLocalTransform(DraggedKey, CurrentLocalTransform, true, true, true);
					DebuggedHierarchy->SetInitialLocalTransform(DraggedKey, InitialLocalTransform, true, true);
				}
				else
				{
					DebuggedHierarchy->SetInitialGlobalTransform(DraggedKey, InitialGlobalTransform, true, true);
					DebuggedHierarchy->SetGlobalTransform(DraggedKey, CurrentGlobalTransform, false, true, true);
					Hierarchy->SetInitialGlobalTransform(DraggedKey, InitialGlobalTransform, true, true);
					Hierarchy->SetGlobalTransform(DraggedKey, CurrentGlobalTransform, false, true, true);
				}
			}
		}
	}

	TLLRN_ControlRigBlueprint->PropagateHierarchyFromBPToInstances();

	if(bReparentItems)
	{
		TGuardValue<bool> GuardTLLRN_RigHierarchyChanges(bIsChangingTLLRN_RigHierarchy, true);
		TLLRN_ControlRigBlueprint->BroadcastRefreshEditor();
		RefreshTreeView();
	}

	if (Hierarchy)
	{
		Hierarchy->GetController()->SetSelection(SelectedKeys);
	}
	
	return FReply::Handled();

}

FReply STLLRN_RigHierarchy::ResolveConnector(const FTLLRN_RigElementKey& DraggedKey, const FTLLRN_RigElementKey& TargetKey)
{
	if (UTLLRN_ControlRigBlueprint* Blueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint())
	{
		if (const UTLLRN_RigHierarchy* DebuggedHierarchy = GetHierarchy())
		{
			if(DebuggedHierarchy->Contains(DraggedKey))
			{
				if(Blueprint->ResolveConnector(DraggedKey, TargetKey))
				{
					RefreshTreeView();
					return FReply::Handled();
				}
			}
		}
	}
	return FReply::Unhandled();
}

void STLLRN_RigHierarchy::HandleSetInitialTransformFromClosestBone()
{
	if (IsControlOrNullSelected(false))
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint())
		{
			if (UTLLRN_RigHierarchy* DebuggedHierarchy = GetHierarchy())
			{
				UTLLRN_RigHierarchy* Hierarchy = GetDefaultHierarchy();
  				check(Hierarchy);

				FScopedTransaction Transaction(LOCTEXT("HierarchySetInitialTransforms", "Set Initial Transforms"));

				TArray<FTLLRN_RigElementKey> SelectedKeys = GetSelectedKeys();
				TMap<FTLLRN_RigElementKey, FTransform> ClosestTransforms;
				TMap<FTLLRN_RigElementKey, FTransform> ParentGlobalTransforms;

				for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
				{
					if (SelectedKey.Type == ETLLRN_RigElementType::Control || SelectedKey.Type == ETLLRN_RigElementType::Null)
					{
						const FTransform GlobalTransform = DebuggedHierarchy->GetGlobalTransform(SelectedKey);
						FTransform ClosestTransform;
						FName ClosestTLLRN_RigElement;

						if (!FindClosestBone(GlobalTransform.GetLocation(), ClosestTLLRN_RigElement, ClosestTransform))
						{
							continue;
						}

						ClosestTransforms.FindOrAdd(SelectedKey) = ClosestTransform;
						ParentGlobalTransforms.FindOrAdd(SelectedKey) = DebuggedHierarchy->GetParentTransform(SelectedKey);
					}
				}

				for (const FTLLRN_RigElementKey& SelectedKey : SelectedKeys)
				{
					if (!ClosestTransforms.Contains(SelectedKey))
					{
						continue;
					}
					FTransform GlobalTransform = ClosestTransforms[SelectedKey];
					FTransform LocalTransform = GlobalTransform.GetRelativeTransform(ParentGlobalTransforms[SelectedKey]);

					if (SelectedKey.Type == ETLLRN_RigElementType::Control)
					{
						if(FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(SelectedKey))
						{
							Hierarchy->SetControlOffsetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::InitialLocal, true, true, false, true);
							Hierarchy->SetControlOffsetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, true, false, true);
							Hierarchy->SetTransform(ControlElement, FTransform::Identity, ETLLRN_RigTransformType::InitialLocal, true, true, false, true);
							Hierarchy->SetTransform(ControlElement, FTransform::Identity, ETLLRN_RigTransformType::CurrentLocal, true, true, false, true);
						}
						if(FTLLRN_RigControlElement* ControlElement = DebuggedHierarchy->Find<FTLLRN_RigControlElement>(SelectedKey))
						{
							DebuggedHierarchy->SetControlOffsetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::InitialLocal, true, true);
							DebuggedHierarchy->SetControlOffsetTransform(ControlElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, true);
							DebuggedHierarchy->SetTransform(ControlElement, FTransform::Identity, ETLLRN_RigTransformType::InitialLocal, true, true);
							DebuggedHierarchy->SetTransform(ControlElement, FTransform::Identity, ETLLRN_RigTransformType::CurrentLocal, true, true);
						}
					}
					else if (SelectedKey.Type == ETLLRN_RigElementType::Null ||
                        SelectedKey.Type == ETLLRN_RigElementType::Bone)
					{
						FTransform InitialTransform = LocalTransform;

						if(FTLLRN_RigTransformElement* TransformElement = Hierarchy->Find<FTLLRN_RigTransformElement>(SelectedKey))
						{
							Hierarchy->SetTransform(TransformElement, LocalTransform, ETLLRN_RigTransformType::InitialLocal, true, true, false, true);
							Hierarchy->SetTransform(TransformElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, true, false, true);
						}
						if(FTLLRN_RigTransformElement* TransformElement = DebuggedHierarchy->Find<FTLLRN_RigTransformElement>(SelectedKey))
						{
							DebuggedHierarchy->SetTransform(TransformElement, LocalTransform, ETLLRN_RigTransformType::InitialLocal, true, true);
							DebuggedHierarchy->SetTransform(TransformElement, LocalTransform, ETLLRN_RigTransformType::CurrentLocal, true, true);
						}
					}
				}
			}
		}
	}
}

void STLLRN_RigHierarchy::HandleSetShapeTransformFromCurrent()
{
	if (IsControlSelected(false))
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint())
		{
			if (UTLLRN_RigHierarchy* DebuggedHierarchy = GetHierarchy())
			{
				FScopedTransaction Transaction(LOCTEXT("HierarchySetShapeTransforms", "Set Shape Transforms"));

				FTLLRN_RigHierarchyInteractionBracket InteractionBracket(GetHierarchy());
				FTLLRN_RigHierarchyInteractionBracket DebuggedInteractionBracket(DebuggedHierarchy);

				TArray<TSharedPtr<FRigTreeElement>> SelectedItems = TreeView->GetSelectedItems();
				for (const TSharedPtr<FRigTreeElement>& SelectedItem : SelectedItems)
				{
					if(FTLLRN_RigControlElement* ControlElement = DebuggedHierarchy->Find<FTLLRN_RigControlElement>(SelectedItem->Key))
					{
						const FTLLRN_RigElementKey Key = ControlElement->GetKey();
						
						if (ControlElement->Settings.SupportsShape())
						{
							const FTransform OffsetGlobalTransform = DebuggedHierarchy->GetGlobalControlOffsetTransform(Key); 
							const FTransform ShapeGlobalTransform = DebuggedHierarchy->GetGlobalControlShapeTransform(Key);
							const FTransform ShapeLocalTransform = ShapeGlobalTransform.GetRelativeTransform(OffsetGlobalTransform);
							
							DebuggedHierarchy->SetControlShapeTransform(Key, ShapeLocalTransform, true, true);
							DebuggedHierarchy->SetControlShapeTransform(Key, ShapeLocalTransform, false, true);
							GetHierarchy()->SetControlShapeTransform(Key, ShapeLocalTransform, true, true);
							GetHierarchy()->SetControlShapeTransform(Key, ShapeLocalTransform, false, true);

							DebuggedHierarchy->SetLocalTransform(Key, FTransform::Identity, false, true, true);
							DebuggedHierarchy->SetLocalTransform(Key, FTransform::Identity, true, true, true);
							GetHierarchy()->SetLocalTransform(Key, FTransform::Identity, false, true, true, true);
							GetHierarchy()->SetLocalTransform(Key, FTransform::Identity, true, true, true, true);
						}

						if (FTLLRN_ControlRigEditorEditMode* EditMode = TLLRN_ControlRigEditor.Pin()->GetEditMode())
						{
							EditMode->RequestToRecreateControlShapeActors();
						}
					}
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

