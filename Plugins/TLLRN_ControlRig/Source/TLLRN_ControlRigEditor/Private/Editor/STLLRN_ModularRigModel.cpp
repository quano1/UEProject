// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/STLLRN_ModularRigModel.h"
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
#include "TLLRN_ControlRigTLLRN_ModularRigCommands.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "Graph/TLLRN_ControlRigGraphNode.h"
#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "GraphEditorModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AnimationRuntime.h"
#include "ClassViewerFilter.h"
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
#include "Sequencer/TLLRN_ControlRigLayerInstance.h"
#include "Algo/MinElement.h"
#include "Algo/MaxElement.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Editor/RigVMEditorTools.h"
#include "Kismet2/SClassPickerDialog.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"
#include "Preferences/PersonaOptions.h"
#include "Widgets/SRigVMBulkEditDialog.h"
#include "Widgets/SRigVMSwapAssetReferencesWidget.h"

#define LOCTEXT_NAMESPACE "STLLRN_ModularRigModel"

///////////////////////////////////////////////////////////

const FName STLLRN_ModularRigModel::ContextMenuName = TEXT("TLLRN_ControlRigEditor.TLLRN_ModularRigModel.ContextMenu");

STLLRN_ModularRigModel::~STLLRN_ModularRigModel()
{
	const FTLLRN_ControlRigEditor* Editor = TLLRN_ControlRigEditor.IsValid() ? TLLRN_ControlRigEditor.Pin().Get() : nullptr;
	OnEditorClose(Editor, TLLRN_ControlRigBlueprint.Get());
}

void STLLRN_ModularRigModel::Construct(const FArguments& InArgs, TSharedRef<FTLLRN_ControlRigEditor> InTLLRN_ControlRigEditor)
{
	TLLRN_ControlRigEditor = InTLLRN_ControlRigEditor;

	TLLRN_ControlRigBlueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint();

	TLLRN_ControlRigBlueprint->OnRefreshEditor().AddRaw(this, &STLLRN_ModularRigModel::HandleRefreshEditorFromBlueprint);
	TLLRN_ControlRigBlueprint->OnSetObjectBeingDebugged().AddRaw(this, &STLLRN_ModularRigModel::HandleSetObjectBeingDebugged);
	TLLRN_ControlRigBlueprint->OnTLLRN_ModularRigPreCompiled().AddRaw(this, &STLLRN_ModularRigModel::HandlePreCompileTLLRN_ModularRigs);
	TLLRN_ControlRigBlueprint->OnTLLRN_ModularRigCompiled().AddRaw(this, &STLLRN_ModularRigModel::HandlePostCompileTLLRN_ModularRigs);

	if(UTLLRN_ModularTLLRN_RigController* TLLRN_ModularTLLRN_RigController = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
	{
		TLLRN_ModularTLLRN_RigController->OnModified().AddSP(this, &STLLRN_ModularRigModel::OnTLLRN_ModularRigModified);
	}

	// for deleting, renaming, dragging
	CommandList = MakeShared<FUICommandList>();

	UEditorEngine* Editor = Cast<UEditorEngine>(GEngine);
	if (Editor != nullptr)
	{
		Editor->RegisterForUndo(this);
	}

	BindCommands();

	bShowSecondaryConnectors = false;
	bShowOptionalConnectors = false;
	bShowUnresolvedConnectors = true;
	bIsPerformingSelection = false;

	// setup all delegates for the modular rig model widget
	FTLLRN_ModularRigTreeDelegates Delegates;
	Delegates.OnGetTLLRN_ModularRig = FOnGetTLLRN_ModularRigTreeRig::CreateSP(this, &STLLRN_ModularRigModel::GetTLLRN_ModularRigForTreeView);
	Delegates.OnContextMenuOpening = FOnContextMenuOpening::CreateSP(this, &STLLRN_ModularRigModel::CreateContextMenuWidget);
	Delegates.OnDragDetected = FOnDragDetected::CreateSP(this, &STLLRN_ModularRigModel::OnDragDetected);
	Delegates.OnCanAcceptDrop = FOnTLLRN_ModularRigTreeCanAcceptDrop::CreateSP(this, &STLLRN_ModularRigModel::OnCanAcceptDrop);
	Delegates.OnAcceptDrop = FOnTLLRN_ModularRigTreeAcceptDrop::CreateSP(this, &STLLRN_ModularRigModel::OnAcceptDrop);
	Delegates.OnMouseButtonClick = FOnTLLRN_ModularRigTreeMouseButtonClick::CreateSP(this, &STLLRN_ModularRigModel::OnItemClicked);
	Delegates.OnMouseButtonDoubleClick = FOnTLLRN_ModularRigTreeMouseButtonClick::CreateSP(this, &STLLRN_ModularRigModel::OnItemDoubleClicked);
	Delegates.OnRequestDetailsInspection = FOnTLLRN_ModularRigTreeRequestDetailsInspection::CreateSP(this, &STLLRN_ModularRigModel::OnRequestDetailsInspection);
	Delegates.OnRenameElement = FOnTLLRN_ModularRigTreeRenameElement::CreateSP(this, &STLLRN_ModularRigModel::HandleRenameModule);
	Delegates.OnVerifyModuleNameChanged = FOnTLLRN_ModularRigTreeVerifyElementNameChanged::CreateSP(this, &STLLRN_ModularRigModel::HandleVerifyNameChanged);
	Delegates.OnResolveConnector = FOnTLLRN_ModularRigTreeResolveConnector::CreateSP(this, &STLLRN_ModularRigModel::HandleConnectorResolved);
	Delegates.OnDisconnectConnector = FOnTLLRN_ModularRigTreeDisconnectConnector::CreateSP(this, &STLLRN_ModularRigModel::HandleConnectorDisconnect);
	Delegates.OnSelectionChanged = FOnTLLRN_ModularRigTreeSelectionChanged::CreateSP(this, &STLLRN_ModularRigModel::HandleSelectionChanged);

	HeaderRowWidget = SNew(SHeaderRow)
		.Visibility(EVisibility::Visible);

	HeaderRowWidget->AddColumn(
		SHeaderRow::Column(STLLRN_ModularRigTreeView::Column_Module)
		.DefaultLabel(FText::FromName(STLLRN_ModularRigTreeView::Column_Module))
		.HAlignCell(HAlign_Left)
		.HAlignHeader(HAlign_Left)
		.VAlignCell(VAlign_Fill)
	);
	HeaderRowWidget->AddColumn(
		SHeaderRow::Column(STLLRN_ModularRigTreeView::Column_Tags)
		.DefaultLabel(FText())
		.HAlignCell(HAlign_Fill)
		.HAlignHeader(HAlign_Fill)
		.FixedWidth(16.f)
		.VAlignCell(VAlign_Center)
	);
	HeaderRowWidget->AddColumn(
		SHeaderRow::Column(STLLRN_ModularRigTreeView::Column_Connector)
		.DefaultLabel(FText::FromName(STLLRN_ModularRigTreeView::Column_Connector))
		.HAlignCell(HAlign_Left)
		.HAlignHeader(HAlign_Left)
		.VAlignCell(VAlign_Center)
	);
	HeaderRowWidget->AddColumn(
		SHeaderRow::Column(STLLRN_ModularRigTreeView::Column_Buttons)
		.DefaultLabel(FText::FromName(STLLRN_ModularRigTreeView::Column_Buttons))
		.ManualWidth(60)
		.HAlignCell(HAlign_Left)
		.HAlignHeader(HAlign_Left)
		.VAlignCell(VAlign_Center)
	);
	
	ChildSlot
	[
		SNew(SVerticalBox)
		
		+SVerticalBox::Slot()
		.Padding(0.0f, 0.0f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			
			+SHorizontalBox::Slot()
			.Padding(0.0f, 0.0f)
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(SComboButton)
			   .ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButtonWithIcon"))
			   .ForegroundColor(FSlateColor::UseStyle())
			   .ToolTipText(LOCTEXT("OptionsToolTip", "Open the Options Menu ."))
			   .OnGetMenuContent(this, &STLLRN_ModularRigModel::OnGetOptionsMenu)
			   .ContentPadding(FMargin(1, 0))
			   .ButtonContent()
			   [
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Filter"))
					.ColorAndOpacity(FSlateColor::UseForeground())
			   ]
			]

			+SHorizontalBox::Slot()
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			.HAlign(HAlign_Fill)
			[
				SAssignNew(FilterBox, SSearchBox)
				.OnTextChanged(this, &STLLRN_ModularRigModel::OnFilterTextChanged)
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
					SAssignNew(TreeView, STLLRN_ModularRigTreeView)
					.HeaderRow(HeaderRowWidget)
					.RigTreeDelegates(Delegates)
					.AutoScrollEnabled(true)
					.FilterText_Lambda([this]() { return FilterText; })
					.ShowSecondaryConnectors_Lambda([this]() { return bShowSecondaryConnectors; })
					.ShowOptionalConnectors_Lambda([this]() { return bShowOptionalConnectors; })
					.ShowUnresolvedConnectors_Lambda([this]() { return bShowUnresolvedConnectors; })
				]
			]
		]
	];

	RefreshTreeView();

	if (TLLRN_ControlRigEditor.IsValid())
	{
		TLLRN_ControlRigEditor.Pin()->GetKeyDownDelegate().BindLambda([&](const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)->FReply {
			return OnKeyDown(MyGeometry, InKeyEvent);
		});
		TLLRN_ControlRigEditor.Pin()->OnGetViewportContextMenu().BindSP(this, &STLLRN_ModularRigModel::GetContextMenu);
		TLLRN_ControlRigEditor.Pin()->OnViewportContextMenuCommands().BindSP(this, &STLLRN_ModularRigModel::GetContextMenuCommands);
		TLLRN_ControlRigEditor.Pin()->OnEditorClosed().AddSP(this, &STLLRN_ModularRigModel::OnEditorClose);
	}
	
	CreateContextMenu();

	if(const UTLLRN_ModularRig* Rig = GetTLLRN_ModularRigForTreeView())
	{
		if(UTLLRN_RigHierarchy* Hierarchy = Rig->GetHierarchy())
		{
			Hierarchy->OnModified().AddSP(this, &STLLRN_ModularRigModel::OnHierarchyModified);
		}
	}
}

