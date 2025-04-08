// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/TLLRN_ControlRigEditor.h"
#include "Modules/ModuleManager.h"
#include "TLLRN_ControlRigEditorModule.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "SBlueprintEditorToolbar.h"
#include "Editor/TLLRN_ControlRigEditorMode.h"
#include "SKismetInspector.h"
#include "SEnumCombo.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/Commands/GenericCommands.h"
#include "Editor.h"
#include "Editor/Transactor.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "BlueprintActionDatabase.h"
#include "TLLRN_ControlRigEditorCommands.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "IPersonaToolkit.h"
#include "PersonaModule.h"
#include "Editor/TLLRN_ControlRigEditorEditMode.h"
#include "EditMode/TLLRN_ControlRigEditModeSettings.h"
#include "EditorModeManager.h"
#include "RigVMBlueprintGeneratedClass.h"
#include "AnimCustomInstanceHelper.h"
#include "Sequencer/TLLRN_ControlRigLayerInstance.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "IPersonaPreviewScene.h"
#include "Animation/AnimData/BoneMaskFilter.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ModularRig.h"
#include "Editor/TLLRN_ControlRigSkeletalMeshComponent.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "RigVMBlueprintUtils.h"
#include "EditorViewportClient.h"
#include "AnimationEditorPreviewActor.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "Editor/RigVMEditorStyle.h"
#include "EditorFontGlyphs.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Editor/STLLRN_RigHierarchy.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Units/Hierarchy/TLLRN_RigUnit_BoneName.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetRelativeTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetRelativeTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_OffsetTransform.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetControlTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetControlTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_ControlChannel.h"
#include "Units/Execution/TLLRN_RigUnit_Collection.h"
#include "Units/Highlevel/Hierarchy/TLLRN_RigUnit_TransformConstraint.h"
#include "Units/Hierarchy/TLLRN_RigUnit_GetControlTransform.h"
#include "Units/Hierarchy/TLLRN_RigUnit_SetCurveValue.h"
#include "Units/Hierarchy/TLLRN_RigUnit_AddBoneTransform.h"
#include "EdGraph/NodeSpawners/RigVMEdGraphUnitNodeSpawner.h"
#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "TLLRN_ControlRigObjectVersion.h"
#include "EdGraphUtilities.h"
#include "EdGraphNode_Comment.h"
#include "HAL/PlatformApplicationMisc.h"
#include "SNodePanel.h"
#include "SMyBlueprint.h"
#include "SBlueprintEditorSelectedDebugObjectWidget.h"
#include "Exporters/Exporter.h"
#include "UnrealExporter.h"
#include "TLLRN_ControlTLLRN_RigElementDetails.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "Settings/TLLRN_ControlRigSettings.h"
#include "Widgets/Docking/SDockTab.h"
#include "BlueprintCompilationManager.h"
#include "AssetEditorModeManager.h"
#include "IPersonaEditorModeManager.h"
#include "BlueprintEditorTabs.h"
#include "RigVMModel/Nodes/RigVMFunctionEntryNode.h"
#include "RigVMModel/Nodes/RigVMFunctionReturnNode.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "IMessageLogListing.h"
#include "Widgets/SRigVMGraphFunctionLocalizationWidget.h"
#include "Widgets/SRigVMGraphFunctionBulkEditWidget.h"
#include "Widgets/SRigVMGraphBreakLinksWidget.h"
#include "Widgets/SRigVMGraphChangePinType.h"
#include "SGraphPanel.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "RigVMFunctions/Execution/RigVMFunction_Sequence.h"
#include "Editor/TLLRN_ControlRigContextMenuContext.h"
#include "Types/ISlateMetaData.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "Kismet2/WatchedPin.h"
#include "ToolMenus.h"
#include "Styling/AppStyle.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "MaterialDomain.h"
#include "RigVMFunctions/RigVMFunction_ControlFlow.h"
#include "RigVMModel/Nodes/RigVMAggregateNode.h"
#include "AnimationEditorViewportClient.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Editor/RigVMEditorTools.h"
#include "SchematicGraphPanel/SSchematicGraphPanel.h"
#include "RigVMCore/RigVMExecuteContext.h"
#include "Editor/RigVMGraphDetailCustomization.h"
#include "Widgets/SRigVMSwapAssetReferencesWidget.h"
#include "Widgets/SRigVMBulkEditDialog.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigEditor"

TAutoConsoleVariable<bool> CVarTLLRN_ControlRigShowTestingToolbar(TEXT("TLLRN_ControlRig.Test.EnableTestingToolbar"), false, TEXT("When true we'll show the testing toolbar in Control Rig Editor."));
TAutoConsoleVariable<bool> CVarShowSchematicPanelOverlay(TEXT("TLLRN_ControlRig.Preview.ShowSchematicPanelOverlay"), true, TEXT("When true we'll add an overlay to the persona viewport to show modular rig information."));

const FName FTLLRN_ControlRigEditorModes::TLLRN_ControlRigEditorMode = TEXT("Rigging");
const TArray<FName> FTLLRN_ControlRigEditor::ForwardsSolveEventQueue = {FTLLRN_RigUnit_BeginExecution::EventName};
const TArray<FName> FTLLRN_ControlRigEditor::BackwardsSolveEventQueue = {FTLLRN_RigUnit_InverseExecution::EventName};
const TArray<FName> FTLLRN_ControlRigEditor::ConstructionEventQueue = {FTLLRN_RigUnit_PrepareForExecution::EventName};
const TArray<FName> FTLLRN_ControlRigEditor::BackwardsAndForwardsSolveEventQueue = {FTLLRN_RigUnit_InverseExecution::EventName, FTLLRN_RigUnit_BeginExecution::EventName};

FTLLRN_ControlRigEditor::FTLLRN_ControlRigEditor()
	: ITLLRN_ControlRigEditor()
	, PreviewInstance(nullptr)
	, ActiveController(nullptr)
	, bExecutionTLLRN_ControlRig(true)
	, TLLRN_RigHierarchyTabCount(0)
	, TLLRN_ModularTLLRN_RigHierarchyTabCount(0)
	, bIsConstructionEventRunning(false)
	, LastHierarchyHash(INDEX_NONE)
	, bRefreshDirectionManipulationTargetsRequired(false)
	, bSchematicViewPortIsHidden(false)
{
	LastEventQueue = ConstructionEventQueue;
}

FTLLRN_ControlRigEditor::~FTLLRN_ControlRigEditor()
{
	UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint();

	if (RigBlueprint)
	{
		UTLLRN_ControlRigBlueprint::sCurrentlyOpenedRigBlueprints.Remove(RigBlueprint);

		RigBlueprint->OnHierarchyModified().RemoveAll(this);
		if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
		{
			RigBlueprint->OnHierarchyModified().RemoveAll(EditMode);
			EditMode->OnEditorClosed();
		}

		RigBlueprint->OnRigTypeChanged().RemoveAll(this);
		if (RigBlueprint->IsTLLRN_ModularRig())
		{
			RigBlueprint->GetTLLRN_ModularTLLRN_RigController()->OnModified().RemoveAll(this);
			RigBlueprint->OnTLLRN_ModularRigCompiled().RemoveAll(this);

			RigBlueprint->OnSetObjectBeingDebugged().RemoveAll(&SchematicModel);
			RigBlueprint->OnHierarchyModified().RemoveAll(&SchematicModel);
			RigBlueprint->GetTLLRN_ModularTLLRN_RigController()->OnModified().RemoveAll(&SchematicModel);
		}
	}

	if (PersonaToolkit.IsValid())
	{
		constexpr bool bSetPreviewMeshInAsset = false;
		PersonaToolkit->SetPreviewMesh(nullptr, bSetPreviewMeshInAsset);
	}
}

UObject* FTLLRN_ControlRigEditor::GetOuterForHost() const
{
	UTLLRN_ControlRigSkeletalMeshComponent* EditorSkelComp = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent());
	if(EditorSkelComp)
	{
		return EditorSkelComp;
	}
	return FRigVMEditor::GetOuterForHost();
}

UClass* FTLLRN_ControlRigEditor::GetDetailWrapperClass() const
{
	return UTLLRN_ControlRigWrapperObject::StaticClass();
}

bool FTLLRN_ControlRigEditor::IsSectionVisible(NodeSectionID::Type InSectionID) const
{
	if(!ITLLRN_ControlRigEditor::IsSectionVisible(InSectionID))
	{
		return false;
	}

	if(const UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint())
	{
		if(IsTLLRN_ModularRig())
		{
			switch (InSectionID)
			{
				case NodeSectionID::GRAPH:
				{
					return RigBlueprint->SupportsEventGraphs();
				}
				case NodeSectionID::FUNCTION:
				{
					return RigBlueprint->SupportsFunctions();
				}
				default:
				{
					break;
				}
			}
		}
	}
	return true;
}

bool FTLLRN_ControlRigEditor::NewDocument_IsVisibleForType(ECreatedDocumentType GraphType) const
{
	if(!ITLLRN_ControlRigEditor::NewDocument_IsVisibleForType(GraphType))
	{
		return false;
	}

	if(const UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint())
	{
		if(IsTLLRN_ModularRig())
		{
			switch(GraphType)
			{
				case CGT_NewEventGraph:
				{
					return RigBlueprint->SupportsEventGraphs();
				}
				case CGT_NewFunctionGraph:
				{
					return RigBlueprint->SupportsFunctions();
				}
				default:
				{
					break;
				}
			}
		}
	}

	return true;
}

FReply FTLLRN_ControlRigEditor::OnViewportDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	const FReply SuperReply = ITLLRN_ControlRigEditor::OnViewportDrop(MyGeometry, DragDropEvent);
	if(SuperReply.IsEventHandled())
	{
		return SuperReply;
	}

	if(IsTLLRN_ModularRig())
	{
		const TSharedPtr<FAssetDragDropOp> AssetDragDropOperation = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
		if (AssetDragDropOperation)
		{
			for (const FAssetData& AssetData : AssetDragDropOperation->GetAssets())
			{
				const UClass* AssetClass = AssetData.GetClass();
				if (!AssetClass->IsChildOf(UTLLRN_ControlRigBlueprint::StaticClass()))
				{
					continue;
				}

				if(const UTLLRN_ControlRigBlueprint* AssetBlueprint = Cast<UTLLRN_ControlRigBlueprint>(AssetData.GetAsset()))
				{
					UClass* TLLRN_ControlRigClass = AssetBlueprint->GetTLLRN_ControlRigClass();
					if(AssetBlueprint->IsTLLRN_ControlTLLRN_RigModule() && TLLRN_ControlRigClass)
					{
						FSlateApplication::Get().DismissAllMenus();

						UTLLRN_ModularTLLRN_RigController* Controller = GetTLLRN_ControlRigBlueprint()->GetTLLRN_ModularTLLRN_RigController();
						FString ClassName = TLLRN_ControlRigClass->GetName();
						ClassName.RemoveFromEnd(TEXT("_C"));
						const FTLLRN_RigName Name = Controller->GetSafeNewName(FString(), FTLLRN_RigName(ClassName));
						const FString ModulePath = Controller->AddModule(Name, TLLRN_ControlRigClass, FString());
						if (!ModulePath.IsEmpty())
						{
							return FReply::Handled();
						}
					}
				}
			}
		}
	}

	return FReply::Unhandled();
}

void FTLLRN_ControlRigEditor::CreateEmptyGraphContent(URigVMController* InController)
{
	URigVMNode* Node = InController->AddUnitNode(FTLLRN_RigUnit_BeginExecution::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), FVector2D::ZeroVector, FString(), false);
	if (Node)
	{
		TArray<FName> NodeNames;
		NodeNames.Add(Node->GetFName());
		InController->SetNodeSelection(NodeNames, false);
	}
}

UTLLRN_ControlRigBlueprint* FTLLRN_ControlRigEditor::GetTLLRN_ControlRigBlueprint() const
{
	return Cast<UTLLRN_ControlRigBlueprint>(GetRigVMBlueprint());
}

UTLLRN_ControlRig* FTLLRN_ControlRigEditor::GetTLLRN_ControlRig() const
{
	return Cast<UTLLRN_ControlRig>(GetRigVMHost());
}

UTLLRN_RigHierarchy* FTLLRN_ControlRigEditor::GetHierarchyBeingDebugged() const
{
	if(UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint())
	{
		if(UTLLRN_ControlRig* RigBeingDebugged = Cast<UTLLRN_ControlRig>(RigBlueprint->GetObjectBeingDebugged()))
		{
			return RigBeingDebugged->GetHierarchy();
		}
		return RigBlueprint->Hierarchy;
	}
	return nullptr;
}

void FTLLRN_ControlRigEditor::InitRigVMEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, URigVMBlueprint* InRigVMBlueprint)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FRigVMEditor::InitRigVMEditor(Mode, InitToolkitHost, InRigVMBlueprint);

	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = CastChecked<UTLLRN_ControlRigBlueprint>(InRigVMBlueprint);

	CreatePersonaToolKitIfRequired();
	UTLLRN_ControlRigBlueprint::sCurrentlyOpenedRigBlueprints.AddUnique(TLLRN_ControlRigBlueprint);

	if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
	{
		EditMode->OnGetTLLRN_RigElementTransform() = FOnGetTLLRN_RigElementTransform::CreateSP(this, &FTLLRN_ControlRigEditor::GetTLLRN_RigElementTransform);
		EditMode->OnSetTLLRN_RigElementTransform() = FOnSetTLLRN_RigElementTransform::CreateSP(this, &FTLLRN_ControlRigEditor::SetTLLRN_RigElementTransform);
		EditMode->OnGetContextMenu() = FOnGetContextMenu::CreateSP(this, &FTLLRN_ControlRigEditor::HandleOnGetViewportContextMenuDelegate);
		EditMode->OnContextMenuCommands() = FNewMenuCommandsDelegate::CreateSP(this, &FTLLRN_ControlRigEditor::HandleOnViewportContextMenuCommandsDelegate);
		EditMode->OnAnimSystemInitialized().Add(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTLLRN_ControlRigEditor::OnAnimInitialized));
	
		PersonaToolkit->GetPreviewScene()->SetRemoveAttachedComponentFilter(FOnRemoveAttachedComponentFilter::CreateSP(EditMode, &FTLLRN_ControlRigEditMode::CanRemoveFromPreviewScene));
	}

	{
		// listening to the BP's event instead of BP's Hierarchy's Event ensure a propagation order of
		// 1. Hierarchy change in BP
		// 2. BP propagate to instances
		// 3. Editor forces propagation again, and reflects hierarchy change in either instances or BP
		// 
		// if directly listening to BP's Hierarchy's Event, this ordering is not guaranteed due to multicast,
		// a problematic order we have encountered looks like:
		// 1. Hierarchy change in BP
		// 2. FTLLRN_ControlRigEditor::OnHierarchyModified performs propagation from BP to instances, refresh UI
		// 3. BP performs propagation again in UTLLRN_ControlRigBlueprint::HandleHierarchyModified, invalidates the rig element
		//    that the UI is observing
		// 4. Editor UI shows an invalid rig element
		TLLRN_ControlRigBlueprint->OnHierarchyModified().AddSP(this, &FTLLRN_ControlRigEditor::OnHierarchyModified);

		if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
		{
			TLLRN_ControlRigBlueprint->OnHierarchyModified().AddSP(EditMode, &FTLLRN_ControlRigEditMode::OnHierarchyModified_AnyThread);
		}

		if (TLLRN_ControlRigBlueprint->IsTLLRN_ModularRig())
		{
			SchematicModel.SetEditor(SharedThis(this));
			TLLRN_ControlRigBlueprint->OnSetObjectBeingDebugged().AddRaw(&SchematicModel, &FTLLRN_ControlRigSchematicModel::OnSetObjectBeingDebugged);
			TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController()->OnModified().AddRaw(&SchematicModel, &FTLLRN_ControlRigSchematicModel::HandleTLLRN_ModularRigModified);
		}

		TLLRN_ControlRigBlueprint->OnRigTypeChanged().AddSP(this, &FTLLRN_ControlRigEditor::HandleRigTypeChanged);
		if (TLLRN_ControlRigBlueprint->IsTLLRN_ModularRig())
		{
			TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController()->OnModified().AddSP(this, &FTLLRN_ControlRigEditor::HandleTLLRN_ModularRigModified);
			TLLRN_ControlRigBlueprint->OnTLLRN_ModularRigCompiled().AddRaw(this, &FTLLRN_ControlRigEditor::HandlePostCompileTLLRN_ModularRigs);
		}
	}

	CreateTLLRN_RigHierarchyToGraphDragAndDropMenu();

	if(SchematicViewport.IsValid())
	{
		SchematicModel.UpdateTLLRN_ControlRigContent();
	}
}

void FTLLRN_ControlRigEditor::CreatePersonaToolKitIfRequired()
{
	if(PersonaToolkit.IsValid())
	{
		return;
	}
	
	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = GetTLLRN_ControlRigBlueprint();

	FPersonaModule& PersonaModule = FModuleManager::GetModuleChecked<FPersonaModule>("Persona");

	FPersonaToolkitArgs PersonaToolkitArgs;
	PersonaToolkitArgs.OnPreviewSceneCreated = FOnPreviewSceneCreated::FDelegate::CreateSP(this, &FTLLRN_ControlRigEditor::HandlePreviewSceneCreated);
	PersonaToolkitArgs.bPreviewMeshCanUseDifferentSkeleton = true;
	USkeleton* Skeleton = nullptr;
	if(USkeletalMesh* PreviewMesh = TLLRN_ControlRigBlueprint->GetPreviewMesh())
	{
		Skeleton = PreviewMesh->GetSkeleton();
	}
	PersonaToolkit = PersonaModule.CreatePersonaToolkit(TLLRN_ControlRigBlueprint, PersonaToolkitArgs, Skeleton);

	// set delegate prior to setting mesh
	// otherwise, you don't get delegate
	PersonaToolkit->GetPreviewScene()->RegisterOnPreviewMeshChanged(FOnPreviewMeshChanged::CreateSP(this, &FTLLRN_ControlRigEditor::HandlePreviewMeshChanged));
	
	// Set a default preview mesh, if any
	TGuardValue<bool> AutoResolveGuard(TLLRN_ControlRigBlueprint->TLLRN_ModularRigSettings.bAutoResolve, false);
	PersonaToolkit->SetPreviewMesh(TLLRN_ControlRigBlueprint->GetPreviewMesh(), false);
}

const FName FTLLRN_ControlRigEditor::GetEditorAppName() const
{
	static const FName TLLRN_ControlRigEditorAppName(TEXT("TLLRN_ControlRigEditorApp"));
	return TLLRN_ControlRigEditorAppName;
}

const FName FTLLRN_ControlRigEditor::GetEditorModeName() const
{
	if(IsTLLRN_ModularRig())
	{
		return FTLLRN_ModularRigEditorEditMode::ModeName;
	}
	return FTLLRN_ControlRigEditorEditMode::ModeName;
}

TSharedPtr<FApplicationMode> FTLLRN_ControlRigEditor::CreateEditorMode()
{
	CreatePersonaToolKitIfRequired();

	if(IsTLLRN_ModularRig())
	{
		return MakeShareable(new FTLLRN_ModularRigEditorMode(SharedThis(this)));
	}
	return MakeShareable(new FTLLRN_ControlRigEditorMode(SharedThis(this)));
}

const FSlateBrush* FTLLRN_ControlRigEditor::GetDefaultTabIcon() const
{
	static const FSlateIcon TabIcon = FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_ControlRig.Editor.TabIcon");
	return TabIcon.GetIcon();
}

FText FTLLRN_ControlRigEditor::GetTestAssetName() const
{
	if(TestDataStrongPtr.IsValid())
	{
		return FText::FromString(TestDataStrongPtr->GetName());
	}

	static const FText NoTestAsset = LOCTEXT("NoTestAsset", "No Test Asset");
	return NoTestAsset;
}

FText FTLLRN_ControlRigEditor::GetTestAssetTooltip() const
{
	if(TestDataStrongPtr.IsValid())
	{
		return FText::FromString(TestDataStrongPtr->GetPathName());
	}
	static const FText NoTestAssetTooltip = LOCTEXT("NoTestAssetTooltip", "Click the record button to the left to record a new frame");
	return NoTestAssetTooltip;
}

bool FTLLRN_ControlRigEditor::SetTestAssetPath(const FString& InAssetPath)
{
	if(TestDataStrongPtr.IsValid())
	{
		if(TestDataStrongPtr->GetPathName().Equals(InAssetPath, ESearchCase::CaseSensitive))
		{
			return false;
		}
	}

	if(TestDataStrongPtr.IsValid())
	{
		TestDataStrongPtr->ReleaseReplay();
		TestDataStrongPtr.Reset();
	}

	if(!InAssetPath.IsEmpty())
	{
		if(UTLLRN_ControlRigTestData* TestData = LoadObject<UTLLRN_ControlRigTestData>(GetTLLRN_ControlRigBlueprint(), *InAssetPath))
		{
			TestDataStrongPtr = TStrongObjectPtr<UTLLRN_ControlRigTestData>(TestData);
		}
	}
	return true;
}

TSharedRef<SWidget> FTLLRN_ControlRigEditor::GenerateTestAssetModeMenuContent()
{
	FMenuBuilder MenuBuilder(true, GetToolkitCommands());

	MenuBuilder.BeginSection(TEXT("Default"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ClearTestAssset", "Clear"),
		LOCTEXT("ClearTestAsset_ToolTip", "Clears the test asset"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				SetTestAssetPath(FString());
			}
		)	
	));
	MenuBuilder.EndSection();

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> TestDataAssets;
	FARFilter AssetFilter;
	AssetFilter.ClassPaths.Add(UTLLRN_ControlRigTestData::StaticClass()->GetClassPathName());
	AssetRegistryModule.Get().GetAssets(AssetFilter, TestDataAssets);

	const FString CurrentObjectPath = GetTLLRN_ControlRigBlueprint()->GetPathName();
	TestDataAssets.RemoveAll([CurrentObjectPath](const FAssetData& InAssetData)
	{
		const FString TLLRN_ControlRigObjectPath = InAssetData.GetTagValueRef<FString>(GET_MEMBER_NAME_CHECKED(UTLLRN_ControlRigTestData, TLLRN_ControlRigObjectPath));
		return TLLRN_ControlRigObjectPath != CurrentObjectPath;
	});

	if(!TestDataAssets.IsEmpty())
	{
		MenuBuilder.BeginSection(TEXT("Assets"));
		for(const FAssetData& TestDataAsset : TestDataAssets)
		{
			const FString TestDataObjectPath = TestDataAsset.GetObjectPathString();
			FString Right;
			if(TestDataObjectPath.Split(TEXT("."), nullptr, &Right))
			{
				MenuBuilder.AddMenuEntry(FText::FromString(Right), FText::FromString(TestDataObjectPath), FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([this, TestDataObjectPath]()
						{
							SetTestAssetPath(TestDataObjectPath);
						}
					)	
				));
			}
		}
		MenuBuilder.EndSection();
	}
	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FTLLRN_ControlRigEditor::GenerateTestAssetRecordMenuContent()
{
	FMenuBuilder MenuBuilder(true, GetToolkitCommands());

	MenuBuilder.AddMenuEntry(
		LOCTEXT("TestAssetRecordSingleFrame", "Single Frame"),
		LOCTEXT("TestAssetRecordSingleFrame_ToolTip", "Records a single frame into the test data asset"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				RecordTestData(0);
			}
		)	
	));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("TestAssetRecordOneSecond", "1 Second"),
		LOCTEXT("TestAssetRecordOneSecond_ToolTip", "Records 1 second of animation into the test data asset"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				RecordTestData(1);
			}
		)	
	));

	MenuBuilder.AddMenuEntry(
	LOCTEXT("TestAssetRecordFiveSeconds", "5 Seconds"),
	LOCTEXT("TestAssetRecordFiveSeconds_ToolTip", "Records 5 seconds of animation into the test data asset"),
	FSlateIcon(),
	FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				RecordTestData(5);
			}
		)	
	));

	MenuBuilder.AddMenuEntry(
	LOCTEXT("TestAssetRecordTenSeconds", "10 Seconds"),
	LOCTEXT("TestAssetRecordTenSeconds_ToolTip", "Records 10 seconds of animation into the test data asset"),
	FSlateIcon(),
	FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				RecordTestData(10);
			}
		)	
	));

	return MenuBuilder.MakeWidget();
}

bool FTLLRN_ControlRigEditor::RecordTestData(double InRecordingDuration)
{
	if(GetTLLRN_ControlRig() == nullptr)
	{
		return false;
	}
	
	if(!TestDataStrongPtr.IsValid())
	{
		// create a new test asset
		static const FString Folder = TEXT("/Game/Animation/TLLRN_ControlRig/NoCook/");
		const FString DesiredPackagePath =  FString::Printf(TEXT("%s/%s_TestData"), *Folder, *GetTLLRN_ControlRigBlueprint()->GetName());

		if(UTLLRN_ControlRigTestData* TestData = UTLLRN_ControlRigTestData::CreateNewAsset(DesiredPackagePath, GetTLLRN_ControlRigBlueprint()->GetPathName()))
		{
			SetTestAssetPath(TestData->GetPathName());
		}
	}
	
	if(UTLLRN_ControlRigTestData* TestData = TestDataStrongPtr.Get())
	{
		TestData->Record(GetTLLRN_ControlRig(), InRecordingDuration);
	}
	return true;
}

void FTLLRN_ControlRigEditor::ToggleTestData()
{
	if(TestDataStrongPtr.IsValid())
	{
		switch(TestDataStrongPtr->GetPlaybackMode())
		{
			case ETLLRN_ControlRigTestDataPlaybackMode::ReplayInputs:
			{
				TestDataStrongPtr->ReleaseReplay();
				break;
			}
			case ETLLRN_ControlRigTestDataPlaybackMode::GroundTruth:
			{
				TestDataStrongPtr->SetupReplay(GetTLLRN_ControlRig(), false);
				break;
			}
			default:
			{
				TestDataStrongPtr->SetupReplay(GetTLLRN_ControlRig(), true);
				break;
			}
		}
	}
}

