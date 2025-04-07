// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigEditorModule.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "PropertyEditorModule.h"
#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigComponent.h"
#include "GraphEditorActions.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ISequencerModule.h"
#include "IAssetTools.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "Editor/RigVMEditorStyle.h"
#include "Framework/Docking/LayoutExtender.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyle.h"
#include "Modules/ModuleManager.h"
#include "EditorModeRegistry.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditorModeManager.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigSectionDetailsCustomization.h"
#include "EditMode/ControlsProxyDetailCustomization.h"
#include "EditMode/TLLRN_ControlRigEditModeCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "ContentBrowserMenuContexts.h"
#include "Components/SkeletalMeshComponent.h"
#include "ContentBrowserModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/TLLRN_ControlRigEditor.h"
#include "TLLRN_ControlRigBlueprintActions.h"
#include "RigVMBlueprintGeneratedClass.h"
#include "TLLRN_ControlRigGizmoLibraryActions.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "EdGraphUtilities.h"
#include "Graph/TLLRN_ControlRigGraphPanelPinFactory.h"
#include <Editor/TLLRN_ControlRigEditorCommands.h>
#include "TLLRN_ControlTLLRN_RigHierarchyCommands.h"
#include "TLLRN_ControlRigTLLRN_ModularRigCommands.h"
#include "Animation/AnimSequence.h"
#include "Editor/TLLRN_ControlRigEditorEditMode.h"
#include "TLLRN_ControlTLLRN_RigElementDetails.h"
#include "TLLRN_ControlTLLRN_RigModuleDetails.h"
#include "TLLRN_ControlRigCompilerDetails.h"
#include "TLLRN_ControlRigDrawingDetails.h"
#include "TLLRN_ControlRigAnimGraphDetails.h"
#include "TLLRN_ControlRigBlueprintDetails.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Sequencer/TLLRN_ControlRigParameterTrackEditor.h"
#include "ActorFactories/ActorFactorySkeletalMesh.h"
#include "TLLRN_ControlRigThumbnailRenderer.h"
#include "RigVMModel/RigVMController.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "SKismetInspector.h"
#include "Dialogs/Dialogs.h"
#include "Settings/TLLRN_ControlRigSettings.h"
#include "EditMode/TLLRN_ControlTLLRN_RigControlsProxy.h"
#include "IPersonaToolkit.h"
#include "LevelSequence.h"
#include "AnimSequenceLevelSequenceLink.h"
#include "LevelSequenceActor.h"
#include "ISequencer.h"
#include "ILevelSequenceEditorToolkit.h"
#include "ClassViewerModule.h"
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "Animation/SkeletalMeshActor.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "TLLRN_ControlRigObjectBinding.h"
#include "LevelSequenceAnimSequenceLink.h"
#include "Rigs/FKTLLRN_ControlRig.h"
#include "SBakeToTLLRN_ControlRigDialog.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"
#include "Widgets/SRigVMGraphPinVariableBinding.h"
#include "TLLRN_ControlRigPythonLogDetails.h"
#include "Dialog/SCustomDialog.h"
#include "Sequencer/MovieSceneTLLRN_ControlTLLRN_RigSpaceChannel.h"
#include "SequencerChannelInterface.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ICurveEditorModule.h"
#include "Channels/SCurveEditorKeyBarView.h"
#include "TLLRN_ControlTLLRN_RigSpaceChannelCurveModel.h"
#include "TLLRN_ControlTLLRN_RigSpaceChannelEditors.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#include "UObject/FieldIterator.h"
#include "AnimationToolMenuContext.h"
#include "ControlConstraintChannelInterface.h"
#include "RigVMModel/Nodes/RigVMAggregateNode.h"
#include "RigVMUserWorkflowRegistry.h"
#include "Units/TLLRN_ControlRigNodeWorkflow.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"
#include "Constraints/TLLRN_ControlTLLRN_RigTransformableHandle.h"
#include "Constraints/TransformConstraintChannelInterface.h"
#include "Async/TaskGraphInterfaces.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "SequencerUtilities.h"
#include "Bindings/MovieSceneSpawnableActorBinding.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigEditorModule"

DEFINE_LOG_CATEGORY(LogTLLRN_ControlRigEditor);