void STLLRN_ModularRigModel::OnEditorClose(const FRigVMEditor* InEditor, URigVMBlueprint* InBlueprint)
{
	if (InEditor)
	{
		FTLLRN_ControlRigEditor* Editor = (FTLLRN_ControlRigEditor*)InEditor;  
		Editor->OnGetViewportContextMenu().Unbind();
		Editor->OnViewportContextMenuCommands().Unbind();
	}

	if (UTLLRN_ControlRigBlueprint* BP = Cast<UTLLRN_ControlRigBlueprint>(InBlueprint))
	{
		InBlueprint->OnRefreshEditor().RemoveAll(this);
		InBlueprint->OnSetObjectBeingDebugged().RemoveAll(this);
		BP->OnTLLRN_ModularRigPreCompiled().RemoveAll(this);
		BP->OnTLLRN_ModularRigCompiled().RemoveAll(this);
		if(UTLLRN_ModularTLLRN_RigController* TLLRN_ModularTLLRN_RigController = BP->GetTLLRN_ModularTLLRN_RigController())
		{
			TLLRN_ModularTLLRN_RigController->OnModified().RemoveAll(this);
		}
	}

	if(const UTLLRN_ModularRig* Rig = GetTLLRN_ModularRigForTreeView())
	{
		if(UTLLRN_RigHierarchy* Hierarchy = Rig->GetHierarchy())
		{
			Hierarchy->OnModified().RemoveAll(this);
		}
	}

	TLLRN_ControlRigEditor.Reset();
	TLLRN_ControlRigBlueprint.Reset();
}

void STLLRN_ModularRigModel::BindCommands()
{
	// create new command
	const FTLLRN_ControlRigTLLRN_ModularRigCommands& Commands = FTLLRN_ControlRigTLLRN_ModularRigCommands::Get();

	CommandList->MapAction(Commands.AddModuleItem,
		FExecuteAction::CreateSP(this, &STLLRN_ModularRigModel::HandleNewItem),
		FCanExecuteAction());

	CommandList->MapAction(Commands.RenameModuleItem,
		FExecuteAction::CreateSP(this, &STLLRN_ModularRigModel::HandleRenameModule),
		FCanExecuteAction());

	CommandList->MapAction(Commands.DeleteModuleItem,
		FExecuteAction::CreateSP(this, &STLLRN_ModularRigModel::HandleDeleteModules),
		FCanExecuteAction());

	CommandList->MapAction(Commands.MirrorModuleItem,
		FExecuteAction::CreateSP(this, &STLLRN_ModularRigModel::HandleMirrorModules),
		FCanExecuteAction());

	CommandList->MapAction(Commands.ReresolveModuleItem,
		FExecuteAction::CreateSP(this, &STLLRN_ModularRigModel::HandleReresolveModules),
		FCanExecuteAction());

	CommandList->MapAction(Commands.SwapModuleClassItem,
		FExecuteAction::CreateSP(this, &STLLRN_ModularRigModel::HandleSwapClassForModules),
		FCanExecuteAction::CreateSP(this, &STLLRN_ModularRigModel::CanSwapModules));
}