void FTLLRN_ControlRigEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder, bool bEndSection)
{
	FRigVMEditor::FillToolbar(ToolbarBuilder, false);
	
	{
		if(CVarTLLRN_ControlTLLRN_RigHierarchyEnableModules.GetValueOnAnyThread())
		{
			TWeakObjectPtr<UTLLRN_ControlRigBlueprint> WeakBlueprint = GetTLLRN_ControlRigBlueprint();
			ToolbarBuilder.AddToolBarButton(
				FUIAction(
					FExecuteAction::CreateLambda([WeakBlueprint]()
					{
						if(WeakBlueprint.IsValid())
						{
							if(WeakBlueprint->IsTLLRN_ControlTLLRN_RigModule())
							{
								WeakBlueprint->TurnIntoStandaloneRig();
							}
							else
							{
								if(!WeakBlueprint->CanTurnIntoTLLRN_ControlTLLRN_RigModule(false))
								{
									static const FText Message(LOCTEXT("TurnIntoTLLRN_ControlTLLRN_RigModuleMessage", "This rig requires some changes to the hierarchy to turn it into a module.\n\nWe'll try to recreate the hierarchy by relying on nodes in the construction event instead.\n\nDo you want to continue?"));
									EAppReturnType::Type Ret = FMessageDialog::Open(EAppMsgType::YesNo, Message);
									if(Ret == EAppReturnType::No)
									{
										return;
									}
								}
								WeakBlueprint->TurnIntoTLLRN_ControlTLLRN_RigModule(true);
							}
						}
					}),
					FCanExecuteAction::CreateLambda([WeakBlueprint]
					{
						if(WeakBlueprint.IsValid())
						{
							if(WeakBlueprint->IsTLLRN_ControlTLLRN_RigModule())
							{
								return WeakBlueprint->CanTurnIntoStandaloneRig();
							}
							return WeakBlueprint->CanTurnIntoTLLRN_ControlTLLRN_RigModule(true);
						}
						return false;
					})
				),
				NAME_None,
				TAttribute<FText>::CreateLambda([WeakBlueprint]()
				{
					static const FText StandaloneRig = LOCTEXT("SwitchToTLLRN_RigModule", "Switch to Rig Module"); 
					static const FText TLLRN_RigModule = LOCTEXT("SwitchToStandaloneRig", "Switch to Standalone Rig");
					if(WeakBlueprint.IsValid())
					{
						if(WeakBlueprint->IsTLLRN_ControlTLLRN_RigModule())
						{
							return TLLRN_RigModule;
						}
					}
					return StandaloneRig;
				}),
				TAttribute<FText>::CreateLambda([WeakBlueprint]()
				{
					static const FText StandaloneRigTooltip = LOCTEXT("StandaloneRigTooltip", "A standalone control rig."); 
					static const FText TLLRN_RigModuleTooltip = LOCTEXT("TLLRN_RigModuleTooltip", "A rig module used to build rigs.");
					if(WeakBlueprint.IsValid())
					{
						if(!WeakBlueprint->IsTLLRN_ControlTLLRN_RigModule())
						{
							FString FailureReason;
							if(!WeakBlueprint->CanTurnIntoTLLRN_ControlTLLRN_RigModule(true, &FailureReason))
							{
								return FText::Format(
									LOCTEXT("StandaloneRigTooltipFormat", "{0}\n\nThis rig cannot be turned into a module:\n\n{1}"),
									StandaloneRigTooltip,
									FText::FromString(FailureReason)
								);
							}
							return StandaloneRigTooltip;
						}
					}
					return TLLRN_RigModuleTooltip;
				}),
				TAttribute<FSlateIcon>::CreateLambda([WeakBlueprint]()
				{
					static const FSlateIcon ModuleIcon = FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_ControlRig.Tree.Connector");
					static const FSlateIcon RigIcon = FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(),"ClassIcon.TLLRN_ControlRigBlueprint"); 
					if(WeakBlueprint.IsValid())
					{
						if(WeakBlueprint->IsTLLRN_ControlTLLRN_RigModule())
						{
							return ModuleIcon;
						}
					}
					return RigIcon;
				}),
				EUserInterfaceActionType::Button
			);
		}

		if(CVarTLLRN_ControlRigShowTestingToolbar.GetValueOnAnyThread())
		{
			ToolbarBuilder.AddSeparator();

			FUIAction TestAssetAction;
			ToolbarBuilder.AddComboButton(
				FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([this]()
				{
					if(TestDataStrongPtr.IsValid())
					{
						return !TestDataStrongPtr->IsReplaying() && !TestDataStrongPtr->IsRecording();
					}
					return true;
				})),
				FOnGetContent::CreateSP(this, &FTLLRN_ControlRigEditor::GenerateTestAssetModeMenuContent),
				TAttribute<FText>::CreateSP(this, &FTLLRN_ControlRigEditor::GetTestAssetName),
				TAttribute<FText>::CreateSP(this, &FTLLRN_ControlRigEditor::GetTestAssetTooltip),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "AutomationTools.TestAutomation"),
				false);

			ToolbarBuilder.AddToolBarButton(FUIAction(
					FExecuteAction::CreateLambda([this]()
					{
						RecordTestData(0.0);
					}),
					FCanExecuteAction::CreateLambda([this]()
					{
						if(TestDataStrongPtr.IsValid())
						{
							return !TestDataStrongPtr->IsReplaying() && !TestDataStrongPtr->IsRecording();
						}
						return true;
					})
				),
				NAME_None,
				LOCTEXT("TestDataRecordButton", "Record"),
				LOCTEXT("TestDataRecordButton_Tooltip", "Records a new frame into the test data asset.\nA test data asset will be created if necessary."),
				FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(),"TLLRN_ControlRig.TestData.Record")
				);
			ToolbarBuilder.AddComboButton(
				FUIAction(),
				FOnGetContent::CreateSP(this, &FTLLRN_ControlRigEditor::GenerateTestAssetRecordMenuContent),
				LOCTEXT("TestDataRecordMenu_Label", "Recording Modes"),
				LOCTEXT("TestDataRecordMenu_ToolTip", "Pick between different modes for recording"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Recompile"),
				true);

			ToolbarBuilder.AddToolBarButton(FUIAction(
					FExecuteAction::CreateLambda([this]()
					{
						ToggleTestData();
					}),
					FCanExecuteAction::CreateLambda([this]
					{
						if(TestDataStrongPtr.IsValid())
						{
							return !TestDataStrongPtr->IsRecording();
						}
						return false;
					})
				),
				NAME_None,
				TAttribute<FText>::CreateLambda([this]()
				{
					static const FText LiveStatus = LOCTEXT("LiveStatus", "Live"); 
					static const FText ReplayInputsStatus = LOCTEXT("ReplayInputsStatus", "Replay Inputs");
					static const FText GroundTruthStatus = LOCTEXT("GroundTruthStatus", "Ground Truth");
					if(TestDataStrongPtr.IsValid())
					{
						switch(TestDataStrongPtr->GetPlaybackMode())
						{
							case ETLLRN_ControlRigTestDataPlaybackMode::ReplayInputs:
							{
								return ReplayInputsStatus;
							}
							case ETLLRN_ControlRigTestDataPlaybackMode::GroundTruth:
							{
								return GroundTruthStatus;
							}
							default:
							{
								break;
							}
						}
					}
					return LiveStatus;
				}),
				TAttribute<FText>::CreateLambda([this]()
				{
					static const FText LiveStatusTooltip = LOCTEXT("LiveStatusTooltip", "The test data is not affecting the rig."); 
					static const FText ReplayInputsStatusTooltip = LOCTEXT("ReplayInputsStatusTooltip", "The test data inputs are being replayed onto the rig.");
					static const FText GroundTruthStatusTooltip = LOCTEXT("GroundTruthStatusTooltip", "The complete result of the rig is being overwritten.");
					if(TestDataStrongPtr.IsValid())
					{
						switch(TestDataStrongPtr->GetPlaybackMode())
						{
							case ETLLRN_ControlRigTestDataPlaybackMode::ReplayInputs:
							{
								return ReplayInputsStatusTooltip;
							}
							case ETLLRN_ControlRigTestDataPlaybackMode::GroundTruth:
							{
								return GroundTruthStatusTooltip;
							}
							default:
							{
								break;
							}
						}
					}
					return LiveStatusTooltip;
				}),
				TAttribute<FSlateIcon>::CreateLambda([this]()
				{
					static const FSlateIcon LiveIcon = FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(),"ClassIcon.TLLRN_ControlRigBlueprint"); 
					static const FSlateIcon ReplayIcon = FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(),"ClassIcon.TLLRN_ControlRigSequence"); 
					if(TestDataStrongPtr.IsValid() && TestDataStrongPtr->IsReplaying())
					{
						return ReplayIcon;
					}
					return LiveIcon;
				}),
				EUserInterfaceActionType::Button
			);
		}
	}
	
	if(bEndSection)
	{
		ToolbarBuilder.EndSection();
	}
}

TArray<FName> FTLLRN_ControlRigEditor::GetDefaultEventQueue() const
{
	return ForwardsSolveEventQueue;
}

void FTLLRN_ControlRigEditor::SetEventQueue(TArray<FName> InEventQueue, bool bCompile)
{
	if (GetEventQueue() == InEventQueue)
	{
		return;
	}

	TArray<FTLLRN_RigElementKey> PreviousSelection;
	if (UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
	{
		if(bCompile)
		{
			if (RigBlueprint->GetAutoVMRecompile())
			{
				RigBlueprint->RequestAutoVMRecompilation();
			}
			RigBlueprint->Validator->SetTLLRN_ControlRig(GetTLLRN_ControlRig());
		}
		
		// need to clear selection before remove transient control
		// because active selection will trigger transient control recreation after removal	
		PreviousSelection = GetHierarchyBeingDebugged()->GetSelectedKeys();
		RigBlueprint->GetHierarchyController()->ClearSelection();
		
		// need to copy here since the removal changes the iterator
		if (GetTLLRN_ControlRig())
		{
			RigBlueprint->ClearTransientControls();
		}
	}

	FRigVMEditor::SetEventQueue(InEventQueue, bCompile);

	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		if (InEventQueue.Num() > 0)
		{
			if (UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
			{
				RigBlueprint->Validator->SetTLLRN_ControlRig(TLLRN_ControlRig);

				if (LastEventQueue == ConstructionEventQueue)
				{
					// This will propagate any user bone transformation done during construction to the preview instance
					ResetAllBoneModification();
				}
			}
		}

		// Reset transforms only for construction and forward solve to not interrupt any animation that might be playing
		if (InEventQueue.Contains(FTLLRN_RigUnit_PrepareForExecution::EventName) ||
			InEventQueue.Contains(FTLLRN_RigUnit_BeginExecution::EventName))
		{
			if(UTLLRN_ControlRigEditorSettings::Get()->bResetPoseWhenTogglingEventQueue)
			{
				TLLRN_ControlRig->GetHierarchy()->ResetPoseToInitial(ETLLRN_RigElementType::All);
			}
		}
	}

	if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
	{
		EditMode->RecreateControlShapeActors();

		UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
		Settings->bDisplayNulls = IsConstructionModeEnabled();
	}

	if (PreviousSelection.Num() > 0)
	{
		GetHierarchyBeingDebugged()->GetController(true)->SetSelection(PreviousSelection);
		SetDetailViewForTLLRN_RigElements();
	}
}

int32 FTLLRN_ControlRigEditor::GetEventQueueComboValue() const
{
	const TArray<FName> EventQueue = GetEventQueue();
	if(EventQueue == ForwardsSolveEventQueue)
	{
		return 0;
	}
	if(EventQueue == ConstructionEventQueue)
	{
		return 1;
	}
	if(EventQueue == BackwardsSolveEventQueue)
	{
		return 2;
	}
	if(EventQueue == BackwardsAndForwardsSolveEventQueue)
	{
		return 3;
	}
	return FRigVMEditor::GetEventQueueComboValue();
}

FText FTLLRN_ControlRigEditor::GetEventQueueLabel() const
{
	TArray<FName> EventQueue = GetEventQueue();

	if(EventQueue == ConstructionEventQueue)
	{
		return FTLLRN_RigUnit_PrepareForExecution::StaticStruct()->GetDisplayNameText();
	}
	if(EventQueue == ForwardsSolveEventQueue)
	{
		return FTLLRN_RigUnit_BeginExecution::StaticStruct()->GetDisplayNameText();
	}
	if(EventQueue == BackwardsSolveEventQueue)
	{
		return FTLLRN_RigUnit_InverseExecution::StaticStruct()->GetDisplayNameText();
	}
	if(EventQueue == BackwardsAndForwardsSolveEventQueue)
	{
		return FText::FromString(FString::Printf(TEXT("%s and %s"),
			*FTLLRN_RigUnit_InverseExecution::StaticStruct()->GetDisplayNameText().ToString(),
			*FTLLRN_RigUnit_BeginExecution::StaticStruct()->GetDisplayNameText().ToString()));
	}

	if(EventQueue.Num() == 1)
	{
		FString EventName = EventQueue[0].ToString();
		if(!EventName.EndsWith(TEXT("Event")))
		{
			EventName += TEXT(" Event");
		}
		return FText::FromString(EventName);
	}
	
	return LOCTEXT("CustomEventQueue", "Custom Event Queue");
}

FSlateIcon FTLLRN_ControlRigEditor::GetEventQueueIcon(const TArray<FName>& InEventQueue) const
{
	if(InEventQueue == ConstructionEventQueue)
	{
		return FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_ControlRig.ConstructionMode");
	}
	if(InEventQueue == ForwardsSolveEventQueue)
	{
		return FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_ControlRig.ForwardsSolveEvent");
	}
	if(InEventQueue == BackwardsSolveEventQueue)
	{
		return FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_ControlRig.BackwardsSolveEvent");
	}
	if(InEventQueue == BackwardsAndForwardsSolveEventQueue)
	{
		return FSlateIcon(FTLLRN_ControlRigEditorStyle::Get().GetStyleSetName(), "TLLRN_ControlRig.BackwardsAndForwardsSolveEvent");
	}

	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Event_16x");
}

void FTLLRN_ControlRigEditor::HandleSetObjectBeingDebugged(UObject* InObject)
{
	FRigVMEditor::HandleSetObjectBeingDebugged(InObject);
	
	UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InObject);
	if(UTLLRN_ControlRig* PreviouslyDebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetBlueprintObj()->GetObjectBeingDebugged()))
	{
		if(!URigVMHost::IsGarbageOrDestroyed(PreviouslyDebuggedTLLRN_ControlRig))
		{
			PreviouslyDebuggedTLLRN_ControlRig->GetHierarchy()->OnModified().RemoveAll(this);
			PreviouslyDebuggedTLLRN_ControlRig->OnPreForwardsSolve_AnyThread().RemoveAll(this);
			PreviouslyDebuggedTLLRN_ControlRig->OnPreConstructionForUI_AnyThread().RemoveAll(this);
			PreviouslyDebuggedTLLRN_ControlRig->OnPreConstruction_AnyThread().RemoveAll(this);
			PreviouslyDebuggedTLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
			PreviouslyDebuggedTLLRN_ControlRig->ControlModified().RemoveAll(this);
		}
	}

	if (UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
	{
		RigBlueprint->Validator->SetTLLRN_ControlRig(DebuggedTLLRN_ControlRig);
	}

	if (DebuggedTLLRN_ControlRig)
	{
		const bool bShouldExecute = ShouldExecuteTLLRN_ControlRig(DebuggedTLLRN_ControlRig);
		GetTLLRN_ControlRigBlueprint()->Hierarchy->HierarchyForSelectionPtr = DebuggedTLLRN_ControlRig->DynamicHierarchy;

		UTLLRN_ControlRigSkeletalMeshComponent* EditorSkelComp = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent());
		if (EditorSkelComp)
		{
			UTLLRN_ControlRigLayerInstance* AnimInstance = Cast<UTLLRN_ControlRigLayerInstance>(EditorSkelComp->GetAnimInstance());
			if (AnimInstance)
			{
				FTLLRN_ControlRigIOSettings IOSettings = FTLLRN_ControlRigIOSettings::MakeEnabled();
				IOSettings.bUpdatePose = bShouldExecute;
				IOSettings.bUpdateCurves = bShouldExecute;

				// we might want to move this into another method
				FInputBlendPose Filter;
				AnimInstance->ResetTLLRN_ControlRigTracks();
				AnimInstance->AddTLLRN_ControlRigTrack(0, DebuggedTLLRN_ControlRig);
				AnimInstance->UpdateTLLRN_ControlRigTrack(0, 1.0f, IOSettings, bShouldExecute);
				AnimInstance->RecalcRequiredBones();

				// since rig has changed, rebuild draw skeleton
				EditorSkelComp->SetTLLRN_ControlRigBeingDebugged(DebuggedTLLRN_ControlRig);

				if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
				{
					EditMode->SetObjects(DebuggedTLLRN_ControlRig, EditorSkelComp,nullptr);
				}
			}
			
			// get the bone intial transforms from the preview skeletal mesh
			if(bShouldExecute)
			{
				DebuggedTLLRN_ControlRig->SetBoneInitialTransformsFromSkeletalMeshComponent(EditorSkelComp);
				if(UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
				{
					// copy the initial transforms back to the blueprint
					// no need to call modify here since this code only modifies the bp if the preview mesh changed
					RigBlueprint->Hierarchy->CopyPose(DebuggedTLLRN_ControlRig->GetHierarchy(), false, true, false);
				}
			}
		}

		DebuggedTLLRN_ControlRig->GetHierarchy()->OnModified().RemoveAll(this);
		DebuggedTLLRN_ControlRig->OnPreForwardsSolve_AnyThread().RemoveAll(this);
		DebuggedTLLRN_ControlRig->OnPreConstructionForUI_AnyThread().RemoveAll(this);
		DebuggedTLLRN_ControlRig->OnPreConstruction_AnyThread().RemoveAll(this);
		DebuggedTLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
		DebuggedTLLRN_ControlRig->ControlModified().RemoveAll(this);

		DebuggedTLLRN_ControlRig->GetHierarchy()->OnModified().AddSP(this, &FTLLRN_ControlRigEditor::OnHierarchyModified_AnyThread);
		DebuggedTLLRN_ControlRig->OnPreForwardsSolve_AnyThread().AddSP(this, &FTLLRN_ControlRigEditor::OnPreForwardsSolve_AnyThread);
		DebuggedTLLRN_ControlRig->OnPreConstructionForUI_AnyThread().AddSP(this, &FTLLRN_ControlRigEditor::OnPreConstructionForUI_AnyThread);
		DebuggedTLLRN_ControlRig->OnPreConstruction_AnyThread().AddSP(this, &FTLLRN_ControlRigEditor::OnPreConstruction_AnyThread);
		DebuggedTLLRN_ControlRig->OnPostConstruction_AnyThread().AddSP(this, &FTLLRN_ControlRigEditor::OnPostConstruction_AnyThread);
		DebuggedTLLRN_ControlRig->ControlModified().AddSP(this, &FTLLRN_ControlRigEditor::HandleOnControlModified);
		
		LastHierarchyHash = INDEX_NONE;

		if(EditorSkelComp)
		{
			EditorSkelComp->SetComponentToWorld(FTransform::Identity);
		}

		if(!bShouldExecute)
		{
			if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
			{
				EditMode->RequestToRecreateControlShapeActors();
			}
		}
	}
	else
	{
		if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
		{
			EditMode->SetObjects(nullptr,  nullptr,nullptr);
		}
	}
}

void FTLLRN_ControlRigEditor::SetDetailViewForTLLRN_RigElements()
{
	UTLLRN_RigHierarchy* HierarchyBeingDebugged = GetHierarchyBeingDebugged();
	SetDetailViewForTLLRN_RigElements(HierarchyBeingDebugged->GetSelectedKeys());
}

void FTLLRN_ControlRigEditor::SetDetailViewForTLLRN_RigElements(const TArray<FTLLRN_RigElementKey>& InKeys)
{
	if(IsDetailsPanelRefreshSuspended())
	{
		return;
	}

	TArray<FTLLRN_RigElementKey> Keys = InKeys;
	if(Keys.IsEmpty())
	{
		TArray< TWeakObjectPtr<UObject> > SelectedObjects = GetSelectedObjects();
		for (TWeakObjectPtr<UObject> SelectedObject : SelectedObjects)
		{
			if (SelectedObject.IsValid())
			{
				if(const URigVMDetailsViewWrapperObject* WrapperObject = Cast<URigVMDetailsViewWrapperObject>(SelectedObject.Get()))
				{
					if(const UScriptStruct* WrappedStruct = WrapperObject->GetWrappedStruct())
					{
						if (WrappedStruct->IsChildOf(FTLLRN_RigBaseElement::StaticStruct()))
						{
							Keys.Add(WrapperObject->GetContent<FTLLRN_RigBaseElement>().Key);
						}
					}
				}
			}
		}
	}

	ClearDetailObject();

	UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj());
	UTLLRN_RigHierarchy* HierarchyBeingDebugged = GetHierarchyBeingDebugged();
	TArray<UObject*> Objects;

	for(const FTLLRN_RigElementKey& Key : Keys)
	{
		FTLLRN_RigBaseElement* Element = HierarchyBeingDebugged->Find(Key);
		if (Element == nullptr)
		{
			continue;
		}

		URigVMDetailsViewWrapperObject* WrapperObject = URigVMDetailsViewWrapperObject::MakeInstance(GetDetailWrapperClass(), GetBlueprintObj(), Element->GetElementStruct(), (uint8*)Element, HierarchyBeingDebugged);
		WrapperObject->GetWrappedPropertyChangedChainEvent().AddSP(this, &FTLLRN_ControlRigEditor::OnWrappedPropertyChangedChainEvent);
		WrapperObject->AddToRoot();

		Objects.Add(WrapperObject);
	}
	
	SetDetailObjects(Objects);
}

void FTLLRN_ControlRigEditor::SetDetailObjects(const TArray<UObject*>& InObjects)
{
	// if no modules should be selected - we need to deselect all modules
	if (!InObjects.ContainsByPredicate([](const UObject* InObject) -> bool
	{
		return IsValid(InObject) && InObject->IsA<UTLLRN_ControlRig>();
	}))
	{
		ModulesSelected.Reset();
	}
	
	ITLLRN_ControlRigEditor::SetDetailObjects(InObjects);
}

void FTLLRN_ControlRigEditor::RefreshDetailView()
{
	if(DetailViewShowsAnyTLLRN_RigElement())
	{
		SetDetailViewForTLLRN_RigElements();
		return;
	}
	else if(!ModulesSelected.IsEmpty())
	{
		SetDetailViewForTLLRN_RigModules();
		return;
	}

	FRigVMEditor::RefreshDetailView();
}

bool FTLLRN_ControlRigEditor::DetailViewShowsAnyTLLRN_RigElement() const
{
	return DetailViewShowsStruct(FTLLRN_RigBaseElement::StaticStruct());
}