void FTLLRN_ControlRigEditorModule::StartupModule()
{
	FTLLRN_ControlRigEditModeCommands::Register();
	FTLLRN_ControlRigEditorCommands::Register();
	FTLLRN_ControlTLLRN_RigHierarchyCommands::Register();
	FTLLRN_ControlRigTLLRN_ModularRigCommands::Register();
	FTLLRN_ControlRigEditorStyle::Get();

	EdGraphPanelPinFactory = MakeShared<FTLLRN_ControlRigGraphPanelPinFactory>();
	FEdGraphUtilities::RegisterVisualPinFactory(EdGraphPanelPinFactory);

	StartupModuleCommon();

	// Register details customizations for animation controller nodes
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	ClassesToUnregisterOnShutdown.Reset();

	ClassesToUnregisterOnShutdown.Add(UMovieSceneTLLRN_ControlRigParameterSection::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(ClassesToUnregisterOnShutdown.Last(), FOnGetDetailCustomizationInstance::CreateStatic(&FMovieSceneTLLRN_ControlRigSectionDetailsCustomization::MakeInstance));

	ClassesToUnregisterOnShutdown.Add(UTLLRN_ControlRigBlueprint::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(ClassesToUnregisterOnShutdown.Last(), FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigBlueprintDetails::MakeInstance));

	ClassesToUnregisterOnShutdown.Add(UTLLRN_ControlRig::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(ClassesToUnregisterOnShutdown.Last(), FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_RigModuleInstanceDetails::MakeInstance));

	ClassesToUnregisterOnShutdown.Add(UTLLRN_ControlRig::StaticClass()->GetFName());

	// proxies
	const FName PropertyEditorModuleName("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditorModuleName);

	ClassesToUnregisterOnShutdown.Add(UTLLRN_AnimDetailControlsProxyFloat::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_AnimDetailControlsProxyFloat::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateLambda([]() {
		FName CategoryName = FName(TEXT("Float"));
		return FTLLRN_AnimDetailProxyDetails::MakeInstance(CategoryName);
		}));

	{	
		TSharedRef<FPropertySection> Section = RegisterPropertySection(PropertyModule, "TLLRN_AnimDetailControlsProxyFloat", "General", LOCTEXT("General", "General"));
		Section->AddCategory("Attributes");
	}

	ClassesToUnregisterOnShutdown.Add(UTLLRN_AnimDetailControlsProxyInteger::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_AnimDetailControlsProxyInteger::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateLambda([]() {
		FName CategoryName = FName(TEXT("Integer"));
		return FTLLRN_AnimDetailProxyDetails::MakeInstance(CategoryName);
		}));

	{
		TSharedRef<FPropertySection> Section = RegisterPropertySection(PropertyModule, "TLLRN_AnimDetailControlsProxyInteger", "General", LOCTEXT("General", "General"));
		Section->AddCategory("Attributes");
	}

	ClassesToUnregisterOnShutdown.Add(UTLLRN_AnimDetailControlsProxyBool::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_AnimDetailControlsProxyBool::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateLambda([]() {
		FName CategoryName = FName(TEXT("Bool"));
		return FTLLRN_AnimDetailProxyDetails::MakeInstance(CategoryName);
		}));

	{
		TSharedRef<FPropertySection> Section = RegisterPropertySection(PropertyModule, "TLLRN_AnimDetailControlsProxyBool", "General", LOCTEXT("General", "General"));
		Section->AddCategory("Attributes");
	}

	ClassesToUnregisterOnShutdown.Add(UTLLRN_AnimDetailControlsProxyTransform::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_AnimDetailControlsProxyTransform::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateLambda([]() {
		FName CategoryName = FName(TEXT("Transform"));
		return FTLLRN_AnimDetailProxyDetails::MakeInstance(CategoryName);
		}));

	{
		TSharedRef<FPropertySection> Section = RegisterPropertySection(PropertyModule, "TLLRN_AnimDetailControlsProxyTransform", "General", LOCTEXT("General", "General"));
		Section->AddCategory("Attributes");
	}

	ClassesToUnregisterOnShutdown.Add(UTLLRN_AnimDetailControlsProxyLocation::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_AnimDetailControlsProxyLocation::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateLambda([]() {
		FName CategoryName = FName(TEXT("Location"));
		return FTLLRN_AnimDetailProxyDetails::MakeInstance(CategoryName);
		}));

	{
		TSharedRef<FPropertySection> Section = RegisterPropertySection(PropertyModule, "TLLRN_AnimDetailControlsProxyLocation", "General", LOCTEXT("General", "General"));
		Section->AddCategory("Attributes");
	}

	ClassesToUnregisterOnShutdown.Add(UTLLRN_AnimDetailControlsProxyRotation::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_AnimDetailControlsProxyRotation::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateLambda([]() {
		FName CategoryName = FName(TEXT("Rotation"));
		return FTLLRN_AnimDetailProxyDetails::MakeInstance(CategoryName);
		}));

	{
		TSharedRef<FPropertySection> Section = RegisterPropertySection(PropertyModule, "TLLRN_AnimDetailControlsProxyRotation", "General", LOCTEXT("General", "General"));
		Section->AddCategory("Attributes");
	}

	ClassesToUnregisterOnShutdown.Add(UTLLRN_AnimDetailControlsProxyScale::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_AnimDetailControlsProxyScale::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateLambda([]() {
		FName CategoryName = FName(TEXT("Scale"));
		return FTLLRN_AnimDetailProxyDetails::MakeInstance(CategoryName);
		}));
	{
		TSharedRef<FPropertySection> Section = RegisterPropertySection(PropertyModule, "TLLRN_AnimDetailControlsProxyScale", "General", LOCTEXT("General", "General"));
		Section->AddCategory("Attributes");
	}

	ClassesToUnregisterOnShutdown.Add(UTLLRN_AnimDetailControlsProxyVector2D::StaticClass()->GetFName());
	PropertyEditorModule.RegisterCustomClassLayout(UTLLRN_AnimDetailControlsProxyVector2D::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateLambda([]() {
		FName CategoryName = FName(TEXT("Vector2D"));
		return FTLLRN_AnimDetailProxyDetails::MakeInstance(CategoryName);
		}));

	{
		TSharedRef<FPropertySection> Section = RegisterPropertySection(PropertyModule, "TLLRN_AnimDetailControlsProxyVector2D", "General", LOCTEXT("General", "General"));
		Section->AddCategory("Attributes");
	}
	
	// same as ClassesToUnregisterOnShutdown but for properties, there is none right now
	PropertiesToUnregisterOnShutdown.Reset();
	PropertiesToUnregisterOnShutdown.Add(FRigVMCompileSettings::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FRigVMCompileSettingsDetails::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_AnimDetailProxyFloat::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_AnimDetailValueCustomization::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_AnimDetailProxyInteger::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_AnimDetailValueCustomization::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_AnimDetailProxyBool::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_AnimDetailValueCustomization::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_AnimDetailProxyVector2D::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_AnimDetailValueCustomization::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_AnimDetailProxyLocation::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_AnimDetailValueCustomization::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_AnimDetailProxyRotation::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_AnimDetailValueCustomization::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_AnimDetailProxyScale::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_AnimDetailValueCustomization::MakeInstance));
	
	PropertiesToUnregisterOnShutdown.Add(FRigVMPythonSettings::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_ControlRigPythonLogDetails::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FRigVMDrawContainer::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_ControlRigDrawContainerDetails::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_ControlRigEnumControlProxyValue::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEnumControlProxyValueDetails::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_RigElementKey::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_RigElementKeyDetails::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_RigComputedTransform::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_RigComputedTransformDetails::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_ControlRigAnimNodeEventName::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_ControlRigAnimNodeEventNameDetails::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(StaticEnum<ETLLRN_RigControlTransformChannel>()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_RigControlTransformChannelDetails::MakeInstance));

	PropertiesToUnregisterOnShutdown.Add(FTLLRN_RigConnectionRuleStash::StaticStruct()->GetFName());
	PropertyEditorModule.RegisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown.Last(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTLLRN_RigConnectionRuleDetails::MakeInstance));

	FTLLRN_RigBaseElementDetails::RegisterSectionMappings(PropertyEditorModule);

	// Register asset tools
	auto RegisterAssetTypeAction = [this](const TSharedRef<IAssetTypeActions>& InAssetTypeAction)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisteredAssetTypeActions.Add(InAssetTypeAction);
		AssetTools.RegisterAssetTypeActions(InAssetTypeAction);
	};

	RegisterAssetTypeAction(MakeShareable(new FTLLRN_ControlRigBlueprintActions()));
	RegisterAssetTypeAction(MakeShareable(new FTLLRN_ControlRigShapeLibraryActions()));
	
	// Register sequencer track editor
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerModule.RegisterChannelInterface<FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel>();
	TLLRN_ControlRigParameterTrackCreateEditorHandle = SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FTLLRN_ControlRigParameterTrackEditor::CreateTrackEditor));

	// register UTLLRN_TransformableControlHandle animatable interface
	FConstraintChannelInterfaceRegistry& ConstraintChannelInterfaceRegistry = FConstraintChannelInterfaceRegistry::Get();
	ConstraintChannelInterfaceRegistry.RegisterConstraintChannelInterface<UTLLRN_TransformableControlHandle>(MakeUnique<FControlConstraintChannelInterface>());
	
	AddTLLRN_ControlRigExtenderToToolMenu("AssetEditor.AnimationEditor.ToolBar");

	FEditorModeRegistry::Get().RegisterMode<FTLLRN_ControlRigEditMode>(
		FTLLRN_ControlRigEditMode::ModeName,
		NSLOCTEXT("AnimationModeToolkit", "DisplayName", "Animation"),
		FSlateIcon(FRigVMEditorStyle::Get().GetStyleSetName(), "RigVMEditMode", "RigVMEditMode.Small"),
		true,
		8000);

	FEditorModeRegistry::Get().RegisterMode<FTLLRN_ControlRigEditorEditMode>(
		FTLLRN_ControlRigEditorEditMode::ModeName,
		NSLOCTEXT("RiggingModeToolkit", "DisplayName", "Rigging"),
		FSlateIcon(FRigVMEditorStyle::Get().GetStyleSetName(), "RigVMEditMode", "RigVMEditMode.Small"),
		false,
		8500);

	FEditorModeRegistry::Get().RegisterMode<FTLLRN_ModularRigEditorEditMode>(
		FTLLRN_ModularRigEditorEditMode::ModeName,
		NSLOCTEXT("RiggingModeToolkit", "DisplayName", "Rigging"),
		FSlateIcon(FRigVMEditorStyle::Get().GetStyleSetName(), "RigVMEditMode", "RigVMEditMode.Small"),
		false,
		9000);

	ICurveEditorModule& CurveEditorModule = FModuleManager::LoadModuleChecked<ICurveEditorModule>("CurveEditor");
	FTLLRN_ControlTLLRN_RigSpaceChannelCurveModel::ViewID = CurveEditorModule.RegisterView(FOnCreateCurveEditorView::CreateStatic(
		[](TWeakPtr<FCurveEditor> WeakCurveEditor) -> TSharedRef<SCurveEditorView>
		{
			return SNew(SCurveEditorKeyBarView, WeakCurveEditor);
		}
	));

	FTLLRN_ControlRigBlueprintActions::ExtendSketalMeshToolMenu();
	ExtendAnimSequenceMenu();

	UActorFactorySkeletalMesh::RegisterDelegatesForAssetClass(
		UTLLRN_ControlRigBlueprint::StaticClass(),
		FGetSkeletalMeshFromAssetDelegate::CreateStatic(&FTLLRN_ControlRigBlueprintActions::GetSkeletalMeshFromTLLRN_ControlRigBlueprint),
		FPostSkeletalMeshActorSpawnedDelegate::CreateStatic(&FTLLRN_ControlRigBlueprintActions::PostSpawningSkeletalMeshActor)
	);

	UThumbnailManager::Get().RegisterCustomRenderer(UTLLRN_ControlRigBlueprint::StaticClass(), UTLLRN_ControlRigThumbnailRenderer::StaticClass());
	//UThumbnailManager::Get().RegisterCustomRenderer(UTLLRN_ControlTLLRN_RigPoseAsset::StaticClass(), UTLLRN_ControlTLLRN_RigPoseThumbnailRenderer::StaticClass());

	bFilterAssetBySkeleton = true;

	URigVMUserWorkflowRegistry* WorkflowRegistry = URigVMUserWorkflowRegistry::Get();

	// register the workflow provider for ANY node
	FRigVMUserWorkflowProvider Provider;
	Provider.BindUFunction(UTLLRN_ControlTLLRN_RigTransformWorkflowOptions::StaticClass()->GetDefaultObject(), TEXT("ProvideWorkflows"));
	WorkflowHandles.Add(WorkflowRegistry->RegisterProvider(nullptr, Provider));
}