FReply STLLRN_ModularRigModel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList.IsValid() && CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply STLLRN_ModularRigModel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FReply Reply = SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
	if(Reply.IsEventHandled())
	{
		return Reply;
	}

	if(MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		if(const TSharedPtr<FTLLRN_ModularRigTreeElement>* ItemPtr = TreeView->FindItemAtPosition(MouseEvent.GetScreenSpacePosition()))
		{
			if(const TSharedPtr<FTLLRN_ModularRigTreeElement>& Item = *ItemPtr)
			{
				if (TLLRN_ControlRigBlueprint.IsValid())
				{
					UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
					check(Controller);

					if(const FTLLRN_RigModuleReference* Module = Controller->FindModule(Item->ModulePath))
					{
						TArray<const FTLLRN_RigModuleReference*> ModulesToSelect = {Module};
						TArray<FString> ModulePaths;
						for(int32 Index = 0; Index < ModulesToSelect.Num(); Index++)
						{
							ModulePaths.Add(ModulesToSelect[Index]->GetPath());
							for(const FTLLRN_RigModuleReference* ChildModule : ModulesToSelect[Index]->CachedChildren)
							{
								ModulesToSelect.AddUnique(ChildModule);
							}
						}
						
						Controller->SetModuleSelection(ModulePaths);
					}
				}
			}
		}
	}

	return FReply::Unhandled();
}

void STLLRN_ModularRigModel::RefreshTreeView(bool bRebuildContent)
{
	bool bDummySuspensionFlag = false;
	bool* SuspensionFlagPtr = &bDummySuspensionFlag;
	if (TLLRN_ControlRigEditor.IsValid())
	{
		SuspensionFlagPtr = &TLLRN_ControlRigEditor.Pin()->GetSuspendDetailsPanelRefreshFlag();
	}
	TGuardValue<bool> SuspendDetailsPanelRefreshGuard(*SuspensionFlagPtr, true);

	TreeView->RefreshTreeView(bRebuildContent);
}

TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> STLLRN_ModularRigModel::GetSelectedItems() const
{
	TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedItems = TreeView->GetSelectedItems();
	SelectedItems.Remove(TSharedPtr<FTLLRN_ModularRigTreeElement>(nullptr));
	return SelectedItems;
}

TArray<FString> STLLRN_ModularRigModel::GetSelectedKeys() const
{
	const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedItems = GetSelectedItems();
	
	TArray<FString> SelectedKeys;
	for (const TSharedPtr<FTLLRN_ModularRigTreeElement>& SelectedItem : SelectedItems)
	{
		if (SelectedItem.IsValid())
		{
			if(!SelectedItem->Key.IsEmpty())
			{
				SelectedKeys.AddUnique(SelectedItem->Key);
			}
		}
	}

	return SelectedKeys;
}


void STLLRN_ModularRigModel::HandlePreCompileTLLRN_ModularRigs(URigVMBlueprint* InBlueprint)
{
}

void STLLRN_ModularRigModel::HandlePostCompileTLLRN_ModularRigs(URigVMBlueprint* InBlueprint)
{
	RefreshTreeView();
	if (TLLRN_ControlRigEditor.IsValid())
	{
		TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedElements;
		Algo::Transform(TLLRN_ControlRigEditor.Pin()->ModulesSelected, SelectedElements, [this](const FString& Path)
		{
			return TreeView->FindElement(Path);
		});
		TreeView->SetSelection(SelectedElements);
		TLLRN_ControlRigEditor.Pin()->RefreshDetailView();
	}
}

void STLLRN_ModularRigModel::HandleRefreshEditorFromBlueprint(URigVMBlueprint* InBlueprint)
{
	RefreshTreeView();
}

void STLLRN_ModularRigModel::HandleSetObjectBeingDebugged(UObject* InObject)
{
	if(TLLRN_ControlRigBeingDebuggedPtr.Get() == InObject)
	{
		return;
	}

	if(TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy())
		{
			Hierarchy->OnModified().RemoveAll(this);
		}
	}

	TLLRN_ControlRigBeingDebuggedPtr.Reset();

	if(UTLLRN_ModularRig* TLLRN_ControlRig = Cast<UTLLRN_ModularRig>(InObject))
	{
		TLLRN_ControlRigBeingDebuggedPtr = TLLRN_ControlRig;

		if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
		{
			Hierarchy->OnModified().AddSP(this, &STLLRN_ModularRigModel::OnHierarchyModified);
		}
	}

	RefreshTreeView();
}

TSharedRef<SWidget> STLLRN_ModularRigModel::OnGetOptionsMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	const FCanExecuteAction CanExecuteAction = FCanExecuteAction::CreateLambda([]() { return true; });

	MenuBuilder.BeginSection("FilterOptions", LOCTEXT("FilterOptions", "Filter Options"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SecondaryConnectors", "Secondary Connectors"),
			LOCTEXT("SecondaryConnectorsToolTip", "Toggle the display of secondary connectors"),
			FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), TEXT("TLLRN_ControlRig.ConnectorSecondary")),
			FUIAction(
				FExecuteAction::CreateLambda([this](){
					bShowSecondaryConnectors = !bShowSecondaryConnectors;
					RefreshTreeView(true);
				}),
				CanExecuteAction,
				FIsActionChecked::CreateLambda([this]()
				{
					return bShowSecondaryConnectors;
				})
			),
			NAME_None,
			EUserInterfaceActionType::Check
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("OptionalConnectors", "Optional Connectors"),
			LOCTEXT("OptionalConnectorsToolTip", "Toggle the display of secondary connectors"),
			FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), TEXT("TLLRN_ControlRig.ConnectorOptional")),
			FUIAction(
				FExecuteAction::CreateLambda([this](){
					bShowOptionalConnectors = !bShowOptionalConnectors;
					RefreshTreeView(true);
				}),
				CanExecuteAction,
				FIsActionChecked::CreateLambda([this]()
				{
					return bShowOptionalConnectors;
				})
			),
			NAME_None,
			EUserInterfaceActionType::Check
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("UnresolvedConnectors", "Unresolved Connectors"),
			LOCTEXT("UnresolvedConnectorsToolTip", "Toggle the display of unresolved connectors"),
			FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), TEXT("TLLRN_ControlRig.ConnectorWarning")),
			FUIAction(
				FExecuteAction::CreateLambda([this](){
					bShowUnresolvedConnectors = !bShowUnresolvedConnectors;
					RefreshTreeView(true);
				}),
				CanExecuteAction,
				FIsActionChecked::CreateLambda([this]()
				{
					return bShowUnresolvedConnectors;
				})
			),
			NAME_None,
			EUserInterfaceActionType::Check
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void STLLRN_ModularRigModel::OnFilterTextChanged(const FText& SearchText)
{
	FilterText = SearchText;
	RefreshTreeView(true);
}