bool FTLLRN_ControlRigEditor::DetailViewShowsTLLRN_RigElement(FTLLRN_RigElementKey InKey) const
{
	TArray< TWeakObjectPtr<UObject> > SelectedObjects = Inspector->GetSelectedObjects();
	for (TWeakObjectPtr<UObject> SelectedObject : SelectedObjects)
	{
		if (SelectedObject.IsValid())
		{
			if(URigVMDetailsViewWrapperObject* WrapperObject = Cast<URigVMDetailsViewWrapperObject>(SelectedObject.Get()))
			{
				if (const UScriptStruct* WrappedStruct = WrapperObject->GetWrappedStruct())
				{
					if (WrappedStruct->IsChildOf(FTLLRN_RigBaseElement::StaticStruct()))
					{
						if(WrapperObject->GetContent<FTLLRN_RigBaseElement>().GetKey() == InKey)
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

TArray<FTLLRN_RigElementKey> FTLLRN_ControlRigEditor::GetSelectedTLLRN_RigElementsFromDetailView() const
{
	TArray<FTLLRN_RigElementKey> Elements;
	
	TArray< TWeakObjectPtr<UObject> > SelectedObjects = Inspector->GetSelectedObjects();
	for (TWeakObjectPtr<UObject> SelectedObject : SelectedObjects)
	{
		if (SelectedObject.IsValid())
		{
			if(URigVMDetailsViewWrapperObject* WrapperObject = Cast<URigVMDetailsViewWrapperObject>(SelectedObject.Get()))
			{
				if (const UScriptStruct* WrappedStruct = WrapperObject->GetWrappedStruct())
				{
					if (WrappedStruct->IsChildOf(FTLLRN_RigBaseElement::StaticStruct()))
					{
						Elements.Add(WrapperObject->GetContent<FTLLRN_RigBaseElement>().GetKey());
					}
				}
			}
		}
	}

	return Elements;
}

void FTLLRN_ControlRigEditor::SetDetailViewForTLLRN_RigModules()
{
	SetDetailViewForTLLRN_RigModules(ModulesSelected);
}

void FTLLRN_ControlRigEditor::SetDetailViewForTLLRN_RigModules(const TArray<FString> InKeys)
{
	if(IsDetailsPanelRefreshSuspended())
	{
		return;
	}

	ClearDetailObject();

	UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj());
	UTLLRN_ModularRig* RigBeingDebugged = Cast<UTLLRN_ModularRig>(RigBlueprint->GetDebuggedTLLRN_ControlRig());

	if (!RigBeingDebugged)
	{
		return;
	}

	ModulesSelected = InKeys;
	TArray<UObject*> Objects;

	for(const FString& Key : InKeys)
	{
		const FTLLRN_RigModuleInstance* Element = RigBeingDebugged->FindModule(Key);
		if (Element == nullptr)
		{
			continue;
		}

		if(UTLLRN_ControlRig* ModuleInstance = Element->GetRig())
		{
			Objects.Add(ModuleInstance);
		}
	}
	
	SetDetailObjects(Objects);

	// In case the modules selected are still not available, lets set them again
	if (Objects.IsEmpty())
	{
		ModulesSelected = InKeys;
	}
}

bool FTLLRN_ControlRigEditor::DetailViewShowsAnyTLLRN_RigModule() const
{
	return DetailViewShowsStruct(FTLLRN_RigModuleInstance::StaticStruct());
}

bool FTLLRN_ControlRigEditor::DetailViewShowsTLLRN_RigModule(FString InKey) const
{
	TArray< TWeakObjectPtr<UObject> > SelectedObjects = Inspector->GetSelectedObjects();
	for (TWeakObjectPtr<UObject> SelectedObject : SelectedObjects)
	{
		if (SelectedObject.IsValid())
		{
			if(URigVMDetailsViewWrapperObject* WrapperObject = Cast<URigVMDetailsViewWrapperObject>(SelectedObject.Get()))
			{
				if (const UScriptStruct* WrappedStruct = WrapperObject->GetWrappedStruct())
				{
					if (WrappedStruct->IsChildOf(FTLLRN_RigModuleInstance::StaticStruct()))
					{
						if(WrapperObject->GetContent<FTLLRN_RigModuleInstance>().GetPath() == InKey)
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

void FTLLRN_ControlRigEditor::Compile()
{
	{
		DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

		TUniquePtr<UTLLRN_ControlRigBlueprint::FControlValueScope> ValueScope;
		if (!UTLLRN_ControlRigEditorSettings::Get()->bResetControlsOnCompile) // if we need to retain the controls
		{
			ValueScope = MakeUnique<UTLLRN_ControlRigBlueprint::FControlValueScope>(GetTLLRN_ControlRigBlueprint());
		}

		UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = GetTLLRN_ControlRigBlueprint();
		if (TLLRN_ControlRigBlueprint == nullptr)
		{
			return;
		}

		TArray< TWeakObjectPtr<UObject> > SelectedObjects = Inspector->GetSelectedObjects();

		if(IsConstructionModeEnabled())
		{
			SetEventQueue(ForwardsSolveEventQueue, false);
		}

		// clear transient controls such that we don't leave
		// a phantom shape in the viewport
		// have to do this before compile() because during compile
		// a new control rig instance is created without the transient controls
		// so clear is never called for old transient controls
		TLLRN_ControlRigBlueprint->ClearTransientControls();

		// default to always reset all bone modifications 
		ResetAllBoneModification(); 

		{
			FRigVMEditor::Compile();
		}

		TLLRN_ControlRigBlueprint->RecompileTLLRN_ModularRig();

		// ensure the skeletal mesh is still bound
		UTLLRN_ControlRigSkeletalMeshComponent* SkelMeshComponent = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent());
		if (SkelMeshComponent)
		{
			bool bWasCreated = false;
			FAnimCustomInstanceHelper::BindToSkeletalMeshComponent<UTLLRN_ControlRigLayerInstance>(SkelMeshComponent, bWasCreated);
			if (bWasCreated)
			{
				OnAnimInitialized();
			}
		}
		
		if (SelectedObjects.Num() > 0)
		{
			RefreshDetailView();
		}

		if (UTLLRN_ControlRigEditorSettings::Get()->bResetControlTransformsOnCompile)
		{
			TLLRN_ControlRigBlueprint->Hierarchy->ForEach<FTLLRN_RigControlElement>([TLLRN_ControlRigBlueprint](FTLLRN_RigControlElement* ControlElement) -> bool
            {
				const FTransform Transform = TLLRN_ControlRigBlueprint->Hierarchy->GetInitialLocalTransform(ControlElement->GetIndex());

				/*/
				if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
				{
					TLLRN_ControlRig->Modify();
					TLLRN_ControlRig->GetControlHierarchy().SetLocalTransform(Control.Index, Transform);
					TLLRN_ControlRig->ControlModified().Broadcast(TLLRN_ControlRig, Control, ETLLRN_ControlRigSetKey::DoNotCare);
				}
				*/

				TLLRN_ControlRigBlueprint->Hierarchy->SetLocalTransform(ControlElement->GetIndex(), Transform);
				return true;
			});
		}

		TLLRN_ControlRigBlueprint->PropagatePoseFromBPToInstances();

		if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
		{
			EditMode->RecreateControlShapeActors();
		}
	}
}

void FTLLRN_ControlRigEditor::HandleModifiedEvent(ERigVMGraphNotifType InNotifType, URigVMGraph* InGraph, UObject* InSubject)
{
	ITLLRN_ControlRigEditor::HandleModifiedEvent(InNotifType, InGraph, InSubject);

	if(InNotifType == ERigVMGraphNotifType::NodeSelected)
	{
		if(const URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(InSubject))
		{
			SetDirectionManipulationSubject(UnitNode);
		}
	}
	else if(InNotifType == ERigVMGraphNotifType::NodeSelectionChanged)
	{
		bool bNeedsToClearManipulationSubject = true;
		const TArray<FName> SelectedNodes = InGraph->GetSelectNodes();
		if(SelectedNodes.Num() == 1)
		{
			if(const URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(InGraph->FindNodeByName(SelectedNodes[0])))
			{
				SetDirectionManipulationSubject(UnitNode);
				bNeedsToClearManipulationSubject = false;
			}
		}

		if(bNeedsToClearManipulationSubject)
		{
			ClearDirectManipulationSubject();
		}
	}
	else if(InNotifType == ERigVMGraphNotifType::PinDefaultValueChanged)
	{
		if(const URigVMPin* Pin = Cast<URigVMPin>(InSubject))
		{
			if(Pin->GetNode() == DirectManipulationSubject.Get())
			{
				bRefreshDirectionManipulationTargetsRequired = true;
			}
		}
	}
	else if(InNotifType == ERigVMGraphNotifType::LinkAdded ||
		InNotifType == ERigVMGraphNotifType::LinkRemoved)
	{
		if(const URigVMLink* Link = Cast<URigVMLink>(InSubject))
		{
			if((Link->GetSourceNode() == DirectManipulationSubject.Get()) ||
				(Link->GetTargetNode() == DirectManipulationSubject.Get()))
			{
				bRefreshDirectionManipulationTargetsRequired = true;
			}
		}
	}
}

void FTLLRN_ControlRigEditor::OnCreateGraphEditorCommands(TSharedPtr<FUICommandList> GraphEditorCommandsList)
{
	ITLLRN_ControlRigEditor::OnCreateGraphEditorCommands(GraphEditorCommandsList);

	GraphEditorCommandsList->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().RequestDirectManipulationPosition,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::HandleRequestDirectManipulationPosition));
	GraphEditorCommandsList->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().RequestDirectManipulationRotation,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::HandleRequestDirectManipulationRotation));
	GraphEditorCommandsList->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().RequestDirectManipulationScale,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::HandleRequestDirectManipulationScale));
}

void FTLLRN_ControlRigEditor::HandleVMCompiledEvent(UObject* InCompiledObject, URigVM* InVM, FRigVMExtendedExecuteContext& InContext)
{
	ITLLRN_ControlRigEditor::HandleVMCompiledEvent(InCompiledObject, InVM, InContext);

	if(bRefreshDirectionManipulationTargetsRequired)
	{
		RefreshDirectManipulationTextList();
		bRefreshDirectionManipulationTargetsRequired = false;
	}

	if(UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = GetTLLRN_ControlRigBlueprint())
	{
		if(UTLLRN_ControlRig* TLLRN_ControlRig = InVM->GetTypedOuter<UTLLRN_ControlRig>())
		{
			TLLRN_ControlRigBlueprint->UpdateElementKeyRedirector(TLLRN_ControlRig);
		}
	}
}

void FTLLRN_ControlRigEditor::SaveAsset_Execute()
{
	FRigVMEditor::SaveAsset_Execute();

	// Save the new state of the hierarchy in the default object, so that it has the correct values on load
	UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj());
	if(const UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		UTLLRN_ControlRig* CDO = TLLRN_ControlRig->GetClass()->GetDefaultObject<UTLLRN_ControlRig>();
		CDO->DynamicHierarchy->CopyHierarchy(RigBlueprint->Hierarchy);
		RigBlueprint->UpdateElementKeyRedirector(CDO);
	}

	FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
	ActionDatabase.ClearAssetActions(UTLLRN_ControlRigBlueprint::StaticClass());
	ActionDatabase.RefreshClassActions(UTLLRN_ControlRigBlueprint::StaticClass());
}

void FTLLRN_ControlRigEditor::SaveAssetAs_Execute()
{
	FRigVMEditor::SaveAssetAs_Execute();

	// Save the new state of the hierarchy in the default object, so that it has the correct values on load
	UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj());
	if(const UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		UTLLRN_ControlRig* CDO = TLLRN_ControlRig->GetClass()->GetDefaultObject<UTLLRN_ControlRig>();
		CDO->DynamicHierarchy->CopyHierarchy(RigBlueprint->Hierarchy);
		RigBlueprint->UpdateElementKeyRedirector(CDO);
	}

	FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
	ActionDatabase.ClearAssetActions(UTLLRN_ControlRigBlueprint::StaticClass());
	ActionDatabase.RefreshClassActions(UTLLRN_ControlRigBlueprint::StaticClass());
}

bool FTLLRN_ControlRigEditor::IsTLLRN_ModularRig() const
{
	if(UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
	{
		return RigBlueprint->IsTLLRN_ModularRig();
	}
	return false;
}

bool FTLLRN_ControlRigEditor::IsTLLRN_RigModule() const
{
	if(UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
	{
		return RigBlueprint->IsTLLRN_ControlTLLRN_RigModule();
	}
	return false;
}

FName FTLLRN_ControlRigEditor::GetToolkitFName() const
{
	return FName("TLLRN_ControlRigEditor");
}

FText FTLLRN_ControlRigEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "TLLRN Control Rig Editor");
}

FString FTLLRN_ControlRigEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "TLLRN Control Rig Editor ").ToString();
}

FReply FTLLRN_ControlRigEditor::OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph)
{
	const FReply SuperReply = FRigVMEditor::OnSpawnGraphNodeByShortcut(InChord, InPosition, InGraph);
	if(SuperReply.IsEventHandled())
	{
		return SuperReply;
	}

	if(!InChord.HasAnyModifierKeys())
	{
		if(UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(InGraph))
		{
			if(URigVMController* Controller = RigGraph->GetController())
			{
				if(InChord.Key == EKeys::S)
				{
					Controller->AddUnitNode(FRigVMFunction_Sequence::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), InPosition, FString(), true, true);
				}
				else if(InChord.Key == EKeys::One)
				{
					Controller->AddUnitNode(FTLLRN_RigUnit_GetTransform::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), InPosition, FString(), true, true);
				}
				else if(InChord.Key == EKeys::Two)
				{
					Controller->AddUnitNode(FTLLRN_RigUnit_SetTransform::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), InPosition, FString(), true, true);
				}
				else if(InChord.Key == EKeys::Three)
				{
					Controller->AddUnitNode(FTLLRN_RigUnit_ParentConstraint::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), InPosition, FString(), true, true);
				}
				else if(InChord.Key == EKeys::Four)
				{
					Controller->AddUnitNode(FTLLRN_RigUnit_GetControlFloat::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), InPosition, FString(), true, true);
				}
				else if(InChord.Key == EKeys::Five)
				{
					Controller->AddUnitNode(FTLLRN_RigUnit_SetCurveValue::StaticStruct(), FTLLRN_RigUnit::GetMethodName(), InPosition, FString(), true, true);
				}
			}
		}
	}

	return FReply::Unhandled();
}

void FTLLRN_ControlRigEditor::PostTransaction(bool bSuccess, const FTransaction* Transaction, bool bIsRedo)
{
	EnsureValidTLLRN_RigElementsInDetailPanel();

	if (UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
	{
		// Do not compile here. TLLRN_ControlRigBlueprint::PostTransacted decides when it is necessary to compile depending
		// on the properties that are affected.
		//Compile();

		UpdateRigVMHost();
		
		USkeletalMesh* PreviewMesh = GetPersonaToolkit()->GetPreviewScene()->GetPreviewMesh();
		if (PreviewMesh != RigBlueprint->GetPreviewMesh())
		{
			RigBlueprint->SetPreviewMesh(PreviewMesh);
			GetPersonaToolkit()->SetPreviewMesh(PreviewMesh, true);
		}

		if (UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(RigBlueprint->GetObjectBeingDebugged()))
		{
			if(UTLLRN_RigHierarchy* Hierarchy = DebuggedTLLRN_ControlRig->GetHierarchy())
			{
				if(Hierarchy->Num() == 0)
				{
					OnHierarchyChanged();
				}
			}
		}

		if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
		{
			EditMode->RequestToRecreateControlShapeActors();
		}
	}
}

void FTLLRN_ControlRigEditor::EnsureValidTLLRN_RigElementsInDetailPanel()
{
	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBP = GetTLLRN_ControlRigBlueprint();
	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBP->Hierarchy; 

	TArray< TWeakObjectPtr<UObject> > SelectedObjects = Inspector->GetSelectedObjects();
	for (TWeakObjectPtr<UObject> SelectedObject : SelectedObjects)
	{
		if (SelectedObject.IsValid())
		{
			if(URigVMDetailsViewWrapperObject* WrapperObject = Cast<URigVMDetailsViewWrapperObject>(SelectedObject.Get()))
			{
				if(const UScriptStruct* WrappedStruct = WrapperObject->GetWrappedStruct())
				{
					if (WrappedStruct->IsChildOf(FTLLRN_RigBaseElement::StaticStruct()))
					{
						FTLLRN_RigElementKey Key = WrapperObject->GetContent<FTLLRN_RigBaseElement>().GetKey();
						if(!Hierarchy->Contains(Key))
						{
							ClearDetailObject();
						}
					}
				}
			}
		}
	}
}

void FTLLRN_ControlRigEditor::OnAnimInitialized()
{
	UTLLRN_ControlRigSkeletalMeshComponent* EditorSkelComp = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent());
	if (EditorSkelComp)
	{
		EditorSkelComp->bRequiredBonesUpToDateDuringTick = 0;

		UTLLRN_ControlRigLayerInstance* AnimInstance = Cast<UTLLRN_ControlRigLayerInstance>(EditorSkelComp->GetAnimInstance());
		if (AnimInstance && GetTLLRN_ControlRig())
		{
			// update control rig data to anim instance since animation system has been reinitialized
			FInputBlendPose Filter;
			AnimInstance->ResetTLLRN_ControlRigTracks();
			AnimInstance->AddTLLRN_ControlRigTrack(0, GetTLLRN_ControlRig());
			AnimInstance->UpdateTLLRN_ControlRigTrack(0, 1.0f, FTLLRN_ControlRigIOSettings::MakeEnabled(), bExecutionTLLRN_ControlRig);
		}
	}
}

void FTLLRN_ControlRigEditor::HandleVMExecutedEvent(URigVMHost* InHost, const FName& InEventName)
{
	FRigVMEditor::HandleVMExecutedEvent(InHost, InEventName);

	if (UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBP = GetTLLRN_ControlRigBlueprint())
	{
		UTLLRN_RigHierarchy* Hierarchy = GetHierarchyBeingDebugged(); 

		TArray< TWeakObjectPtr<UObject> > SelectedObjects = Inspector->GetSelectedObjects();
		for (TWeakObjectPtr<UObject> SelectedObject : SelectedObjects)
		{
			if (SelectedObject.IsValid())
			{
				if(URigVMDetailsViewWrapperObject* WrapperObject = Cast<URigVMDetailsViewWrapperObject>(SelectedObject.Get()))
				{
					if (const UScriptStruct* Struct = WrapperObject->GetWrappedStruct())
					{
						if(Struct->IsChildOf(FTLLRN_RigBaseElement::StaticStruct()))
						{
							const FTLLRN_RigElementKey Key = WrapperObject->GetContent<FTLLRN_RigBaseElement>().GetKey();

							FTLLRN_RigBaseElement* Element = Hierarchy->Find(Key);
							if(Element == nullptr)
							{
								ClearDetailObject();
								break;
							}

							if(FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Element))
							{
								// compute all transforms
								Hierarchy->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal);
								Hierarchy->GetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
								Hierarchy->GetTransform(ControlElement, ETLLRN_RigTransformType::InitialGlobal);
								Hierarchy->GetTransform(ControlElement, ETLLRN_RigTransformType::InitialLocal);
								Hierarchy->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal);
								Hierarchy->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
								Hierarchy->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::InitialGlobal);
								Hierarchy->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::InitialLocal);
								Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal);
								Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
								Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::InitialGlobal);
								Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::InitialLocal);

								WrapperObject->SetContent<FTLLRN_RigControlElement>(*ControlElement);
							}
							else if(FTLLRN_RigTransformElement* TransformElement = Cast<FTLLRN_RigTransformElement>(Element))
							{
								// compute all transforms
								Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentGlobal);
								Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentLocal);
								Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::InitialGlobal);
								Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::InitialLocal);

								WrapperObject->SetContent<FTLLRN_RigTransformElement>(*TransformElement);
							}
							else
							{
								WrapperObject->SetContent<FTLLRN_RigBaseElement>(*Element);
							}
						}
					}
				}
			}
		}

		// update transient controls on nodes / pins
		if(UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(TLLRN_ControlRigBP->GetObjectBeingDebugged()))
		{
			if(!DebuggedTLLRN_ControlRig->TLLRN_RigUnitManipulationInfos.IsEmpty())
			{
				const FTLLRN_RigHierarchyRedirectorGuard RedirectorGuard(DebuggedTLLRN_ControlRig);
				FTLLRN_ControlRigExecuteContext& ExecuteContext = DebuggedTLLRN_ControlRig->GetRigVMExtendedExecuteContext().GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
				
				for(const TSharedPtr<FRigDirectManipulationInfo>& ManipulationInfo : DebuggedTLLRN_ControlRig->TLLRN_RigUnitManipulationInfos)
				{
					if(const URigVMUnitNode* Node = ManipulationInfo->Node.Get())
					{
						const UScriptStruct* ScriptStruct = Node->GetScriptStruct();
						if(ScriptStruct == nullptr)
						{
							continue;
						}

						TSharedPtr<FStructOnScope> NodeInstance = Node->ConstructLiveStructInstance(DebuggedTLLRN_ControlRig);
						if(!NodeInstance.IsValid() || !NodeInstance->IsValid())
						{
							continue;
						}

						// if we are not manipulating right now - reset the info so that it can follow the hierarchy
						if (FTLLRN_ControlRigEditorEditMode* EditMode = GetEditMode())
						{
							if(!EditMode->bIsTracking)
							{
								ManipulationInfo->Reset();
							}
						}
				
						FTLLRN_RigUnit* UnitInstance = UTLLRN_ControlRig::GetTLLRN_RigUnitInstanceFromScope(NodeInstance);
						UnitInstance->UpdateHierarchyForDirectManipulation(Node, NodeInstance, ExecuteContext, ManipulationInfo);
						ManipulationInfo->bInitialized = true;
						UnitInstance->PerformDebugDrawingForDirectManipulation(Node, NodeInstance, ExecuteContext, ManipulationInfo);
					}
				}
			}
		}		
	}
}

void FTLLRN_ControlRigEditor::CreateEditorModeManager()
{
	EditorModeManager = MakeShareable(FModuleManager::LoadModuleChecked<FPersonaModule>("Persona").CreatePersonaEditorModeManager());
}

void FTLLRN_ControlRigEditor::Tick(float DeltaTime)
{
	FRigVMEditor::Tick(DeltaTime);

	bool bDrawHierarchyBones = false;

	// tick the control rig in case we don't have skeletal mesh
	if (UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig();
		if (Blueprint->GetPreviewMesh() == nullptr && 
			TLLRN_ControlRig != nullptr && 
			bExecutionTLLRN_ControlRig)
		{
			{
				// prevent transient controls from getting reset
				UTLLRN_ControlRig::FTransientControlPoseScope	PoseScope(TLLRN_ControlRig);
				// reset transforms here to prevent additive transforms from accumulating to INF
				TLLRN_ControlRig->GetHierarchy()->ResetPoseToInitial(ETLLRN_RigElementType::Bone);
			}

			if (PreviewInstance)
			{
				// since we don't have a preview mesh the anim instance cannot deal with the modify bone
				// functionality. we need to perform this manually to ensure the pose is kept.
				const TArray<FAnimNode_ModifyBone>& BoneControllers = PreviewInstance->GetBoneControllers();
				for(const FAnimNode_ModifyBone& ModifyBone : BoneControllers)
				{
					const FTLLRN_RigElementKey BoneKey(ModifyBone.BoneToModify.BoneName, ETLLRN_RigElementType::Bone);
					const FTransform BoneTransform(ModifyBone.Rotation, ModifyBone.Translation, ModifyBone.Scale);
					TLLRN_ControlRig->GetHierarchy()->SetLocalTransform(BoneKey, BoneTransform);
				}
			}
			
			TLLRN_ControlRig->SetDeltaTime(DeltaTime);
			TLLRN_ControlRig->Evaluate_AnyThread();
			bDrawHierarchyBones = true;
		}
	}

	if (FTLLRN_ControlRigEditorEditMode* EditMode = GetEditMode())
	{
		if (bDrawHierarchyBones)
		{
			EditMode->bDrawHierarchyBones = bDrawHierarchyBones;
		}
	}

	if(WeakGroundActorPtr.IsValid())
	{
		const TSharedRef<IPersonaPreviewScene> CurrentPreviewScene = GetPersonaToolkit()->GetPreviewScene();
		const float FloorOffset = CurrentPreviewScene->GetFloorOffset();
		const FTransform FloorTransform(FRotator(0, 0, 0), FVector(0, 0, -(FloorOffset)), FVector(4.0f, 4.0f, 1.0f));
		WeakGroundActorPtr->GetStaticMeshComponent()->SetRelativeTransform(FloorTransform);
	}
}