void FTLLRN_ControlRigEditorModule::ShutdownModule()
{
	if (ICurveEditorModule* CurveEditorModule = FModuleManager::GetModulePtr<ICurveEditorModule>("CurveEditor"))
	{
		CurveEditorModule->UnregisterView(FTLLRN_ControlTLLRN_RigSpaceChannelCurveModel::ViewID);
	}

	ShutdownModuleCommon();

	//UThumbnailManager::Get().UnregisterCustomRenderer(UTLLRN_ControlRigBlueprint::StaticClass());
	//UActorFactorySkeletalMesh::UnregisterDelegatesForAssetClass(UTLLRN_ControlRigBlueprint::StaticClass());

	FEditorModeRegistry::Get().UnregisterMode(FTLLRN_ModularRigEditorEditMode::ModeName);
	FEditorModeRegistry::Get().UnregisterMode(FTLLRN_ControlRigEditorEditMode::ModeName);
	FEditorModeRegistry::Get().UnregisterMode(FTLLRN_ControlRigEditMode::ModeName);

	FEdGraphUtilities::UnregisterVisualPinFactory(EdGraphPanelPinFactory);

	ISequencerModule* SequencerModule = FModuleManager::GetModulePtr<ISequencerModule>("Sequencer");
	if (SequencerModule)
	{
		SequencerModule->UnRegisterTrackEditor(TLLRN_ControlRigParameterTrackCreateEditorHandle);
	}

	FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");
	if (AssetToolsModule)
	{
		for (TSharedRef<IAssetTypeActions> RegisteredAssetTypeAction : RegisteredAssetTypeActions)
		{
			AssetToolsModule->Get().UnregisterAssetTypeActions(RegisteredAssetTypeAction);
		}
	}

	FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");
	if (PropertyEditorModule)
	{
		for (int32 Index = 0; Index < ClassesToUnregisterOnShutdown.Num(); ++Index)
		{
			PropertyEditorModule->UnregisterCustomClassLayout(ClassesToUnregisterOnShutdown[Index]);
		}

		for (int32 Index = 0; Index < PropertiesToUnregisterOnShutdown.Num(); ++Index)
		{
			PropertyEditorModule->UnregisterCustomPropertyTypeLayout(PropertiesToUnregisterOnShutdown[Index]);
		}
	}

	if (UObjectInitialized())
	{
		for(const int32 WorkflowHandle : WorkflowHandles)
		{
			if(URigVMUserWorkflowRegistry::StaticClass()->GetDefaultObject(false) != nullptr)
			{
				URigVMUserWorkflowRegistry::Get()->UnregisterProvider(WorkflowHandle);
			}
		}
	}
	WorkflowHandles.Reset();

	UnregisterPropertySectionMappings();
}

UClass* FTLLRN_ControlRigEditorModule::GetRigVMBlueprintClass() const
{
	return UTLLRN_ControlRigBlueprint::StaticClass();
}

TSharedRef<FPropertySection> FTLLRN_ControlRigEditorModule::RegisterPropertySection(FPropertyEditorModule& PropertyModule, FName ClassName, FName SectionName, FText DisplayName)
{
	TSharedRef<FPropertySection> PropertySection = PropertyModule.FindOrCreateSection(ClassName, SectionName, DisplayName);
	RegisteredPropertySections.Add(ClassName, SectionName);

	return PropertySection;
}

void FTLLRN_ControlRigEditorModule::UnregisterPropertySectionMappings()
{
	const FName PropertyEditorModuleName("PropertyEditor");
	FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>(PropertyEditorModuleName);

	if (!PropertyModule)
	{
		return;
	}

	for (TMultiMap<FName, FName>::TIterator PropertySectionIterator = RegisteredPropertySections.CreateIterator(); PropertySectionIterator; ++PropertySectionIterator)
	{
		PropertyModule->RemoveSection(PropertySectionIterator->Key, PropertySectionIterator->Value);
		PropertySectionIterator.RemoveCurrent();
	}

	RegisteredPropertySections.Empty();
}