TSharedPtr< SWidget > STLLRN_ModularRigModel::CreateContextMenuWidget()
{
	UToolMenus* ToolMenus = UToolMenus::Get();

	if (UToolMenu* Menu = GetContextMenu())
	{
		return ToolMenus->GenerateWidget(Menu);
	}
	
	return SNullWidget::NullWidget;
}

void STLLRN_ModularRigModel::OnItemClicked(TSharedPtr<FTLLRN_ModularRigTreeElement> InItem)
{
}

void STLLRN_ModularRigModel::OnItemDoubleClicked(TSharedPtr<FTLLRN_ModularRigTreeElement> InItem)
{

}

void STLLRN_ModularRigModel::CreateContextMenu()
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
				
				if (STLLRN_ModularRigModel* ModelPanel = MainContext->GetTLLRN_ModularRigModelPanel())
				{
					const FTLLRN_ControlRigTLLRN_ModularRigCommands& Commands = FTLLRN_ControlRigTLLRN_ModularRigCommands::Get(); 
				
					FToolMenuSection& ModulesSection = InMenu->AddSection(TEXT("Modules"), LOCTEXT("ModulesHeader", "Modules"));
					ModulesSection.AddSubMenu(TEXT("New"), LOCTEXT("New", "New"), LOCTEXT("New_ToolTip", "Create New Modules"),
						FNewToolMenuDelegate::CreateLambda([Commands, ModelPanel](UToolMenu* InSubMenu)
						{
							FToolMenuSection& DefaultSection = InSubMenu->AddSection(NAME_None);
							DefaultSection.AddMenuEntry(Commands.AddModuleItem);
						})
					);
					ModulesSection.AddMenuEntry(Commands.RenameModuleItem);
					ModulesSection.AddMenuEntry(Commands.DeleteModuleItem);
					ModulesSection.AddMenuEntry(Commands.MirrorModuleItem);
					ModulesSection.AddMenuEntry(Commands.ReresolveModuleItem);
					ModulesSection.AddMenuEntry(Commands.SwapModuleClassItem);
				}
			})
		);
	}
}

UToolMenu* STLLRN_ModularRigModel::GetContextMenu()
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
	MenuSpecificContext.TLLRN_ModularRigModelPanel = SharedThis(this);
	ContextMenuContext->Init(TLLRN_ControlRigEditor, MenuSpecificContext);

	FToolMenuContext MenuContext(CommandList);
	MenuContext.AddObject(ContextMenuContext);

	UToolMenu* Menu = ToolMenus->GenerateMenu(MenuName, MenuContext);

	return Menu;
}

TSharedPtr<FUICommandList> STLLRN_ModularRigModel::GetContextMenuCommands() const
{
	return CommandList;
}

bool STLLRN_ModularRigModel::IsSingleSelected() const
{
	if(GetSelectedKeys().Num() == 1)
	{
		return true;
	}
	return false;
}

/** Filter class to show only TLLRN_RigModules. */
class FClassViewerTLLRN_RigModulesFilter : public IClassViewerFilter
{
public:
	FClassViewerTLLRN_RigModulesFilter()
		: AssetRegistry(FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get())
	{}
	
	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		if(InClass)
		{
			const bool bChildOfObjectClass = InClass->IsChildOf(UTLLRN_ControlRig::StaticClass());
			const bool bMatchesFlags = !InClass->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated | CLASS_Abstract);
			const bool bNotNative = !InClass->IsNative();

			// Allow any class contained in the extra picker common classes array
			if (InInitOptions.ExtraPickerCommonClasses.Contains(InClass))
			{
				return true;
			}
			
			if (bChildOfObjectClass && bMatchesFlags && bNotNative)
			{
				const FAssetData AssetData(InClass);
				return MatchesFilter(AssetData);
			}
		}
		return false;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		const bool bChildOfObjectClass = InUnloadedClassData->IsChildOf(UTLLRN_ControlRig::StaticClass());
		const bool bMatchesFlags = !InUnloadedClassData->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated | CLASS_Abstract);
		if (bChildOfObjectClass && bMatchesFlags)
		{
			const FString GeneratedClassPathString = InUnloadedClassData->GetClassPathName().ToString();
			const FString BlueprintPath = GeneratedClassPathString.LeftChop(2); // Chop off _C
			const FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(BlueprintPath));
			return MatchesFilter(AssetData);

		}
		return false;
	}

private:
	bool MatchesFilter(const FAssetData& AssetData)
	{
		static const UEnum* ControlTypeEnum = StaticEnum<ETLLRN_ControlRigType>();
		const FString TLLRN_ControlRigTypeStr = AssetData.GetTagValueRef<FString>(TEXT("TLLRN_ControlRigType"));
		if (TLLRN_ControlRigTypeStr.IsEmpty())
		{
			return false;
		}

		const ETLLRN_ControlRigType TLLRN_ControlRigType = (ETLLRN_ControlRigType)(ControlTypeEnum->GetValueByName(*TLLRN_ControlRigTypeStr));
		return TLLRN_ControlRigType == ETLLRN_ControlRigType::TLLRN_RigModule;
	}

	const IAssetRegistry& AssetRegistry;
};

/** Create Item */
void STLLRN_ModularRigModel::HandleNewItem()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	FString ParentPath;
	if (IsSingleSelected())
	{
		const TSharedPtr<FTLLRN_ModularRigTreeElement> ParentElement = TreeView->FindElement(GetSelectedKeys()[0]);
		if (ParentElement.IsValid())
		{
			ParentPath = ParentElement->ModulePath;
		}
	}
	
	FClassViewerInitializationOptions Options;
	Options.bShowUnloadedBlueprints = true;
	Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;

	TSharedPtr<FClassViewerTLLRN_RigModulesFilter> ClassFilter = MakeShareable(new FClassViewerTLLRN_RigModulesFilter());
	Options.ClassFilters.Add(ClassFilter.ToSharedRef());
	Options.bShowNoneOption = false;
	
	UClass* ChosenClass;
	const FText TitleText = LOCTEXT("TLLRN_ModularRigModelPickModuleClass", "Pick Rig Module Class");
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UTLLRN_ControlRig::StaticClass());
	if (bPressedOk)
	{
		HandleNewItem(ChosenClass, ParentPath);
	}
}