void FTLLRN_ControlRigEditor::HandleViewportCreated(const TSharedRef<class IPersonaViewport>& InViewport)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	PreviewViewport = InViewport;

	// TODO: this is duplicated code from FAnimBlueprintEditor, would be nice to consolidate. 
	auto GetCompilationStateText = [this]()
	{
		if (UBlueprint* Blueprint = GetBlueprintObj())
		{
			switch (Blueprint->Status)
			{
			case BS_UpToDate:
			case BS_UpToDateWithWarnings:
				// Fall thru and return empty string
				break;
			case BS_Dirty:
				return LOCTEXT("TLLRN_ControlRigBP_Dirty", "Preview out of date");
			case BS_Error:
				return LOCTEXT("TLLRN_ControlRigBP_CompileError", "Compile Error");
			default:
				return LOCTEXT("TLLRN_ControlRigBP_UnknownStatus", "Unknown Status");
			}
		}

		return FText::GetEmpty();
	};

	auto GetCompilationStateVisibility = [this]()
	{
		if (const UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
		{
			if(Blueprint->IsTLLRN_ModularRig())
			{
				if(Blueprint->GetPreviewMesh() == nullptr)
				{
					return EVisibility::Collapsed;
				}
			}
			const bool bUpToDate = (Blueprint->Status == BS_UpToDate) || (Blueprint->Status == BS_UpToDateWithWarnings);
			return bUpToDate ? EVisibility::Collapsed : EVisibility::Visible;
		}

		return EVisibility::Collapsed;
	};

	auto GetCompileButtonVisibility = [this]()
	{
		if (const UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
		{
			return (Blueprint->Status == BS_Dirty) ? EVisibility::Visible : EVisibility::Collapsed;
		}
		return EVisibility::Collapsed;
	};

	auto CompileBlueprint = [this]()
	{
		if (UBlueprint* Blueprint = GetBlueprintObj())
		{
			if (!Blueprint->IsUpToDate())
			{
				Compile();
			}
		}

		return FReply::Handled();
	};

	auto GetErrorSeverity = [this]()
	{
		if (UBlueprint* Blueprint = GetBlueprintObj())
		{
			return (Blueprint->Status == BS_Error) ? EMessageSeverity::Error : EMessageSeverity::Warning;
		}

		return EMessageSeverity::Warning;
	};

	auto GetIcon = [this]()
	{
		if (UBlueprint* Blueprint = GetBlueprintObj())
		{
			return (Blueprint->Status == BS_Error) ? FEditorFontGlyphs::Exclamation_Triangle : FEditorFontGlyphs::Eye;
		}

		return FEditorFontGlyphs::Eye;
	};

	auto GetChangingShapeTransformText = [this]()
	{
		if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
		{
			FText HotKeyText = EditMode->GetToggleControlShapeTransformEditHotKey();

			if (!HotKeyText.IsEmpty())
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("HotKey"), HotKeyText);
				return FText::Format(LOCTEXT("TLLRN_ControlRigBPViewportShapeTransformEditNotificationPress", "Currently Manipulating Shape Transform - Press {HotKey} to Exit"), Args);
			}
		}
		
		return LOCTEXT("TLLRN_ControlRigBPViewportShapeTransformEditNotificationAssign", "Currently Manipulating Shape Transform - Assign a Hotkey and Use It to Exit");
	};

	auto GetChangingShapeTransformTextVisibility = [this]()
	{
		if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
		{
			return EditMode->bIsChangingControlShapeTransform ? EVisibility::Visible : EVisibility::Collapsed;
		}
		return EVisibility::Collapsed;
	};

	{
		FPersonaViewportNotificationOptions DirectManipulationNotificationOptions(TAttribute<EVisibility>::CreateSP(this, &FTLLRN_ControlRigEditor::GetDirectManipulationVisibility));
		DirectManipulationNotificationOptions.OnGetBrushOverride = TAttribute<const FSlateBrush*>(FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Viewport.Notification.DirectManipulation"));

		InViewport->AddNotification(
			EMessageSeverity::Info,
			false,
			SNew(SHorizontalBox)
			.Visibility(this, &FTLLRN_ControlRigEditor::GetDirectManipulationVisibility)
			.ToolTipText(LOCTEXT("DirectManipulation", "Direct Manipulation"))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4.0f, 4.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
					.Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
					.Text(FEditorFontGlyphs::Crosshairs)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SAssignNew(DirectManipulationCombo, SComboBox<TSharedPtr<FString>>)
					.ContentPadding(FMargin(4.0f, 2.0f))
					.OptionsSource(&DirectManipulationTextList)
					.OnGenerateWidget_Lambda([this](TSharedPtr<FString> Item)
					{ 
						return SNew(SBox)
							.MaxDesiredWidth(600.0f)
							[
								SNew(STextBlock)
								.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
								.Text(FText::FromString(*Item))
							];
					} )	
					.OnSelectionChanged(this, &FTLLRN_ControlRigEditor::OnDirectManipulationChanged)
					[
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
						.Text(this, &FTLLRN_ControlRigEditor::GetDirectionManipulationText)
					]
				]
			],
			DirectManipulationNotificationOptions
		);
	}

	{
		InViewport->AddNotification(
			EMessageSeverity::Warning,
			false,
			SNew(SHorizontalBox)
			.Visibility(this, &FTLLRN_ControlRigEditor::GetConnectorWarningVisibility)
			.ToolTipText(LOCTEXT("ConnectorWarningTooltip", "This rig has unresolved connectors."))
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4.0f, 4.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
					.Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
					.Text(FEditorFontGlyphs::Exclamation_Triangle)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
					.Text(this, &FTLLRN_ControlRigEditor::GetConnectorWarningText)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ForegroundColor(FSlateColor::UseForeground())
					.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
					.ToolTipText(LOCTEXT("ConnectorWarningNavigateTooltip", "Navigate to the first unresolved connector in the hierarchy"))
					.OnClicked(this, &FTLLRN_ControlRigEditor::OnNavigateToConnectorWarning)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.0f, 0.0f, 4.0f, 0.0f)
						[
							SNew(STextBlock)
							.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
							.Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
							.Text(FEditorFontGlyphs::Cog)
						]
						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
							.Text(LOCTEXT("ConnectorWarningNavigateButtonLabel", "Discover"))
						]
					]
				]
			],
			FPersonaViewportNotificationOptions(TAttribute<EVisibility>::CreateSP(this, &FTLLRN_ControlRigEditor::GetConnectorWarningVisibility))
		);
	}	

	InViewport->AddNotification(MakeAttributeLambda(GetErrorSeverity),
		false,
		SNew(SHorizontalBox)
		.Visibility_Lambda(GetCompilationStateVisibility)
		+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(4.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			.ToolTipText_Lambda(GetCompilationStateText)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(STextBlock)
				.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
				.Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
				.Text_Lambda(GetIcon)
			]
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text_Lambda(GetCompilationStateText)
				.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
			]
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 0.0f)
		[
			SNew(SButton)
			.ForegroundColor(FSlateColor::UseForeground())
			.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
			.Visibility_Lambda(GetCompileButtonVisibility)
			.ToolTipText(LOCTEXT("TLLRN_ControlRigBPViewportCompileButtonToolTip", "Compile this Animation Blueprint to update the preview to reflect any recent changes."))
			.OnClicked_Lambda(CompileBlueprint)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
					.Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
					.Text(FEditorFontGlyphs::Cog)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
					.Text(LOCTEXT("TLLRN_ControlRigBPViewportCompileButtonLabel", "Compile"))
				]
			]
		],
		FPersonaViewportNotificationOptions(TAttribute<EVisibility>::Create(GetCompilationStateVisibility))
	);

	FPersonaViewportNotificationOptions ChangePreviewMeshNotificationOptions;
	ChangePreviewMeshNotificationOptions.OnGetVisibility = IsTLLRN_ModularRig() ? EVisibility::Visible : EVisibility::Collapsed;
	//ChangePreviewMeshNotificationOptions.OnGetBrushOverride = TAttribute<const FSlateBrush*>(FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Viewport.Notification.ChangeShapeTransform"));

	// notification to allow to change the preview mesh directly in the viewport
	InViewport->AddNotification(TAttribute<EMessageSeverity::Type>::CreateLambda([this]()
		{
			if(const UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
			{
				if(Blueprint->GetPreviewMesh() == nullptr)
				{
					return EMessageSeverity::Warning;
				}
			}
			return EMessageSeverity::Info;
		}),
		false,
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MissingPreviewMesh", "Please choose a preview mesh!"))
			.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
			.Visibility_Lambda([this]()
			{
				if(const UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
				{
					if(Blueprint->GetPreviewMesh())
					{
						return EVisibility::Collapsed;
					}
				}
				return EVisibility::Visible;
			})
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 4.0f)
		[
			SNew(SObjectPropertyEntryBox)
			.ObjectPath_Lambda([this]()
			{
				if(const UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
				{
					if(const USkeletalMesh* PreviewMesh = Blueprint->GetPreviewMesh())
					{
						return PreviewMesh->GetPathName();
					}
				}
				return FString();
			})
			.AllowedClass(USkeletalMesh::StaticClass())
			.OnObjectChanged_Lambda([this](const FAssetData& InAssetData)
			{
				if(const UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
				{
					if(USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(InAssetData.GetAsset()))
					{
						const TSharedRef<IPersonaPreviewScene> CurrentPreviewScene = GetPersonaToolkit()->GetPreviewScene();
						CurrentPreviewScene->SetPreviewMesh(SkeletalMesh);
					}
				}
			})
			.AllowCreate(false)
			.AllowClear(false)
			.DisplayUseSelected(false)
			.DisplayBrowse(false)
			.NewAssetFactories(TArray<UFactory*>())
		],
		ChangePreviewMeshNotificationOptions
	);

	FPersonaViewportNotificationOptions ChangeShapeTransformNotificationOptions;
	ChangeShapeTransformNotificationOptions.OnGetVisibility = TAttribute<EVisibility>::Create(GetChangingShapeTransformTextVisibility);
	ChangeShapeTransformNotificationOptions.OnGetBrushOverride = TAttribute<const FSlateBrush*>(FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Viewport.Notification.ChangeShapeTransform"));

	// notification that shows when users enter the mode that allows them to change shape transform
	InViewport->AddNotification(EMessageSeverity::Type::Info,
		false,
		SNew(SHorizontalBox)
		.Visibility_Lambda(GetChangingShapeTransformTextVisibility)
		+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(4.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			.ToolTipText_Lambda(GetChangingShapeTransformText)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda(GetChangingShapeTransformText)
				.TextStyle(FAppStyle::Get(), "AnimViewport.MessageText")
			]
		],
		ChangeShapeTransformNotificationOptions
	);

	InViewport->AddToolbarExtender(TEXT("AnimViewportDefaultCamera"), FMenuExtensionDelegate::CreateLambda(
		[&](FMenuBuilder& InMenuBuilder)
		{
			InMenuBuilder.AddMenuSeparator(TEXT("TLLRN Control Rig"));
			InMenuBuilder.BeginSection("TLLRN_ControlRig", LOCTEXT("TLLRN_ControlRig_Label", "TLLRN Control Rig"));
			{
				InMenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().ToggleControlVisibility);
				InMenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().ToggleControlsAsOverlay);
				InMenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().ToggleDrawNulls);
				InMenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().ToggleDrawSockets);
				InMenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().ToggleDrawAxesOnSelection);

				InMenuBuilder.AddWidget(
					SNew(SBox)
					.HAlign(HAlign_Right)
					[
						SNew(SBox)
						.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
						.WidthOverride(100.0f)
						[
							SNew(SNumericEntryBox<float>)
							.Font(FAppStyle::GetFontStyle(TEXT("MenuItem.Font")))
							.AllowSpin(true)
							.MinSliderValue(0.0f)
							.MaxSliderValue(100.0f)
							.Value(this, &FTLLRN_ControlRigEditor::GetToolbarAxesScale)
							.OnValueChanged(this, &FTLLRN_ControlRigEditor::OnToolbarAxesScaleChanged)
							.ToolTipText(LOCTEXT("TLLRN_ControlRigAxesScaleToolTip", "Scale of axes drawn for selected rig elements"))
						]
					], 
					LOCTEXT("TLLRN_ControlRigAxesScale", "Axes Scale")
				);
			}
			InMenuBuilder.EndSection();
		}
	));

	auto GetBorderColorAndOpacity = [this]()
	{
		FLinearColor Color = FLinearColor::Transparent;
		const TArray<FName> EventQueue = GetEventQueue();
		if(EventQueue == ConstructionEventQueue)
		{
			Color = UTLLRN_ControlRigEditorSettings::Get()->ConstructionEventBorderColor;
		}
		if(EventQueue == BackwardsSolveEventQueue)
		{
			Color = UTLLRN_ControlRigEditorSettings::Get()->BackwardsSolveBorderColor;
		}
		if(EventQueue == BackwardsAndForwardsSolveEventQueue)
		{
			Color = UTLLRN_ControlRigEditorSettings::Get()->BackwardsAndForwardsBorderColor;
		}
		return Color;
	};

	auto GetBorderVisibility = [this]()
	{
		EVisibility Visibility = EVisibility::Collapsed;
		if (GetEventQueueComboValue() != 0)
		{
			Visibility = EVisibility::HitTestInvisible;
		}
		return Visibility;
	};
	
	InViewport->AddOverlayWidget(
		SNew(SBorder)
        .BorderImage(FTLLRN_ControlRigEditorStyle::Get().GetBrush( "TLLRN_ControlRig.Viewport.Border"))
        .BorderBackgroundColor_Lambda(GetBorderColorAndOpacity)
        .Visibility_Lambda(GetBorderVisibility)
        .Padding(0.0f)
        .ShowEffectWhenDisabled(false)
	);

	if (CVarShowSchematicPanelOverlay->GetBool())
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
		{
			if (Blueprint->IsTLLRN_ModularRig())
			{
				SchematicViewport = SNew(SSchematicGraphPanel)
																.GraphData(&SchematicModel)
																.IsOverlay(true)
																.PaddingLeft(30)
																.PaddingRight(30)
																.PaddingTop(60)
																.PaddingBottom(60)
																.PaddingInterNode(5)
																.Visibility(this, &FTLLRN_ControlRigEditor::GetSchematicOverlayVisibility)
				;
				InViewport->AddOverlayWidget(SchematicViewport.ToSharedRef());

				
				InViewport->AddToolbarExtender(TEXT("TLLRN_ControlRig"), FMenuExtensionDelegate::CreateLambda([&](FMenuBuilder& InMenuBuilder)
					{
						InMenuBuilder.AddMenuSeparator(TEXT("Modular Rig"));
						InMenuBuilder.BeginSection("TLLRN_ModularRig", LOCTEXT("TLLRN_ModularRig_Label", "Modular Rig"));
						{
							InMenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().ToggleSchematicViewportVisibility);
						}
						InMenuBuilder.EndSection();
					}
				));
			}
		}
	}
	
	InViewport->GetKeyDownDelegate().BindLambda([&](const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) -> FReply {
		if (OnKeyDownDelegate.IsBound())
		{
			FReply Reply = OnKeyDownDelegate.Execute(MyGeometry, InKeyEvent);
			if(Reply.IsEventHandled())
			{
				return Reply;
			}
		}
		if(GetToolkitCommands()->ProcessCommandBindings(InKeyEvent.GetKey(), InKeyEvent.GetModifierKeys(), false))
		{
			return FReply::Handled();
		}
		return FReply::Unhandled();
	});

	// register callbacks to allow control rig asset to store the Bone Size viewport setting
	FEditorViewportClient& ViewportClient = InViewport->GetViewportClient();
	if (FAnimationViewportClient* AnimViewportClient = static_cast<FAnimationViewportClient*>(&ViewportClient))
	{
		AnimViewportClient->OnSetBoneSize.BindLambda([this](float InBoneSize)
		{
			if (UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
			{
				RigBlueprint->Modify();
				RigBlueprint->DebugBoneRadius = InBoneSize;
			}
		});
		
		AnimViewportClient->OnGetBoneSize.BindLambda([this]() -> float
		{
			if (UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
			{
				return RigBlueprint->DebugBoneRadius;
			}

			return 1.0f;
		});
	}
}

TOptional<float> FTLLRN_ControlRigEditor::GetToolbarAxesScale() const
{
	if (const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		return Settings->AxisScale;
	}
	return 0.f;
}

void FTLLRN_ControlRigEditor::OnToolbarAxesScaleChanged(float InValue)
{
	if (UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		Settings->AxisScale = InValue;
	}
}

void FTLLRN_ControlRigEditor::HandleToggleControlVisibility()
{
	if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
	{
		EditMode->ToggleAllManipulators();
	}
}

bool FTLLRN_ControlRigEditor::AreControlsVisible() const
{
	if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
	{
		return EditMode->AreControlsVisible();
	}
	return false;
}

void FTLLRN_ControlRigEditor::HandleToggleControlsAsOverlay()
{
	if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
	{
		EditMode->bShowControlsAsOverlay = !EditMode->bShowControlsAsOverlay;
		EditMode->UpdateSelectabilityOnSkeletalMeshes(GetTLLRN_ControlRig(), !EditMode->bShowControlsAsOverlay);
		EditMode->RequestToRecreateControlShapeActors();
	}
}

bool FTLLRN_ControlRigEditor::AreControlsAsOverlay() const
{
	if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
	{
		return EditMode->bShowControlsAsOverlay;
	}
	return false;
}

void FTLLRN_ControlRigEditor::HandleToggleSchematicViewport()
{
	if(SchematicViewport.IsValid())
	{
		SchematicModel.UpdateTLLRN_ControlRigContent();
		bSchematicViewPortIsHidden = !bSchematicViewPortIsHidden; 
	}
}

bool FTLLRN_ControlRigEditor::IsSchematicViewportActive() const
{
	if (SchematicViewport.IsValid())
	{
		return SchematicViewport->GetVisibility() != EVisibility::Hidden;
	}
	return false;
}

EVisibility FTLLRN_ControlRigEditor::GetSchematicOverlayVisibility() const
{
	if(bSchematicViewPortIsHidden)
	{
		return EVisibility::Hidden;
	}
	
	if(const UTLLRN_RigHierarchy* Hierarchy = GetHierarchyBeingDebugged())
	{
		TArray<const FTLLRN_RigBaseElement*> SelectedElements = Hierarchy->GetSelectedElements();
		if(SelectedElements.ContainsByPredicate([](const FTLLRN_RigBaseElement* InSelectedElement) -> bool
		{
			return InSelectedElement->IsA<FTLLRN_RigControlElement>();
		}))
		{
			return EVisibility::Hidden;
		}
	}
	return EVisibility::SelfHitTestInvisible;
}

bool FTLLRN_ControlRigEditor::GetToolbarDrawAxesOnSelection() const
{
	if (const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		return Settings->bDisplayAxesOnSelection;
	}
	return false;
}

void FTLLRN_ControlRigEditor::HandleToggleToolbarDrawAxesOnSelection()
{
	if (UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		Settings->bDisplayAxesOnSelection = !Settings->bDisplayAxesOnSelection;
	}
}

bool FTLLRN_ControlRigEditor::IsToolbarDrawNullsEnabled() const
{
	if (const UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		if (!TLLRN_ControlRig->IsConstructionModeEnabled())
		{
			return true;
		}
	}
	return false;
}

bool FTLLRN_ControlRigEditor::GetToolbarDrawNulls() const
{
	if (const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		return Settings->bDisplayNulls;
	}
	return false;
}

void FTLLRN_ControlRigEditor::HandleToggleToolbarDrawNulls()
{
	if (UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		Settings->bDisplayNulls = !Settings->bDisplayNulls;
	}
}

bool FTLLRN_ControlRigEditor::IsToolbarDrawSocketsEnabled() const
{
	if (const UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		if (!TLLRN_ControlRig->IsConstructionModeEnabled())
		{
			return true;
		}
	}
	return false;
}

bool FTLLRN_ControlRigEditor::GetToolbarDrawSockets() const
{
	if (const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		return Settings->bDisplaySockets;
	}
	return false;
}

void FTLLRN_ControlRigEditor::HandleToggleToolbarDrawSockets()
{
	if (UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		Settings->bDisplaySockets = !Settings->bDisplaySockets;
	}
}

bool FTLLRN_ControlRigEditor::IsConstructionModeEnabled() const
{
	return GetEventQueue() == ConstructionEventQueue;
}

bool FTLLRN_ControlRigEditor::IsDebuggingExternalTLLRN_ControlRig(const UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	if(InTLLRN_ControlRig == nullptr)
	{
		if(const UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = GetTLLRN_ControlRigBlueprint())
		{
			InTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(TLLRN_ControlRigBlueprint->GetObjectBeingDebugged());
		}
	}
	return InTLLRN_ControlRig != GetTLLRN_ControlRig();
}

bool FTLLRN_ControlRigEditor::ShouldExecuteTLLRN_ControlRig(const UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	return (!IsDebuggingExternalTLLRN_ControlRig(InTLLRN_ControlRig)) && bExecutionTLLRN_ControlRig;
}

void FTLLRN_ControlRigEditor::HandlePreviewSceneCreated(const TSharedRef<IPersonaPreviewScene>& InPersonaPreviewScene)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	// load a ground mesh
	static const TCHAR* GroundAssetPath = TEXT("/Engine/MapTemplates/SM_Template_Map_Floor.SM_Template_Map_Floor");
	UStaticMesh* FloorMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, GroundAssetPath, NULL, LOAD_None, NULL));
	UMaterial* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	check(FloorMesh);
	check(DefaultMaterial);

	// leave some metadata on the world used for debug object labeling
	if(FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(InPersonaPreviewScene->GetWorld()))
	{
		static constexpr TCHAR Format[] = TEXT("TLLRN_ControlRigEditor (%s)");
		WorldContext->CustomDescription = FString::Printf(Format, *GetBlueprintObj()->GetName());
	}

	// create ground mesh actor
	AStaticMeshActor* GroundActor = InPersonaPreviewScene->GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform::Identity);
	GroundActor->SetFlags(RF_Transient);
	GroundActor->GetStaticMeshComponent()->SetStaticMesh(FloorMesh);
	GroundActor->GetStaticMeshComponent()->SetMaterial(0, DefaultMaterial);
	GroundActor->SetMobility(EComponentMobility::Static);
	GroundActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GroundActor->GetStaticMeshComponent()->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	GroundActor->GetStaticMeshComponent()->bSelectable = false;
	// this will be an invisible collision box that users can use to test traces
	GroundActor->GetStaticMeshComponent()->SetVisibility(false);

	WeakGroundActorPtr = GroundActor;

	AAnimationEditorPreviewActor* Actor = InPersonaPreviewScene->GetWorld()->SpawnActor<AAnimationEditorPreviewActor>(AAnimationEditorPreviewActor::StaticClass(), FTransform::Identity);
	Actor->SetFlags(RF_Transient);
	InPersonaPreviewScene->SetActor(Actor);

	// Create the preview component
	UTLLRN_ControlRigSkeletalMeshComponent* EditorSkelComp = NewObject<UTLLRN_ControlRigSkeletalMeshComponent>(Actor);
	EditorSkelComp->SetSkeletalMesh(InPersonaPreviewScene->GetPersonaToolkit()->GetPreviewMesh());
	InPersonaPreviewScene->SetPreviewMeshComponent(EditorSkelComp);
	bool bWasCreated = false;
	FAnimCustomInstanceHelper::BindToSkeletalMeshComponent<UTLLRN_ControlRigLayerInstance>(EditorSkelComp, bWasCreated);
	InPersonaPreviewScene->AddComponent(EditorSkelComp, FTransform::Identity);

	// set root component, so we can attach to it. 
	Actor->SetRootComponent(EditorSkelComp);
	EditorSkelComp->bSelectable = false;
	EditorSkelComp->MarkRenderStateDirty();
	
	InPersonaPreviewScene->SetAllowMeshHitProxies(false);
	InPersonaPreviewScene->SetAdditionalMeshesSelectable(false);

	PreviewInstance = nullptr;
	if (UTLLRN_ControlRigLayerInstance* TLLRN_ControlRigLayerInstance = Cast<UTLLRN_ControlRigLayerInstance>(EditorSkelComp->GetAnimInstance()))
	{
		PreviewInstance = Cast<UAnimPreviewInstance>(TLLRN_ControlRigLayerInstance->GetSourceAnimInstance());
	}
	else
	{
		PreviewInstance = Cast<UAnimPreviewInstance>(EditorSkelComp->GetAnimInstance());
	}

	// remove the preview scene undo handling - it has unwanted side effects
	InPersonaPreviewScene->UnregisterForUndo();
}

void FTLLRN_ControlRigEditor::UpdateRigVMHost()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FRigVMEditor::UpdateRigVMHost();

	UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj());
	if(UClass* Class = Blueprint->GeneratedClass)
	{
		UTLLRN_ControlRigSkeletalMeshComponent* EditorSkelComp = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent());
		UTLLRN_ControlRigLayerInstance* AnimInstance = Cast<UTLLRN_ControlRigLayerInstance>(EditorSkelComp->GetAnimInstance());
		UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig();

		if (AnimInstance && TLLRN_ControlRig)
		{
 			PreviewInstance = Cast<UAnimPreviewInstance>(AnimInstance->GetSourceAnimInstance());
			TLLRN_ControlRig->PreviewInstance = PreviewInstance;

			if (UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(Class->GetDefaultObject()))
			{
				CDO->ShapeLibraries = GetTLLRN_ControlRigBlueprint()->ShapeLibraries;
			}

			CacheNameLists();

			// When the control rig is re-instanced on compile, it loses its binding, so we refresh it here if needed
			if (!TLLRN_ControlRig->GetObjectBinding().IsValid())
			{
				TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
			}

			// initialize is moved post reinstance
			AnimInstance->ResetTLLRN_ControlRigTracks();
			AnimInstance->AddTLLRN_ControlRigTrack(0, TLLRN_ControlRig);
			AnimInstance->UpdateTLLRN_ControlRigTrack(0, 1.0f, FTLLRN_ControlRigIOSettings::MakeEnabled(), bExecutionTLLRN_ControlRig);
			AnimInstance->RecalcRequiredBones();

			// since rig has changed, rebuild draw skeleton
			EditorSkelComp->RebuildDebugDrawSkeleton();
			if (FTLLRN_ControlRigEditMode* EditMode = GetEditMode())
			{
				EditMode->SetObjects(TLLRN_ControlRig, EditorSkelComp,nullptr);
			}

			TLLRN_ControlRig->OnPreForwardsSolve_AnyThread().RemoveAll(this);
			TLLRN_ControlRig->ControlModified().RemoveAll(this);

			TLLRN_ControlRig->OnPreForwardsSolve_AnyThread().AddSP(this, &FTLLRN_ControlRigEditor::OnPreForwardsSolve_AnyThread);
			TLLRN_ControlRig->ControlModified().AddSP(this, &FTLLRN_ControlRigEditor::HandleOnControlModified);
		}

		if(IsTLLRN_ModularRig() && TLLRN_ControlRig)
		{
			if(SchematicModel.TLLRN_ControlRigBlueprint.IsValid())
			{
				SchematicModel.OnSetObjectBeingDebugged(TLLRN_ControlRig);
			}
		}
	}
}

void FTLLRN_ControlRigEditor::UpdateRigVMHost_PreClearOldHost(URigVMHost* InPreviousHost)
{
	if(TestDataStrongPtr.IsValid())
	{
		TestDataStrongPtr->ReleaseReplay();
	}
}

void FTLLRN_ControlRigEditor::CacheNameLists()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FRigVMEditor::CacheNameLists();

	if (UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBP = GetTLLRN_ControlRigBlueprint())
	{
		TArray<UEdGraph*> EdGraphs;
		TLLRN_ControlRigBP->GetAllGraphs(EdGraphs);

		UTLLRN_RigHierarchy* Hierarchy = GetHierarchyBeingDebugged();
		for (UEdGraph* Graph : EdGraphs)
		{
			UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(Graph);
			if (RigGraph == nullptr)
			{
				continue;
			}

			const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>* ShapeLibraries = &TLLRN_ControlRigBP->ShapeLibraries;
			if(const UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Hierarchy->GetTypedOuter<UTLLRN_ControlRig>())
			{
				ShapeLibraries = &DebuggedTLLRN_ControlRig->GetShapeLibraries();
			}
			RigGraph->CacheNameLists(Hierarchy, &TLLRN_ControlRigBP->DrawContainer, *ShapeLibraries);
		}
	}
}

FVector2D FTLLRN_ControlRigEditor::ComputePersonaProjectedScreenPos(const FVector& InWorldPos, bool bClampToScreenRectangle)
{
	if (PreviewViewport.IsValid())
	{
		FEditorViewportClient& Client = PreviewViewport->GetViewportClient();
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
							Client.Viewport,
							Client.GetScene(),
							Client.EngineShowFlags));
		// SceneView is deleted with the ViewFamily
		FSceneView* SceneView = Client.CalcSceneView(&ViewFamily);
	
		// Compute the MinP/MaxP in pixel coord, relative to View.ViewRect.Min
		const FMatrix& WorldToView = SceneView->ViewMatrices.GetViewMatrix();
		const FMatrix& ViewToProj = SceneView->ViewMatrices.GetProjectionMatrix();
		const float NearClippingDistance = SceneView->NearClippingDistance + SMALL_NUMBER;
		const FIntRect ViewRect = SceneView->UnconstrainedViewRect;

		// Clamp position on the near plane to get valid rect even if bounds' points are behind the camera
		FPlane P_View = WorldToView.TransformFVector4(FVector4(InWorldPos, 1.f));
		if (P_View.Z <= NearClippingDistance)
		{
			P_View.Z = NearClippingDistance;
		}

		// Project from view to projective space
		FVector2D MinP(FLT_MAX, FLT_MAX);
		FVector2D MaxP(-FLT_MAX, -FLT_MAX);
		FVector2D ScreenPos;
		const bool bIsValid = FSceneView::ProjectWorldToScreen(P_View, ViewRect, ViewToProj, ScreenPos);

		// Clamp to pixel border
		ScreenPos = FIntPoint(FMath::FloorToInt(ScreenPos.X), FMath::FloorToInt(ScreenPos.Y));

		// Clamp to screen rect
		if(bClampToScreenRectangle)
		{
			ScreenPos.X = FMath::Clamp(ScreenPos.X, ViewRect.Min.X, ViewRect.Max.X);
			ScreenPos.Y = FMath::Clamp(ScreenPos.Y, ViewRect.Min.Y, ViewRect.Max.Y);
		}

		return FVector2D(ScreenPos.X, ScreenPos.Y);
	}
	return FVector2D::ZeroVector;
}

void FTLLRN_ControlRigEditor::FindReferencesOfItem(const FTLLRN_RigElementKey& InKey)
{
	static constexpr TCHAR Format[] = TEXT("Type,%s,Name,%s");
	static const UEnum* TypeEnum = StaticEnum<ETLLRN_RigElementType>();
	const FText TypeText = TypeEnum->GetDisplayNameTextByValue((int64)InKey.Type);
	const FString Query = FString::Printf(Format, *TypeText.ToString(), *InKey.Name.ToString()); 
	SummonSearchUI(true, Query, true);
}