void FTLLRN_ControlRigEditorModule::GetNodeContextMenuActions(IRigVMClientHost* RigVMClientHost,
	const URigVMEdGraphNode* EdGraphNode, URigVMNode* ModelNode, UToolMenu* Menu) const
{
	FRigVMEditorModule::GetNodeContextMenuActions(RigVMClientHost, EdGraphNode, ModelNode, Menu);

	UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(RigVMClientHost);
	if(TLLRN_ControlRigBlueprint == nullptr)
	{
		return;
	}

	URigVMGraph* Model = RigVMClientHost->GetRigVMClient()->GetModel(EdGraphNode->GetGraph());
	URigVMController* Controller = RigVMClientHost->GetRigVMClient()->GetController(Model);

	TArray<FName> SelectedNodeNames = Model->GetSelectNodes();
	SelectedNodeNames.AddUnique(ModelNode->GetFName());

	UTLLRN_RigHierarchy* TemporaryHierarchy = NewObject<UTLLRN_RigHierarchy>();
	TemporaryHierarchy->CopyHierarchy(TLLRN_ControlRigBlueprint->Hierarchy);

	TArray<FTLLRN_RigElementKey> TLLRN_RigElementsToSelect;
	TMap<const URigVMPin*, FTLLRN_RigElementKey> PinToKey;

	for(const FName& SelectedNodeName : SelectedNodeNames)
	{
		if (URigVMNode* FoundNode = Model->FindNodeByName(SelectedNodeName))
		{
			TSharedPtr<FStructOnScope> StructOnScope;
			FRigVMStruct* StructMemory = nullptr;
			UScriptStruct* ScriptStruct = nullptr;
			if (URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(FoundNode))
			{
				ScriptStruct = UnitNode->GetScriptStruct();
				if(ScriptStruct)
				{
					StructOnScope = UnitNode->ConstructStructInstance(false);
					if(StructOnScope->GetStruct()->IsChildOf(FRigVMStruct::StaticStruct()))
					{
						StructMemory = (FRigVMStruct*)StructOnScope->GetStructMemory();
						StructMemory->Execute();
					}
				}
			}

			const TArray<URigVMPin*> AllPins = FoundNode->GetAllPinsRecursively();
			for (const URigVMPin* Pin : AllPins)
			{
				if (Pin->GetCPPType() == TEXT("FName"))
				{
					FTLLRN_RigElementKey Key;
					if (Pin->GetCustomWidgetName() == TEXT("BoneName"))
					{
						Key = FTLLRN_RigElementKey(*Pin->GetDefaultValue(), ETLLRN_RigElementType::Bone);
					}
					else if (Pin->GetCustomWidgetName() == TEXT("ControlName"))
					{
						Key = FTLLRN_RigElementKey(*Pin->GetDefaultValue(), ETLLRN_RigElementType::Control);
					}
					else if (Pin->GetCustomWidgetName() == TEXT("SpaceName"))
					{
						Key = FTLLRN_RigElementKey(*Pin->GetDefaultValue(), ETLLRN_RigElementType::Null);
					}
					else if (Pin->GetCustomWidgetName() == TEXT("CurveName"))
					{
						Key = FTLLRN_RigElementKey(*Pin->GetDefaultValue(), ETLLRN_RigElementType::Curve);
					}
					else
					{
						continue;
					}

					TLLRN_RigElementsToSelect.AddUnique(Key);
					PinToKey.Add(Pin, Key);
				}
				else if (Pin->GetCPPTypeObject() == FTLLRN_RigElementKey::StaticStruct() && !Pin->IsArray())
				{
					if (StructMemory == nullptr)
					{
						FString DefaultValue = Pin->GetDefaultValue();
						if (!DefaultValue.IsEmpty())
						{
							FTLLRN_RigElementKey Key;
							FTLLRN_RigElementKey::StaticStruct()->ImportText(*DefaultValue, &Key, nullptr, EPropertyPortFlags::PPF_None, nullptr, FTLLRN_RigElementKey::StaticStruct()->GetName(), true);
							if (Key.IsValid())
							{
								TLLRN_RigElementsToSelect.AddUnique(Key);
								if (URigVMPin* NamePin = Pin->FindSubPin(TEXT("Name")))
								{
									PinToKey.Add(NamePin, Key);
								}
							}
						}
					}
					else
					{
						check(ScriptStruct);

						TArray<FString> PropertyNames; 
						if(!URigVMPin::SplitPinPath(Pin->GetSegmentPath(true), PropertyNames))
						{
							PropertyNames.Add(Pin->GetName());
						}

						UScriptStruct* Struct = ScriptStruct;
						uint8* Memory = (uint8*)StructMemory; 

						while(!PropertyNames.IsEmpty())
						{
							FString PropertyName;
							PropertyNames.HeapPop(PropertyName);
							
							const FProperty* Property = ScriptStruct->FindPropertyByName(*PropertyName);
							if(Property == nullptr)
							{
								Memory = nullptr;
								break;
							}

							Memory = Property->ContainerPtrToValuePtr<uint8>(Memory);
							
							if(PropertyNames.IsEmpty())
							{
								continue;
							}
							
							if(const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
							{
								PropertyNames.HeapPop(PropertyName);

								int32 ArrayIndex = FCString::Atoi(*PropertyName);
								FScriptArrayHelper Helper(ArrayProperty, Memory);
								if(!Helper.IsValidIndex(ArrayIndex))
								{
									Memory = nullptr;
									break;
								}

								Memory = Helper.GetRawPtr(ArrayIndex);
								Property = ArrayProperty->Inner;
							}

							if(const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
							{
								Struct = StructProperty->Struct;
							}
						}

						if(Memory)
						{
							const FTLLRN_RigElementKey& Key = *(const FTLLRN_RigElementKey*)Memory;
							if (Key.IsValid())
							{
								TLLRN_RigElementsToSelect.AddUnique(Key);

								if (URigVMPin* NamePin = Pin->FindSubPin(TEXT("Name")))
								{
									PinToKey.Add(NamePin, Key);
								}
							}
						}
					}
				}
				else if (Pin->GetCPPTypeObject() == FTLLRN_RigElementKeyCollection::StaticStruct() && Pin->GetDirection() == ERigVMPinDirection::Output)
				{
					if (StructMemory == nullptr)
					{
						// not supported for now
					}
					else
					{
						check(ScriptStruct);
						if (const FProperty* Property = ScriptStruct->FindPropertyByName(Pin->GetFName()))
						{
							const FTLLRN_RigElementKeyCollection& Collection = *Property->ContainerPtrToValuePtr<FTLLRN_RigElementKeyCollection>(StructMemory);

							if (Collection.Num() > 0)
							{
								TLLRN_RigElementsToSelect.Reset();
								for (const FTLLRN_RigElementKey& Item : Collection)
								{
									TLLRN_RigElementsToSelect.AddUnique(Item);
								}
								break;
							}
						}
					}
				}
			}
		}
	}

	GetDirectManipulationMenuActions(RigVMClientHost, ModelNode, nullptr, Menu);

	if (TLLRN_RigElementsToSelect.Num() > 0)
	{
		FToolMenuSection& Section = Menu->AddSection("RigVMEditorContextMenuHierarchy", LOCTEXT("HierarchyHeader", "Hierarchy"));
		Section.AddMenuEntry(
			"SelectTLLRN_RigElements",
			LOCTEXT("SelectTLLRN_RigElements", "Select Rig Elements"),
			LOCTEXT("SelectTLLRN_RigElements_Tooltip", "Selects the bone, controls or nulls associated with this node."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([TLLRN_ControlRigBlueprint, TLLRN_RigElementsToSelect]() {

				TLLRN_ControlRigBlueprint->GetHierarchyController()->SetSelection(TLLRN_RigElementsToSelect);
				
			})
		));
	}

	if (TLLRN_RigElementsToSelect.Num() > 0)
	{
		FToolMenuSection& Section = Menu->AddSection("RigVMEditorContextMenuHierarchy", LOCTEXT("ToolsHeader", "Tools"));
		Section.AddMenuEntry(
			"SearchAndReplaceNames",
			LOCTEXT("SearchAndReplaceNames", "Search & Replace / Mirror"),
			LOCTEXT("SearchAndReplaceNames_Tooltip", "Searches within all names and replaces with a different text."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([TLLRN_ControlRigBlueprint, Controller, PinToKey]() {

				FRigVMMirrorSettings Settings;
				TSharedPtr<FStructOnScope> StructToDisplay = MakeShareable(new FStructOnScope(FRigVMMirrorSettings::StaticStruct(), (uint8*)&Settings));

				TSharedRef<SKismetInspector> KismetInspector = SNew(SKismetInspector);
				KismetInspector->ShowSingleStruct(StructToDisplay);

				TSharedRef<SCustomDialog> MirrorDialog = SNew(SCustomDialog)
					.Title(FText(LOCTEXT("TLLRN_ControlTLLRN_RigHierarchyMirror", "Mirror Graph")))
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
					Controller->OpenUndoBracket(TEXT("Mirroring Graph"));
					int32 ReplacedNames = 0;
					TArray<FString> UnchangedItems;

					for (const TPair<const URigVMPin*, FTLLRN_RigElementKey>& Pair : PinToKey)
					{
						const URigVMPin* Pin = Pair.Key;
						FTLLRN_RigElementKey Key = Pair.Value;

						if (Key.Name.IsNone())
						{
							continue;
						}

						FString OldNameStr = Key.Name.ToString();
						FString NewNameStr = OldNameStr.Replace(*Settings.SearchString, *Settings.ReplaceString, ESearchCase::CaseSensitive);
						if(NewNameStr != OldNameStr)
						{
							Key.Name = *NewNameStr;
							if(TLLRN_ControlRigBlueprint->Hierarchy->GetIndex(Key) != INDEX_NONE)
							{
								Controller->SetPinDefaultValue(Pin->GetPinPath(), NewNameStr, false, true, false, true);
								ReplacedNames++;
							}
							else
							{
								// save the names of the items that we skipped during this search & replace
								UnchangedItems.AddUnique(OldNameStr);
							} 
						}
					}

					if (UnchangedItems.Num() > 0)
					{
						FString ListOfUnchangedItems;
						for (int Index = 0; Index < UnchangedItems.Num(); Index++)
						{
							// construct the string "item1, item2, item3"
							ListOfUnchangedItems += UnchangedItems[Index];
							if (Index != UnchangedItems.Num() - 1)
							{
								ListOfUnchangedItems += TEXT(", ");
							}
						}
						
						// inform the user that some items were skipped due to invalid new names
						Controller->ReportAndNotifyError(FString::Printf(TEXT("Invalid Names after Search & Replace, action skipped for %s"), *ListOfUnchangedItems));
					}

					if (ReplacedNames > 0)
					{ 
						Controller->CloseUndoBracket();
					}
					else
					{
						Controller->CancelUndoBracket();
					}
				}
			})
		));
	}

	if (URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(EdGraphNode->GetModelNode()))
	{
		FToolMenuSection& SettingsSection = Menu->AddSection("RigVMEditorContextMenuSettings", LOCTEXT("SettingsHeader", "Settings"));
		SettingsSection.AddMenuEntry(
			"Save Default Expansion State",
			LOCTEXT("SaveDefaultExpansionState", "Save Default Expansion State"),
			LOCTEXT("SaveDefaultExpansionState_Tooltip", "Saves the expansion state of all pins of the node as the default."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([UnitNode]() {

#if WITH_EDITORONLY_DATA

				FScopedTransaction Transaction(LOCTEXT("TLLRN_RigUnitDefaultExpansionStateChanged", "Changed Rig Unit Default Expansion State"));
				UTLLRN_ControlRigEditorSettings::Get()->Modify();

				FTLLRN_ControlRigSettingsPerPinBool& ExpansionMap = UTLLRN_ControlRigEditorSettings::Get()->TLLRN_RigUnitPinExpansion.FindOrAdd(UnitNode->GetScriptStruct()->GetName());
				ExpansionMap.Values.Empty();

				TArray<URigVMPin*> Pins = UnitNode->GetAllPinsRecursively();
				for (URigVMPin* Pin : Pins)
				{
					if (Pin->GetSubPins().Num() == 0)
					{
						continue;
					}

					FString PinPath = Pin->GetPinPath();
					FString NodeName, RemainingPath;
					URigVMPin::SplitPinPathAtStart(PinPath, NodeName, RemainingPath);
					ExpansionMap.Values.FindOrAdd(RemainingPath) = Pin->IsExpanded();
				}
#endif
			})
		));
	}
}

void FTLLRN_ControlRigEditorModule::GetPinContextMenuActions(IRigVMClientHost* RigVMClientHost, const UEdGraphPin* EdGraphPin, URigVMPin* ModelPin, UToolMenu* Menu) const
{
	FRigVMEditorModule::GetPinContextMenuActions(RigVMClientHost, EdGraphPin, ModelPin, Menu);
	GetDirectManipulationMenuActions(RigVMClientHost, ModelPin->GetNode(), ModelPin, Menu);
}

bool FTLLRN_ControlRigEditorModule::AssetsPublicFunctionsAllowed(const FAssetData& InAssetData) const
{
	// Looking for public functions in cooked assets only happens in UEFN
	// Make sure we allow only TLLRN_ControlRig/TLLRN_ControlRigSpline/TLLRN_ControlTLLRN_RigModules functions
	// (to avoid adding actions for internal rigs public functions)
	const FString AssetClassPath = InAssetData.AssetClassPath.ToString();
	if (AssetClassPath.Contains(TEXT("TLLRN_ControlRigBlueprintGeneratedClass"))
		|| AssetClassPath.Contains(TEXT("RigVMBlueprintGeneratedClass")))
	{
		const FString PathString = InAssetData.PackagePath.ToString();
		if (!PathString.StartsWith(TEXT("/TLLRN_ControlRig/"))
			&& !PathString.StartsWith(TEXT("/TLLRN_ControlRigSpline/"))
			&& !PathString.StartsWith(TEXT("/TLLRN_ControlTLLRN_RigModules/")))
		{
			return false;
		}
	}
	
	return ITLLRN_ControlRigEditorModule::AssetsPublicFunctionsAllowed(InAssetData);
}

void FTLLRN_ControlRigEditorModule::GetDirectManipulationMenuActions(IRigVMClientHost* RigVMClientHost, URigVMNode* InNode, URigVMPin* ModelPin, UToolMenu* Menu) const
{
    // Add direct manipulation context menu entries
	if(UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = Cast<UTLLRN_ControlRigBlueprint>(RigVMClientHost))
	{
		UTLLRN_ControlRig* DebuggedRig = Cast<UTLLRN_ControlRig>(TLLRN_ControlRigBlueprint->GetObjectBeingDebugged());
		if(DebuggedRig == nullptr)
		{
			return;
		}
		
		if(const URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(InNode))
		{
			if(!UnitNode->IsPartOfRuntime(DebuggedRig))
			{
				return;
			}
			
			const UScriptStruct* ScriptStruct = UnitNode->GetScriptStruct();
			if(ScriptStruct == nullptr)
			{
				return;
			}

			TSharedPtr<FStructOnScope> NodeInstance = UnitNode->ConstructStructInstance(false);
			if(!NodeInstance.IsValid() || !NodeInstance->IsValid())
			{
				return;
			}

			const FTLLRN_RigUnit* UnitInstance = UTLLRN_ControlRig::GetTLLRN_RigUnitInstanceFromScope(NodeInstance);
			TArray<FRigDirectManipulationTarget> Targets;
			if(UnitInstance->GetDirectManipulationTargets(UnitNode, NodeInstance, DebuggedRig->GetHierarchy(), Targets, nullptr))
			{
				if(ModelPin)
				{
					Targets.RemoveAll([ModelPin, UnitNode, UnitInstance](const FRigDirectManipulationTarget& Target) -> bool
					{
						const TArray<const URigVMPin*> AffectedPins = UnitInstance->GetPinsForDirectManipulation(UnitNode, Target);
						return !AffectedPins.Contains(ModelPin);
					});
				}

				if(!Targets.IsEmpty())
				{
					FToolMenuSection& Section = Menu->AddSection("RigVMEditorContextMenuControlNode", LOCTEXT("ControlNodeDirectManipulation", "Direct Manipulation"));

					bool bHasPosition = false;
					bool bHasRotation = false;
					bool bHasScale = false;
					
					for(const FRigDirectManipulationTarget& Target : Targets)
					{
						auto IsSliced = [UnitNode]() -> bool
						{
							return UnitNode->IsWithinLoop();
						};
						
						auto HasNoUnconstrainedAffectedPin = [NodeInstance, UnitNode, Target]() -> bool
						{
							const FTLLRN_RigUnit* UnitInstance = UTLLRN_ControlRig::GetTLLRN_RigUnitInstanceFromScope(NodeInstance);
							const TArray<const URigVMPin*> AffectedPins = UnitInstance->GetPinsForDirectManipulation(UnitNode, Target);

							int32 NumAffectedPinsWithRootLinks = 0;
							for(const URigVMPin* AffectedPin : AffectedPins)
							{
								if(!AffectedPin->GetRootPin()->GetSourceLinks().IsEmpty())
								{
									NumAffectedPinsWithRootLinks++;
								}
							}
							return NumAffectedPinsWithRootLinks == AffectedPins.Num();
						};

						FText Suffix;
						static const FText SuffixPosition = LOCTEXT("DirectManipulationPosition", " (W)"); 
						static const FText SuffixRotation = LOCTEXT("DirectManipulationRotation", " (E)"); 
						static const FText SuffixScale = LOCTEXT("DirectManipulationScale", " (R)"); 
						TSharedPtr< const FUICommandInfo > CommandInfo;
						if(!CommandInfo.IsValid() && !bHasPosition)
						{
							if(Target.ControlType == ETLLRN_RigControlType::EulerTransform || Target.ControlType == ETLLRN_RigControlType::Position)
							{
								CommandInfo = FTLLRN_ControlRigEditorCommands::Get().RequestDirectManipulationPosition;
								Suffix = SuffixPosition;
								bHasPosition = true;
							}
						}
						if(!CommandInfo.IsValid() && !bHasRotation)
						{
							if(Target.ControlType == ETLLRN_RigControlType::EulerTransform || Target.ControlType == ETLLRN_RigControlType::Rotator)
							{
								CommandInfo = FTLLRN_ControlRigEditorCommands::Get().RequestDirectManipulationRotation;
								Suffix = SuffixRotation;
								bHasRotation = true;
							}
						}
						if(!CommandInfo.IsValid() && !bHasScale)
						{
							if(Target.ControlType == ETLLRN_RigControlType::EulerTransform)
							{
								CommandInfo = FTLLRN_ControlRigEditorCommands::Get().RequestDirectManipulationScale;
								Suffix = SuffixScale;
								bHasScale = true;
							}
						}

						const FText Label = FText::Format(LOCTEXT("ControlNodeLabelFormat", "Manipulate {0}{1}"), FText::FromString(Target.Name), Suffix);
						TAttribute<FText> ToolTipAttribute = TAttribute<FText>::CreateLambda([HasNoUnconstrainedAffectedPin, IsSliced, Target]() -> FText
						{
							if(HasNoUnconstrainedAffectedPin())
							{
								return FText::Format(LOCTEXT("ControlNodeLabelFormat_Tooltip_FullyConstrained", "The value of {0} cannot be manipulated, its pins have links fully constraining it."), FText::FromString(Target.Name));
							}
							if(IsSliced())
							{
								return FText::Format(LOCTEXT("ControlNodeLabelFormat_Tooltip_Sliced", "The value of {0} cannot be manipulated, the node is linked to a loop."), FText::FromString(Target.Name));
							}
							return FText::Format(LOCTEXT("ControlNodeLabelFormat_Tooltip", "Manipulate the value of {0} interactively"), FText::FromString(Target.Name));
						});

						FToolMenuEntry& MenuEntry = Section.AddMenuEntry(
							*Target.Name,
							Label,
							ToolTipAttribute,
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([TLLRN_ControlRigBlueprint, UnitNode, Target]() {

								// disable literal folding for the moment
								if(TLLRN_ControlRigBlueprint->VMCompileSettings.ASTSettings.bFoldLiterals)
								{
									TLLRN_ControlRigBlueprint->VMCompileSettings.ASTSettings.bFoldLiterals = false;
									TLLRN_ControlRigBlueprint->RecompileVM();
								}

								// run the task after a bit so that the rig has the opportunity to run first
								FFunctionGraphTask::CreateAndDispatchWhenReady([TLLRN_ControlRigBlueprint, UnitNode, Target]()
								{
									TLLRN_ControlRigBlueprint->AddTransientControl(UnitNode, Target);
								}, TStatId(), NULL, ENamedThreads::GameThread);
							}),
							FCanExecuteAction::CreateLambda([HasNoUnconstrainedAffectedPin, IsSliced]() -> bool
							{
								if(HasNoUnconstrainedAffectedPin() || IsSliced())
								{
									return false;
								}
								return true;
							})
						));
					}
				}
			}
		}
	}
}