void STLLRN_ModularRigModel::HandleNewItem(UClass* InClass, const FString &InParentPath)
{
	UTLLRN_ControlRig* TLLRN_ControlRig = InClass->GetDefaultObject<UTLLRN_ControlRig>();
	if (!TLLRN_ControlRig)
	{
		return;
	}

	FSlateApplication::Get().DismissAllMenus();
	
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
		
		FString ClassName = InClass->GetName();
		ClassName.RemoveFromEnd(TEXT("_C"));
		const FTLLRN_RigName Name = Controller->GetSafeNewName(InParentPath, FTLLRN_RigName(ClassName));
		const FString NewPath = Controller->AddModule(Name, InClass, InParentPath);
		TSharedPtr<FTLLRN_ModularRigTreeElement> Element = TreeView->FindElement(NewPath);
		if (Element.IsValid())
		{
			TreeView->SetSelection({Element});
			TreeView->bRequestRenameSelected = true;
		}
	}
}

bool STLLRN_ModularRigModel::CanRenameModule() const
{
	return IsSingleSelected() && TreeView->FindElement(GetSelectedKeys()[0])->bIsPrimary;
}

void STLLRN_ModularRigModel::HandleRenameModule()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	if (!CanRenameModule())
	{
		return;
	}

	UTLLRN_ModularRig* Rig = GetDefaultTLLRN_ModularRig();
	if (Rig)
	{
		FScopedTransaction Transaction(LOCTEXT("TLLRN_ModularRigModelRenameSelected", "Rename selected module"));

		const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedItems = GetSelectedItems();
		if (SelectedItems.Num() == 1)
		{
			SelectedItems[0]->RequestRename();
		}
	}

	return;
}

FName STLLRN_ModularRigModel::HandleRenameModule(const FString& InOldPath, const FName& InNewName)
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		FScopedTransaction Transaction(LOCTEXT("TLLRN_ModularRigModelRename", "Rename Module"));

		UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
		check(Controller);

		if (!Controller->RenameModule(InOldPath, InNewName).IsEmpty())
		{
			return InNewName;
		}
	}

	return NAME_None;
}

bool STLLRN_ModularRigModel::HandleVerifyNameChanged(const FString& InOldPath, const FName& InNewName, FText& OutErrorMessage)
{
	if (InNewName.IsNone())
	{
		return false;
	}

	FString ParentPath;
	FString OldName = InOldPath;
	(void)UTLLRN_RigHierarchy::SplitNameSpace(InOldPath, &ParentPath, &OldName);

	if (InNewName == OldName)
	{
		return true;
	}
	
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
		check(Controller);

		return Controller->CanRenameModule(InOldPath, InNewName, OutErrorMessage);
	}

	return false;
}

void STLLRN_ModularRigModel::HandleDeleteModules()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	UTLLRN_ModularRig* Rig = GetDefaultTLLRN_ModularRig();
	if (Rig)
	{
		FScopedTransaction Transaction(LOCTEXT("TLLRN_ModularRigModelDeleteSelected", "Delete selected modules"));

		const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedItems = GetSelectedItems();
		TArray<FString> SelectedPaths;
		Algo::Transform(SelectedItems, SelectedPaths, [](const TSharedPtr<FTLLRN_ModularRigTreeElement>& Element)
		{
			if (Element.IsValid())
			{
				return Element->ModulePath;
			}
			return FString();
		});
		HandleDeleteModules(SelectedPaths);
	}

	return;
}

void STLLRN_ModularRigModel::HandleDeleteModules(const TArray<FString>& InPaths)
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		FScopedTransaction Transaction(LOCTEXT("TLLRN_ModularRigModelDelete", "Delete Modules"));

		UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
		check(Controller);

		// Make sure we delete the modules from children to root
		TArray<FString> SortedPaths = Controller->Model->SortPaths(InPaths);
		Algo::Reverse(SortedPaths);
		for (const FString& Path : SortedPaths)
		{
			Controller->DeleteModule(Path);
		}
	}
}

void STLLRN_ModularRigModel::HandleReparentModules(const TArray<FString>& InPaths, const FString& InParentPath)
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		FScopedTransaction Transaction(LOCTEXT("TLLRN_ModularRigModelReparent", "Reparent Modules"));

		UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
		check(Controller);

		for (const FString& Path : InPaths)
		{
			Controller->ReparentModule(Path, InParentPath);
		}
	}
}

void STLLRN_ModularRigModel::HandleMirrorModules()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	UTLLRN_ModularRig* Rig = GetDefaultTLLRN_ModularRig();
	if (Rig)
	{
		const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedItems = GetSelectedItems();
		TArray<FString> SelectedPaths;
		Algo::Transform(SelectedItems, SelectedPaths, [](const TSharedPtr<FTLLRN_ModularRigTreeElement>& Element)
		{
			if (Element.IsValid())
			{
				return Element->ModulePath;
			}
			return FString();
		});
		HandleMirrorModules(SelectedPaths);
	}
}

void STLLRN_ModularRigModel::HandleMirrorModules(const TArray<FString>& InPaths)
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		FRigVMMirrorSettings Settings;
		TSharedPtr<FStructOnScope> StructToDisplay = MakeShareable(new FStructOnScope(FRigVMMirrorSettings::StaticStruct(), (uint8*)&Settings));
		
		TSharedRef<SKismetInspector> KismetInspector = SNew(SKismetInspector);
		KismetInspector->ShowSingleStruct(StructToDisplay);
		
		TSharedRef<SCustomDialog> MirrorDialog = SNew(SCustomDialog)
			.Title(FText(LOCTEXT("ControlModularModelMirror", "Mirror Selected Modules")))
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
			FScopedTransaction Transaction(LOCTEXT("TLLRN_ModularRigModelMirror", "Mirror Modules"));

			UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
			check(Controller);

			// Make sure we mirror the modules from root to children
			TArray<FString> SortedPaths = Controller->Model->SortPaths(InPaths);
			for (const FString& Path : SortedPaths)
			{
				Controller->MirrorModule(Path, Settings);
			}
		}
	}
}

void STLLRN_ModularRigModel::HandleReresolveModules()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	UTLLRN_ModularRig* Rig = GetDefaultTLLRN_ModularRig();
	if (Rig)
	{
		const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedItems = GetSelectedItems();
		TArray<FString> SelectedPaths;
		Algo::Transform(SelectedItems, SelectedPaths, [](const TSharedPtr<FTLLRN_ModularRigTreeElement>& Element)
		{
			if (Element.IsValid())
			{
				if(Element->ConnectorName.IsEmpty())
				{
					return Element->ModulePath;
				}
				return FString::Printf(TEXT("%s|%s"), *Element->ModulePath, *Element->ConnectorName);
			}
			return FString();
		});
		HandleReresolveModules(SelectedPaths);
	}
}