void FTLLRN_ControlRigEditor::HandlePreviewMeshChanged(USkeletalMesh* InOldSkeletalMesh, USkeletalMesh* InNewSkeletalMesh)
{
	RebindToSkeletalMeshComponent();

	if (GetObjectsCurrentlyBeingEdited()->Num() > 0)
	{
		if (UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBP = GetTLLRN_ControlRigBlueprint())
		{
			TLLRN_ControlRigBP->SetPreviewMesh(InNewSkeletalMesh);
			UTLLRN_RigHierarchy* BPHierarchy = TLLRN_ControlRigBP->GetHierarchy();

			FTLLRN_ModularRigConnections PreviousConnections;
			if(IsTLLRN_ModularRig())
			{
				PreviousConnections = TLLRN_ControlRigBP->TLLRN_ModularRigModel.Connections;
				{
					TGuardValue<bool> SuspendBlueprintNotifs(TLLRN_ControlRigBP->bSuspendAllNotifications, true);
					if(UTLLRN_RigHierarchyController* Controller = TLLRN_ControlRigBP->GetHierarchyController())
					{
						// remove all connectors / sockets. keeping them around may mess up the order of the elements
						// in the hierarchy, such as [bone,bone,bone,connector,connector,bone,bone,bone].
						// if the element is manually created, remember it to create it after importing the skeleton element
						TArray<FTLLRN_RigElementKey> ConnectorsAndSockets = Controller->GetHierarchy()->GetConnectorKeys();
						ConnectorsAndSockets.Append(Controller->GetHierarchy()->GetSocketKeys());

						TArray<TTuple<FTLLRN_RigElementKey, FTLLRN_RigElementKey, FTransform>> ConnectorsAndSocketsToParents;
						ConnectorsAndSocketsToParents.Reserve(ConnectorsAndSockets.Num());
						
						for(const FTLLRN_RigElementKey& Key : ConnectorsAndSockets)
						{
							// Remember manually created elements to apply them again
							if (BPHierarchy->GetNameSpace(Key).IsEmpty())
							{
								const FTLLRN_RigElementKey& Parent = BPHierarchy->GetDefaultParent(Key);
								ConnectorsAndSocketsToParents.Emplace(Key, Parent, BPHierarchy->GetLocalTransform(Key));
							}
							(void)Controller->RemoveElement(Key, true, true);
						}
						
						USkeleton* Skeleton = InNewSkeletalMesh ? InNewSkeletalMesh->GetSkeleton() : nullptr;
						Controller->ImportBones(Skeleton, NAME_None, true, true, false, true, true);
						if(InNewSkeletalMesh)
						{
							Controller->ImportCurvesFromSkeletalMesh(InNewSkeletalMesh, NAME_None, false, true, true);
						}
						else
						{
							Controller->ImportCurves(Skeleton, NAME_None, false, true, true);
						}

						// Recreate manually created elements
						for (const TTuple<FTLLRN_RigElementKey, FTLLRN_RigElementKey, FTransform>& Tuple : ConnectorsAndSocketsToParents)
						{
							const FTLLRN_RigElementKey& Key = Tuple.Get<0>();
							const FTLLRN_RigElementKey& Parent = Tuple.Get<1>();
							const FTransform& Transform = Tuple.Get<2>();
							
							if (!Parent.IsValid() || BPHierarchy->Contains(Parent))
							{
								switch (Key.Type)
								{
									case ETLLRN_RigElementType::Socket: Controller->AddSocket(Key.Name, Parent, Transform, false); break;
									case ETLLRN_RigElementType::Connector: Controller->AddConnector(Key.Name); break;
								}
							}
						}
					}
				}
				TLLRN_ControlRigBP->PropagateHierarchyFromBPToInstances();
			}
			
			UpdateRigVMHost();
			
			if(UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(TLLRN_ControlRigBP->GetObjectBeingDebugged()))
			{
				DebuggedTLLRN_ControlRig->GetHierarchy()->Notify(ETLLRN_RigHierarchyNotification::HierarchyReset, nullptr);
				DebuggedTLLRN_ControlRig->Initialize(true);
			}

			Compile();

			if(IsTLLRN_ModularRig())
			{
				if(UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(TLLRN_ControlRigBP->GetObjectBeingDebugged()))
				{
					DebuggedTLLRN_ControlRig->RequestConstruction();
					DebuggedTLLRN_ControlRig->Execute(FTLLRN_RigUnit_PrepareForExecution::EventName);
					
					if(UTLLRN_RigHierarchy* Hierarchy = DebuggedTLLRN_ControlRig->GetHierarchy())
					{
						FTLLRN_ModularRigModel* Model = &TLLRN_ControlRigBP->TLLRN_ModularRigModel;
						
						// try to reestablish the connections.
						UTLLRN_ModularTLLRN_RigController* TLLRN_ModularTLLRN_RigController = TLLRN_ControlRigBP->GetTLLRN_ModularTLLRN_RigController();
						const bool bAutoResolve = TLLRN_ControlRigBP->TLLRN_ModularRigSettings.bAutoResolve;
						Model->ForEachModule(
							[Model, Hierarchy, TLLRN_ModularTLLRN_RigController, PreviousConnections, bAutoResolve]
							(const FTLLRN_RigModuleReference* Module) -> bool
							{
								bool bContinueResolval;
								TArray<uint32> AttemptedTargets;
								do
								{
									bContinueResolval = false;
									
									const TArray<const FTLLRN_RigConnectorElement*> Connectors = Module->FindConnectors(Hierarchy);
									TArray<FTLLRN_RigElementKey> PrimaryConnectors, SecondaryConnectors, OptionalConnectors;
									for(const FTLLRN_RigConnectorElement* ExistingConnector : Connectors)
									{
										if(ExistingConnector->IsPrimary())
										{
											PrimaryConnectors.Add(ExistingConnector->GetKey());
										}
										else if(ExistingConnector->IsOptional())
										{
											OptionalConnectors.Add(ExistingConnector->GetKey());
										}
										else
										{
											SecondaryConnectors.Add(ExistingConnector->GetKey());
										}
									}
									TArray<FTLLRN_RigElementKey> ConnectorKeys;
									ConnectorKeys.Append(PrimaryConnectors);
									ConnectorKeys.Append(SecondaryConnectors);
									ConnectorKeys.Append(OptionalConnectors);
									
									for(const FTLLRN_RigElementKey& ConnectorKey : ConnectorKeys)
									{
										const bool bIsPrimary = ConnectorKey == ConnectorKeys[0];
										const bool bIsSecondary = !bIsPrimary;
										
										if(!Model->Connections.HasConnection(ConnectorKey, Hierarchy))
										{
											// try to reapply the connection
											if(PreviousConnections.HasConnection(ConnectorKey, Hierarchy))
											{
												const FTLLRN_RigElementKey Target = PreviousConnections.FindTargetFromConnector(ConnectorKey);
												if(TLLRN_ModularTLLRN_RigController->ConnectConnectorToElement(ConnectorKey, Target, true))
												{
													bContinueResolval = true;
												}
											}

											// try to auto resolve it
											if(!bContinueResolval && bIsSecondary && bAutoResolve)
											{
												if(TLLRN_ModularTLLRN_RigController->AutoConnectSecondaryConnectors({ConnectorKey}, true, true))
												{
													bContinueResolval = true;
												}
											}

											// only do one connector at a time
											break;
										}
									}

									// Avoid looping forever
									if (bContinueResolval)
									{
										uint32 Attempt = 0;
										for(const FTLLRN_RigElementKey& ConnectorKey : ConnectorKeys)
										{
											const FString ConnectionStr = FString::Printf(TEXT("%s -> %s"), *ConnectorKey.ToString(), *Model->Connections.FindTargetFromConnector(ConnectorKey).ToString());
											const uint32 ConnectionHash = HashCombine(GetTypeHash(ConnectorKey), GetTypeHash(Model->Connections.FindTargetFromConnector(ConnectorKey)));
											Attempt = HashCombine(Attempt, ConnectionHash);
										}
										if (AttemptedTargets.Contains(Attempt))
										{
										   bContinueResolval = false;
										}
										else
										{
										   AttemptedTargets.Add(Attempt);
										}
									}
								}
								while (bContinueResolval);

								return true; // continue to the next module
							}
						);
					}
				}
			}
		}
	}
}

void FTLLRN_ControlRigEditor::RebindToSkeletalMeshComponent()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	UDebugSkelMeshComponent* MeshComponent = GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent();
	if (MeshComponent)
	{
		bool bWasCreated = false;
		FAnimCustomInstanceHelper::BindToSkeletalMeshComponent<UTLLRN_ControlRigLayerInstance>(MeshComponent , bWasCreated);
	}
}

void FTLLRN_ControlRigEditor::GenerateEventQueueMenuContent(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection(TEXT("Events"));
    MenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().ConstructionEvent, TEXT("Setup"), TAttribute<FText>(), TAttribute<FText>(), GetEventQueueIcon(ConstructionEventQueue));
    MenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().ForwardsSolveEvent, TEXT("TLLRN3 Forwards Solve"), TAttribute<FText>(), TAttribute<FText>(), GetEventQueueIcon(ForwardsSolveEventQueue));
    MenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().BackwardsSolveEvent, TEXT("Backwards Solve"), TAttribute<FText>(), TAttribute<FText>(), GetEventQueueIcon(BackwardsSolveEventQueue));
    MenuBuilder.EndSection();

    MenuBuilder.BeginSection(TEXT("Validation"));
    MenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().BackwardsAndForwardsSolveEvent, TEXT("BackwardsAndForwards"), TAttribute<FText>(), TAttribute<FText>(), GetEventQueueIcon(BackwardsAndForwardsSolveEventQueue));
    MenuBuilder.EndSection();

    if (const UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj()))
    {
    	URigVMEdGraphSchema* Schema = CastChecked<URigVMEdGraphSchema>(RigBlueprint->GetRigVMEdGraphSchemaClass()->GetDefaultObject());
    	
    	bool bFoundUserDefinedEvent = false;
    	const TArray<FName> EntryNames = RigBlueprint->GetRigVMClient()->GetEntryNames();
    	for(const FName& EntryName : EntryNames)
    	{
    		if(Schema->IsRigVMDefaultEvent(EntryName))
    		{
    			continue;
    		}

    		if(!bFoundUserDefinedEvent)
    		{
    			MenuBuilder.AddSeparator();
    			bFoundUserDefinedEvent = true;
    		}

    		FString EventNameStr = EntryName.ToString();
    		if(!EventNameStr.EndsWith(TEXT("Event")))
    		{
    			EventNameStr += TEXT(" Event");
    		}

    		MenuBuilder.AddMenuEntry(
    			FText::FromString(EventNameStr),
    			FText::FromString(FString::Printf(TEXT("Runs the user defined %s"), *EventNameStr)),
    			GetEventQueueIcon({EntryName}),
    			FUIAction(
    				FExecuteAction::CreateSP(this, &FRigVMEditor::SetEventQueue, TArray<FName>({EntryName})),
    				FCanExecuteAction()
    			)
    		);
    	}
    }
}

void FTLLRN_ControlRigEditor::FilterDraggedKeys(TArray<FTLLRN_RigElementKey>& Keys, bool bRemoveNameSpace)
{
	// if the keys being dragged contain something mapped to a connector - use that instead
	if(UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = GetTLLRN_ControlRigBlueprint())
	{
		TArray<FTLLRN_RigElementKey> FilteredKeys;
		FilteredKeys.Reserve(Keys.Num());
		for (FTLLRN_RigElementKey Key : Keys)
		{
			for(const FTLLRN_ModularRigSingleConnection& Connection : TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections)
			{
				if(Connection.Target == Key)
				{
					Key = Connection.Connector;
					break;
				}
			}

			if(bRemoveNameSpace)
			{
				const FString Name = Key.Name.ToString();
				int32 LastCharIndex = INDEX_NONE;
				if(Name.FindLastChar(TEXT(':'), LastCharIndex))
				{
					Key.Name = *Name.Mid(LastCharIndex+1);
				}
			}
			else
			{
				if(const UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(TLLRN_ControlRigBlueprint->GetObjectBeingDebugged()))
				{
					if(!DebuggedTLLRN_ControlRig->GetHierarchy()->Contains(Key))
					{
						const FString NameSpace = DebuggedTLLRN_ControlRig->GetTLLRN_RigModuleNameSpace();
						if(!NameSpace.IsEmpty())
						{
							static constexpr TCHAR Format[] = TEXT("%s%s");
							Key.Name = *FString::Printf(Format, *NameSpace, *Key.Name.ToString());
						}
					}
				}
			}
			FilteredKeys.Add(Key);
		}
		Keys = FilteredKeys;
	}
}

FTransform FTLLRN_ControlRigEditor::GetTLLRN_RigElementTransform(const FTLLRN_RigElementKey& InElement, bool bLocal, bool bOnDebugInstance) const
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if(bOnDebugInstance)
	{
		UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetBlueprintObj()->GetObjectBeingDebugged());
		if (DebuggedTLLRN_ControlRig == nullptr)
		{
			DebuggedTLLRN_ControlRig = GetTLLRN_ControlRig();
		}

		if (DebuggedTLLRN_ControlRig)
		{
			if (bLocal)
			{
				return DebuggedTLLRN_ControlRig->GetHierarchy()->GetLocalTransform(InElement);
			}
			return DebuggedTLLRN_ControlRig->GetHierarchy()->GetGlobalTransform(InElement);
		}
	}

	if (bLocal)
	{
		return GetHierarchyBeingDebugged()->GetLocalTransform(InElement);
	}
	return GetHierarchyBeingDebugged()->GetGlobalTransform(InElement);
}

void FTLLRN_ControlRigEditor::SetTLLRN_RigElementTransform(const FTLLRN_RigElementKey& InElement, const FTransform& InTransform, bool bLocal)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	FScopedTransaction Transaction(LOCTEXT("Move Bone", "Move Bone transform"));
	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBP = GetTLLRN_ControlRigBlueprint();
	TLLRN_ControlRigBP->Modify();

	switch (InElement.Type)
	{
		case ETLLRN_RigElementType::Bone:
		case ETLLRN_RigElementType::Connector:
		case ETLLRN_RigElementType::Socket:
		{
			FTransform Transform = InTransform;
			if (bLocal)
			{
				FTransform ParentTransform = FTransform::Identity;
				FTLLRN_RigElementKey ParentKey = TLLRN_ControlRigBP->Hierarchy->GetFirstParent(InElement);
				if (ParentKey.IsValid())
				{
					ParentTransform = GetTLLRN_RigElementTransform(ParentKey, false, false);
				}
				Transform = Transform * ParentTransform;
				Transform.NormalizeRotation();
			}

			TLLRN_ControlRigBP->Hierarchy->SetInitialGlobalTransform(InElement, Transform);
			TLLRN_ControlRigBP->Hierarchy->SetGlobalTransform(InElement, Transform);
			OnHierarchyChanged();
			break;
		}
		case ETLLRN_RigElementType::Control:
		{
			FTransform LocalTransform = InTransform;
			FTransform GlobalTransform = InTransform;
			if (!bLocal)
			{
				TLLRN_ControlRigBP->Hierarchy->SetGlobalTransform(InElement, InTransform);
				LocalTransform = TLLRN_ControlRigBP->Hierarchy->GetLocalTransform(InElement);
			}
			else
			{
				TLLRN_ControlRigBP->Hierarchy->SetLocalTransform(InElement, InTransform);
				GlobalTransform = TLLRN_ControlRigBP->Hierarchy->GetGlobalTransform(InElement);
			}
			TLLRN_ControlRigBP->Hierarchy->SetInitialLocalTransform(InElement, LocalTransform);
			TLLRN_ControlRigBP->Hierarchy->SetGlobalTransform(InElement, GlobalTransform);
			OnHierarchyChanged();
			break;
		}
		case ETLLRN_RigElementType::Null:
		{
			FTransform LocalTransform = InTransform;
			FTransform GlobalTransform = InTransform;
			if (!bLocal)
			{
				TLLRN_ControlRigBP->Hierarchy->SetGlobalTransform(InElement, InTransform);
				LocalTransform = TLLRN_ControlRigBP->Hierarchy->GetLocalTransform(InElement);
			}
			else
			{
				TLLRN_ControlRigBP->Hierarchy->SetLocalTransform(InElement, InTransform);
				GlobalTransform = TLLRN_ControlRigBP->Hierarchy->GetGlobalTransform(InElement);
			}

			TLLRN_ControlRigBP->Hierarchy->SetInitialLocalTransform(InElement, LocalTransform);
			TLLRN_ControlRigBP->Hierarchy->SetGlobalTransform(InElement, GlobalTransform);
			OnHierarchyChanged();
			break;
		}
		default:
		{
			ensureMsgf(false, TEXT("Unsupported TLLRN_RigElement Type : %d"), InElement.Type);
			break;
		}
	}
	
	UTLLRN_ControlRigSkeletalMeshComponent* EditorSkelComp = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent());
	if (EditorSkelComp)
	{
		EditorSkelComp->RebuildDebugDrawSkeleton();
	}
}

void FTLLRN_ControlRigEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	FRigVMEditor::OnFinishedChangingProperties(PropertyChangedEvent);
	
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBP = GetTLLRN_ControlRigBlueprint();

	if (TLLRN_ControlRigBP)
	{
		if (PropertyChangedEvent.MemberProperty->GetNameCPP() == GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_ControlRigBlueprint, HierarchySettings))
		{
			TLLRN_ControlRigBP->PropagateHierarchyFromBPToInstances();
		}

		else if (PropertyChangedEvent.MemberProperty->GetNameCPP() == GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_ControlRigBlueprint, DrawContainer))
		{
			TLLRN_ControlRigBP->PropagateDrawInstructionsFromBPToInstances();
		}

		else if (PropertyChangedEvent.MemberProperty->GetNameCPP() == GET_MEMBER_NAME_STRING_CHECKED(UTLLRN_ControlRigBlueprint, TLLRN_RigModuleSettings))
		{
			TLLRN_ControlRigBP->PropagateHierarchyFromBPToInstances();
		}
	}
}

void FTLLRN_ControlRigEditor::OnWrappedPropertyChangedChainEvent(URigVMDetailsViewWrapperObject* InWrapperObject, const FString& InPropertyPath, FPropertyChangedChainEvent& InPropertyChangedChainEvent)
{
	FRigVMEditor::OnWrappedPropertyChangedChainEvent(InWrapperObject, InPropertyPath, InPropertyChangedChainEvent);
	
	check(InWrapperObject);
	check(!GetWrapperObjects().IsEmpty());

	TGuardValue<bool> SuspendDetailsPanelRefresh(GetSuspendDetailsPanelRefreshFlag(), true);

	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBP = GetTLLRN_ControlRigBlueprint();

	FString PropertyPath = InPropertyPath;
	if(UScriptStruct* WrappedStruct = InWrapperObject->GetWrappedStruct())
	{
		if(WrappedStruct->IsChildOf(FTLLRN_RigBaseElement::StaticStruct()))
		{
			check(WrappedStruct == GetWrapperObjects()[0]->GetWrappedStruct());

			UTLLRN_RigHierarchy* Hierarchy = CastChecked<UTLLRN_RigHierarchy>(InWrapperObject->GetSubject());
			const FTLLRN_RigBaseElement WrappedElement = InWrapperObject->GetContent<FTLLRN_RigBaseElement>();
			const FTLLRN_RigBaseElement FirstWrappedElement = GetWrapperObjects()[0]->GetContent<FTLLRN_RigBaseElement>();
			const FTLLRN_RigElementKey& Key = WrappedElement.GetKey();
			if(!Hierarchy->Contains(Key))
			{
				return;
			}

			static constexpr TCHAR PropertyChainElementFormat[] = TEXT("%s->");
			static const FString PoseString = FString::Printf(PropertyChainElementFormat, GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigTransformElement, PoseStorage));
			static const FString OffsetString = FString::Printf(PropertyChainElementFormat, GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigControlElement, OffsetStorage));
			static const FString ShapeString = FString::Printf(PropertyChainElementFormat, GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigControlElement, ShapeStorage));
			static const FString SettingsString = FString::Printf(PropertyChainElementFormat, GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigControlElement, Settings));

			struct Local
			{
				static ETLLRN_RigTransformType::Type GetTransformTypeFromPath(FString& PropertyPath)
				{
					static const FString InitialString = FString::Printf(PropertyChainElementFormat, GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigCurrentAndInitialTransform, Initial));
					static const FString CurrentString = FString::Printf(PropertyChainElementFormat, GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigCurrentAndInitialTransform, Current));
					static const FString GlobalString = FString::Printf(PropertyChainElementFormat, GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigLocalAndGlobalTransform, Global));
					static const FString LocalString = FString::Printf(PropertyChainElementFormat, GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigLocalAndGlobalTransform, Local));

					ETLLRN_RigTransformType::Type TransformType = ETLLRN_RigTransformType::CurrentLocal;

					if(PropertyPath.RemoveFromStart(InitialString))
					{
						TransformType = ETLLRN_RigTransformType::MakeInitial(TransformType);
					}
					else
					{
						verify(PropertyPath.RemoveFromStart(CurrentString));
						TransformType = ETLLRN_RigTransformType::MakeCurrent(TransformType);
					}

					if(PropertyPath.RemoveFromStart(GlobalString))
					{
						TransformType = ETLLRN_RigTransformType::MakeGlobal(TransformType);
					}
					else
					{
						verify(PropertyPath.RemoveFromStart(LocalString));
						TransformType = ETLLRN_RigTransformType::MakeLocal(TransformType);
					}

					return TransformType;
				}
			};

			bool bIsInitial = false;
			if(PropertyPath.RemoveFromStart(PoseString))
			{
				const ETLLRN_RigTransformType::Type TransformType = Local::GetTransformTypeFromPath(PropertyPath);
				bIsInitial = bIsInitial || ETLLRN_RigTransformType::IsInitial(TransformType);
				
				if(ETLLRN_RigTransformType::IsInitial(TransformType) || IsConstructionModeEnabled())
				{
					Hierarchy = TLLRN_ControlRigBP->Hierarchy;
				}

				FTLLRN_RigTransformElement* TransformElement = Hierarchy->Find<FTLLRN_RigTransformElement>(WrappedElement.GetKey());
				if(TransformElement == nullptr)
				{
					return;
				}

				const FTransform Transform = InWrapperObject->GetContent<FTLLRN_RigTransformElement>().GetTransform().Get(TransformType);

				if(ETLLRN_RigTransformType::IsLocal(TransformType) && TransformElement->IsA<FTLLRN_RigControlElement>())
				{
					FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement);
							
					FTLLRN_RigControlValue Value;
					Value.SetFromTransform(Transform, ControlElement->Settings.ControlType, ControlElement->Settings.PrimaryAxis);
							
					if(ETLLRN_RigTransformType::IsInitial(TransformType) || IsConstructionModeEnabled())
					{
						Hierarchy->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Initial, true, true, true);
					}
					Hierarchy->SetControlValue(ControlElement, Value, ETLLRN_RigControlValueType::Current, true, true, true);
				}
				else
				{
					Hierarchy->SetTransform(TransformElement, Transform, TransformType, true, true, false, true);
				}
			}
			else if(PropertyPath.RemoveFromStart(OffsetString))
			{
				FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRigBP->Hierarchy->Find<FTLLRN_RigControlElement>(WrappedElement.GetKey());
				if(ControlElement == nullptr)
				{
					return;
				}
				
				ETLLRN_RigTransformType::Type TransformType = Local::GetTransformTypeFromPath(PropertyPath);
				bIsInitial = bIsInitial || ETLLRN_RigTransformType::IsInitial(TransformType);

				const FTransform Transform = GetWrapperObjects()[0]->GetContent<FTLLRN_RigControlElement>().GetOffsetTransform().Get(TransformType);
				
				TLLRN_ControlRigBP->Hierarchy->SetControlOffsetTransform(ControlElement, Transform, ETLLRN_RigTransformType::MakeInitial(TransformType), true, true, false, true);
			}
			else if(PropertyPath.RemoveFromStart(ShapeString))
			{
				FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRigBP->Hierarchy->Find<FTLLRN_RigControlElement>(WrappedElement.GetKey());
				if(ControlElement == nullptr)
				{
					return;
				}

				ETLLRN_RigTransformType::Type TransformType = Local::GetTransformTypeFromPath(PropertyPath);
				bIsInitial = bIsInitial || ETLLRN_RigTransformType::IsInitial(TransformType);

				const FTransform Transform = GetWrapperObjects()[0]->GetContent<FTLLRN_RigControlElement>().GetShapeTransform().Get(TransformType);
				
				TLLRN_ControlRigBP->Hierarchy->SetControlShapeTransform(ControlElement, Transform, ETLLRN_RigTransformType::MakeInitial(TransformType), true, false, true);
			}
			else if(PropertyPath.RemoveFromStart(SettingsString))
			{
				if(Key.Type == ETLLRN_RigElementType::Control)
				{
					const FTLLRN_RigControlSettings Settings  = InWrapperObject->GetContent<FTLLRN_RigControlElement>().Settings;

					FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRigBP->Hierarchy->Find<FTLLRN_RigControlElement>(WrappedElement.GetKey());
					if(ControlElement == nullptr)
					{
						return;
					}

					TLLRN_ControlRigBP->Hierarchy->SetControlSettings(ControlElement, Settings, true, false, true);
				}
				else if(Key.Type == ETLLRN_RigElementType::Connector)
				{
					const FTLLRN_RigConnectorSettings Settings  = InWrapperObject->GetContent<FTLLRN_RigConnectorElement>().Settings;

					FTLLRN_RigConnectorElement* ConnectorElement = TLLRN_ControlRigBP->Hierarchy->Find<FTLLRN_RigConnectorElement>(WrappedElement.GetKey());
					if(ConnectorElement == nullptr)
					{
						return;
					}

					TLLRN_ControlRigBP->Hierarchy->SetConnectorSettings(ConnectorElement, Settings, true, false, true);
				}
			}

			if(IsConstructionModeEnabled() || bIsInitial)
			{
				TLLRN_ControlRigBP->PropagatePoseFromBPToInstances();
				TLLRN_ControlRigBP->Modify();
				TLLRN_ControlRigBP->MarkPackageDirty();
			}
		}
	}
}

bool FTLLRN_ControlRigEditor::HandleRequestDirectManipulation(ETLLRN_RigControlType InControlType) const
{
	TArray<FRigDirectManipulationTarget> Targets = GetDirectManipulationTargets();
	for(const FRigDirectManipulationTarget& Target : Targets)
	{
		if(Target.ControlType == InControlType || Target.ControlType == ETLLRN_RigControlType::EulerTransform)
		{
			if (FTLLRN_ControlRigEditorEditMode* EditMode = GetEditMode())
			{
				switch(InControlType)
				{
					case ETLLRN_RigControlType::Position:
					{
						EditMode->RequestTransformWidgetMode(UE::Widget::WM_Translate);
						break;
					}
					case ETLLRN_RigControlType::Rotator:
					{
						EditMode->RequestTransformWidgetMode(UE::Widget::WM_Rotate);
						break;
					}
					case ETLLRN_RigControlType::Scale:
					{
						EditMode->RequestTransformWidgetMode(UE::Widget::WM_Scale);
						break;
					}
					default:
					{
						break;
					}
				}
			}

			if(UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
			{
				Blueprint->AddTransientControl(DirectManipulationSubject.Get(), Target);
			}
			return true;
		}
	}
	return false;
}

bool FTLLRN_ControlRigEditor::SetDirectionManipulationSubject(const URigVMUnitNode* InNode)
{
	if(DirectManipulationSubject.Get() == InNode)
	{
		return false;
	}
	if(UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
	{
		Blueprint->ClearTransientControls();
	}
	DirectManipulationSubject = InNode;

	// update the direct manipulation target list
	RefreshDirectManipulationTextList();
	return true;
}

bool FTLLRN_ControlRigEditor::IsDirectManipulationEnabled() const
{
	return !GetDirectManipulationTargets().IsEmpty();
}

EVisibility FTLLRN_ControlRigEditor::GetDirectManipulationVisibility() const
{
	return IsDirectManipulationEnabled() ? EVisibility::Visible : EVisibility::Hidden;
}

FText FTLLRN_ControlRigEditor::GetDirectionManipulationText() const
{
	if (UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetBlueprintObj()->GetObjectBeingDebugged()))
	{
		if (UTLLRN_RigHierarchy* Hierarchy = DebuggedTLLRN_ControlRig->GetHierarchy())
		{
			TArray<FTLLRN_RigControlElement*> TransientControls = Hierarchy->GetTransientControls();
			for(const FTLLRN_RigControlElement* TransientControl : TransientControls)
			{
				const FString Target = UTLLRN_ControlRig::GetTargetFromTransientControl(TransientControl->GetKey());
				if(!Target.IsEmpty())
				{
					return FText::FromString(Target);
				}
			}
		}
	}
	static const FText DefaultText = LOCTEXT("TLLRN_ControlRigDirectManipulation", "Direct Manipulation");
	return DefaultText;
}

void FTLLRN_ControlRigEditor::OnDirectManipulationChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if(!NewValue.IsValid())
	{
		return;
	}
	
	const URigVMUnitNode* UnitNode = DirectManipulationSubject.Get();
	if(UnitNode == nullptr)
	{
		return;
	}
	
	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = CastChecked<UTLLRN_ControlRigBlueprint>(GetBlueprintObj());
	if(TLLRN_ControlRigBlueprint == nullptr)
	{
		return;
	}

	// disable literal folding for the moment
	if(TLLRN_ControlRigBlueprint->VMCompileSettings.ASTSettings.bFoldLiterals)
	{
		TLLRN_ControlRigBlueprint->VMCompileSettings.ASTSettings.bFoldLiterals = false;
		TLLRN_ControlRigBlueprint->RecompileVM();
	}

	const FString& DesiredTarget = *NewValue.Get();
	const TArray<FRigDirectManipulationTarget> Targets = GetDirectManipulationTargets();
	for(const FRigDirectManipulationTarget& Target : Targets)
	{
		if(Target.Name.Equals(DesiredTarget, ESearchCase::CaseSensitive))
		{
			// run the task after a bit so that the rig has the opportunity to run first
			FFunctionGraphTask::CreateAndDispatchWhenReady([TLLRN_ControlRigBlueprint, UnitNode, Target]()
			{
				TLLRN_ControlRigBlueprint->AddTransientControl(UnitNode, Target);
			}, TStatId(), NULL, ENamedThreads::GameThread);
			break;
		}
	}
}