TSharedRef< SWidget > FTLLRN_ControlRigEditorModule::GenerateAnimationMenu(TWeakPtr<IAnimationEditor> InAnimationEditor)
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr);
	
	if(InAnimationEditor.IsValid())
	{
		TSharedRef<IAnimationEditor> AnimationEditor = InAnimationEditor.Pin().ToSharedRef();
		USkeleton* Skeleton = AnimationEditor->GetPersonaToolkit()->GetSkeleton();
		USkeletalMesh* SkeletalMesh = AnimationEditor->GetPersonaToolkit()->GetPreviewMesh();
		if (!SkeletalMesh) //if no preview mesh just get normal mesh
		{
			SkeletalMesh = AnimationEditor->GetPersonaToolkit()->GetMesh();
		}
		
		UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationEditor->GetPersonaToolkit()->GetAnimationAsset());
		if (Skeleton && SkeletalMesh && AnimSequence)
		{
			FUIAction EditWithFKTLLRN_ControlRig(
				FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditorModule::EditWithFKTLLRN_ControlRig, AnimSequence, SkeletalMesh, Skeleton));

			FUIAction OpenIt(
				FExecuteAction::CreateStatic(&FTLLRN_ControlRigEditorModule::OpenLevelSequence, AnimSequence),
				FCanExecuteAction::CreateLambda([AnimSequence]()
					{
						if (AnimSequence)
						{
							if (IInterface_AssetUserData* AnimAssetUserData = Cast< IInterface_AssetUserData >(AnimSequence))
							{
								UAnimSequenceLevelSequenceLink* AnimLevelLink = AnimAssetUserData->GetAssetUserData< UAnimSequenceLevelSequenceLink >();
								if (AnimLevelLink)
								{
									ULevelSequence* LevelSequence = AnimLevelLink->ResolveLevelSequence();
									if (LevelSequence)
									{
										return true;
									}
								}
							}
						}
						return false;
					}
				)

			);

			FUIAction UnLinkIt(
				FExecuteAction::CreateStatic(&FTLLRN_ControlRigEditorModule::UnLinkLevelSequence, AnimSequence),
				FCanExecuteAction::CreateLambda([AnimSequence]()
					{
						if (AnimSequence)
						{
							if (IInterface_AssetUserData* AnimAssetUserData = Cast< IInterface_AssetUserData >(AnimSequence))
							{
								UAnimSequenceLevelSequenceLink* AnimLevelLink = AnimAssetUserData->GetAssetUserData< UAnimSequenceLevelSequenceLink >();
								if (AnimLevelLink)
								{
									ULevelSequence* LevelSequence = AnimLevelLink->ResolveLevelSequence();
									if (LevelSequence)
									{
										return true;
									}
								}
							}
						}
						return false;
					}
				)

			);

			FUIAction ToggleFilterAssetBySkeleton(
				FExecuteAction::CreateLambda([this]()
					{
						bFilterAssetBySkeleton = bFilterAssetBySkeleton ? false : true;
					}
				),
				FCanExecuteAction(),
						FIsActionChecked::CreateLambda([this]()
							{
								return bFilterAssetBySkeleton;
							}
						)
						);
			if (Skeleton)
			{
				MenuBuilder.BeginSection("TLLRN Control Rig", LOCTEXT("TLLRN_ControlRig", "TLLRN Control Rig"));
				{
					MenuBuilder.AddMenuEntry(LOCTEXT("EditWithFKTLLRN_ControlRig", "Edit With FK Control Rig"),
						FText(), FSlateIcon(), EditWithFKTLLRN_ControlRig, NAME_None, EUserInterfaceActionType::Button);


					MenuBuilder.AddMenuEntry(
						LOCTEXT("FilterAssetBySkeleton", "Filter Asset By Skeleton"),
						LOCTEXT("FilterAssetBySkeletonTooltip", "Filters Control Rig Assets To Match Current Skeleton"),
						FSlateIcon(),
						ToggleFilterAssetBySkeleton,
						NAME_None,
						EUserInterfaceActionType::ToggleButton);

					MenuBuilder.AddSubMenu(
						LOCTEXT("BakeToTLLRN_ControlRig", "Bake To Control Rig"), NSLOCTEXT("AnimationModeToolkit", "BakeToTLLRN_ControlRigTooltip", "This Control Rig will Drive This Animation."),
						FNewMenuDelegate::CreateLambda([this, AnimSequence, SkeletalMesh, Skeleton](FMenuBuilder& InSubMenuBuilder)
							{
								FClassViewerInitializationOptions Options;
								Options.bShowUnloadedBlueprints = true;
								Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;

								TSharedPtr<FTLLRN_ControlRigClassFilter> ClassFilter = MakeShareable(new FTLLRN_ControlRigClassFilter(bFilterAssetBySkeleton, false, true, Skeleton));
								Options.ClassFilters.Add(ClassFilter.ToSharedRef());
								Options.bShowNoneOption = false;

								FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

								TSharedRef<SWidget> ClassViewer = ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateRaw(this, &FTLLRN_ControlRigEditorModule::BakeToTLLRN_ControlRig, AnimSequence, SkeletalMesh, Skeleton));
								InSubMenuBuilder.AddWidget(ClassViewer, FText::GetEmpty(), true);
							})
					);
				}
				MenuBuilder.EndSection();

			}

			MenuBuilder.AddMenuEntry(LOCTEXT("OpenLevelSequence", "Open Level Sequence"),
				FText(), FSlateIcon(), OpenIt, NAME_None, EUserInterfaceActionType::Button);

			MenuBuilder.AddMenuEntry(LOCTEXT("UnlinkLevelSequence", "Unlink Level Sequence"),
				FText(), FSlateIcon(), UnLinkIt, NAME_None, EUserInterfaceActionType::Button);

		}
	}
	return MenuBuilder.MakeWidget();

}