void STLLRN_ModularRigModel::HandleReresolveModules(const TArray<FString>& InPaths)
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
		check(Controller);

		const UTLLRN_ModularRig* Rig = GetDefaultTLLRN_ModularRig();
		if (Rig == nullptr)
		{
			return;
		}

		const UTLLRN_RigHierarchy* Hierarchy = Rig->GetHierarchy();
		if(Hierarchy == nullptr)
		{
			return;
		}

		TArray<FTLLRN_RigElementKey> ConnectorKeys;
		for (const FString& PathAndConnector : InPaths)
		{
			FString ModulePath = PathAndConnector;
			FString ConnectorName;
			(void)PathAndConnector.Split(TEXT("|"), &ModulePath, &ConnectorName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

			const FTLLRN_RigModuleReference* Module = Controller->Model->FindModule(ModulePath);
			if (!Module)
			{
				UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Could not find module %s"), *ModulePath);
				return;
			}

			if(!ConnectorName.IsEmpty())
			{
				// if we are executing this on a primary connector we want to re-resolve all secondaries
				const FTLLRN_RigConnectorElement* PrimaryConnector = Module->FindPrimaryConnector(Hierarchy);
				const FName DesiredName = Hierarchy->GetNameMetadata(PrimaryConnector->GetKey(), UTLLRN_RigHierarchy::DesiredNameMetadataName, NAME_None);
				if(!DesiredName.IsNone() && DesiredName.ToString().Equals(ConnectorName, ESearchCase::IgnoreCase))
				{
					ConnectorName.Reset();
				}
			}

			const TArray<const FTLLRN_RigConnectorElement*> Connectors = Module->FindConnectors(Hierarchy);
			for(const FTLLRN_RigConnectorElement* Connector : Connectors)
			{
				if(Connector->IsSecondary())
				{
					if(ConnectorName.IsEmpty())
					{
						ConnectorKeys.AddUnique(Connector->GetKey());
					}
					else
					{
						const FName DesiredName = Hierarchy->GetNameMetadata(Connector->GetKey(), UTLLRN_RigHierarchy::DesiredNameMetadataName, NAME_None);
						if(!DesiredName.IsNone() && DesiredName.ToString().Equals(ConnectorName, ESearchCase::IgnoreCase))
						{
							ConnectorKeys.AddUnique(Connector->GetKey());
							break;
						}
					}
				}
			}
		}

		Controller->AutoConnectSecondaryConnectors(ConnectorKeys, true, true);
	}
}

bool STLLRN_ModularRigModel::CanSwapModules() const
{
	// Only if all modules selected have the same module class
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return false;
	}

	UTLLRN_ModularRig* Rig = GetDefaultTLLRN_ModularRig();
	if (Rig)
	{
		TSoftClassPtr<UTLLRN_ControlRig> CommonClass = nullptr;
		const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedItems = GetSelectedItems();
		for (const TSharedPtr<FTLLRN_ModularRigTreeElement>& SelectedItem : SelectedItems)
		{
			if(!SelectedItem.IsValid())
			{
				continue;
			}
			TSoftClassPtr<UTLLRN_ControlRig> ModuleClass;
			if (const FTLLRN_RigModuleReference* Module = TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.FindModule(SelectedItem->ModulePath))
			{
				if (Module->Class.IsValid())
				{
					ModuleClass = Module->Class;
				}
			}
			if(!ModuleClass)
			{
				return false;
			}
			if(!CommonClass)
			{
				CommonClass = ModuleClass;
			}
			if(ModuleClass != CommonClass)
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

void STLLRN_ModularRigModel::HandleSwapClassForModules()
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}

	UTLLRN_ModularRig* Rig = GetDefaultTLLRN_ModularRig();
	if (Rig)
	{
		const TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> SelectedItems = GetSelectedItems();
		TArray<FString> SelectedPaths;
		Algo::Transform(SelectedItems, SelectedPaths, [](const TSharedPtr<FTLLRN_ModularRigTreeElement>& Element)
		{
			if (Element.IsValid())
			{
				return Element->ModulePath;
			}
			return FString();
		});
		HandleSwapClassForModules(SelectedPaths);
	}
}

void STLLRN_ModularRigModel::HandleSwapClassForModules(const TArray<FString>& InPaths)
{
	TArray<FSoftObjectPath> ModulePaths;
	Algo::Transform(InPaths, ModulePaths, [this](const FString& Path)
	{
		FSoftObjectPath ModulePath(TLLRN_ControlRigBlueprint->GetPathName());
		ModulePath.SetSubPathString(Path);
		return ModulePath;
	});

	TSoftClassPtr<UTLLRN_ControlRig> SourceClass = nullptr;
	if (FTLLRN_RigModuleReference* Module = TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.FindModule(InPaths[0]))
	{
		SourceClass = Module->Class;
	}

	if (!SourceClass)
	{
		return;
	}

	const FAssetData SourceAsset = UE::RigVM::Editor::Tools::FindAssetFromAnyPath(SourceClass.GetLongPackageName(), true);
	
	SRigVMSwapAssetReferencesWidget::FArguments WidgetArgs;
	FRigVMAssetDataFilter FilterModules = FRigVMAssetDataFilter::CreateLambda([](const FAssetData& AssetData)
		{
			return UTLLRN_ControlRigBlueprint::GetRigType(AssetData) == ETLLRN_ControlRigType::TLLRN_RigModule;
		});
	TArray<FRigVMAssetDataFilter> SourceFilters = {FilterModules};
	TArray<FRigVMAssetDataFilter> TargetFilters = {FilterModules};
	
	WidgetArgs
		.EnableUndo(true)
		.CloseOnSuccess(true)
		.Source(SourceAsset)
		.ReferencePaths(ModulePaths)
		.SkipPickingRefs(true)
		.OnSwapReference_Lambda([](const FSoftObjectPath& ModulePath, const FAssetData& NewModuleAsset) -> bool
		{
			TSubclassOf<UTLLRN_ControlRig> NewModuleClass = nullptr;
			if (const UTLLRN_ControlRigBlueprint* ModuleBlueprint = Cast<UTLLRN_ControlRigBlueprint>(NewModuleAsset.GetAsset()))
			{
				NewModuleClass = ModuleBlueprint->GetRigVMBlueprintGeneratedClass();
			}
			if (NewModuleClass)
			{
				if (UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(ModulePath.GetWithoutSubPath().ResolveObject()))
				{
					return RigBlueprint->GetTLLRN_ModularTLLRN_RigController()->SwapModuleClass(ModulePath.GetSubPathString(), NewModuleClass);
				}
			}
			return false;
		})
		.SourceAssetFilters(SourceFilters)
		.TargetAssetFilters(TargetFilters);

	const TSharedRef<SRigVMBulkEditDialog<SRigVMSwapAssetReferencesWidget>> SwapModulesDialog =
		SNew(SRigVMBulkEditDialog<SRigVMSwapAssetReferencesWidget>)
		.WindowSize(FVector2D(800.0f, 640.0f))
		.WidgetArgs(WidgetArgs);
	
	SwapModulesDialog->ShowNormal();
}