const TArray<FRigDirectManipulationTarget> FTLLRN_ControlRigEditor::GetDirectManipulationTargets() const
{
	if(DirectManipulationSubject.IsValid())
	{
		if (UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetBlueprintObj()->GetObjectBeingDebugged()))
		{
			if(const URigVMUnitNode* Node = DirectManipulationSubject.Get())
			{
				if(Node->IsPartOfRuntime(DebuggedTLLRN_ControlRig))
				{
					const TSharedPtr<FStructOnScope> NodeInstance = Node->ConstructLiveStructInstance(DebuggedTLLRN_ControlRig);
					if(NodeInstance.IsValid() && NodeInstance->IsValid())
					{
						if(const FTLLRN_RigUnit* UnitInstance = UTLLRN_ControlRig::GetTLLRN_RigUnitInstanceFromScope(NodeInstance))
						{
							TArray<FRigDirectManipulationTarget> Targets;
							if(UnitInstance->GetDirectManipulationTargets(Node, NodeInstance, DebuggedTLLRN_ControlRig->GetHierarchy(), Targets, nullptr))
							{
								return Targets;
							}
						}
					}
				}
			}
		}
	}

	static const TArray<FRigDirectManipulationTarget> EmptyTargets;
	return EmptyTargets;
}

const TArray<TSharedPtr<FString>>& FTLLRN_ControlRigEditor::GetDirectManipulationTargetTextList() const
{
	if(DirectManipulationTextList.IsEmpty())
	{
		const TArray<FRigDirectManipulationTarget> Targets = GetDirectManipulationTargets();
		for(const FRigDirectManipulationTarget& Target : Targets)
		{
			DirectManipulationTextList.Emplace(new FString(Target.Name));
		}
	}
	return DirectManipulationTextList;
}

void FTLLRN_ControlRigEditor::RefreshDirectManipulationTextList()
{
	DirectManipulationTextList.Reset();
	(void)GetDirectManipulationTargetTextList();
	if(DirectManipulationCombo.IsValid())
	{
		DirectManipulationCombo->RefreshOptions();
	}
}

EVisibility FTLLRN_ControlRigEditor::GetConnectorWarningVisibility() const
{
	if(GetConnectorWarningText().IsEmpty())
	{
		return EVisibility::Hidden;
	}
	return EVisibility::Visible;
}

FText FTLLRN_ControlRigEditor::GetConnectorWarningText() const
{
	if (const UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
	{
		if(Blueprint->IsTLLRN_ControlTLLRN_RigModule())
		{
			if(UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
			{
				FString FailureReason;
				if(!TLLRN_ControlRig->AllConnectorsAreResolved(&FailureReason))
				{
					if(FailureReason.IsEmpty())
					{
						static const FText ConnectorWarningDefault = LOCTEXT("ConnectorWarningDefault", "This rig has unresolved connectors.");
						return ConnectorWarningDefault;
					}
					return FText::FromString(FailureReason);
				}
			}
		}
	}
	return FText();
}

FReply FTLLRN_ControlRigEditor::OnNavigateToConnectorWarning() const
{
	RequestNavigateToConnectorWarningDelegate.Broadcast();
	return FReply::Handled();
}

void FTLLRN_ControlRigEditor::BindCommands()
{
	FRigVMEditor::BindCommands();

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().ConstructionEvent,
		FExecuteAction::CreateSP(this, &FRigVMEditor::SetEventQueue, TArray<FName>(ConstructionEventQueue)),
		FCanExecuteAction());

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().ForwardsSolveEvent,
		FExecuteAction::CreateSP(this, &FRigVMEditor::SetEventQueue, TArray<FName>(ForwardsSolveEventQueue)),
		FCanExecuteAction());

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().BackwardsSolveEvent,
		FExecuteAction::CreateSP(this, &FRigVMEditor::SetEventQueue, TArray<FName>(BackwardsSolveEventQueue)),
		FCanExecuteAction());

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().BackwardsAndForwardsSolveEvent,
		FExecuteAction::CreateSP(this, &FRigVMEditor::SetEventQueue, TArray<FName>(BackwardsAndForwardsSolveEventQueue)),
		FCanExecuteAction());

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().ToggleControlVisibility,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::HandleToggleControlVisibility),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigEditor::AreControlsVisible));

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().ToggleControlsAsOverlay,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::HandleToggleControlsAsOverlay),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigEditor::AreControlsAsOverlay));

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().ToggleDrawNulls,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::HandleToggleToolbarDrawNulls),
		FCanExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::IsToolbarDrawNullsEnabled),
		FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigEditor::GetToolbarDrawNulls));

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().ToggleDrawSockets,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::HandleToggleToolbarDrawSockets),
		FCanExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::IsToolbarDrawSocketsEnabled),
		FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigEditor::GetToolbarDrawSockets));

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().ToggleDrawAxesOnSelection,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::HandleToggleToolbarDrawAxesOnSelection),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigEditor::GetToolbarDrawAxesOnSelection));

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().ToggleSchematicViewportVisibility,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::HandleToggleSchematicViewport),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTLLRN_ControlRigEditor::IsSchematicViewportActive));

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().SwapModuleWithinAsset,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::SwapModuleWithinAsset),
		FCanExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::IsTLLRN_ModularRig));

	GetToolkitCommands()->MapAction(
		FTLLRN_ControlRigEditorCommands::Get().SwapModuleAcrossProject,
		FExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::SwapModuleAcrossProject),
		FCanExecuteAction::CreateSP(this, &FTLLRN_ControlRigEditor::IsTLLRN_RigModule));
}

FMenuBuilder FTLLRN_ControlRigEditor::GenerateBulkEditMenu()
{
	FMenuBuilder MenuBuilder = ITLLRN_ControlRigEditor::GenerateBulkEditMenu();
	MenuBuilder.BeginSection(TEXT("Asset"), LOCTEXT("Asset", "Asset"));
	if (UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint())
	{
		if (Blueprint->IsTLLRN_ModularRig())
		{
			MenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().SwapModuleWithinAsset, TEXT("SwapModuleWithinAsset"), TAttribute<FText>(), TAttribute<FText>(), FSlateIcon());
		}
		else if (Blueprint->IsTLLRN_ControlTLLRN_RigModule())
		{
			MenuBuilder.AddMenuEntry(FTLLRN_ControlRigEditorCommands::Get().SwapModuleAcrossProject, TEXT("SwapModuleAcrossProject"), TAttribute<FText>(), TAttribute<FText>(), FSlateIcon());
		}
	}
	MenuBuilder.EndSection();
	return MenuBuilder;
}

void FTLLRN_ControlRigEditor::OnHierarchyChanged()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if (UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBP = GetTLLRN_ControlRigBlueprint())
	{
		{
			TGuardValue<bool> GuardNotifs(TLLRN_ControlRigBP->bSuspendAllNotifications, true);
			TLLRN_ControlRigBP->PropagateHierarchyFromBPToInstances();
		}

		FBlueprintEditorUtils::MarkBlueprintAsModified(GetTLLRN_ControlRigBlueprint());
		
		TArray<const FTLLRN_RigBaseElement*> SelectedElements = GetHierarchyBeingDebugged()->GetSelectedElements();
		for(const FTLLRN_RigBaseElement* SelectedElement : SelectedElements)
		{
			TLLRN_ControlRigBP->Hierarchy->OnModified().Broadcast(ETLLRN_RigHierarchyNotification::ElementSelected, TLLRN_ControlRigBP->Hierarchy, SelectedElement);
		}
		GetTLLRN_ControlRigBlueprint()->RequestAutoVMRecompilation();

		SynchronizeViewportBoneSelection();

		UTLLRN_ControlRigSkeletalMeshComponent* EditorSkelComp = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent());
		// since rig has changed, rebuild draw skeleton
		if (EditorSkelComp)
		{ 
			EditorSkelComp->RebuildDebugDrawSkeleton(); 
		}

		RefreshDetailView();
	}
	else
	{
		ClearDetailObject();
	}
	
	CacheNameLists();
}