void FTLLRN_ControlRigEditorModule::ToggleIsDrivenByLevelSequence(UAnimSequence* AnimSequence) const
{

//todo what?
}

bool FTLLRN_ControlRigEditorModule::IsDrivenByLevelSequence(UAnimSequence* AnimSequence) const
{
	if (AnimSequence->GetClass()->ImplementsInterface(UInterface_AssetUserData::StaticClass()))
	{
		if (IInterface_AssetUserData* AnimAssetUserData = Cast< IInterface_AssetUserData >(AnimSequence))
		{
			UAnimSequenceLevelSequenceLink* AnimLevelLink = AnimAssetUserData->GetAssetUserData< UAnimSequenceLevelSequenceLink >();
			return (AnimLevelLink != nullptr) ? true : false;
		}
	}
	return false;
}

void FTLLRN_ControlRigEditorModule::EditWithFKTLLRN_ControlRig(UAnimSequence* AnimSequence, USkeletalMesh* SkelMesh, USkeleton* InSkeleton)
{
	BakeToTLLRN_ControlRig(UFKTLLRN_ControlRig::StaticClass(),AnimSequence, SkelMesh, InSkeleton);
}


void FTLLRN_ControlRigEditorModule::BakeToTLLRN_ControlRig(UClass* TLLRN_ControlRigClass, UAnimSequence* AnimSequence, USkeletalMesh* SkelMesh, USkeleton* InSkeleton)
{
	FSlateApplication::Get().DismissAllMenus();

	UWorld* World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr;

	if (World)
	{
		FTLLRN_ControlRigEditorModule::UnLinkLevelSequence(AnimSequence);

		FString SequenceName = FString::Printf(TEXT("Driving_%s"), *AnimSequence->GetName());
		FString PackagePath = AnimSequence->GetOutermost()->GetName();
		
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FString UniquePackageName;
		FString UniqueAssetName;
		AssetToolsModule.Get().CreateUniqueAssetName(PackagePath / SequenceName, TEXT(""), UniquePackageName, UniqueAssetName);

		UPackage* Package = CreatePackage(*UniquePackageName);
		ULevelSequence* LevelSequence = NewObject<ULevelSequence>(Package, *UniqueAssetName, RF_Public | RF_Standalone);
					
		FAssetRegistryModule::AssetCreated(LevelSequence);

		LevelSequence->Initialize(); //creates movie scene
		LevelSequence->MarkPackageDirty();
		UMovieScene* MovieScene = LevelSequence->GetMovieScene();

		FFrameRate TickResolution = MovieScene->GetTickResolution();
		float Duration = AnimSequence->GetPlayLength();
		MovieScene->SetPlaybackRange(0, (Duration * TickResolution).FloorToFrame().Value);
		FFrameRate SequenceFrameRate = AnimSequence->GetSamplingFrameRate();
		MovieScene->SetDisplayRate(SequenceFrameRate);


		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LevelSequence);

		IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSequence, false);
		ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
		TWeakPtr<ISequencer> WeakSequencer = LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;

		if (WeakSequencer.IsValid())
		{
			TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
			ASkeletalMeshActor* MeshActor = World->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), FTransform::Identity);
			MeshActor->SetActorLabel(AnimSequence->GetName());

			FString StringName = MeshActor->GetActorLabel();
			FString AnimName = AnimSequence->GetName();
			StringName = StringName + FString(TEXT(" --> ")) + AnimName;
			MeshActor->SetActorLabel(StringName);
			if (SkelMesh)
			{
				MeshActor->GetSkeletalMeshComponent()->SetSkeletalMesh(SkelMesh);
			}
			MeshActor->RegisterAllComponents();
			TArray<TWeakObjectPtr<AActor> > ActorsToAdd;
			ActorsToAdd.Add(MeshActor);
			TArray<FGuid> ActorTracks = SequencerPtr->AddActors(ActorsToAdd, false);
			FGuid ActorTrackGuid = ActorTracks[0];

			// By default, convert this to a spawnable and delete the existing actor. If for some reason, 
			// the spawnable couldn't be generated, use the existing actor as a possessable (this could 
			// eventually be an option)

			if (FMovieScenePossessable* Possessable = FSequencerUtilities::ConvertToCustomBinding(SequencerPtr.ToSharedRef(), ActorTrackGuid, UMovieSceneSpawnableActorBinding::StaticClass(), 0))
			{ 
				ActorTrackGuid = Possessable->GetGuid();

				UObject* SpawnedMesh = SequencerPtr->FindSpawnedObjectOrTemplate(ActorTrackGuid);

				if (SpawnedMesh)
				{
					GCurrentLevelEditingViewportClient->GetWorld()->EditorDestroyActor(MeshActor, true);
					MeshActor = Cast<ASkeletalMeshActor>(SpawnedMesh);
					if (SkelMesh)
					{
						MeshActor->GetSkeletalMeshComponent()->SetSkeletalMesh(SkelMesh);
					}
					MeshActor->RegisterAllComponents();
				}
			}

			//Delete binding from default animating rig
			//if we have skel mesh component binding we can just delete that
			FGuid CompGuid = SequencerPtr->FindObjectId(*(MeshActor->GetSkeletalMeshComponent()), SequencerPtr->GetFocusedTemplateID());
			if (CompGuid.IsValid())
			{
				if (!MovieScene->RemovePossessable(CompGuid))
				{
					MovieScene->RemoveSpawnable(CompGuid);
				}
			}
			else //otherwise if not delete the track
			{
				if (UMovieSceneTrack* ExistingTrack = MovieScene->FindTrack<UMovieSceneTLLRN_ControlRigParameterTrack>(ActorTrackGuid))
				{
					MovieScene->RemoveTrack(*ExistingTrack);
				}
			}

			UMovieSceneTLLRN_ControlRigParameterTrack* Track = MovieScene->AddTrack<UMovieSceneTLLRN_ControlRigParameterTrack>(ActorTrackGuid);
			if (Track)
			{
				USkeletalMeshComponent* SkelMeshComp = MeshActor->GetSkeletalMeshComponent();
				USkeletalMesh* SkeletalMesh = SkelMeshComp->GetSkeletalMeshAsset();

				FString ObjectName = (TLLRN_ControlRigClass->GetName());
				ObjectName.RemoveFromEnd(TEXT("_C"));

				UTLLRN_ControlRig* TLLRN_ControlRig = NewObject<UTLLRN_ControlRig>(Track, TLLRN_ControlRigClass, FName(*ObjectName), RF_Transactional);
				TLLRN_ControlRig->SetObjectBinding(MakeShared<FTLLRN_ControlRigObjectBinding>());
				TLLRN_ControlRig->GetObjectBinding()->BindToObject(MeshActor);
				TLLRN_ControlRig->GetDataSourceRegistry()->RegisterDataSource(UTLLRN_ControlRig::OwnerComponent, TLLRN_ControlRig->GetObjectBinding()->GetBoundObject());
				TLLRN_ControlRig->Initialize();
				TLLRN_ControlRig->Evaluate_AnyThread();

				SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);

				Track->Modify();
				UMovieSceneSection* NewSection = Track->CreateTLLRN_ControlRigSection(0, TLLRN_ControlRig, true);
				//mz todo need to have multiple rigs with same class
				Track->SetTrackName(FName(*ObjectName));
				Track->SetDisplayName(FText::FromString(ObjectName));
				UMovieSceneTLLRN_ControlRigParameterSection* ParamSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(NewSection);
				FBakeToControlDelegate BakeCallback = FBakeToControlDelegate::CreateLambda([this, LevelSequence,
					AnimSequence, MovieScene, TLLRN_ControlRig, ParamSection,ActorTrackGuid, SkelMeshComp]
				(bool bKeyReduce, float KeyReduceTolerance, FFrameRate BakeFrameRate, bool bResetControls)
				{
					IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSequence, false);
					ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
					TWeakPtr<ISequencer> WeakSequencer = LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;
					if (WeakSequencer.IsValid())
					{
						TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
						if (ParamSection)
						{
							FSmartReduceParams SmartReduce;
							SmartReduce.TolerancePercentage = KeyReduceTolerance;
							SmartReduce.SampleRate = BakeFrameRate;
							FTLLRN_ControlRigParameterTrackEditor::LoadAnimationIntoSection(SequencerPtr, AnimSequence, SkelMeshComp, FFrameNumber(0),
								bKeyReduce, SmartReduce, bResetControls, ParamSection);
						}
						SequencerPtr->EmptySelection();
						SequencerPtr->SelectSection(ParamSection);
						SequencerPtr->ThrobSectionSelection();
						SequencerPtr->ObjectImplicitlyAdded(TLLRN_ControlRig);
						SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
						FTLLRN_ControlRigEditMode* TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
						if (!TLLRN_ControlRigEditMode)
						{
							GLevelEditorModeTools().ActivateMode(FTLLRN_ControlRigEditMode::ModeName);
							TLLRN_ControlRigEditMode = static_cast<FTLLRN_ControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FTLLRN_ControlRigEditMode::ModeName));
						}
						if (TLLRN_ControlRigEditMode)
						{
							TLLRN_ControlRigEditMode->AddTLLRN_ControlRigObject(TLLRN_ControlRig, SequencerPtr);
						}

						//create soft links to each other
						if (IInterface_AssetUserData* AssetUserDataInterface = Cast< IInterface_AssetUserData >(LevelSequence))
						{
							ULevelSequenceAnimSequenceLink* LevelAnimLink = NewObject<ULevelSequenceAnimSequenceLink>(LevelSequence, NAME_None, RF_Public | RF_Transactional);
							FLevelSequenceAnimSequenceLinkItem LevelAnimLinkItem;
							LevelAnimLinkItem.SkelTrackGuid = ActorTrackGuid;
							LevelAnimLinkItem.PathToAnimSequence = FSoftObjectPath(AnimSequence);
							LevelAnimLinkItem.bExportMorphTargets = true;
							LevelAnimLinkItem.bExportAttributeCurves = true;
							LevelAnimLinkItem.Interpolation = EAnimInterpolationType::Linear;
							LevelAnimLinkItem.CurveInterpolation = ERichCurveInterpMode::RCIM_Linear;
							LevelAnimLinkItem.bExportMaterialCurves = true;
							LevelAnimLinkItem.bExportTransforms = true;
							LevelAnimLinkItem.bRecordInWorldSpace = false;
							LevelAnimLinkItem.bEvaluateAllSkeletalMeshComponents = true;
							LevelAnimLink->AnimSequenceLinks.Add(LevelAnimLinkItem);
							AssetUserDataInterface->AddAssetUserData(LevelAnimLink);
						}
						if (IInterface_AssetUserData* AnimAssetUserData = Cast< IInterface_AssetUserData >(AnimSequence))
						{
							UAnimSequenceLevelSequenceLink* AnimLevelLink = AnimAssetUserData->GetAssetUserData< UAnimSequenceLevelSequenceLink >();
							if (!AnimLevelLink)
							{
								AnimLevelLink = NewObject<UAnimSequenceLevelSequenceLink>(AnimSequence, NAME_None, RF_Public | RF_Transactional);
								AnimAssetUserData->AddAssetUserData(AnimLevelLink);
							}
							AnimLevelLink->SetLevelSequence(LevelSequence);
							AnimLevelLink->SkelTrackGuid = ActorTrackGuid;
						}
					}
				});

				FOnWindowClosed BakeClosedCallback = FOnWindowClosed::CreateLambda([](const TSharedRef<SWindow>&) { });

				BakeToTLLRN_ControlRigDialog::GetBakeParams(BakeCallback, BakeClosedCallback);
			}
		}
	}
}