void STLLRN_ModularRigModel::HandleConnectorResolved(const FTLLRN_RigElementKey& InConnector, const FTLLRN_RigElementKey& InTarget)
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		FScopedTransaction Transaction(LOCTEXT("TLLRN_ModularRigModelResolveConnector", "Resolve Connector"));

		UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
		check(Controller);

		if (const UTLLRN_ModularRig* TLLRN_ModularRig = GetTLLRN_ModularRig())
		{
			Controller->ConnectConnectorToElement(InConnector, InTarget, true, TLLRN_ModularRig->GetTLLRN_ModularRigSettings().bAutoResolve);
		}
	}
}

void STLLRN_ModularRigModel::HandleConnectorDisconnect(const FTLLRN_RigElementKey& InConnector)
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		FScopedTransaction Transaction(LOCTEXT("TLLRN_ModularRigModelDisconnectConnector", "Disconnect Connector"));

		UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
		check(Controller);

		Controller->DisconnectConnector(InConnector, false, true);
	}
}

void STLLRN_ModularRigModel::HandleSelectionChanged(TSharedPtr<FTLLRN_ModularRigTreeElement> Selection, ESelectInfo::Type SelectInfo)
{
	if(bIsPerformingSelection)
	{
		return;
	}

	TreeView->ClearHighlightedItems();
	
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController();
		check(Controller);

		const TGuardValue<bool> GuardSelection(bIsPerformingSelection, true);
		const TArray<FString> NewSelection = TreeView->GetSelectedKeys();
		Controller->SetModuleSelection(NewSelection);
	}
}

void STLLRN_ModularRigModel::OnTLLRN_ModularRigModified(ETLLRN_ModularRigNotification InNotif, const FTLLRN_RigModuleReference* InModule)
{
	if (!TLLRN_ControlRigBlueprint.IsValid())
	{
		return;
	}

	switch(InNotif)
	{
		case ETLLRN_ModularRigNotification::ModuleSelected:
		case ETLLRN_ModularRigNotification::ModuleDeselected:
		{
			if(!bIsPerformingSelection)
			{
				const TGuardValue<bool> GuardSelection(bIsPerformingSelection, true);
				if(const UTLLRN_ModularTLLRN_RigController* TLLRN_ModularTLLRN_RigController = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
				{
					const TArray<FString> SelectedModulePaths = TLLRN_ModularTLLRN_RigController->GetSelectedModules();
					TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> NewSelection;
					for(const FString& SelectedModulePath : SelectedModulePaths)
					{
						if(const TSharedPtr<FTLLRN_ModularRigTreeElement> Module = TreeView->FindElement(SelectedModulePath))
						{
							NewSelection.Add(Module);
						}
					}
					TreeView->SetSelection(NewSelection);
				}
			}
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleAdded:
		case ETLLRN_ModularRigNotification::ModuleRenamed:
		case ETLLRN_ModularRigNotification::ModuleRemoved:
		case ETLLRN_ModularRigNotification::ModuleReparented:
		case ETLLRN_ModularRigNotification::ConnectionChanged:
		case ETLLRN_ModularRigNotification::ModuleConfigValueChanged:
		case ETLLRN_ModularRigNotification::ModuleShortNameChanged:
		case ETLLRN_ModularRigNotification::ModuleClassChanged:
		{
			TreeView->RefreshTreeView();
			break;
		}
		default:
		{
			break;
		}
	}
}

void STLLRN_ModularRigModel::OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	if(!TLLRN_ControlRigBlueprint.IsValid())
	{
		return;
	}
	
	switch(InNotif)
	{
		case ETLLRN_RigHierarchyNotification::ElementSelected:
		case ETLLRN_RigHierarchyNotification::ElementDeselected:
		{
			FString ModulePathOrConnectorName = InHierarchy->GetModulePath(InElement->GetKey());

			if(const FTLLRN_RigConnectorElement* Connector = Cast<FTLLRN_RigConnectorElement>(InElement))
			{
				if(Connector->IsPrimary())
            	{
					ModulePathOrConnectorName = Connector->GetName();
            	}
			}

			if(!ModulePathOrConnectorName.IsEmpty())
			{
				if(TSharedPtr<FTLLRN_ModularRigTreeElement> Item = TreeView->FindElement(ModulePathOrConnectorName))
				{
					const bool bSelected = InNotif == ETLLRN_RigHierarchyNotification::ElementSelected;
					TreeView->SetItemHighlighted(Item, bSelected);
					TreeView->RequestScrollIntoView(Item);
				}
			}
		}
		default:
		{
			break;
		}
	}
}

class STLLRN_ModularRigModelPasteTransformsErrorPipe : public FOutputDevice
{
public:

	int32 NumErrors;

	STLLRN_ModularRigModelPasteTransformsErrorPipe()
		: FOutputDevice()
		, NumErrors(0)
	{
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
	{
		UE_LOG(LogTLLRN_ControlRig, Error, TEXT("Error importing transforms to Model: %s"), V);
		NumErrors++;
	}
};

UTLLRN_ModularRig* STLLRN_ModularRigModel::GetTLLRN_ModularRig() const
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		if (UTLLRN_ControlRig* DebuggedRig = TLLRN_ControlRigBeingDebuggedPtr.Get())
		{
			return Cast<UTLLRN_ModularRig>(DebuggedRig);
		}
		if(UTLLRN_ControlRig* DebuggedRig = TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig())
		{
			return Cast<UTLLRN_ModularRig>(DebuggedRig);
		}
	}
	if (TLLRN_ControlRigEditor.IsValid())
	{
		if (UTLLRN_ControlRig* CurrentRig = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRig())
		{
			return Cast<UTLLRN_ModularRig>(CurrentRig);
		}
	}
	return nullptr;
}

UTLLRN_ModularRig* STLLRN_ModularRigModel::GetDefaultTLLRN_ModularRig() const
{
	if (TLLRN_ControlRigBlueprint.IsValid())
	{
		UTLLRN_ControlRig* DebuggedRig = TLLRN_ControlRigBeingDebuggedPtr.Get();
		if (!DebuggedRig)
		{
			DebuggedRig = TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig();
		}
		
		if (DebuggedRig)
		{
			return Cast<UTLLRN_ModularRig>(DebuggedRig);
		}
	}
	return nullptr;
}