void FTLLRN_ControlRigEditor::OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	UTLLRN_ControlRigBlueprint* RigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj());
	if(RigBlueprint == nullptr)
	{
		return;
	}

	if (RigBlueprint->bSuspendAllNotifications)
	{
		return;
	}

	if(InHierarchy != RigBlueprint->Hierarchy)
	{
		return;
	}

	switch(InNotif)
	{
		case ETLLRN_RigHierarchyNotification::ElementAdded:
		{
			if (!RigBlueprint->IsTLLRN_ModularRig())
			{
				if(InElement->GetType() == ETLLRN_RigElementType::Connector)
				{
					if(InHierarchy->GetConnectors().Num() == 1)
					{
						FNotificationInfo Info(LOCTEXT("FirstConnectorEncountered", "Looks like you have added the first connector. This rig will now be configured as a module, settings can be found in the class settings Hierarchy -> Module Settings."));
						Info.bFireAndForget = true;
						Info.FadeOutDuration = 5.0f;
						Info.ExpireDuration = 5.0f;

						TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
						NotificationPtr->SetCompletionState(SNotificationItem::CS_Success);

						RigBlueprint->TurnIntoTLLRN_ControlTLLRN_RigModule();
					}
				}
			}
			// no break - fall through
		}
		case ETLLRN_RigHierarchyNotification::ParentChanged:
		case ETLLRN_RigHierarchyNotification::HierarchyReset:
		{
			OnHierarchyChanged();
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementRemoved:
		{
			UEnum* TLLRN_RigElementTypeEnum = StaticEnum<ETLLRN_RigElementType>();
			if (TLLRN_RigElementTypeEnum == nullptr)
			{
				return;
			}

			CacheNameLists();

			const FString RemovedElementName = InElement->GetName();
			const ETLLRN_RigElementType RemovedElementType = InElement->GetType();

			TArray<UEdGraph*> EdGraphs;
			RigBlueprint->GetAllGraphs(EdGraphs);

			for (UEdGraph* Graph : EdGraphs)
			{
				UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(Graph);
				if (RigGraph == nullptr)
				{
					continue;
				}

				for (UEdGraphNode* Node : RigGraph->Nodes)
				{
					if (UTLLRN_ControlRigGraphNode* RigNode = Cast<UTLLRN_ControlRigGraphNode>(Node))
					{
						if (URigVMNode* ModelNode = RigNode->GetModelNode())
						{
							TArray<URigVMPin*> ModelPins = ModelNode->GetAllPinsRecursively();
							for (URigVMPin* ModelPin : ModelPins)
							{
								if ((ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("BoneName") && RemovedElementType == ETLLRN_RigElementType::Bone) ||
									(ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("ControlName") && RemovedElementType == ETLLRN_RigElementType::Control) ||
									(ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("SpaceName") && RemovedElementType == ETLLRN_RigElementType::Null) ||
									(ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("CurveName") && RemovedElementType == ETLLRN_RigElementType::Curve) ||
									(ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("ConnectorName") && RemovedElementType == ETLLRN_RigElementType::Connector))
								{
									if (ModelPin->GetDefaultValue() == RemovedElementName)
									{
										RigNode->ReconstructNode();
										break;
									}
								}
								else if (ModelPin->GetCPPTypeObject() == FTLLRN_RigElementKey::StaticStruct())
								{
									if (URigVMPin* TypePin = ModelPin->FindSubPin(TEXT("Type")))
									{
										FString TypeStr = TypePin->GetDefaultValue();
										int64 TypeValue = TLLRN_RigElementTypeEnum->GetValueByNameString(TypeStr);
										if (TypeValue == (int64)RemovedElementType)
										{
											if (URigVMPin* NamePin = ModelPin->FindSubPin(TEXT("Name")))
											{
												FString NameStr = NamePin->GetDefaultValue();
												if (NameStr == RemovedElementName)
												{
													RigNode->ReconstructNode();
													break;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}

			OnHierarchyChanged();
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementRenamed:
		{
			UEnum* TLLRN_RigElementTypeEnum = StaticEnum<ETLLRN_RigElementType>();
			if (TLLRN_RigElementTypeEnum == nullptr)
			{
				return;
			}

			const FString OldNameStr = InHierarchy->GetPreviousName(InElement->GetKey()).ToString();
			const FString NewNameStr = InElement->GetName();
			const ETLLRN_RigElementType ElementType = InElement->GetType(); 

			TArray<UEdGraph*> EdGraphs;
			RigBlueprint->GetAllGraphs(EdGraphs);

			for (UEdGraph* Graph : EdGraphs)
			{
				UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(Graph);
				if (RigGraph == nullptr)
				{
					continue;
				}

				URigVMController* Controller = RigGraph->GetController();
				if(Controller == nullptr)
				{
					continue;
				}

				{
					FRigVMBlueprintCompileScope CompileScope(RigBlueprint);
					for (UEdGraphNode* Node : RigGraph->Nodes)
					{
						if (UTLLRN_ControlRigGraphNode* RigNode = Cast<UTLLRN_ControlRigGraphNode>(Node))
						{
							if (URigVMNode* ModelNode = RigNode->GetModelNode())
							{
								TArray<URigVMPin*> ModelPins = ModelNode->GetAllPinsRecursively();
								for (URigVMPin * ModelPin : ModelPins)
								{
									if ((ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("BoneName") && ElementType == ETLLRN_RigElementType::Bone) ||
										(ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("ControlName") && ElementType == ETLLRN_RigElementType::Control) ||
										(ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("SpaceName") && ElementType == ETLLRN_RigElementType::Null) ||
										(ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("CurveName") && ElementType == ETLLRN_RigElementType::Curve) ||
										(ModelPin->GetCPPType() == TEXT("FName") && ModelPin->GetCustomWidgetName() == TEXT("ConnectorName") && ElementType == ETLLRN_RigElementType::Connector))
									{
										if (ModelPin->GetDefaultValue() == OldNameStr)
										{
											Controller->SetPinDefaultValue(ModelPin->GetPinPath(), NewNameStr, false);
										}
									}
									else if (ModelPin->GetCPPTypeObject() == FTLLRN_RigElementKey::StaticStruct())
									{
										if (URigVMPin* TypePin = ModelPin->FindSubPin(TEXT("Type")))
										{
											const FString TypeStr = TypePin->GetDefaultValue();
											const int64 TypeValue = TLLRN_RigElementTypeEnum->GetValueByNameString(TypeStr);
											if (TypeValue == (int64)ElementType)
											{
												if (URigVMPin* NamePin = ModelPin->FindSubPin(TEXT("Name")))
												{
													FString NameStr = NamePin->GetDefaultValue();
													if (NameStr == OldNameStr)
													{
														Controller->SetPinDefaultValue(NamePin->GetPinPath(), NewNameStr);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
				
			OnHierarchyChanged();

			break;
		}
		default:
		{
			break;
		}
	}
}

void FTLLRN_ControlRigEditor::OnHierarchyModified_AnyThread(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	if(bIsConstructionEventRunning)
	{
		return;
	}

	if(SchematicViewport)
	{
		SchematicModel.OnHierarchyModified(InNotif, InHierarchy, InElement);
	}
	
	FTLLRN_RigElementKey Key;
	if(InElement)
	{
		Key = InElement->GetKey();
	}

	if(IsInGameThread())
	{
		UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint();
		check(RigBlueprint);

		if(RigBlueprint->bSuspendAllNotifications)
		{
			return;
		}
	}

	TWeakObjectPtr<UTLLRN_RigHierarchy> WeakHierarchy = InHierarchy;
	auto Task = [this, InNotif, WeakHierarchy, Key]()
    {
		if(!WeakHierarchy.IsValid())
    	{
    		return;
    	}

        FTLLRN_RigBaseElement* Element = WeakHierarchy.Get()->Find(Key);

		switch(InNotif)
		{
			case ETLLRN_RigHierarchyNotification::ElementSelected:
			case ETLLRN_RigHierarchyNotification::ElementDeselected:
			{
				if(Element)
				{
					const bool bSelected = InNotif == ETLLRN_RigHierarchyNotification::ElementSelected;

					if (Element->GetType() == ETLLRN_RigElementType::Bone)
					{
						SynchronizeViewportBoneSelection();
					}

					if (bSelected)
					{
						SetDetailViewForTLLRN_RigElements();
					}
					else
					{
						TArray<FTLLRN_RigElementKey> CurrentSelection = GetHierarchyBeingDebugged()->GetSelectedKeys();
						if (CurrentSelection.Num() > 0)
						{
							if(FTLLRN_RigBaseElement* LastSelectedElement = WeakHierarchy.Get()->Find(CurrentSelection.Last()))
							{
								OnHierarchyModified(ETLLRN_RigHierarchyNotification::ElementSelected,  WeakHierarchy.Get(), LastSelectedElement);
							}
						}
						else
						{
							// only clear the details if we are not looking at a transient control
							if (UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetBlueprintObj()->GetObjectBeingDebugged()))
							{
								if(DebuggedTLLRN_ControlRig->TLLRN_RigUnitManipulationInfos.IsEmpty())
								{
									ClearDetailObject();
								}
							}
						}
					}
				}
				break;
			}
			case ETLLRN_RigHierarchyNotification::ElementAdded:
			case ETLLRN_RigHierarchyNotification::ElementRemoved:
			case ETLLRN_RigHierarchyNotification::ElementRenamed:
			{
				if (Key.IsValid() && Key.Type == ETLLRN_RigElementType::Connector)
				{
					UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint();
					check(RigBlueprint);
					RigBlueprint->UpdateExposedModuleConnectors();
				}
				// Fallthrough to next case
			}
			case ETLLRN_RigHierarchyNotification::ParentChanged:
            case ETLLRN_RigHierarchyNotification::HierarchyReset:
			{
				CacheNameLists();
				break;
			}
			case ETLLRN_RigHierarchyNotification::ControlSettingChanged:
			{
				if(DetailViewShowsTLLRN_RigElement(Key))
				{
					UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint();
					check(RigBlueprint);

					const FTLLRN_RigControlElement* SourceControlElement = Cast<FTLLRN_RigControlElement>(Element);
					FTLLRN_RigControlElement* TargetControlElement = RigBlueprint->Hierarchy->Find<FTLLRN_RigControlElement>(Key);

					if(SourceControlElement && TargetControlElement)
					{
						TargetControlElement->Settings = SourceControlElement->Settings;
					}
				}
				break;
			}
			case ETLLRN_RigHierarchyNotification::ControlShapeTransformChanged:
			{
				if(DetailViewShowsTLLRN_RigElement(Key))
				{
					UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint();
					check(RigBlueprint);

					FTLLRN_RigControlElement* SourceControlElement = Cast<FTLLRN_RigControlElement>(Element);
					if(SourceControlElement)
					{
						FTransform InitialShapeTransform = WeakHierarchy.Get()->GetControlShapeTransform(SourceControlElement, ETLLRN_RigTransformType::InitialLocal);

						// set current shape transform = initial shape transform so that the viewport reflects this change
						WeakHierarchy.Get()->SetControlShapeTransform(SourceControlElement, InitialShapeTransform, ETLLRN_RigTransformType::CurrentLocal, false); 

						RigBlueprint->Hierarchy->SetControlShapeTransform(Key, WeakHierarchy.Get()->GetControlShapeTransform(SourceControlElement, ETLLRN_RigTransformType::InitialLocal), true);
						RigBlueprint->Hierarchy->SetControlShapeTransform(Key, WeakHierarchy.Get()->GetControlShapeTransform(SourceControlElement, ETLLRN_RigTransformType::CurrentLocal), false);

						RigBlueprint->Modify();
						RigBlueprint->MarkPackageDirty();
					}
				}
				break;
			}
			case ETLLRN_RigHierarchyNotification::ConnectorSettingChanged:
			{
				UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint();
				check(RigBlueprint);
				RigBlueprint->UpdateExposedModuleConnectors();
				RigBlueprint->RecompileTLLRN_ModularRig();
				break;
			}
			default:
			{
				break;
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

void FTLLRN_ControlRigEditor::HandleRigTypeChanged(UTLLRN_ControlRigBlueprint* InBlueprint)
{
	// todo: fire a notification.
	// todo: reapply the preview mesh and react to it accordingly.

	Compile();
}

void FTLLRN_ControlRigEditor::HandleTLLRN_ModularRigModified(ETLLRN_ModularRigNotification InNotification, const FTLLRN_RigModuleReference* InModule)
{
	UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint();
	if(RigBlueprint == nullptr)
	{
		return;
	}
	
	UTLLRN_ModularTLLRN_RigController* TLLRN_ModularTLLRN_RigController = RigBlueprint->GetTLLRN_ModularTLLRN_RigController();
	if(TLLRN_ModularTLLRN_RigController == nullptr)
	{
		return;
	}

	switch(InNotification)
	{
		case ETLLRN_ModularRigNotification::ModuleAdded:
		{
			TLLRN_ModularTLLRN_RigController->SelectModule(InModule->GetPath());
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleRemoved:
		{
			if (DetailViewShowsAnyTLLRN_RigModule())
			{
				ClearDetailObject();
			}

			// todo: update SchematicGraph
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleReparented:
		case ETLLRN_ModularRigNotification::ModuleRenamed:
		{
			FString OldPath;
			if (InNotification == ETLLRN_ModularRigNotification::ModuleRenamed)
			{
				OldPath = UTLLRN_RigHierarchy::JoinNameSpace(InModule->ParentPath, InModule->PreviousName.ToString());
			}
			else
			{
				OldPath = UTLLRN_RigHierarchy::JoinNameSpace(InModule->PreviousParentPath, InModule->Name.ToString());
			}
			break;
		}
		case ETLLRN_ModularRigNotification::ConnectionChanged:
		{
			// todo: update SchematicGraph
			break;
		}
		case ETLLRN_ModularRigNotification::ModuleSelected:
		case ETLLRN_ModularRigNotification::ModuleDeselected:
		{
			ModulesSelected = TLLRN_ModularTLLRN_RigController->GetSelectedModules();
			SetDetailViewForTLLRN_RigModules(ModulesSelected);
			break;
		}
	}
}

void FTLLRN_ControlRigEditor::HandlePostCompileTLLRN_ModularRigs(URigVMBlueprint* InBlueprint)
{
	RefreshDetailView();
}

void FTLLRN_ControlRigEditor::SwapModuleWithinAsset()
{
	const UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint();
	const FAssetData Asset = UE::RigVM::Editor::Tools::FindAssetFromAnyPath(GetRigVMBlueprint()->GetPathName(), true);
	SRigVMSwapAssetReferencesWidget::FArguments WidgetArgs;

	FRigVMAssetDataFilter FilterModules = FRigVMAssetDataFilter::CreateLambda([](const FAssetData& AssetData)
	{
		return UTLLRN_ControlRigBlueprint::GetRigType(AssetData) == ETLLRN_ControlRigType::TLLRN_RigModule;
	});
	FRigVMAssetDataFilter FilterSourceModules = FRigVMAssetDataFilter::CreateLambda([Blueprint](const FAssetData& AssetData)
	{
		if (Blueprint)
		{
			return !Blueprint->TLLRN_ModularRigModel.FindModuleInstancesOfClass(AssetData).IsEmpty();
		}
		return false;
	});

	TArray<FRigVMAssetDataFilter> SourceFilters = {FilterModules, FilterSourceModules};
	TArray<FRigVMAssetDataFilter> TargetFilters = {FilterModules};
	
	WidgetArgs
		.EnableUndo(true)
		.CloseOnSuccess(true)
		.OnGetReferences_Lambda([Blueprint, Asset](const FAssetData& ReferencedAsset) -> TArray<FSoftObjectPath>
		{
			TArray<FSoftObjectPath> Result;
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			IAssetRegistry& AssetRegistry = AssetRegistryModule.GetRegistry();

			UClass* ReferencedClass = nullptr;
			if (UTLLRN_ControlRigBlueprint* ReferencedBlueprint = Cast<UTLLRN_ControlRigBlueprint>(ReferencedAsset.GetAsset()))
			{
				ReferencedClass = ReferencedBlueprint->GetRigVMBlueprintGeneratedClass();
			}

			if (Blueprint)
			{
				if (Blueprint->IsTLLRN_ModularRig())
				{
					TArray<const FTLLRN_RigModuleReference*> Modules = Blueprint->TLLRN_ModularRigModel.FindModuleInstancesOfClass(ReferencedAsset);
					for (const FTLLRN_RigModuleReference* Module : Modules)
					{
						FSoftObjectPath ModulePath = Asset.GetSoftObjectPath();
						ModulePath.SetSubPathString(Module->GetPath());
						Result.Add(ModulePath);
					}
				}
			}
			
			return Result;
		})
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

void FTLLRN_ControlRigEditor::SwapModuleAcrossProject()
{
	const UTLLRN_ControlRigBlueprint* Blueprint = GetTLLRN_ControlRigBlueprint();
	const FAssetData Asset = UE::RigVM::Editor::Tools::FindAssetFromAnyPath(GetRigVMBlueprint()->GetPathName(), true);
	SRigVMSwapAssetReferencesWidget::FArguments WidgetArgs;

	FRigVMAssetDataFilter FilterModules = FRigVMAssetDataFilter::CreateLambda([](const FAssetData& AssetData)
	{
		return UTLLRN_ControlRigBlueprint::GetRigType(AssetData) == ETLLRN_ControlRigType::TLLRN_RigModule;
	});

	TArray<FRigVMAssetDataFilter> TargetFilters = {FilterModules};
	
	WidgetArgs
		.EnableUndo(false)
		.CloseOnSuccess(true)
		.OnGetReferences_Lambda([Blueprint, Asset](const FAssetData& ReferencedAsset) -> TArray<FSoftObjectPath>
		{
			return UTLLRN_ControlRigBlueprint::GetReferencesToTLLRN_RigModule(Asset);
		})
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
		.Source(Asset)
		.TargetAssetFilters(TargetFilters);

	const TSharedRef<SRigVMBulkEditDialog<SRigVMSwapAssetReferencesWidget>> SwapModulesDialog =
		SNew(SRigVMBulkEditDialog<SRigVMSwapAssetReferencesWidget>)
		.WindowSize(FVector2D(800.0f, 640.0f))
		.WidgetArgs(WidgetArgs);
	
	SwapModulesDialog->ShowNormal();
}

void FTLLRN_ControlRigEditor::SynchronizeViewportBoneSelection()
{
	UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint();
	if (RigBlueprint == nullptr)
	{
		return;
	}

	UTLLRN_ControlRigSkeletalMeshComponent* EditorSkelComp = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent());
	if (EditorSkelComp)
	{
		EditorSkelComp->BonesOfInterest.Reset();

		TArray<const FTLLRN_RigBaseElement*> SelectedBones = GetHierarchyBeingDebugged()->GetSelectedElements(ETLLRN_RigElementType::Bone);
		for (const FTLLRN_RigBaseElement* SelectedBone : SelectedBones)
		{
 			const int32 BoneIndex = EditorSkelComp->GetReferenceSkeleton().FindBoneIndex(SelectedBone->GetFName());
			if(BoneIndex != INDEX_NONE)
			{
				EditorSkelComp->BonesOfInterest.AddUnique(BoneIndex);
			}
		}
	}
}

void FTLLRN_ControlRigEditor::UpdateBoneModification(FName BoneName, const FTransform& LocalTransform)
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{ 
		if (PreviewInstance)
		{ 
			if (FAnimNode_ModifyBone* Modify = PreviewInstance->FindModifiedBone(BoneName))
			{
				Modify->Translation = LocalTransform.GetTranslation();
				Modify->Rotation = LocalTransform.GetRotation().Rotator();
				Modify->TranslationSpace = EBoneControlSpace::BCS_ParentBoneSpace;
				Modify->RotationSpace = EBoneControlSpace::BCS_ParentBoneSpace; 
			}
		}
		
		TMap<FName, FTransform>* TransformOverrideMap = &TLLRN_ControlRig->TransformOverrideForUserCreatedBones;
		if (UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetBlueprintObj()->GetObjectBeingDebugged()))
		{
			TransformOverrideMap = &DebuggedTLLRN_ControlRig->TransformOverrideForUserCreatedBones;
		}

		if (FTransform* Transform = TransformOverrideMap->Find(BoneName))
		{
			*Transform = LocalTransform;
		}
	}
}

void FTLLRN_ControlRigEditor::RemoveBoneModification(FName BoneName)
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		if (PreviewInstance)
		{
			PreviewInstance->RemoveBoneModification(BoneName);
		}

		TMap<FName, FTransform>* TransformOverrideMap = &TLLRN_ControlRig->TransformOverrideForUserCreatedBones;
		if (UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetBlueprintObj()->GetObjectBeingDebugged()))
		{
			TransformOverrideMap = &DebuggedTLLRN_ControlRig->TransformOverrideForUserCreatedBones;
		}

		TransformOverrideMap->Remove(BoneName);
	}
}

void FTLLRN_ControlRigEditor::ResetAllBoneModification()
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = GetTLLRN_ControlRig())
	{
		if (IsValid(PreviewInstance))
		{
			PreviewInstance->ResetModifiedBone();
		}

		TMap<FName, FTransform>* TransformOverrideMap = &TLLRN_ControlRig->TransformOverrideForUserCreatedBones;
		if (UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetBlueprintObj()->GetObjectBeingDebugged()))
		{
			TransformOverrideMap = &DebuggedTLLRN_ControlRig->TransformOverrideForUserCreatedBones;
		}

		TransformOverrideMap->Reset();
	}
}

FTLLRN_ControlRigEditorEditMode* FTLLRN_ControlRigEditor::GetEditMode() const
{
	return static_cast<FTLLRN_ControlRigEditorEditMode*>(GetEditorModeManager().GetActiveMode(GetEditorModeName()));
}


void FTLLRN_ControlRigEditor::OnCurveContainerChanged()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	ClearDetailObject();

	FBlueprintEditorUtils::MarkBlueprintAsModified(GetTLLRN_ControlRigBlueprint());

	UTLLRN_ControlRigSkeletalMeshComponent* EditorSkelComp = Cast<UTLLRN_ControlRigSkeletalMeshComponent>(GetPersonaToolkit()->GetPreviewScene()->GetPreviewMeshComponent());
	if (EditorSkelComp)
	{
		// restart animation 
		EditorSkelComp->InitAnim(true);
		UpdateRigVMHost();
	}
	CacheNameLists();

	// notification
	FNotificationInfo Info(LOCTEXT("CurveContainerChangeHelpMessage", "CurveContainer has been successfully modified."));
	Info.bFireAndForget = true;
	Info.FadeOutDuration = 5.0f;
	Info.ExpireDuration = 5.0f;

	TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationPtr->SetCompletionState(SNotificationItem::CS_Success);
}

void FTLLRN_ControlRigEditor::CreateTLLRN_RigHierarchyToGraphDragAndDropMenu() const
{
	const FName MenuName = TLLRN_RigHierarchyToGraphDragAndDropMenuName;
	UToolMenus* ToolMenus = UToolMenus::Get();
	
	if(!ensure(ToolMenus))
	{
		return;
	}

	if (!ToolMenus->IsMenuRegistered(MenuName))
	{
		UToolMenu* Menu = ToolMenus->RegisterMenu(MenuName);

		Menu->AddDynamicSection(NAME_None, FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu)
			{
				UTLLRN_ControlRigContextMenuContext* MainContext = InMenu->FindContext<UTLLRN_ControlRigContextMenuContext>();
				
				if (FTLLRN_ControlRigEditor* TLLRN_ControlRigEditor = MainContext->GetTLLRN_ControlRigEditor())
				{
					const FTLLRN_ControlRigTLLRN_RigHierarchyToGraphDragAndDropContext& DragDropContext = MainContext->GetTLLRN_RigHierarchyToGraphDragAndDropContext();

					UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigEditor->GetHierarchyBeingDebugged();
					TArray<FTLLRN_RigElementKey> DraggedKeys = DragDropContext.DraggedElementKeys;
					TLLRN_ControlRigEditor->FilterDraggedKeys(DraggedKeys, true);
					
					UEdGraph* Graph = DragDropContext.Graph.Get();
					const FVector2D& NodePosition = DragDropContext.NodePosition;
					
					// if multiple types are selected, we show Get Elements/Set Elements
					bool bMultipleTypeSelected = false;

					ETLLRN_RigElementType LastType = ETLLRN_RigElementType::None;
			
					if (DraggedKeys.Num() > 0)
					{
						LastType = DraggedKeys[0].Type;
					}
			
					uint8 DraggedTypes = 0;
					uint8 DraggedAnimationTypes = 2;
					for (const FTLLRN_RigElementKey& DraggedKey : DragDropContext.DraggedElementKeys)
					{
						if (DraggedKey.Type != LastType)
						{
							bMultipleTypeSelected = true;
						}
						else if(DraggedKey.Type == ETLLRN_RigElementType::Control)
						{
							if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(DraggedKey))
							{
								const uint8 DraggedAnimationType = ControlElement->IsAnimationChannel() ? 1 : 0; 
								if(DraggedAnimationTypes == 2)
								{
									DraggedAnimationTypes = DraggedAnimationType;
								}
								else
								{
									if(DraggedAnimationTypes != DraggedAnimationType)
									{
										bMultipleTypeSelected = true;
									}
								}
							}
						}
				
						DraggedTypes = DraggedTypes | (uint8)DraggedKey.Type;
					}
					
					const FText SectionText = FText::FromString(DragDropContext.GetSectionTitle());
					FToolMenuSection& Section = InMenu->AddSection(NAME_None, SectionText);

					FText GetterLabel = LOCTEXT("GetElement","Get Element");
					FText GetterTooltip = LOCTEXT("GetElement_ToolTip", "Getter For Element");
					FText SetterLabel = LOCTEXT("SetElement","Set Element");
					FText SetterTooltip = LOCTEXT("SetElement_ToolTip", "Setter For Element");
					// if multiple types are selected, we show Get Elements/Set Elements
					if (bMultipleTypeSelected)
					{
						GetterLabel = LOCTEXT("GetElements","Get Elements");
						GetterTooltip = LOCTEXT("GetElements_ToolTip", "Getter For Elements");
						SetterLabel = LOCTEXT("SetElements","Set Elements");
						SetterTooltip = LOCTEXT("SetElements_ToolTip", "Setter For Elements");
					}
					else
					{
						// otherwise, we show "Get Bone/NUll/Control"
						if ((DraggedTypes & (uint8)ETLLRN_RigElementType::Bone) != 0)
						{
							GetterLabel = LOCTEXT("GetBone","Get Bone");
							GetterTooltip = LOCTEXT("GetBone_ToolTip", "Getter For Bone");
							SetterLabel = LOCTEXT("SetBone","Set Bone");
							SetterTooltip = LOCTEXT("SetBone_ToolTip", "Setter For Bone");
						}
						else if ((DraggedTypes & (uint8)ETLLRN_RigElementType::Null) != 0)
						{
							GetterLabel = LOCTEXT("GetNull","Get Null");
							GetterTooltip = LOCTEXT("GetNull_ToolTip", "Getter For Null");
							SetterLabel = LOCTEXT("SetNull","Set Null");
							SetterTooltip = LOCTEXT("SetNull_ToolTip", "Setter For Null");
						}
						else if ((DraggedTypes & (uint8)ETLLRN_RigElementType::Control) != 0)
						{
							if(DraggedAnimationTypes == 0)
							{
								GetterLabel = LOCTEXT("GetControl","Get Control");
								GetterTooltip = LOCTEXT("GetControl_ToolTip", "Getter For Control");
								SetterLabel = LOCTEXT("SetControl","Set Control");
								SetterTooltip = LOCTEXT("SetControl_ToolTip", "Setter For Control");
							}
							else
							{
								GetterLabel = LOCTEXT("GetAnimationChannel","Get Animation Channel");
								GetterTooltip = LOCTEXT("GetAnimationChannel_ToolTip", "Getter For Animation Channel");
								SetterLabel = LOCTEXT("SetAnimationChannel","Set Animation Channel");
								SetterTooltip = LOCTEXT("SetAnimationChannel_ToolTip", "Setter For Animation Channel");
							}
						}
						else if ((DraggedTypes & (uint8)ETLLRN_RigElementType::Connector) != 0)
						{
							GetterLabel = LOCTEXT("GetConnector","Get Connector");
							GetterTooltip = LOCTEXT("GetConnector_ToolTip", "Getter For Connector");
							SetterLabel = LOCTEXT("SetConnector","Set Connector");
							SetterTooltip = LOCTEXT("SetConnector_ToolTip", "Setter For Connector");
						}
					}

					FToolMenuEntry GetElementsEntry = FToolMenuEntry::InitMenuEntry(
						TEXT("GetElements"),
						GetterLabel,
						GetterTooltip,
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateSP(TLLRN_ControlRigEditor, &FTLLRN_ControlRigEditor::HandleMakeElementGetterSetter, ETLLRN_RigElementGetterSetterType_Transform, true, DraggedKeys, Graph, NodePosition),
							FCanExecuteAction()
						)
					);
					GetElementsEntry.InsertPosition.Name = NAME_None;
					GetElementsEntry.InsertPosition.Position = EToolMenuInsertType::First;
					

					Section.AddEntry(GetElementsEntry);

					FToolMenuEntry SetElementsEntry = FToolMenuEntry::InitMenuEntry(
						TEXT("SetElements"),
						SetterLabel,
						SetterTooltip,
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateSP(TLLRN_ControlRigEditor, &FTLLRN_ControlRigEditor::HandleMakeElementGetterSetter, ETLLRN_RigElementGetterSetterType_Transform, false, DraggedKeys, Graph, NodePosition),
							FCanExecuteAction()
						)
					);
					SetElementsEntry.InsertPosition.Name = GetElementsEntry.Name;
					SetElementsEntry.InsertPosition.Position = EToolMenuInsertType::After;	

					Section.AddEntry(SetElementsEntry);

					if (((DraggedTypes & (uint8)ETLLRN_RigElementType::Bone) != 0) ||
						((DraggedTypes & (uint8)ETLLRN_RigElementType::Control) != 0) ||
						((DraggedTypes & (uint8)ETLLRN_RigElementType::Null) != 0) ||
						((DraggedTypes & (uint8)ETLLRN_RigElementType::Connector) != 0))
					{
						FToolMenuEntry& RotationTranslationSeparator = Section.AddSeparator(TEXT("RotationTranslationSeparator"));
						RotationTranslationSeparator.InsertPosition.Name = SetElementsEntry.Name;
						RotationTranslationSeparator.InsertPosition.Position = EToolMenuInsertType::After;

						FToolMenuEntry SetRotationEntry = FToolMenuEntry::InitMenuEntry(
							TEXT("SetRotation"),
							LOCTEXT("SetRotation","Set Rotation"),
							LOCTEXT("SetRotation_ToolTip","Setter for Rotation"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateSP(TLLRN_ControlRigEditor, &FTLLRN_ControlRigEditor::HandleMakeElementGetterSetter, ETLLRN_RigElementGetterSetterType_Rotation, false, DraggedKeys, Graph, NodePosition),
								FCanExecuteAction()
							)
						);
						
						SetRotationEntry.InsertPosition.Name = RotationTranslationSeparator.Name;
						SetRotationEntry.InsertPosition.Position = EToolMenuInsertType::After;		
						Section.AddEntry(SetRotationEntry);

						FToolMenuEntry SetTranslationEntry = FToolMenuEntry::InitMenuEntry(
							TEXT("SetTranslation"),
							LOCTEXT("SetTranslation","Set Translation"),
							LOCTEXT("SetTranslation_ToolTip","Setter for Translation"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateSP(TLLRN_ControlRigEditor, &FTLLRN_ControlRigEditor::HandleMakeElementGetterSetter, ETLLRN_RigElementGetterSetterType_Translation, false, DraggedKeys, Graph, NodePosition),
								FCanExecuteAction()
							)
						);

						SetTranslationEntry.InsertPosition.Name = SetRotationEntry.Name;
						SetTranslationEntry.InsertPosition.Position = EToolMenuInsertType::After;		
						Section.AddEntry(SetTranslationEntry);

						FToolMenuEntry AddOffsetEntry = FToolMenuEntry::InitMenuEntry(
							TEXT("AddOffset"),
							LOCTEXT("AddOffset","Add Offset"),
							LOCTEXT("AddOffset_ToolTip","Setter for Offset"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateSP(TLLRN_ControlRigEditor, &FTLLRN_ControlRigEditor::HandleMakeElementGetterSetter, ETLLRN_RigElementGetterSetterType_Offset, false, DraggedKeys, Graph, NodePosition),
								FCanExecuteAction()
							)
						);
						
						AddOffsetEntry.InsertPosition.Name = SetTranslationEntry.Name;
						AddOffsetEntry.InsertPosition.Position = EToolMenuInsertType::After;						
						Section.AddEntry(AddOffsetEntry);

						FToolMenuEntry& RelativeTransformSeparator = Section.AddSeparator(TEXT("RelativeTransformSeparator"));
						RelativeTransformSeparator.InsertPosition.Name = AddOffsetEntry.Name;
						RelativeTransformSeparator.InsertPosition.Position = EToolMenuInsertType::After;
						
						FToolMenuEntry GetRelativeTransformEntry = FToolMenuEntry::InitMenuEntry(
							TEXT("GetRelativeTransformEntry"),
							LOCTEXT("GetRelativeTransform", "Get Relative Transform"),
							LOCTEXT("GetRelativeTransform_ToolTip", "Getter for Relative Transform"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateSP(TLLRN_ControlRigEditor, &FTLLRN_ControlRigEditor::HandleMakeElementGetterSetter, ETLLRN_RigElementGetterSetterType_Relative, true, DraggedKeys, Graph, NodePosition),
								FCanExecuteAction()
							)
						);
						
						GetRelativeTransformEntry.InsertPosition.Name = RelativeTransformSeparator.Name;
						GetRelativeTransformEntry.InsertPosition.Position = EToolMenuInsertType::After;	
						Section.AddEntry(GetRelativeTransformEntry);
						
						FToolMenuEntry SetRelativeTransformEntry = FToolMenuEntry::InitMenuEntry(
							TEXT("SetRelativeTransformEntry"),
							LOCTEXT("SetRelativeTransform", "Set Relative Transform"),
							LOCTEXT("SetRelativeTransform_ToolTip", "Setter for Relative Transform"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateSP(TLLRN_ControlRigEditor, &FTLLRN_ControlRigEditor::HandleMakeElementGetterSetter, ETLLRN_RigElementGetterSetterType_Relative, false, DraggedKeys, Graph, NodePosition),
								FCanExecuteAction()
							)
						);
						SetRelativeTransformEntry.InsertPosition.Name = GetRelativeTransformEntry.Name;
						SetRelativeTransformEntry.InsertPosition.Position = EToolMenuInsertType::After;		
						Section.AddEntry(SetRelativeTransformEntry);
					}

					if (DraggedKeys.Num() > 0 && Hierarchy != nullptr)
					{
						FToolMenuEntry& ItemArraySeparator = Section.AddSeparator(TEXT("ItemArraySeparator"));
						ItemArraySeparator.InsertPosition.Name = TEXT("SetRelativeTransformEntry"),
						ItemArraySeparator.InsertPosition.Position = EToolMenuInsertType::After;
						
						FToolMenuEntry CreateItemArrayEntry = FToolMenuEntry::InitMenuEntry(
							TEXT("CreateItemArray"),
							LOCTEXT("CreateItemArray", "Create Item Array"),
							LOCTEXT("CreateItemArray_ToolTip", "Creates an item array from the selected elements in the hierarchy"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateLambda([TLLRN_ControlRigEditor, DraggedKeys, NodePosition]()
									{
										if (URigVMController* Controller = TLLRN_ControlRigEditor->GetFocusedController())
										{
											Controller->OpenUndoBracket(TEXT("Create Item Array From Selection"));

											if (URigVMNode* ItemsNode = Controller->AddUnitNode(FTLLRN_RigUnit_ItemArray::StaticStruct(), TEXT("Execute"), NodePosition))
											{
												if (URigVMPin* ItemsPin = ItemsNode->FindPin(TEXT("Items")))
												{
													Controller->SetArrayPinSize(ItemsPin->GetPinPath(), DraggedKeys.Num());

													TArray<URigVMPin*> ItemPins = ItemsPin->GetSubPins();
													ensure(ItemPins.Num() == DraggedKeys.Num());

													for (int32 ItemIndex = 0; ItemIndex < DraggedKeys.Num(); ItemIndex++)
													{
														FString DefaultValue;
														FTLLRN_RigElementKey::StaticStruct()->ExportText(DefaultValue, &DraggedKeys[ItemIndex], nullptr, nullptr, PPF_None, nullptr);
														Controller->SetPinDefaultValue(ItemPins[ItemIndex]->GetPinPath(), DefaultValue, true, true, false, true);
														Controller->SetPinExpansion(ItemPins[ItemIndex]->GetPinPath(), true, true, true);
													}

													Controller->SetPinExpansion(ItemsPin->GetPinPath(), true, true, true);
												}
											}

											Controller->CloseUndoBracket();
										}
									}
								)
							)
						);
						
						CreateItemArrayEntry.InsertPosition.Name = ItemArraySeparator.Name,
						CreateItemArrayEntry.InsertPosition.Position = EToolMenuInsertType::After;
						Section.AddEntry(CreateItemArrayEntry);
					}
				}
			})
		);
	}
}

void FTLLRN_ControlRigEditor::OnGraphNodeDropToPerform(TSharedPtr<FGraphNodeDragDropOp> InDragDropOp, UEdGraph* InGraph, const FVector2D& InNodePosition, const FVector2D& InScreenPosition)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if (InDragDropOp->IsOfType<FTLLRN_RigElementHierarchyDragDropOp>())
	{
		TSharedPtr<FTLLRN_RigElementHierarchyDragDropOp> TLLRN_RigHierarchyOp = StaticCastSharedPtr<FTLLRN_RigElementHierarchyDragDropOp>(InDragDropOp);

		if (TLLRN_RigHierarchyOp->GetElements().Num() > 0 && FocusedGraphEdPtr.IsValid())
		{
			const FName MenuName = TLLRN_RigHierarchyToGraphDragAndDropMenuName;
			
			UTLLRN_ControlRigContextMenuContext* MenuContext = NewObject<UTLLRN_ControlRigContextMenuContext>();
			FTLLRN_ControlRigMenuSpecificContext MenuSpecificContext;
			MenuSpecificContext.TLLRN_RigHierarchyToGraphDragAndDropContext =
				FTLLRN_ControlRigTLLRN_RigHierarchyToGraphDragAndDropContext(
					TLLRN_RigHierarchyOp->GetElements(),
					InGraph,
					InNodePosition
				);
			MenuContext->Init(SharedThis(this), MenuSpecificContext);
			
			UToolMenus* ToolMenus = UToolMenus::Get();
			TSharedRef<SWidget>	MenuWidget = ToolMenus->GenerateWidget(MenuName, FToolMenuContext(MenuContext));
			
			TSharedRef<SWidget> GraphEditorPanel = FocusedGraphEdPtr.Pin().ToSharedRef();

			// Show menu to choose getter vs setter
			FSlateApplication::Get().PushMenu(
				GraphEditorPanel,
				FWidgetPath(),
				MenuWidget,
				InScreenPosition,
				FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
			);

		}
	}
}

void FTLLRN_ControlRigEditor::HandleMakeElementGetterSetter(ETLLRN_RigElementGetterSetterType Type, bool bIsGetter, TArray<FTLLRN_RigElementKey> Keys, UEdGraph* Graph, FVector2D NodePosition)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if (Keys.Num() == 0)
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = GetHierarchyBeingDebugged();
	if (Hierarchy == nullptr)
	{
		return;
	}
	if (GetFocusedController() == nullptr)
	{
		return;
	}

	GetFocusedController()->OpenUndoBracket(TEXT("Adding Nodes from Hierarchy"));

	struct FNewNodeData
	{
		FName Name;
		FName ValuePinName;
		ETLLRN_RigControlType ValueType;
		FTLLRN_RigControlValue Value;
	};
	TArray<FNewNodeData> NewNodes;

	TArray<FTLLRN_RigElementKey> KeysIncludingNameSpace = Keys;
	FilterDraggedKeys(KeysIncludingNameSpace, false);

	for (int32 Index = 0; Index < Keys.Num(); Index++)
	{
		const FTLLRN_RigElementKey& Key = Keys[Index];
		const FTLLRN_RigElementKey& KeyIncludingNameSpace = KeysIncludingNameSpace[Index];
		
		UScriptStruct* StructTemplate = nullptr;

		FNewNodeData NewNode;
		NewNode.Name = NAME_None;
		NewNode.ValuePinName = NAME_None;

		TArray<FName> ItemPins;
		ItemPins.Add(TEXT("Item"));

		FName NameValue = Key.Name;
		FName ChannelValue = Key.Name;
		TArray<FName> NamePins;
		TArray<FName> ChannelPins;
		TMap<FName, int32> PinsToResolve; 

		if(FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(KeyIncludingNameSpace))
		{
			if(ControlElement->IsAnimationChannel())
			{
				ChannelValue = ControlElement->GetDisplayName();
				
				if(const FTLLRN_RigControlElement* ParentControlElement = Cast<FTLLRN_RigControlElement>(Hierarchy->GetFirstParent(ControlElement)))
				{
					NameValue = ParentControlElement->GetFName();
				}
				else
				{
					NameValue = NAME_None;
				}

				ItemPins.Reset();
				NamePins.Add(TEXT("Control"));
				ChannelPins.Add(TEXT("Channel"));
				static const FName ValueName = GET_MEMBER_NAME_CHECKED(FTLLRN_RigUnit_GetBoolAnimationChannel, Value);

				switch (ControlElement->Settings.ControlType)
				{
					case ETLLRN_RigControlType::Bool:
					{
						if(bIsGetter)
						{
							StructTemplate = FTLLRN_RigUnit_GetBoolAnimationChannel::StaticStruct();
						}
						else
						{
							StructTemplate = FTLLRN_RigUnit_SetBoolAnimationChannel::StaticStruct();
						}
						PinsToResolve.Add(ValueName, RigVMTypeUtils::TypeIndex::Bool);
						break;
					}
					case ETLLRN_RigControlType::Float:
					case ETLLRN_RigControlType::ScaleFloat:
					{
						if(bIsGetter)
						{
							StructTemplate = FTLLRN_RigUnit_GetFloatAnimationChannel::StaticStruct();
						}
						else
						{
							StructTemplate = FTLLRN_RigUnit_SetFloatAnimationChannel::StaticStruct();
						}
						PinsToResolve.Add(ValueName, RigVMTypeUtils::TypeIndex::Float);
						break;
					}
					case ETLLRN_RigControlType::Integer:
					{
						if(bIsGetter)
						{
							StructTemplate = FTLLRN_RigUnit_GetIntAnimationChannel::StaticStruct();
						}
						else
						{
							StructTemplate = FTLLRN_RigUnit_SetIntAnimationChannel::StaticStruct();
						}
						PinsToResolve.Add(ValueName, RigVMTypeUtils::TypeIndex::Int32);
						break;
					}
					case ETLLRN_RigControlType::Vector2D:
					{
						if(bIsGetter)
						{
							StructTemplate = FTLLRN_RigUnit_GetVector2DAnimationChannel::StaticStruct();
						}
						else
						{
							StructTemplate = FTLLRN_RigUnit_SetVector2DAnimationChannel::StaticStruct();
						}

						UScriptStruct* ValueStruct = TBaseStructure<FVector2D>::Get();
						const FRigVMTemplateArgumentType TypeForStruct(*RigVMTypeUtils::GetUniqueStructTypeName(ValueStruct), ValueStruct);
						const int32 TypeIndex = FRigVMRegistry::Get().GetTypeIndex(TypeForStruct);
						PinsToResolve.Add(ValueName, TypeIndex);
						break;
					}
					case ETLLRN_RigControlType::Position:
					case ETLLRN_RigControlType::Scale:
					{
						if(bIsGetter)
						{
							StructTemplate = FTLLRN_RigUnit_GetVectorAnimationChannel::StaticStruct();
						}
						else
						{
							StructTemplate = FTLLRN_RigUnit_SetVectorAnimationChannel::StaticStruct();
						}
						UScriptStruct* ValueStruct = TBaseStructure<FVector>::Get();
						const FRigVMTemplateArgumentType TypeForStruct(*RigVMTypeUtils::GetUniqueStructTypeName(ValueStruct), ValueStruct);
						const int32 TypeIndex = FRigVMRegistry::Get().GetTypeIndex(TypeForStruct);
						PinsToResolve.Add(ValueName, TypeIndex);
						break;
					}
					case ETLLRN_RigControlType::Rotator:
					{
						if(bIsGetter)
						{
							StructTemplate = FTLLRN_RigUnit_GetRotatorAnimationChannel::StaticStruct();
						}
						else
						{
							StructTemplate = FTLLRN_RigUnit_SetRotatorAnimationChannel::StaticStruct();
						}
						UScriptStruct* ValueStruct = TBaseStructure<FRotator>::Get();
						const FRigVMTemplateArgumentType TypeForStruct(*ValueStruct->GetStructCPPName(), ValueStruct);
						const int32 TypeIndex = FRigVMRegistry::Get().GetTypeIndex(TypeForStruct);
						PinsToResolve.Add(ValueName, TypeIndex);
						break;
					}
					case ETLLRN_RigControlType::Transform:
					case ETLLRN_RigControlType::TransformNoScale:
					case ETLLRN_RigControlType::EulerTransform:
					{
						if(bIsGetter)
						{
							StructTemplate = FTLLRN_RigUnit_GetTransformAnimationChannel::StaticStruct();
						}
						else
						{
							StructTemplate = FTLLRN_RigUnit_SetTransformAnimationChannel::StaticStruct();
						}
						UScriptStruct* ValueStruct = TBaseStructure<FTransform>::Get();
						const FRigVMTemplateArgumentType TypeForStruct(*RigVMTypeUtils::GetUniqueStructTypeName(ValueStruct), ValueStruct);
						const int32 TypeIndex = FRigVMRegistry::Get().GetTypeIndex(TypeForStruct);
						PinsToResolve.Add(ValueName, TypeIndex);
						break;
					}
					default:
					{
						break;
					}
				}
			}
		}

		if (bIsGetter && StructTemplate == nullptr)
		{
			switch (Type)
			{
				case ETLLRN_RigElementGetterSetterType_Transform:
				{
					if (Key.Type == ETLLRN_RigElementType::Control)
					{
						FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(KeyIncludingNameSpace);
						if(ControlElement == nullptr)
						{
							return;
						}
						
						switch (ControlElement->Settings.ControlType)
						{
							case ETLLRN_RigControlType::Bool:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_GetControlBool::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Float:
							case ETLLRN_RigControlType::ScaleFloat:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_GetControlFloat::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Integer:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_GetControlInteger::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Vector2D:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_GetControlVector2D::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Position:
							case ETLLRN_RigControlType::Scale:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_GetControlVector::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Rotator:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_GetControlRotator::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Transform:
							case ETLLRN_RigControlType::TransformNoScale:
							case ETLLRN_RigControlType::EulerTransform:
							{
								StructTemplate = FTLLRN_RigUnit_GetTransform::StaticStruct();
								break;
							}
							default:
							{
								break;
							}
						}
					}
					else
					{
						StructTemplate = FTLLRN_RigUnit_GetTransform::StaticStruct();
					}
					break;
				}
				case ETLLRN_RigElementGetterSetterType_Initial:
				{
					StructTemplate = FTLLRN_RigUnit_GetTransform::StaticStruct();
					break;
				}
				case ETLLRN_RigElementGetterSetterType_Relative:
				{
					StructTemplate = FTLLRN_RigUnit_GetRelativeTransformForItem::StaticStruct();
					ItemPins.Reset();
					ItemPins.Add(TEXT("Child"));
					ItemPins.Add(TEXT("Parent"));
					break;
				}
				default:
				{
					break;
				}
			}
		}
		else if(StructTemplate == nullptr)
		{
			switch (Type)
			{
				case ETLLRN_RigElementGetterSetterType_Transform:
				{
					if (Key.Type == ETLLRN_RigElementType::Control)
					{
						FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(KeyIncludingNameSpace);
						if(ControlElement == nullptr)
						{
							return;
						}

						switch (ControlElement->Settings.ControlType)
						{
							case ETLLRN_RigControlType::Bool:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_SetControlBool::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Float:
							case ETLLRN_RigControlType::ScaleFloat:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_SetControlFloat::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Integer:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_SetControlInteger::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Vector2D:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_SetControlVector2D::StaticStruct();
								break;
							}
							case ETLLRN_RigControlType::Position:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_SetControlVector::StaticStruct();
								NewNode.ValuePinName = TEXT("Vector");
								NewNode.ValueType = ETLLRN_RigControlType::Position;
								NewNode.Value = FTLLRN_RigControlValue::Make<FVector>(Hierarchy->GetGlobalTransform(Key).GetLocation());
								break;
							}
							case ETLLRN_RigControlType::Scale:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_SetControlVector::StaticStruct();
								NewNode.ValuePinName = TEXT("Vector");
								NewNode.ValueType = ETLLRN_RigControlType::Scale;
								NewNode.Value = FTLLRN_RigControlValue::Make<FVector>(Hierarchy->GetGlobalTransform(Key).GetScale3D());
								break;
							}
							case ETLLRN_RigControlType::Rotator:
							{
								NamePins.Add(TEXT("Control"));
								StructTemplate = FTLLRN_RigUnit_SetControlRotator::StaticStruct();
								NewNode.ValuePinName = TEXT("Rotator");
								NewNode.ValueType = ETLLRN_RigControlType::Rotator;
								NewNode.Value = FTLLRN_RigControlValue::Make<FRotator>(Hierarchy->GetGlobalTransform(Key).Rotator());
								break;
							}
							case ETLLRN_RigControlType::Transform:
							case ETLLRN_RigControlType::TransformNoScale:
							case ETLLRN_RigControlType::EulerTransform:
							{
								StructTemplate = FTLLRN_RigUnit_SetTransform::StaticStruct();
								NewNode.ValuePinName = TEXT("Transform");
								NewNode.ValueType = ETLLRN_RigControlType::Transform;
								NewNode.Value = FTLLRN_RigControlValue::Make<FTransform>(Hierarchy->GetGlobalTransform(Key));
								break;
							}
							default:
							{
								break;
							}
						}
					}
					else
					{
						StructTemplate = FTLLRN_RigUnit_SetTransform::StaticStruct();
						NewNode.ValuePinName = TEXT("Transform");
						NewNode.ValueType = ETLLRN_RigControlType::Transform;
						NewNode.Value = FTLLRN_RigControlValue::Make<FTransform>(Hierarchy->GetGlobalTransform(Key));
					}
					break;
				}
				case ETLLRN_RigElementGetterSetterType_Relative:
				{
					StructTemplate = FTLLRN_RigUnit_SetRelativeTransformForItem::StaticStruct();
					ItemPins.Reset();
					ItemPins.Add(TEXT("Child"));
					ItemPins.Add(TEXT("Parent"));
					break;
				}
				case ETLLRN_RigElementGetterSetterType_Rotation:
				{
					StructTemplate = FTLLRN_RigUnit_SetRotation::StaticStruct();
					NewNode.ValuePinName = TEXT("Rotation");
					NewNode.ValueType = ETLLRN_RigControlType::Rotator;
					NewNode.Value = FTLLRN_RigControlValue::Make<FRotator>(Hierarchy->GetGlobalTransform(Key).Rotator());
					break;
				}
				case ETLLRN_RigElementGetterSetterType_Translation:
				{
					StructTemplate = FTLLRN_RigUnit_SetTranslation::StaticStruct();
					NewNode.ValuePinName = TEXT("Translation");
					NewNode.ValueType = ETLLRN_RigControlType::Position;
					NewNode.Value = FTLLRN_RigControlValue::Make<FVector>(Hierarchy->GetGlobalTransform(Key).GetLocation());
					break;
				}
				case ETLLRN_RigElementGetterSetterType_Offset:
				{
					StructTemplate = FTLLRN_RigUnit_OffsetTransformForItem::StaticStruct();
					break;
				}
				default:
				{
					break;
				}
			}
		}

		if (StructTemplate == nullptr)
		{
			return;
		}

		FVector2D NodePositionIncrement(0.f, 120.f);
		if (!bIsGetter)
		{
			NodePositionIncrement = FVector2D(380.f, 0.f);
		}

		FName Name = FRigVMBlueprintUtils::ValidateName(GetTLLRN_ControlRigBlueprint(), StructTemplate->GetName());
		if (URigVMUnitNode* ModelNode = GetFocusedController()->AddUnitNode(StructTemplate, FTLLRN_RigUnit::GetMethodName(), NodePosition, FString(), true, true))
		{
			FString ItemTypeStr = StaticEnum<ETLLRN_RigElementType>()->GetDisplayNameTextByValue((int64)Key.Type).ToString();
			NewNode.Name = ModelNode->GetFName();
			NewNodes.Add(NewNode);

			for (const TPair<FName, int32>& PinToResolve : PinsToResolve)
			{
				if(URigVMPin* Pin = ModelNode->FindPin(PinToResolve.Key.ToString()))
				{
					GetFocusedController()->ResolveWildCardPin(Pin, PinToResolve.Value, true, true);
				}
			}

			for (const FName& ItemPin : ItemPins)
			{
				GetFocusedController()->SetPinDefaultValue(FString::Printf(TEXT("%s.%s.Name"), *ModelNode->GetName(), *ItemPin.ToString()), Key.Name.ToString(), true, true, false, true);
				GetFocusedController()->SetPinDefaultValue(FString::Printf(TEXT("%s.%s.Type"), *ModelNode->GetName(), *ItemPin.ToString()), ItemTypeStr, true, true, false, true);
			}

			for (const FName& NamePin : NamePins)
			{
				const FString PinPath = FString::Printf(TEXT("%s.%s"), *ModelNode->GetName(), *NamePin.ToString());
				GetFocusedController()->SetPinDefaultValue(PinPath, NameValue.ToString(), true, true, false, true);
			}

			for (const FName& ChannelPin : ChannelPins)
			{
				const FString PinPath = FString::Printf(TEXT("%s.%s"), *ModelNode->GetName(), *ChannelPin.ToString());
				GetFocusedController()->SetPinDefaultValue(PinPath, ChannelValue.ToString(), true, true, false, true);
			}

			if (!NewNode.ValuePinName.IsNone())
			{
				FString DefaultValue;

				switch (NewNode.ValueType)
				{
					case ETLLRN_RigControlType::Position:
					case ETLLRN_RigControlType::Scale:
					{
						DefaultValue = NewNode.Value.ToString<FVector>();
						break;
					}
					case ETLLRN_RigControlType::Rotator:
					{
						DefaultValue = NewNode.Value.ToString<FRotator>();
						break;
					}
					case ETLLRN_RigControlType::Transform:
					{
						DefaultValue = NewNode.Value.ToString<FTransform>();
						break;
					}
					default:
					{
						break;
					}
				}
				if (!DefaultValue.IsEmpty())
				{
					GetFocusedController()->SetPinDefaultValue(FString::Printf(TEXT("%s.%s"), *ModelNode->GetName(), *NewNode.ValuePinName.ToString()), DefaultValue, true, true, false, true);
				}
			}

			URigVMEdGraphUnitNodeSpawner::HookupMutableNode(ModelNode, GetTLLRN_ControlRigBlueprint());
		}

		NodePosition += NodePositionIncrement;
	}

	if (NewNodes.Num() > 0)
	{
		TArray<FName> NewNodeNames;
		for (const FNewNodeData& NewNode : NewNodes)
		{
			NewNodeNames.Add(NewNode.Name);
		}
		GetFocusedController()->SetNodeSelection(NewNodeNames);
		GetFocusedController()->CloseUndoBracket();
	}
	else
	{
		GetFocusedController()->CancelUndoBracket();
	}
}

void FTLLRN_ControlRigEditor::HandleOnControlModified(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = Cast<UTLLRN_ControlRig>(GetBlueprintObj()->GetObjectBeingDebugged());
	if (Subject != DebuggedTLLRN_ControlRig)
	{
		return;
	}

	UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprintObj());
	if (Blueprint == nullptr)
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = Subject->GetHierarchy();

	if (ControlElement->Settings.bIsTransientControl && !GIsTransacting)
	{
		const URigVMUnitNode* UnitNode = nullptr;
		const FString NodeName = UTLLRN_ControlRig::GetNodeNameFromTransientControl(ControlElement->GetKey());
		const FString PoseTarget = UTLLRN_ControlRig::GetTargetFromTransientControl(ControlElement->GetKey());
		TSharedPtr<FStructOnScope> NodeInstance;
		TSharedPtr<FRigDirectManipulationInfo> ManipulationInfo;

		// try to find the direct manipulation info on the rig. if there's no matching information
		// the manipulation is likely happening on a bone instead.
		if(DebuggedTLLRN_ControlRig && !NodeName.IsEmpty() && !PoseTarget.IsEmpty())
		{
			UnitNode = Cast<URigVMUnitNode>(GetFocusedModel()->FindNode(NodeName));
			if(UnitNode)
			{
				if(UnitNode->GetScriptStruct())
				{
					NodeInstance = UnitNode->ConstructStructInstance(false);
					ManipulationInfo = DebuggedTLLRN_ControlRig->GetTLLRN_RigUnitManipulationInfoForTransientControl(ControlElement->GetKey());
				}
				else
				{
					UnitNode = nullptr;
				}
			}
		}
		
		if (UnitNode && NodeInstance.IsValid() && ManipulationInfo.IsValid())
		{
			FTLLRN_RigUnit* UnitInstance = DebuggedTLLRN_ControlRig->GetTLLRN_RigUnitInstanceFromScope(NodeInstance);
			check(UnitInstance);

			const FTLLRN_RigPose Pose = DebuggedTLLRN_ControlRig->GetHierarchy()->GetPose();

			// update the node based on the incoming pose. once that is done we'll need to compare the node instance
			// with the settings on the node in the graph and update them accordingly.
			FTLLRN_ControlRigExecuteContext& ExecuteContext = DebuggedTLLRN_ControlRig->GetRigVMExtendedExecuteContext().GetPublicDataSafe<FTLLRN_ControlRigExecuteContext>();
			const FTLLRN_RigHierarchyRedirectorGuard RedirectorGuard(DebuggedTLLRN_ControlRig);
			if(UnitInstance->UpdateDirectManipulationFromHierarchy(UnitNode, NodeInstance, ExecuteContext, ManipulationInfo))
			{
				UnitNode->UpdateHostFromStructInstance(DebuggedTLLRN_ControlRig, NodeInstance);
				DebuggedTLLRN_ControlRig->GetHierarchy()->SetPose(Pose);
				
				URigVMController* Controller = Blueprint->GetOrCreateController(UnitNode->GetGraph());
				TMap<FString, FString> PinPathToNewDefaultValue;
				UnitNode->ComputePinValueDifferences(NodeInstance, PinPathToNewDefaultValue);
				if(!PinPathToNewDefaultValue.IsEmpty())
				{
					// we'll disable compilation since the control rig editor module will have disabled folding of literals
					// so each register is free to be edited directly.
					TGuardValue<bool> DisableBlueprintNotifs(Blueprint->bSuspendModelNotificationsForSelf, true);

					if(PinPathToNewDefaultValue.Num() > 1)
					{
						Controller->OpenUndoBracket(TEXT("Set pin defaults during manipulation"));
					}
					bool bChangedSomething = false;

					for(const TPair<FString, FString>& Pair : PinPathToNewDefaultValue)
					{
						if(const URigVMPin* Pin = UnitNode->FindPin(Pair.Key))
						{
							if(Controller->SetPinDefaultValue(Pin->GetPinPath(), Pair.Value, true, true, true, false, false))
							{
								bChangedSomething = true;
							}
						}
					}

					if(PinPathToNewDefaultValue.Num() > 1)
					{
						if(bChangedSomething)
						{
							Controller->CloseUndoBracket();
						}
						else
						{
							Controller->CancelUndoBracket();
						}
					}
				}

			}
		}
		else
		{
			FTLLRN_RigControlValue ControlValue = Hierarchy->GetControlValue(ControlElement, ETLLRN_RigControlValueType::Current);
			const FTLLRN_RigElementKey ElementKey = UTLLRN_ControlRig::GetElementKeyFromTransientControl(ControlElement->GetKey());

			if (ElementKey.Type == ETLLRN_RigElementType::Bone)
			{
				const FTransform CurrentValue = ControlValue.Get<FTLLRN_RigControlValue::FTransform_Float>().ToTransform();
				const FTransform Transform = CurrentValue * Hierarchy->GetControlOffsetTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
				Blueprint->Hierarchy->SetLocalTransform(ElementKey, Transform);
				Hierarchy->SetLocalTransform(ElementKey, Transform);

				if (IsConstructionModeEnabled())
				{
					Blueprint->Hierarchy->SetInitialLocalTransform(ElementKey, Transform);
					Hierarchy->SetInitialLocalTransform(ElementKey, Transform);
				}
				else
				{ 
					UpdateBoneModification(ElementKey.Name, Transform);
				}
			}
			else if (ElementKey.Type == ETLLRN_RigElementType::Null)
			{
				const FTransform GlobalTransform = GetTLLRN_ControlRig()->GetControlGlobalTransform(ControlElement->GetFName());
				Blueprint->Hierarchy->SetGlobalTransform(ElementKey, GlobalTransform);
				Hierarchy->SetGlobalTransform(ElementKey, GlobalTransform);
				if (IsConstructionModeEnabled())
				{
					Blueprint->Hierarchy->SetInitialGlobalTransform(ElementKey, GlobalTransform);
					Hierarchy->SetInitialGlobalTransform(ElementKey, GlobalTransform);
				}
			}
		}
	}
	else if (IsConstructionModeEnabled())
	{
		FTLLRN_RigControlElement* SourceControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(ControlElement->GetKey());
		FTLLRN_RigControlElement* TargetControlElement = Blueprint->Hierarchy->Find<FTLLRN_RigControlElement>(ControlElement->GetKey());
		if(SourceControlElement && TargetControlElement)
		{
			TargetControlElement->Settings = SourceControlElement->Settings;

			// only fire the setting change if the interaction is not currently ongoing
			if(!Subject->ElementsBeingInteracted.Contains(ControlElement->GetKey()))
			{
				Blueprint->Hierarchy->OnModified().Broadcast(ETLLRN_RigHierarchyNotification::ControlSettingChanged, Blueprint->Hierarchy, TargetControlElement);
			}

			// we copy the pose including the weights since we want the topology to align during construction mode.
			// i.e. dynamic reparenting should be reset here.
			TargetControlElement->CopyPose(SourceControlElement, true, true, true);
		}
	}
}

void FTLLRN_ControlRigEditor::HandleRefreshEditorFromBlueprint(URigVMBlueprint* InBlueprint)
{
	OnHierarchyChanged();
	FRigVMEditor::HandleRefreshEditorFromBlueprint(InBlueprint);
}

UToolMenu* FTLLRN_ControlRigEditor::HandleOnGetViewportContextMenuDelegate()
{
	if (OnGetViewportContextMenuDelegate.IsBound())
	{
		return OnGetViewportContextMenuDelegate.Execute();
	}
	return nullptr;
}

TSharedPtr<FUICommandList> FTLLRN_ControlRigEditor::HandleOnViewportContextMenuCommandsDelegate()
{
	if (OnViewportContextMenuCommandsDelegate.IsBound())
	{
		return OnViewportContextMenuCommandsDelegate.Execute();
	}
	return TSharedPtr<FUICommandList>();
}

void FTLLRN_ControlRigEditor::OnPreForwardsSolve_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName)
{
	// if we are debugging a PIE instance, we need to remember the input pose on the
	// rig so we can perform multiple evaluations. this is to avoid double transforms / double forward solve results.
	if(InRig->GetWorld()->IsPlayInEditor())
	{
		if(!InRig->GetWorld()->IsPaused())
		{
			// store the pose while PIE is running
			InRig->InputPoseOnDebuggedRig = InRig->GetHierarchy()->GetPose(false, false);
		}
		else
		{
			// reapply the pose as PIE is paused. during pause the rig won't be updated with the input pose
			// from the animbp / client thus we need to reset the pose to avoid double transformation.
			InRig->GetHierarchy()->SetPose(InRig->InputPoseOnDebuggedRig);
		}
	}
}

void FTLLRN_ControlRigEditor::OnPreConstructionForUI_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName)
{
	bIsConstructionEventRunning = true;

	if(ShouldExecuteTLLRN_ControlRig(InRig))
	{
		const TArrayView<const FTLLRN_RigElementKey> Elements;
		PreConstructionPose = InRig->GetHierarchy()->GetPose(false, ETLLRN_RigElementType::ToResetAfterConstructionEvent, Elements);

		if(const UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint())
		{
			if(RigBlueprint->IsTLLRN_ControlTLLRN_RigModule())
			{
				SocketStates = InRig->GetHierarchy()->GetSocketStates();
				ConnectorStates = RigBlueprint->Hierarchy->GetConnectorStates();
			}
		}
	}
}

void FTLLRN_ControlRigEditor::OnPreConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName)
{
	if(UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint())
	{
		if(RigBlueprint->IsTLLRN_ControlTLLRN_RigModule())
		{
			if(RigBlueprint->PreviewSkeletalMesh)
			{
				if(UTLLRN_RigHierarchy* Hierarchy = InRig->GetHierarchy())
				{
					if(UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController(true))
					{
						// find the instruction index for the construction event
						int32 InstructionIndex = INDEX_NONE;
						if(URigVM* VM = InRig->GetVM())
						{
							const int32 EntryIndex = VM->GetByteCode().FindEntryIndex(FTLLRN_RigUnit_PrepareForExecution::EventName);
							if(EntryIndex != INDEX_NONE)
							{
								InstructionIndex = VM->GetByteCode().GetEntry(EntryIndex).InstructionIndex;
							}
						}

						// import the bones for the preview hierarchy
						// use the ref skeleton so we'll only see the bones that are actually part of the mesh
						const TArray<FTLLRN_RigElementKey> Bones = Controller->ImportBones(RigBlueprint->PreviewSkeletalMesh->GetRefSkeleton(), NAME_None, false, false, false, false);
						for(const FTLLRN_RigElementKey& Bone : Bones)
						{
							if(FTLLRN_RigBaseElement* Element = Hierarchy->Find(Bone))
							{
								Element->CreatedAtInstructionIndex = InstructionIndex;
							}
						}

						// create a null to store controls under
						static const FTLLRN_RigElementKey ControlParentKey(TEXT("Controls"), ETLLRN_RigElementType::Null);
						if(!Hierarchy->Contains(ControlParentKey))
						{
							const FTLLRN_RigElementKey Null = Controller->AddNull(ControlParentKey.Name, FTLLRN_RigElementKey(), FTransform::Identity, true, false, false);
							if(FTLLRN_RigBaseElement* Element = Hierarchy->Find(Null))
							{
								Element->CreatedAtInstructionIndex = InstructionIndex;
							}
						}
					}
				}

				if(ShouldExecuteTLLRN_ControlRig(InRig))
				{
					RigBlueprint->Hierarchy->RestoreSocketsFromStates(SocketStates);
					InRig->GetHierarchy()->RestoreSocketsFromStates(SocketStates);
				}
			}
		}
	}
}

void FTLLRN_ControlRigEditor::OnPostConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName)
{
	bIsConstructionEventRunning = false;
	const bool bShouldExecute = ShouldExecuteTLLRN_ControlRig(InRig);

	if(UTLLRN_ControlRigBlueprint* RigBlueprint = GetTLLRN_ControlRigBlueprint())
	{
		if(bShouldExecute && RigBlueprint->IsTLLRN_ControlTLLRN_RigModule())
		{
			RigBlueprint->Hierarchy->RestoreConnectorsFromStates(ConnectorStates);
		}

		if(bShouldExecute && RigBlueprint->IsTLLRN_ModularRig())
		{
			// auto resolve the root module's primary connector
			if(RigBlueprint->TLLRN_ModularRigModel.Connections.IsEmpty() && RigBlueprint->TLLRN_ModularRigModel.Modules.Num() == 1 && RigBlueprint->Hierarchy->Num(ETLLRN_RigElementType::Bone) > 0)
			{
				const FTLLRN_RigModuleReference& RootModule = RigBlueprint->TLLRN_ModularRigModel.Modules[0];

				const FSoftObjectPath DefaultRootModulePath = UTLLRN_ControlRigSettings::Get()->DefaultRootModule;
				if(const UTLLRN_ControlRigBlueprint* DefaultRootModule = Cast<UTLLRN_ControlRigBlueprint>(DefaultRootModulePath.TryLoad()))
				{
					if(DefaultRootModule->GetTLLRN_ControlRigClass() == RootModule.Class)
					{
						if(const FTLLRN_RigConnectorElement* PrimaryConnector = RootModule.FindPrimaryConnector(RigBlueprint->Hierarchy))
						{
							if(const FTLLRN_RigBoneElement* RootBone = RigBlueprint->Hierarchy->GetBones()[0])
							{
								if(UTLLRN_ModularTLLRN_RigController* TLLRN_ModularTLLRN_RigController = RigBlueprint->TLLRN_ModularRigModel.GetController())
								{
									(void)TLLRN_ModularTLLRN_RigController->ConnectConnectorToElement(PrimaryConnector->GetKey(), RootBone->GetKey(), false);
								}
							}
						}
					}
				}
			}
		}
	}
	
	const int32 HierarchyHash = InRig->GetHierarchy()->GetTopologyHash(false);
	if(LastHierarchyHash != HierarchyHash)
	{
		LastHierarchyHash = HierarchyHash;
		
		auto Task = [this, InRig]()
		{
			CacheNameLists();
			SynchronizeViewportBoneSelection();
			RebindToSkeletalMeshComponent();
			if(DetailViewShowsAnyTLLRN_RigElement())
			{
				const TArray<FTLLRN_RigElementKey> Keys = GetSelectedTLLRN_RigElementsFromDetailView();
				SetDetailViewForTLLRN_RigElements(Keys);
			}
			
			if (FTLLRN_ControlRigEditorEditMode* EditMode = GetEditMode())
            {
				if (InRig)
            	{
            		EditMode->bDrawHierarchyBones = !InRig->GetHierarchy()->GetBones().IsEmpty();
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
	else if(bShouldExecute)
	{
		InRig->GetHierarchy()->SetPose(PreConstructionPose, ETLLRN_RigTransformType::CurrentGlobal);
	}
}

#undef LOCTEXT_NAMESPACE