void FTLLRN_ControlRigEditorModule::UnLinkLevelSequence(UAnimSequence* AnimSequence)
{
	if (IInterface_AssetUserData* AnimAssetUserData = Cast< IInterface_AssetUserData >(AnimSequence))
	{
		UAnimSequenceLevelSequenceLink* AnimLevelLink = AnimAssetUserData->GetAssetUserData< UAnimSequenceLevelSequenceLink >();
		if (AnimLevelLink)
		{
			ULevelSequence* LevelSequence = AnimLevelLink->ResolveLevelSequence();
			if (LevelSequence)
			{
				if (IInterface_AssetUserData* LevelSequenceUserDataInterface = Cast< IInterface_AssetUserData >(LevelSequence))
				{

					ULevelSequenceAnimSequenceLink* LevelAnimLink = LevelSequenceUserDataInterface->GetAssetUserData< ULevelSequenceAnimSequenceLink >();
					if (LevelAnimLink)
					{
						for (int32 Index = 0; Index < LevelAnimLink->AnimSequenceLinks.Num(); ++Index)
						{
							FLevelSequenceAnimSequenceLinkItem& LevelAnimLinkItem = LevelAnimLink->AnimSequenceLinks[Index];
							if (LevelAnimLinkItem.ResolveAnimSequence() == AnimSequence)
							{
								LevelAnimLink->AnimSequenceLinks.RemoveAtSwap(Index);
							
								const FText NotificationText = FText::Format(LOCTEXT("UnlinkLevelSequenceSuccess", "{0} unlinked from "), FText::FromString(AnimSequence->GetName()));
								FNotificationInfo Info(NotificationText);
								Info.ExpireDuration = 5.f;
								Info.Hyperlink = FSimpleDelegate::CreateLambda([=]()
								{
									TArray<UObject*> Assets;
									Assets.Add(LevelSequence);
									GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Assets);
								});
								Info.HyperlinkText = FText::Format(LOCTEXT("OpenUnlinkedLevelSequenceLink", "{0}"), FText::FromString(LevelSequence->GetName()));
								FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
														
								break;
							}
						}
						if (LevelAnimLink->AnimSequenceLinks.Num() <= 0)
						{
							LevelSequenceUserDataInterface->RemoveUserDataOfClass(ULevelSequenceAnimSequenceLink::StaticClass());
						}
					}

				}
			}
			AnimAssetUserData->RemoveUserDataOfClass(UAnimSequenceLevelSequenceLink::StaticClass());
		}
	}
}