void STLLRN_ModularRigModel::OnRequestDetailsInspection(const FString& InKey)
{
	if(!TLLRN_ControlRigEditor.IsValid())
	{
		return;
	}
	TLLRN_ControlRigEditor.Pin()->SetDetailViewForTLLRN_RigModules({InKey});
}

void STLLRN_ModularRigModel::PostRedo(bool bSuccess) 
{
	if (bSuccess)
	{
		RefreshTreeView();
	}
}

void STLLRN_ModularRigModel::PostUndo(bool bSuccess) 
{
	if (bSuccess)
	{
		RefreshTreeView();
	}
}

FReply STLLRN_ModularRigModel::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TArray<FString> DraggedElements = GetSelectedKeys();
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && DraggedElements.Num() > 0)
	{
		if (TLLRN_ControlRigEditor.IsValid())
		{
			TSharedRef<FTLLRN_ModularTLLRN_RigModuleDragDropOp> DragDropOp = FTLLRN_ModularTLLRN_RigModuleDragDropOp::New(MoveTemp(DraggedElements));
			return FReply::Handled().BeginDragDrop(DragDropOp);
		}
	}

	return FReply::Unhandled();
}

TOptional<EItemDropZone> STLLRN_ModularRigModel::OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FTLLRN_ModularRigTreeElement> TargetItem)
{
	const TOptional<EItemDropZone> InvalidDropZone;
	TOptional<EItemDropZone> ReturnDropZone = DropZone;

	if(DropZone == EItemDropZone::BelowItem && !TargetItem.IsValid())
	{
		DropZone = EItemDropZone::OntoItem;
	}
	
	if(DropZone != EItemDropZone::OntoItem)
	{
		return InvalidDropZone;
	}

	TSharedPtr<FAssetDragDropOp> AssetDragDropOperation = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	TSharedPtr<FTLLRN_ModularTLLRN_RigModuleDragDropOp> ModuleDragDropOperation = DragDropEvent.GetOperationAs<FTLLRN_ModularTLLRN_RigModuleDragDropOp>();
	if (AssetDragDropOperation)
	{
		for (const FAssetData& AssetData : AssetDragDropOperation->GetAssets())
		{
			static const UEnum* ControlTypeEnum = StaticEnum<ETLLRN_ControlRigType>();
			const FString TLLRN_ControlRigTypeStr = AssetData.GetTagValueRef<FString>(TEXT("TLLRN_ControlRigType"));
			if (TLLRN_ControlRigTypeStr.IsEmpty())
			{
				ReturnDropZone.Reset();
				break;
			}

			const ETLLRN_ControlRigType TLLRN_ControlRigType = (ETLLRN_ControlRigType)(ControlTypeEnum->GetValueByName(*TLLRN_ControlRigTypeStr));
			if (TLLRN_ControlRigType != ETLLRN_ControlRigType::TLLRN_RigModule)
			{
				ReturnDropZone.Reset();
				break;
			}
		}
	}
	else if(ModuleDragDropOperation)
	{
		if(TargetItem.IsValid())
		{
			// we cannot drag a module onto itself
			if(ModuleDragDropOperation->GetElements().Contains(TargetItem->ModulePath))
			{
				return InvalidDropZone;
			}
		}
	}
	else
	{
		ReturnDropZone.Reset();
	}

	return ReturnDropZone;
}

FReply STLLRN_ModularRigModel::OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FTLLRN_ModularRigTreeElement> TargetItem)
{
	FString ParentPath;
	if (TargetItem.IsValid())
	{
		ParentPath = TargetItem->ModulePath;
	}

	TSharedPtr<FAssetDragDropOp> AssetDragDropOperation = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	TSharedPtr<FTLLRN_ModularTLLRN_RigModuleDragDropOp> ModuleDragDropOperation = DragDropEvent.GetOperationAs<FTLLRN_ModularTLLRN_RigModuleDragDropOp>();
	if (AssetDragDropOperation)
	{
		for (const FAssetData& AssetData : AssetDragDropOperation->GetAssets())
		{
			static const UEnum* ControlTypeEnum = StaticEnum<ETLLRN_ControlRigType>();
			const FString TLLRN_ControlRigTypeStr = AssetData.GetTagValueRef<FString>(TEXT("TLLRN_ControlRigType"));
			if (TLLRN_ControlRigTypeStr.IsEmpty())
			{
				continue;
			}

			const ETLLRN_ControlRigType TLLRN_ControlRigType = (ETLLRN_ControlRigType)(ControlTypeEnum->GetValueByName(*TLLRN_ControlRigTypeStr));
			if (TLLRN_ControlRigType != ETLLRN_ControlRigType::TLLRN_RigModule)
			{
				continue;
			}

			UClass* AssetClass = AssetData.GetClass();
			if (!AssetClass->IsChildOf(UTLLRN_ControlRigBlueprint::StaticClass()))
			{
				continue;
			}

			if(UTLLRN_ControlRigBlueprint* AssetBlueprint = Cast<UTLLRN_ControlRigBlueprint>(AssetData.GetAsset()))
			{
				HandleNewItem(AssetBlueprint->GetTLLRN_ControlRigClass(), ParentPath);
			}
		}

		FReply::Handled();
	}
	else if(ModuleDragDropOperation)
	{
		const TArray<FString> Paths = ModuleDragDropOperation->GetElements();
		HandleReparentModules(Paths, ParentPath);
	}
	
	return FReply::Unhandled();
}

FReply STLLRN_ModularRigModel::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// only allow drops onto empty space of the widget (when there's no target item under the mouse)
	// when dropped onto an item STLLRN_ModularRigModel::OnAcceptDrop will deal with the event
	const TSharedPtr<FTLLRN_ModularRigTreeElement>* ItemAtMouse = TreeView->FindItemAtPosition(DragDropEvent.GetScreenSpacePosition());
	FString ParentPath;
	if (ItemAtMouse && ItemAtMouse->IsValid())
	{
		return SCompoundWidget::OnDrop(MyGeometry, DragDropEvent);
	}
	
	if (OnCanAcceptDrop(DragDropEvent, EItemDropZone::BelowItem, nullptr))
	{
		if (OnAcceptDrop(DragDropEvent, EItemDropZone::BelowItem, nullptr).IsEventHandled())
		{
			return FReply::Handled();
		}
	}
	return SCompoundWidget::OnDrop(MyGeometry, DragDropEvent);
}

#undef LOCTEXT_NAMESPACE