void FTLLRN_ControlRigEditorModule::OpenLevelSequence(UAnimSequence* AnimSequence) 
{
	if (IInterface_AssetUserData* AnimAssetUserData = Cast< IInterface_AssetUserData >(AnimSequence))
	{
		UAnimSequenceLevelSequenceLink* AnimLevelLink = AnimAssetUserData->GetAssetUserData< UAnimSequenceLevelSequenceLink >();
		if (AnimLevelLink)
		{
			ULevelSequence* LevelSequence = AnimLevelLink->ResolveLevelSequence();
			if (LevelSequence)
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LevelSequence);
			}
		}
	}
}

void FTLLRN_ControlRigEditorModule::AddTLLRN_ControlRigExtenderToToolMenu(FName InToolMenuName)
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolMenu = UToolMenus::Get()->ExtendMenu(InToolMenuName);

	FToolUIAction UIAction;
	UIAction.IsActionVisibleDelegate.BindLambda([](const FToolMenuContext& Context) -> bool
	{
		if (const UAnimationToolMenuContext* MenuContext = Context.FindContext<UAnimationToolMenuContext>())
		{					
			if (const UAnimationAsset* AnimationAsset = MenuContext->AnimationEditor.Pin()->GetPersonaToolkit()->GetAnimationAsset())
			{
				return AnimationAsset->GetClass() == UAnimSequence::StaticClass();
			}
		}
		return false;
	});
	
	ToolMenu->AddMenuEntry("Sequencer",
		FToolMenuEntry::InitComboButton(
			"EditInSequencer",
			FToolUIActionChoice(UIAction),
			FNewToolMenuChoice(
				FNewToolMenuDelegate::CreateLambda([this](UToolMenu* InNewToolMenu)
				{
					if (UAnimationToolMenuContext* MenuContext = InNewToolMenu->FindContext<UAnimationToolMenuContext>())
					{
						InNewToolMenu->AddMenuEntry(
							"EditInSequencer", 
							FToolMenuEntry::InitWidget(
								"EditInSequencerMenu", 
								GenerateAnimationMenu(MenuContext->AnimationEditor),
								FText::GetEmpty(),
								true, false, true
							)
						);
					}
				})
			),
			LOCTEXT("EditInSequencer", "Edit in Sequencer"),
			LOCTEXT("EditInSequencer_Tooltip", "Edit this Anim Sequence In Sequencer."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Persona.EditInSequencer")
		)
	);
}

static void ExecuteOpenLevelSequence(const FToolMenuContext& InContext)
{
	if (const UContentBrowserAssetContextMenuContext* CBContext = InContext.FindContext<UContentBrowserAssetContextMenuContext>())
	{
		if (UAnimSequence* AnimSequence = CBContext->LoadFirstSelectedObject<UAnimSequence>())
        {
       		FTLLRN_ControlRigEditorModule::OpenLevelSequence(AnimSequence);
        }
	}
}

static bool CanExecuteOpenLevelSequence(const FToolMenuContext& InContext)
{
	if (const UContentBrowserAssetContextMenuContext* CBContext = InContext.FindContext<UContentBrowserAssetContextMenuContext>())
	{
		if (CBContext->SelectedAssets.Num() != 1)
		{
			return false;
		}

		const FAssetData& SelectedAnimSequence = CBContext->SelectedAssets[0];

		FString PathToLevelSequence;
		if (SelectedAnimSequence.GetTagValue<FString>(GET_MEMBER_NAME_CHECKED(UAnimSequenceLevelSequenceLink, PathToLevelSequence), PathToLevelSequence))
		{
			if (!FSoftObjectPath(PathToLevelSequence).IsNull())
			{
				return true;
			}
		}
	}

	return false;
}

void FTLLRN_ControlRigEditorModule::ExtendAnimSequenceMenu()
{
	UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UAnimSequence::StaticClass());

	FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
	Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{
			{
				const TAttribute<FText> Label = LOCTEXT("OpenLevelSequence", "Open Level Sequence");
				const TAttribute<FText> ToolTip =LOCTEXT("CreateTLLRN_ControlRig_ToolTip", "Opens a Level Sequence if it is driving this Anim Sequence.");
				const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCurveEditor.TabIcon");

				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteOpenLevelSequence);
				UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&CanExecuteOpenLevelSequence);
				InSection.AddMenuEntry("OpenLevelSequence", Label, ToolTip, Icon, UIAction);
			}
		}));
}

TSharedRef<ITLLRN_ControlRigEditor> FTLLRN_ControlRigEditorModule::CreateTLLRN_ControlRigEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, class UTLLRN_ControlRigBlueprint* InBlueprint)
{
	TSharedRef< FTLLRN_ControlRigEditor > NewTLLRN_ControlRigEditor(new FTLLRN_ControlRigEditor());
	NewTLLRN_ControlRigEditor->InitRigVMEditor(Mode, InitToolkitHost, InBlueprint);
	return NewTLLRN_ControlRigEditor;
}

FTLLRN_ControlRigClassFilter::FTLLRN_ControlRigClassFilter(bool bInCheckSkeleton, bool bInCheckAnimatable, bool bInCheckInversion, USkeleton* InSkeleton) :	bFilterAssetBySkeleton(bInCheckSkeleton),
	bFilterExposesAnimatableControls(bInCheckAnimatable),
	bFilterInversion(bInCheckInversion),
	Skeleton(InSkeleton),
	AssetRegistry(FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get())
{
}

bool FTLLRN_ControlRigClassFilter::MatchesFilter(const FAssetData& AssetData)
{
	const bool bExposesAnimatableControls = AssetData.GetTagValueRef<bool>(TEXT("bExposesAnimatableControls"));
	if (bFilterExposesAnimatableControls == true && bExposesAnimatableControls == false)
	{
		return false;
	}
	if (bFilterInversion)
	{		
		FAssetDataTagMapSharedView::FFindTagResult Tag = AssetData.TagsAndValues.FindTag(TEXT("SupportedEventNames"));
		if (Tag.IsSet())
		{
			bool bHasInversion = false;
			FString EventString = FTLLRN_RigUnit_InverseExecution::EventName.ToString();
			FString OldEventString = FString(TEXT("Inverse"));
			TArray<FString> SupportedEventNames;
			Tag.GetValue().ParseIntoArray(SupportedEventNames, TEXT(","), true);

			for (const FString& Name : SupportedEventNames)
			{
				if (Name.Contains(EventString) || Name.Contains(OldEventString))
				{
					bHasInversion = true;
					break;
				}
			}
			if (bHasInversion == false)
			{
				return false;
			}
		}
	}
	if (bFilterAssetBySkeleton)
	{
		FString SkeletonName;
		if (Skeleton)
		{
			SkeletonName = FAssetData(Skeleton).GetExportTextName();
		}
		FString PreviewSkeletalMesh = AssetData.GetTagValueRef<FString>(TEXT("PreviewSkeletalMesh"));
		if (PreviewSkeletalMesh.Len() > 0)
		{
			FAssetData SkelMeshData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(PreviewSkeletalMesh));
			FString PreviewSkeleton = SkelMeshData.GetTagValueRef<FString>(TEXT("Skeleton"));
			if (PreviewSkeleton == SkeletonName)
			{
				return true;
			}
			else if(Skeleton)
			{
				if (Skeleton->IsCompatibleForEditor(PreviewSkeleton))
				{
					return true;
				}
			}
		}
		FString PreviewSkeleton = AssetData.GetTagValueRef<FString>(TEXT("PreviewSkeleton"));
		if (PreviewSkeleton == SkeletonName)
		{
			return true;
		}
		else if (Skeleton)
		{
			if (Skeleton->IsCompatibleForEditor(PreviewSkeleton))
			{
				return true;
			}
		}
		FString SourceHierarchyImport = AssetData.GetTagValueRef<FString>(TEXT("SourceHierarchyImport"));
		if (SourceHierarchyImport == SkeletonName)
		{
			return true;
		}
		else if (Skeleton)
		{
			if (Skeleton->IsCompatibleForEditor(SourceHierarchyImport))
			{
				return true;
			}
		}
		FString SourceCurveImport = AssetData.GetTagValueRef<FString>(TEXT("SourceCurveImport"));
		if (SourceCurveImport == SkeletonName)
		{
			return true;
		}
		else if (Skeleton)
		{
			if (Skeleton->IsCompatibleForEditor(SourceCurveImport))
			{
				return true;
			}
		}
		return false;
	}
	return true;
}

bool FTLLRN_ControlRigClassFilter::IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) 
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

bool FTLLRN_ControlRigClassFilter::IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs)
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

IMPLEMENT_MODULE(FTLLRN_ControlRigEditorModule, TLLRN_ControlRigEditor)

#undef LOCTEXT_NAMESPACE
