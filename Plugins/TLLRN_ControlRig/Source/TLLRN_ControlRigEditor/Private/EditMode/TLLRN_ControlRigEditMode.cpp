// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/TLLRN_ControlRigEditMode.h"

#include "AnimationEditorPreviewActor.h"
#include "Components/StaticMeshComponent.h"
#include "EditMode/TLLRN_ControlRigEditModeToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "EditMode/STLLRN_ControlRigEditModeTools.h"
#include "Algo/Transform.h"
#include "TLLRN_ControlRig.h"
#include "HitProxies.h"
#include "EditMode/TLLRN_ControlRigEditModeSettings.h"
#include "EditMode/STLLRN_ControlRigDetails.h"
#include "EditMode/STLLRN_ControlRigOutliner.h"
#include "ISequencer.h"
#include "ISequencer.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "MVVM/Selection/Selection.h"
#include "MovieScene.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "LevelEditorViewport.h"
#include "Components/SkeletalMeshComponent.h"
#include "EditMode/TLLRN_ControlRigEditModeCommands.h"
#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"
#include "ISequencerModule.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "TLLRN_ControlRigEditorModule.h"
#include "Constraint.h"
#include "EngineUtils.h"
#include "RigVMBlueprintGeneratedClass.h"
#include "ITLLRN_ControlRigObjectBinding.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlRigGizmoActor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SEditorViewport.h"
#include "EditMode/TLLRN_TLLRN_AnimDetailsProxy.h"
#include "ScopedTransaction.h"
#include "RigVMModel/RigVMController.h"
#include "Rigs/AdditiveTLLRN_ControlRig.h"
#include "Rigs/FKTLLRN_ControlRig.h"
#include "TLLRN_ControlRigComponent.h"
#include "EngineUtils.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InteractionExecution.h"
#include "IPersonaPreviewScene.h"
#include "PersonaSelectionProxies.h"
#include "PropertyHandle.h"
#include "Framework/Application/SlateApplication.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Settings/TLLRN_ControlRigSettings.h"
#include "ToolMenus.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Settings/LevelEditorViewportSettings.h"
#include "Editor/STLLRN_RigSpacePickerWidget.h"
#include "TLLRN_ControlTLLRN_RigSpaceChannelEditors.h"
#include "TLLRN_ControlRigSequencerEditorLibrary.h"
#include "LevelSequence.h"
#include "LevelEditor.h"
#include "InteractiveToolManager.h"
#include "EdModeInteractiveToolsContext.h"
#include "Constraints/MovieSceneConstraintChannelHelper.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Widgets/Docking/SDockTab.h"
#include "TransformConstraint.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Materials/Material.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "DragTool_BoxSelect.h"
#include "DragTool_FrustumSelect.h"
#include "AnimationEditorViewportClient.h"
#include "EditorInteractiveGizmoManager.h"
#include "Editor/TLLRN_ControlRigViewportToolbarExtensions.h"
#include "Editor/Sequencer/Private/SSequencer.h"
#include "Slate/SceneViewport.h"
#include "Tools/BakingHelper.h"
#include "Sequencer/TLLRN_TLLRN_AnimLayers/TLLRN_TLLRN_AnimLayers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigEditMode)

TAutoConsoleVariable<bool> CVarClickSelectThroughGizmo(TEXT("TLLRN_ControlRig.Sequencer.ClickSelectThroughGizmo"), false, TEXT("When false you can't click through a gizmo and change selection if you will select the gizmo when in Animation Mode, default to false."));

namespace CRModeLocals
{

	const TCHAR* FocusModeName = TEXT("AnimMode.PendingFocusMode");

static bool bFocusMode = false;
static FAutoConsoleVariableRef CVarSetFocusOnHover(
   FocusModeName,
   bFocusMode,
   TEXT("Force setting focus on the hovered viewport when entering a key.")
   );

IConsoleVariable* GetFocusModeVariable()
{
	return IConsoleManager::Get().FindConsoleVariable(FocusModeName);
}
	
}

void UTLLRN_ControlRigEditModeDelegateHelper::OnPoseInitialized()
{
	if (EditMode)
	{
		EditMode->OnPoseInitialized();
	}
}
void UTLLRN_ControlRigEditModeDelegateHelper::PostPoseUpdate()
{
	if (EditMode)
	{
		EditMode->PostPoseUpdate();
	}
}

void UTLLRN_ControlRigEditModeDelegateHelper::AddDelegates(USkeletalMeshComponent* InSkeletalMeshComponent)
{
	if (BoundComponent.IsValid())
	{
		if (BoundComponent.Get() == InSkeletalMeshComponent)
		{
			return;
		}
	}

	RemoveDelegates();

	BoundComponent = InSkeletalMeshComponent;

	if (BoundComponent.IsValid())
	{
		BoundComponent->OnAnimInitialized.AddDynamic(this, &UTLLRN_ControlRigEditModeDelegateHelper::OnPoseInitialized);
		OnBoneTransformsFinalizedHandle = BoundComponent->RegisterOnBoneTransformsFinalizedDelegate(
			FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &UTLLRN_ControlRigEditModeDelegateHelper::PostPoseUpdate));
	}
}

void UTLLRN_ControlRigEditModeDelegateHelper::RemoveDelegates()
{
	if(BoundComponent.IsValid())
	{
		BoundComponent->OnAnimInitialized.RemoveAll(this);
		BoundComponent->UnregisterOnBoneTransformsFinalizedDelegate(OnBoneTransformsFinalizedHandle);
		OnBoneTransformsFinalizedHandle.Reset();
		BoundComponent = nullptr;
	}
}

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigEditMode"

/** The different parts of a transform that manipulators can support */
enum class ETransformComponent
{
	None,

	Rotation,

	Translation,

	Scale
};

namespace TLLRN_ControlRigSelectionConstants
{
	/** Distance to trace for physics bodies */
	static const float BodyTraceDistance = 100000.0f;
}

FTLLRN_ControlRigEditMode::FTLLRN_ControlRigEditMode()
	: PendingFocus(FPendingWidgetFocus::MakeNoTextEdit())
	, bIsChangingControlShapeTransform(false)
	, bIsTracking(false)
	, bManipulatorMadeChange(false)
	, bSelecting(false)
	, bSelectionChanged(false)
	, RecreateControlShapesRequired(ERecreateTLLRN_ControlRigShape::RecreateNone)
	, bSuspendHierarchyNotifs(false)
	, CurrentViewportClient(nullptr)
	, bIsChangingCoordSystem(false)
	, InteractionType((uint8)ETLLRN_ControlRigInteractionType::None)
	, bShowControlsAsOverlay(false)
	, bIsConstructionEventRunning(false)
{
	ControlProxy = NewObject<UTLLRN_ControlRigDetailPanelControlProxies>(GetTransientPackage(), NAME_None);
	ControlProxy->SetFlags(RF_Transactional);
	DetailKeyFrameCache = MakeShared<FDetailKeyFrameCacheAndHandler>();

	UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
	bShowControlsAsOverlay = Settings->bShowControlsAsOverlay;

	Settings->GizmoScaleDelegate.AddLambda([this](float GizmoScale)
	{
		if (FEditorModeTools* ModeTools = GetModeManager())
		{
			ModeTools->SetWidgetScale(GizmoScale);
		}
	});

	CommandBindings = MakeShareable(new FUICommandList);
	BindCommands();

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &FTLLRN_ControlRigEditMode::OnObjectsReplaced);
#endif
}

FTLLRN_ControlRigEditMode::~FTLLRN_ControlRigEditMode()
{	
	CommandBindings = nullptr;

	DestroyShapesActors(nullptr);
	OnTLLRN_ControlRigAddedOrRemovedDelegate.Clear();
	OnTLLRN_ControlRigSelectedDelegate.Clear();

	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> PreviousRuntimeRigs = RuntimeTLLRN_ControlRigs;
	for (int32 PreviousRuntimeRigIndex = 0; PreviousRuntimeRigIndex < PreviousRuntimeRigs.Num(); PreviousRuntimeRigIndex++)
	{
		if (PreviousRuntimeRigs[PreviousRuntimeRigIndex].IsValid())
		{
			RemoveTLLRN_ControlRig(PreviousRuntimeRigs[PreviousRuntimeRigIndex].Get());
		}
	}
	RuntimeTLLRN_ControlRigs.Reset();

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
#endif

}

bool FTLLRN_ControlRigEditMode::SetSequencer(TWeakPtr<ISequencer> InSequencer)
{
	if (InSequencer != WeakSequencer)
	{
		if (WeakSequencer.IsValid())
		{
			static constexpr bool bDisable = false;
			TSharedPtr<ISequencer> PreviousSequencer = WeakSequencer.Pin();
			TSharedRef<SSequencer> PreviousSequencerWidget = StaticCastSharedRef<SSequencer>(PreviousSequencer->GetSequencerWidget());
			PreviousSequencerWidget->EnablePendingFocusOnHovering(bDisable);
		}
		
		WeakSequencer = InSequencer;

		DetailKeyFrameCache->UnsetDelegates();

		DestroyShapesActors(nullptr);
		TArray<TWeakObjectPtr<UTLLRN_ControlRig>> PreviousRuntimeRigs = RuntimeTLLRN_ControlRigs;
		for (int32 PreviousRuntimeRigIndex = 0; PreviousRuntimeRigIndex < PreviousRuntimeRigs.Num(); PreviousRuntimeRigIndex++)
		{
			if (PreviousRuntimeRigs[PreviousRuntimeRigIndex].IsValid())
			{
				RemoveTLLRN_ControlRig(PreviousRuntimeRigs[PreviousRuntimeRigIndex].Get());
			}
		}
		RuntimeTLLRN_ControlRigs.Reset();
		if (InSequencer.IsValid())
		{
			TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
			if (ULevelSequence* LevelSequence = Cast<ULevelSequence>(Sequencer->GetFocusedMovieSceneSequence()))
			{
				TArray<FTLLRN_ControlRigSequencerBindingProxy> Proxies = UTLLRN_ControlRigSequencerEditorLibrary::GetTLLRN_ControlRigs(LevelSequence);
				for (FTLLRN_ControlRigSequencerBindingProxy& Proxy : Proxies)
				{
					if (UTLLRN_ControlRig* TLLRN_ControlRig = Proxy.TLLRN_ControlRig.Get())
					{
						AddTLLRN_ControlRigInternal(TLLRN_ControlRig);
					}
				}
			}
			LastMovieSceneSig = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetSignature();
			DetailKeyFrameCache->SetDelegates(WeakSequencer, this);
			ControlProxy->SetSequencer(WeakSequencer);

			{
				TSharedRef<SSequencer> SequencerWidget = StaticCastSharedRef<SSequencer>(Sequencer->GetSequencerWidget());
				SequencerWidget->EnablePendingFocusOnHovering(CRModeLocals::bFocusMode);
			}
		}
		SetObjects_Internal();
		if (FTLLRN_ControlRigEditModeToolkit::Details.IsValid())
		{
			FTLLRN_ControlRigEditModeToolkit::Details->SetEditMode(*this);
		}
		if (FTLLRN_ControlRigEditModeToolkit::Outliner.IsValid())
		{
			FTLLRN_ControlRigEditModeToolkit::Outliner->SetEditMode(*this);
		}
	}
	return false;
}

bool FTLLRN_ControlRigEditMode::AddTLLRN_ControlRigObject(UTLLRN_ControlRig* InTLLRN_ControlRig, const TWeakPtr<ISequencer>& InSequencer)
{
	if (InTLLRN_ControlRig)
	{
		if (RuntimeTLLRN_ControlRigs.Contains(InTLLRN_ControlRig) == false)
		{
			if (InSequencer.IsValid())
			{
				if (SetSequencer(InSequencer) == false) //was already there so just add it,otherwise this function will add everything in the active 
				{
					AddTLLRN_ControlRigInternal(InTLLRN_ControlRig);
					SetObjects_Internal();
				}
				return true;
			}		
		}
	}
	return false;
}

void FTLLRN_ControlRigEditMode::SetObjects(UTLLRN_ControlRig* TLLRN_ControlRig,  UObject* BindingObject, TWeakPtr<ISequencer> InSequencer)
{
	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> PreviousRuntimeRigs = RuntimeTLLRN_ControlRigs;
	for (int32 PreviousRuntimeRigIndex = 0; PreviousRuntimeRigIndex < PreviousRuntimeRigs.Num(); PreviousRuntimeRigIndex++)
	{
		if (PreviousRuntimeRigs[PreviousRuntimeRigIndex].IsValid())
		{
			RemoveTLLRN_ControlRig(PreviousRuntimeRigs[PreviousRuntimeRigIndex].Get());
		}
	}
	RuntimeTLLRN_ControlRigs.Reset();

	if (InSequencer.IsValid())
	{
		WeakSequencer = InSequencer;
	
	}
	// if we get binding object, set it to control rig binding object
	if (BindingObject && TLLRN_ControlRig)
	{
		if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
		{
			if (ObjectBinding->GetBoundObject() == nullptr)
			{
				ObjectBinding->BindToObject(BindingObject);
			}
		}

		AddTLLRN_ControlRigInternal(TLLRN_ControlRig);
	}
	else if (TLLRN_ControlRig)
	{
		AddTLLRN_ControlRigInternal(TLLRN_ControlRig);
	}

	SetObjects_Internal();
}

bool FTLLRN_ControlRigEditMode::IsInLevelEditor() const
{
	return GetModeManager() == &GLevelEditorModeTools();
}

void FTLLRN_ControlRigEditMode::SetUpDetailPanel()
{
	if (!AreEditingTLLRN_ControlRigDirectly() && Toolkit)
	{
		StaticCastSharedPtr<STLLRN_ControlRigEditModeTools>(Toolkit->GetInlineContent())->SetSequencer(WeakSequencer.Pin());
		StaticCastSharedPtr<STLLRN_ControlRigEditModeTools>(Toolkit->GetInlineContent())->SetSettingsDetailsObject(GetMutableDefault<UTLLRN_ControlRigEditModeSettings>());	
	}
}

void FTLLRN_ControlRigEditMode::SetObjects_Internal()
{
	bool bHasValidRuntimeTLLRN_ControlRig = false;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* RuntimeTLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			RuntimeTLLRN_ControlRig->ControlModified().RemoveAll(this);
			RuntimeTLLRN_ControlRig->GetHierarchy()->OnModified().RemoveAll(this);

			RuntimeTLLRN_ControlRig->ControlModified().AddSP(this, &FTLLRN_ControlRigEditMode::OnControlModified);
			RuntimeTLLRN_ControlRig->GetHierarchy()->OnModified().AddSP(this, &FTLLRN_ControlRigEditMode::OnHierarchyModified_AnyThread);
			if (USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(GetHostingSceneComponent(RuntimeTLLRN_ControlRig)))
			{
				TStrongObjectPtr<UTLLRN_ControlRigEditModeDelegateHelper>* DelegateHelper = DelegateHelpers.Find(RuntimeTLLRN_ControlRig);
				if (!DelegateHelper)
				{
					DelegateHelpers.Add(RuntimeTLLRN_ControlRig, TStrongObjectPtr<UTLLRN_ControlRigEditModeDelegateHelper>(NewObject<UTLLRN_ControlRigEditModeDelegateHelper>()));
					DelegateHelper = DelegateHelpers.Find(RuntimeTLLRN_ControlRig);
				}
				else if (DelegateHelper->IsValid() == false)
				{
					DelegateHelper->Get()->RemoveDelegates();
					DelegateHelpers.Remove(RuntimeTLLRN_ControlRig);
					*DelegateHelper = TStrongObjectPtr<UTLLRN_ControlRigEditModeDelegateHelper>(NewObject<UTLLRN_ControlRigEditModeDelegateHelper>());
					DelegateHelper->Get()->EditMode = this;
					DelegateHelper->Get()->AddDelegates(MeshComponent);
					DelegateHelpers.Add(RuntimeTLLRN_ControlRig, *DelegateHelper);
				}
				
				if (DelegateHelper && DelegateHelper->IsValid())
				{
					bHasValidRuntimeTLLRN_ControlRig = true;
				}
			}
		}
	}

	if (UsesToolkits() && Toolkit.IsValid())
	{
		StaticCastSharedPtr<STLLRN_ControlRigEditModeTools>(Toolkit->GetInlineContent())->SetTLLRN_ControlRigs(RuntimeTLLRN_ControlRigs);
	}

	if (!bHasValidRuntimeTLLRN_ControlRig)
	{
		DestroyShapesActors(nullptr);
		SetUpDetailPanel();
	}
	else
	{
		// create default manipulation layer
		RequestToRecreateControlShapeActors();
	}
}

bool FTLLRN_ControlRigEditMode::UsesToolkits() const
{
	return true;
}

void FTLLRN_ControlRigEditMode::Enter()
{
	// Call parent implementation
	FEdMode::Enter();
	LastMovieSceneSig = FGuid();
	if (UsesToolkits())
	{
		if (!AreEditingTLLRN_ControlRigDirectly())
		{
			if (WeakSequencer.IsValid() == false)
			{
				SetSequencer(FBakingHelper::GetSequencer());
			}
		}
		if (!Toolkit.IsValid())
		{
			Toolkit = MakeShareable(new FTLLRN_ControlRigEditModeToolkit(*this));
			Toolkit->Init(Owner->GetToolkitHost());
		}

		FEditorModeTools* ModeManager = GetModeManager();

		bIsChangingCoordSystem = false;
		if (CoordSystemPerWidgetMode.Num() < (UE::Widget::WM_Max))
		{
			CoordSystemPerWidgetMode.SetNum(UE::Widget::WM_Max);
			ECoordSystem CoordSystem = ModeManager->GetCoordSystem();
			for (int32 i = 0; i < UE::Widget::WM_Max; ++i)
			{
				CoordSystemPerWidgetMode[i] = CoordSystem;
			}
		}

		ModeManager->OnWidgetModeChanged().AddSP(this, &FTLLRN_ControlRigEditMode::OnWidgetModeChanged);
		ModeManager->OnCoordSystemChanged().AddSP(this, &FTLLRN_ControlRigEditMode::OnCoordSystemChanged);
	}
	WorldPtr = GetWorld();
	OnWorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddSP(this, &FTLLRN_ControlRigEditMode::OnWorldCleanup);
	SetObjects_Internal();

	//set up gizmo scale to what we had last and save what it was.
	PreviousGizmoScale = GetModeManager()->GetWidgetScale();
	if (const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		GetModeManager()->SetWidgetScale(Settings->GizmoScale);

		if (!Settings->OnSettingsChange.IsBoundToObject(this))
		{
			Settings->OnSettingsChange.AddSP(this, &FTLLRN_ControlRigEditMode::OnSettingsChanged);
		}
	}

	UE::TLLRN_ControlRig::PopulateTLLRN_ControlRigViewportToolbarTransformsSubmenu("LevelEditor.ViewportToolbar.Transform");
	UE::TLLRN_ControlRig::PopulateTLLRN_ControlRigViewportToolbarSelectionSubmenu("LevelEditor.ViewportToolbar.Select");
	UE::TLLRN_ControlRig::PopulateTLLRN_ControlRigViewportToolbarShowSubmenu(
		"LevelEditor.ViewportToolbar.Show", GetToolkit()->GetToolkitCommands()
	);

	RegisterPendingFocusMode();
}

//todo get working with Persona
static void ClearOutAnyActiveTools()
{
	if (FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
	{
		TSharedPtr<ILevelEditor> LevelEditorPtr = LevelEditorModule->GetLevelEditorInstance().Pin();

		if (LevelEditorPtr.IsValid())
		{
			FString ActiveToolName = LevelEditorPtr->GetEditorModeManager().GetInteractiveToolsContext()->ToolManager->GetActiveToolName(EToolSide::Left);
			if (ActiveToolName == TEXT("SequencerPivotTool"))
			{
				LevelEditorPtr->GetEditorModeManager().GetInteractiveToolsContext()->ToolManager->DeactivateTool(EToolSide::Left, EToolShutdownType::Completed);
			}
		}
	}
}

void FTLLRN_ControlRigEditMode::Exit()
{
	UE::TLLRN_ControlRig::RemoveTLLRN_ControlRigViewportToolbarExtensions();

	UnregisterPendingFocusMode();
	
	ClearOutAnyActiveTools();
	OnTLLRN_ControlRigAddedOrRemovedDelegate.Clear();
	OnTLLRN_ControlRigSelectedDelegate.Clear();
	for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			TLLRN_ControlRig->ClearControlSelection();
		}
	}

	if (InteractionScopes.Num() >0)
	{

		if (GEditor)
		{
			GEditor->EndTransaction();
		}

		for (TPair<UTLLRN_ControlRig*,FTLLRN_ControlRigInteractionScope*>& InteractionScope : InteractionScopes)
		{
			if (InteractionScope.Value)
			{
				delete InteractionScope.Value;
			}
		}
		InteractionScopes.Reset();
		bManipulatorMadeChange = false;
	}

	if (FTLLRN_ControlRigEditModeToolkit::Details.IsValid())
	{
		FTLLRN_ControlRigEditModeToolkit::Details.Reset();
	}
	if (FTLLRN_ControlRigEditModeToolkit::Outliner.IsValid())
	{
		FTLLRN_ControlRigEditModeToolkit::Outliner.Reset();
	}
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	DestroyShapesActors(nullptr);


	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> PreviousRuntimeRigs = RuntimeTLLRN_ControlRigs;
	for (int32 PreviousRuntimeRigIndex = 0; PreviousRuntimeRigIndex < PreviousRuntimeRigs.Num(); PreviousRuntimeRigIndex++)
	{
		if (PreviousRuntimeRigs[PreviousRuntimeRigIndex].IsValid())
		{
			RemoveTLLRN_ControlRig(PreviousRuntimeRigs[PreviousRuntimeRigIndex].Get());
		}
	}
	RuntimeTLLRN_ControlRigs.Reset();

	//clear delegates
	FEditorModeTools* ModeManager = GetModeManager();
	ModeManager->OnWidgetModeChanged().RemoveAll(this);
	ModeManager->OnCoordSystemChanged().RemoveAll(this);

	//clear proxies
	ControlProxy->RemoveAllProxies();

	//make sure the widget is reset
	ResetControlShapeSize();

	if (const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>())
	{
		Settings->OnSettingsChange.RemoveAll(this);
	}
	
	// Call parent implementation
	FEdMode::Exit();
}

void FTLLRN_ControlRigEditMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);
	
	//if we have don't have a viewport client or viewport, bail we can be in UMG for example
	if (ViewportClient == nullptr || ViewportClient->Viewport == nullptr)
	{
		return;
	}
	CheckMovieSceneSig();

	if(DeferredItemsToFrame.Num() > 0)
	{
		TGuardValue<FEditorViewportClient*> ViewportGuard(CurrentViewportClient, ViewportClient);
		FrameItems(DeferredItemsToFrame);
		DeferredItemsToFrame.Reset();
	}

	if (bSelectionChanged)
	{
		SetUpDetailPanel();
		HandleSelectionChanged();
		bSelectionChanged = false;
	}
	else
	{
		// HandleSelectionChanged() will already update the pivots 
		UpdatePivotTransforms();
	}
	
	if (!AreEditingTLLRN_ControlRigDirectly() == false)
	{
		ViewportClient->Invalidate();
	}

	// Defer creation of shapes if manipulating the viewport
	if (RecreateControlShapesRequired != ERecreateTLLRN_ControlRigShape::RecreateNone && !(FSlateApplication::Get().HasAnyMouseCaptor() || GUnrealEd->IsUserInteracting()))
	{
		RecreateControlShapeActors();
		for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
			{
				TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(TLLRN_ControlRig);
				for (const FTLLRN_RigElementKey& SelectedKey : SelectedTLLRN_RigElements)
				{
					if (SelectedKey.Type == ETLLRN_RigElementType::Control)
					{
						ATLLRN_ControlRigShapeActor* ShapeActor = GetControlShapeFromControlName(TLLRN_ControlRig,SelectedKey.Name);
						if (ShapeActor)
						{
							ShapeActor->SetSelected(true);
						}

						if (!AreEditingTLLRN_ControlRigDirectly())
						{
							FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(SelectedKey.Name);
							if (ControlElement)
							{
								if (!TLLRN_ControlRig->IsCurveControl(ControlElement))
								{
									ControlProxy->AddProxy(TLLRN_ControlRig, ControlElement);
								}
							}
						}
					}
				}
			}
		}
		SetUpDetailPanel();
		HandleSelectionChanged();
		RecreateControlShapesRequired = ERecreateTLLRN_ControlRigShape::RecreateNone;
		TLLRN_ControlRigsToRecreate.SetNum(0);
	}

	{
		// We need to tick here since changing a bone for example
		// might have changed the transform of the Control
		PostPoseUpdate();

		if (!AreEditingTLLRN_ControlRigDirectly() == false) //only do this check if not in level editor
		{
			for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
			{
				if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
				{
					TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(TLLRN_ControlRig);
					UE::Widget::EWidgetMode CurrentWidgetMode = ViewportClient->GetWidgetMode();
					if(!RequestedWidgetModes.IsEmpty())
					{
						if(RequestedWidgetModes.Last() != CurrentWidgetMode)
						{
							CurrentWidgetMode = RequestedWidgetModes.Last();
							ViewportClient->SetWidgetMode(CurrentWidgetMode);
						}
						RequestedWidgetModes.Reset();
					}
					for (FTLLRN_RigElementKey SelectedTLLRN_RigElement : SelectedTLLRN_RigElements)
					{
						//need to loop through the shape actors and set widget based upon the first one
						if (ATLLRN_ControlRigShapeActor* ShapeActor = GetControlShapeFromControlName(TLLRN_ControlRig, SelectedTLLRN_RigElement.Name))
						{
							if (!ModeSupportedByShapeActor(ShapeActor, CurrentWidgetMode))
							{
								if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(SelectedTLLRN_RigElement.Name))
								{
									switch (ControlElement->Settings.ControlType)
									{
									case ETLLRN_RigControlType::Float:
									case ETLLRN_RigControlType::Integer:
									case ETLLRN_RigControlType::Vector2D:
									case ETLLRN_RigControlType::Position:
									case ETLLRN_RigControlType::Transform:
									case ETLLRN_RigControlType::TransformNoScale:
									case ETLLRN_RigControlType::EulerTransform:
									{
										ViewportClient->SetWidgetMode(UE::Widget::WM_Translate);
										break;
									}
									case ETLLRN_RigControlType::Rotator:
									{
										ViewportClient->SetWidgetMode(UE::Widget::WM_Rotate);
										break;
									}
									case ETLLRN_RigControlType::Scale:
									case ETLLRN_RigControlType::ScaleFloat:
									{
										ViewportClient->SetWidgetMode(UE::Widget::WM_Scale);
										break;
									}
									}
									return; //exit if we switchted
								}
							}
						}
					}
				}
			}
		}
	}
	DetailKeyFrameCache->UpdateIfDirty();
}

//Hit proxy for FK Rigs and bones.
struct  HFKTLLRN_RigBoneProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()

	FName BoneName;
	UTLLRN_ControlRig* TLLRN_ControlRig;

	HFKTLLRN_RigBoneProxy()
		: HHitProxy(HPP_Foreground)
		, BoneName(NAME_None)
		, TLLRN_ControlRig(nullptr)
	{}

	HFKTLLRN_RigBoneProxy(FName InBoneName, UTLLRN_ControlRig *InTLLRN_ControlRig)
		: HHitProxy(HPP_Foreground)
		, BoneName(InBoneName)
		, TLLRN_ControlRig(InTLLRN_ControlRig)
	{
	}

	// HHitProxy interface
	virtual EMouseCursor::Type GetMouseCursor() override { return EMouseCursor::Crosshairs; }
	// End of HHitProxy interface
};

IMPLEMENT_HIT_PROXY(HFKTLLRN_RigBoneProxy, HHitProxy)


TSet<FName> FTLLRN_ControlRigEditMode::GetActiveControlsFromSequencer(UTLLRN_ControlRig* TLLRN_ControlRig)
{
	TSet<FName> ActiveControls;
	if (WeakSequencer.IsValid() == false)
	{
		return ActiveControls;
	}
	if (TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding())
	{
		USceneComponent* Component = Cast<USceneComponent>(ObjectBinding->GetBoundObject());
		if (!Component)
		{
			return ActiveControls;
		}
		const bool bCreateHandleIfMissing = false;
		FName CreatedFolderName = NAME_None;
		FGuid ObjectHandle = WeakSequencer.Pin()->GetHandleToObject(Component, bCreateHandleIfMissing);
		if (!ObjectHandle.IsValid())
		{
			UObject* ActorObject = Component->GetOwner();
			ObjectHandle = WeakSequencer.Pin()->GetHandleToObject(ActorObject, bCreateHandleIfMissing);
			if (!ObjectHandle.IsValid())
			{
				return ActiveControls;
			}
		}
		bool bCreateTrack = false;
		UMovieScene* MovieScene = WeakSequencer.Pin()->GetFocusedMovieSceneSequence()->GetMovieScene();
		if (!MovieScene)
		{
			return ActiveControls;
		}
		if (FMovieSceneBinding* Binding = MovieScene->FindBinding(ObjectHandle))
		{
			for (UMovieSceneTrack* Track : Binding->GetTracks())
			{
				if (UMovieSceneTLLRN_ControlRigParameterTrack* TLLRN_ControlRigParameterTrack = Cast<UMovieSceneTLLRN_ControlRigParameterTrack>(Track))
				{
					if (TLLRN_ControlRigParameterTrack->GetTLLRN_ControlRig() == TLLRN_ControlRig)
					{
						TArray<FTLLRN_RigControlElement*> Controls;
						TLLRN_ControlRig->GetControlsInOrder(Controls);
						int Index = 0;
						for (FTLLRN_RigControlElement* ControlElement : Controls)
						{
							UMovieSceneTLLRN_ControlRigParameterSection* ActiveSection = Cast<UMovieSceneTLLRN_ControlRigParameterSection>(TLLRN_ControlRigParameterTrack->GetSectionToKey(ControlElement->GetFName()));
							if (ActiveSection)
							{
								if (ActiveSection->GetControlNameMask(ControlElement->GetFName()))
								{
									ActiveControls.Add(ControlElement->GetFName());
								}
								++Index;
							}
						}
					}
				}
			}
		}
	}
	return ActiveControls;
}


void FTLLRN_ControlRigEditMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	DragToolHandler.Render3DDragTool(View, PDI);

	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
	FEditorViewportClient* EditorViewportClient = (FEditorViewportClient*)Viewport->GetClient();
	const bool bIsInGameView = !AreEditingTLLRN_ControlRigDirectly() ? EditorViewportClient && EditorViewportClient->IsInGameView() : false;
	bool bRender = !Settings->bHideControlShapes;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& TLLRN_ControlRigPtr : RuntimeTLLRN_ControlRigs)
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigPtr.Get();
		//actor game view drawing is handled by not drawing in game via SetActorHiddenInGame().
		if (bRender && TLLRN_ControlRig && TLLRN_ControlRig->GetControlsVisible())
		{
			FTransform ComponentTransform = FTransform::Identity;
			if (!AreEditingTLLRN_ControlRigDirectly())
			{
				ComponentTransform = GetHostingSceneComponentTransform(TLLRN_ControlRig);
			}
			/*
			const TArray<ATLLRN_ControlRigShapeActor*>* ShapeActors = TLLRN_ControlRigShapeActors.Find(TLLRN_ControlRig);
			if (ShapeActors)
			{
				for (ATLLRN_ControlRigShapeActor* Actor : *ShapeActors)
				{
					if (GIsEditor && Actor->GetWorld() != nullptr && !Actor->GetWorld()->IsPlayInEditor())
					{
						Actor->SetIsTemporarilyHiddenInEditor(false);
					}
				}
			}
			*/
			
			//only draw stuff if not in game view
			if (!bIsInGameView)
			{
				UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
				const bool bHasFKRig = (TLLRN_ControlRig->IsA<UAdditiveTLLRN_ControlRig>() || TLLRN_ControlRig->IsA<UFKTLLRN_ControlRig>());
				if (Settings->bDisplayHierarchy || bHasFKRig)
				{
					const bool bBoolSetHitProxies = PDI && PDI->IsHitTesting() && bHasFKRig;
					TSet<FName> ActiveControlName;
					if (bHasFKRig)
					{
						ActiveControlName = GetActiveControlsFromSequencer(TLLRN_ControlRig);
					}
					Hierarchy->ForEach<FTLLRN_RigTransformElement>([PDI, Hierarchy, ComponentTransform, TLLRN_ControlRig, bHasFKRig, bBoolSetHitProxies, ActiveControlName](FTLLRN_RigTransformElement* TransformElement) -> bool
						{
							if(const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(TransformElement))
							{
								if(ControlElement->Settings.AnimationType != ETLLRN_RigControlAnimationType::AnimationControl)
								{
									return true;
								}
							}
						
							const FTransform Transform = Hierarchy->GetTransform(TransformElement, ETLLRN_RigTransformType::CurrentGlobal);

							FTLLRN_RigBaseElementParentArray Parents = Hierarchy->GetParents(TransformElement);
							for (FTLLRN_RigBaseElement* ParentElement : Parents)
							{
								if (FTLLRN_RigTransformElement* ParentTransformElement = Cast<FTLLRN_RigTransformElement>(ParentElement))
								{
									FLinearColor Color = FLinearColor::White;
									if (bHasFKRig)
									{
										FName ControlName = UFKTLLRN_ControlRig::GetControlName(ParentTransformElement->GetFName(), ParentTransformElement->GetType());
										if (ActiveControlName.Num() > 0 && ActiveControlName.Contains(ControlName) == false)
										{
											continue;
										}
										if (TLLRN_ControlRig->IsControlSelected(ControlName))
										{
											Color = FLinearColor::Yellow;
										}
									}
									const FTransform ParentTransform = Hierarchy->GetTransform(ParentTransformElement, ETLLRN_RigTransformType::CurrentGlobal);
									const bool bHitTesting = bBoolSetHitProxies && (ParentTransformElement->GetType() == ETLLRN_RigElementType::Bone);
									if (PDI)
									{
										if (bHitTesting)
										{
											PDI->SetHitProxy(new HFKTLLRN_RigBoneProxy(ParentTransformElement->GetFName(), TLLRN_ControlRig));
										}
										PDI->DrawLine(ComponentTransform.TransformPosition(Transform.GetLocation()), ComponentTransform.TransformPosition(ParentTransform.GetLocation()), Color, SDPG_Foreground);
										if (bHitTesting)
										{
											PDI->SetHitProxy(nullptr);
										}
									}
								}
							}

							FLinearColor Color = FLinearColor::White;
							if (bHasFKRig)
							{
								FName ControlName = UFKTLLRN_ControlRig::GetControlName(TransformElement->GetFName(), TransformElement->GetType());
								if (ActiveControlName.Num() > 0 && ActiveControlName.Contains(ControlName) == false)
								{
									return true;
								}
								if (TLLRN_ControlRig->IsControlSelected(ControlName))
								{
									Color = FLinearColor::Yellow;
								}
							}
							if (PDI)
							{
								const bool bHitTesting = PDI->IsHitTesting() && bBoolSetHitProxies && (TransformElement->GetType() == ETLLRN_RigElementType::Bone);
								if (bHitTesting)
								{
									PDI->SetHitProxy(new HFKTLLRN_RigBoneProxy(TransformElement->GetFName(), TLLRN_ControlRig));
								}
								PDI->DrawPoint(ComponentTransform.TransformPosition(Transform.GetLocation()), Color, 5.0f, SDPG_Foreground);

								if (bHitTesting)
								{
									PDI->SetHitProxy(nullptr);
								}
							}

							return true;
						});
				}

				EWorldType::Type WorldType = Viewport->GetClient()->GetWorld()->WorldType;
				const bool bIsAssetEditor = WorldType == EWorldType::Editor || WorldType == EWorldType::EditorPreview;
				if (bIsAssetEditor && (Settings->bDisplayNulls || TLLRN_ControlRig->IsConstructionModeEnabled()))
				{
					TArray<FTransform> SpaceTransforms;
					TArray<FTransform> SelectedSpaceTransforms;
					Hierarchy->ForEach<FTLLRN_RigNullElement>([&SpaceTransforms, &SelectedSpaceTransforms, Hierarchy](FTLLRN_RigNullElement* NullElement) -> bool
						{
							if (Hierarchy->IsSelected(NullElement->GetIndex()))
							{
								SelectedSpaceTransforms.Add(Hierarchy->GetTransform(NullElement, ETLLRN_RigTransformType::CurrentGlobal));
							}
							else
							{
								SpaceTransforms.Add(Hierarchy->GetTransform(NullElement, ETLLRN_RigTransformType::CurrentGlobal));
							}
							return true;
						});

					TLLRN_ControlRig->DrawInterface.DrawAxes(FTransform::Identity, SpaceTransforms, Settings->AxisScale);
					TLLRN_ControlRig->DrawInterface.DrawAxes(FTransform::Identity, SelectedSpaceTransforms, FLinearColor(1.0f, 0.34f, 0.0f, 1.0f), Settings->AxisScale);
				}

				if (bIsAssetEditor && (Settings->bDisplayAxesOnSelection && Settings->AxisScale > SMALL_NUMBER))
				{
					TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(TLLRN_ControlRig);
					const float Scale = Settings->AxisScale;
					PDI->AddReserveLines(SDPG_Foreground, SelectedTLLRN_RigElements.Num() * 3);

					for (const FTLLRN_RigElementKey& SelectedElement : SelectedTLLRN_RigElements)
					{
						FTransform ElementTransform = Hierarchy->GetGlobalTransform(SelectedElement);
						ElementTransform = ElementTransform * ComponentTransform;

						PDI->DrawLine(ElementTransform.GetTranslation(), ElementTransform.TransformPosition(FVector(Scale, 0.f, 0.f)), FLinearColor::Red, SDPG_Foreground);
						PDI->DrawLine(ElementTransform.GetTranslation(), ElementTransform.TransformPosition(FVector(0.f, Scale, 0.f)), FLinearColor::Green, SDPG_Foreground);
						PDI->DrawLine(ElementTransform.GetTranslation(), ElementTransform.TransformPosition(FVector(0.f, 0.f, Scale)), FLinearColor::Blue, SDPG_Foreground);
					}
				}

				// temporary implementation to draw sockets in 3D
				if (bIsAssetEditor && (Settings->bDisplaySockets || TLLRN_ControlRig->IsConstructionModeEnabled()) && Settings->AxisScale > SMALL_NUMBER)
				{
					const float Scale = Settings->AxisScale;
					PDI->AddReserveLines(SDPG_Foreground, Hierarchy->Num(ETLLRN_RigElementType::Socket) * 3);
					static const FLinearColor SocketColor = FTLLRN_ControlRigEditorStyle::Get().SocketUserInterfaceColor;

					Hierarchy->ForEach<FTLLRN_RigSocketElement>([this, Hierarchy, PDI, ComponentTransform, Scale](FTLLRN_RigSocketElement* Socket)
					{
						FTransform ElementTransform = Hierarchy->GetGlobalTransform(Socket->GetIndex());
						ElementTransform = ElementTransform * ComponentTransform;

						PDI->DrawLine(ElementTransform.GetTranslation(), ElementTransform.TransformPosition(FVector(Scale, 0.f, 0.f)), SocketColor, SDPG_Foreground);
						PDI->DrawLine(ElementTransform.GetTranslation(), ElementTransform.TransformPosition(FVector(0.f, Scale, 0.f)), SocketColor, SDPG_Foreground);
						PDI->DrawLine(ElementTransform.GetTranslation(), ElementTransform.TransformPosition(FVector(0.f, 0.f, Scale)), SocketColor, SDPG_Foreground);

						return true;
					});
				}
				TLLRN_ControlRig->DrawIntoPDI(PDI, ComponentTransform);
			}
		}
	}
}

void FTLLRN_ControlRigEditMode::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	IPersonaEditMode::DrawHUD(ViewportClient, Viewport, View, Canvas);
	DragToolHandler.RenderDragTool(View, Canvas);
}

bool FTLLRN_ControlRigEditMode::InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent)
{
	if (InEvent != IE_Released)
	{
		TGuardValue<FEditorViewportClient*> ViewportGuard(CurrentViewportClient, InViewportClient);

		FModifierKeysState KeyState = FSlateApplication::Get().GetModifierKeys();
		if (CommandBindings->ProcessCommandBindings(InKey, KeyState, (InEvent == IE_Repeat)))
		{
			return true;
		}
		if (IsDragAnimSliderToolPressed(InViewport)) //this is needed to make sure we get all of the processed mouse events, for some reason the above may not return true
		{
			return true;
		}
	}

	return FEdMode::InputKey(InViewportClient, InViewport, InKey, InEvent);
}

bool FTLLRN_ControlRigEditMode::ProcessCapturedMouseMoves(FEditorViewportClient* InViewportClient, FViewport* InViewport, const TArrayView<FIntPoint>& CapturedMouseMoves)
{
	const bool bChange = IsDragAnimSliderToolPressed(InViewport);
	if (CapturedMouseMoves.Num() > 0)
	{
		if (bChange)
		{
			for (int32 Index = 0; Index < CapturedMouseMoves.Num(); ++Index)
			{
				int32 X = CapturedMouseMoves[Index].X;
				if (StartXValue.IsSet() == false)
				{
					StartAnimSliderTool(X);
				}
				else
				{
					int32 Diff = X - StartXValue.GetValue();
					if (Diff != 0)
					{
						FIntPoint Origin, Size;
						InViewportClient->GetViewportDimensions(Origin, Size);
						const double ViewPortSize = (double)Size.X * 0.125; // 1/8 screen drag should do full blend, todo perhaps expose this as a sensitivity
						const double ViewDiff = (double)(Diff) / ViewPortSize;
						DragAnimSliderTool(ViewDiff);
					}
				}
			}
		}
		else if (bisTrackingAnimToolDrag && bChange == false)
		{
			ResetAnimSlider();
		}
		return bisTrackingAnimToolDrag;
	}
	else if(bisTrackingAnimToolDrag && bChange == false)
	{
		ResetAnimSlider();
	}
	return false;
}

bool FTLLRN_ControlRigEditMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if(RuntimeTLLRN_ControlRigs.IsEmpty())
	{
		return false;
	}
	
	//break right out if doing anim slider scrub

	if (IsDragAnimSliderToolPressed(InViewport))
	{
		return true;
	}

	if (IsMovingCamera(InViewport))
	{
		InViewportClient->SetCurrentWidgetAxis(EAxisList::None);
		return true;
	}
	if (IsDoingDrag(InViewport))
	{
		DragToolHandler.MakeDragTool(InViewportClient);
		return DragToolHandler.StartTracking(InViewportClient, InViewport);
	}

	return HandleBeginTransform(InViewportClient);
}

bool FTLLRN_ControlRigEditMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if(RuntimeTLLRN_ControlRigs.IsEmpty())
	{
		return false;
	}

	if (bisTrackingAnimToolDrag)
	{
		ResetAnimSlider();
	}
	if (IsDragAnimSliderToolPressed(InViewport))
	{
		return true;
	}

	if (IsMovingCamera(InViewport))
	{
		return true;
	}
	if (DragToolHandler.EndTracking(InViewportClient, InViewport))
	{
		return true;
	}

	return HandleEndTransform(InViewportClient);
}

bool FTLLRN_ControlRigEditMode::BeginTransform(const FGizmoState& InState)
{
	return HandleBeginTransform(Owner->GetFocusedViewportClient());
}

bool FTLLRN_ControlRigEditMode::EndTransform(const FGizmoState& InState)
{
	return HandleEndTransform(Owner->GetFocusedViewportClient());
}

bool FTLLRN_ControlRigEditMode::HandleBeginTransform(const FEditorViewportClient* InViewportClient)
{
	if (!InViewportClient)
	{
		return false;
	}
	
	InteractionType = GetInteractionType(InViewportClient);
	bIsTracking = true;
	
	if (InteractionScopes.Num() == 0)
	{
		const bool bShouldModify = [this]() -> bool
		{
			if (AreEditingTLLRN_ControlRigDirectly())
			{
				for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
				{
					if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
					{
						TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(TLLRN_ControlRig);
						for (const FTLLRN_RigElementKey& Key : SelectedTLLRN_RigElements)
						{
							if (Key.Type != ETLLRN_RigElementType::Control)
							{
								return true;
							}
						}
					}
				}
			}
			
			return !AreEditingTLLRN_ControlRigDirectly();
		}();

		if (AreEditingTLLRN_ControlRigDirectly())
		{
			for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
			{
				if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
				{
					UObject* Blueprint = TLLRN_ControlRig->GetClass()->ClassGeneratedBy;
					if (Blueprint)
					{
						Blueprint->SetFlags(RF_Transactional);
						if (bShouldModify)
						{
							Blueprint->Modify();
						}
					}
					TLLRN_ControlRig->SetFlags(RF_Transactional);
					if (bShouldModify)
					{
						TLLRN_ControlRig->Modify();
					}
				}
			}
		}

	}

	//in level editor only transact if we have at least one control selected, in editor we only select CR stuff so always transact

	if (!AreEditingTLLRN_ControlRigDirectly())
	{
		for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
			{
				if (AreTLLRN_RigElementSelectedAndMovable(TLLRN_ControlRig))
				{
					//todo need to add multiple
					FTLLRN_ControlRigInteractionScope* InteractionScope = new FTLLRN_ControlRigInteractionScope(TLLRN_ControlRig);
					InteractionScopes.Add(TLLRN_ControlRig,InteractionScope);
					TLLRN_ControlRig->bInteractionJustBegan = true;
				}
				else
				{
					bManipulatorMadeChange = false;
				}
			}
		}
	}
	else if(UTLLRN_ControlRigEditorSettings::Get()->bEnableUndoForPoseInteraction)
	{
		UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeTLLRN_ControlRigs[0].Get();
		FTLLRN_ControlRigInteractionScope* InteractionScope = new FTLLRN_ControlRigInteractionScope(TLLRN_ControlRig);
		InteractionScopes.Add(TLLRN_ControlRig,InteractionScope);
	}
	else
	{
		bManipulatorMadeChange = false;
	}
	return InteractionScopes.Num() != 0;
}

bool FTLLRN_ControlRigEditMode::HandleEndTransform(FEditorViewportClient* InViewportClient)
{
	if (!InViewportClient)
	{
		return false;
	}
	
	const bool bWasInteracting = bManipulatorMadeChange && InteractionType != (uint8)ETLLRN_ControlRigInteractionType::None;
	
	InteractionType = (uint8)ETLLRN_ControlRigInteractionType::None;
	bIsTracking = false;
	
	if (InteractionScopes.Num() > 0)
	{		
		if (bManipulatorMadeChange)
		{
			bManipulatorMadeChange = false;
			GEditor->EndTransaction();
		}

		for (TPair<UTLLRN_ControlRig*, FTLLRN_ControlRigInteractionScope*>& InteractionScope : InteractionScopes)
		{
			if (InteractionScope.Value)
			{
				delete InteractionScope.Value; 
			}
		}
		InteractionScopes.Reset();

		if (bWasInteracting && !AreEditingTLLRN_ControlRigDirectly())
		{
			// We invalidate the hit proxies when in level editor to ensure that the gizmo's hit proxy is up to date.
			// The invalidation is called here to avoid useless viewport update in the FTLLRN_ControlRigEditMode::Tick
			// function (that does an update when not in level editor)
			TickManipulatableObjects(0.f);
			
			static constexpr bool bInvalidateChildViews = false;
			static constexpr bool bInvalidateHitProxies = true;
			InViewportClient->Invalidate(bInvalidateChildViews, bInvalidateHitProxies);
		}
		
		return true;
	}

	bManipulatorMadeChange = false;
	
	return false;
}

bool FTLLRN_ControlRigEditMode::UsesTransformWidget() const
{
	for (const auto& Pairs : TLLRN_ControlRigShapeActors)
	{
		for (const ATLLRN_ControlRigShapeActor* ShapeActor : Pairs.Value)
		{
			if (ShapeActor->IsSelected())
			{
				return true;
			}
		}
		if (AreTLLRN_RigElementSelectedAndMovable(Pairs.Key))
		{
			return true;
		}
	}
	return FEdMode::UsesTransformWidget();
}

bool FTLLRN_ControlRigEditMode::UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const
{
	for (const auto& Pairs : TLLRN_ControlRigShapeActors)
	{
		for (const ATLLRN_ControlRigShapeActor* ShapeActor : Pairs.Value)
		{
			if (ShapeActor->IsSelected())
			{
				return ModeSupportedByShapeActor(ShapeActor, CheckMode);
			}
		}
		if (AreTLLRN_RigElementSelectedAndMovable(Pairs.Key))
		{
			return true;
		}
	}
	return FEdMode::UsesTransformWidget(CheckMode);
}

FVector FTLLRN_ControlRigEditMode::GetWidgetLocation() const
{
	FVector PivotLocation(0.0, 0.0, 0.0);
	int NumSelected = 0;
	for (const auto& Pairs : TLLRN_ControlRigShapeActors)
	{
		if (AreTLLRN_RigElementSelectedAndMovable(Pairs.Key))
		{
			if (const FTransform* PivotTransform = PivotTransforms.Find(Pairs.Key))
			{
				// check that the cached pivot is up-to-date and update it if needed
				FTransform Transform = *PivotTransform;
				UpdatePivotTransformsIfNeeded(Pairs.Key, Transform);
				const FTransform ComponentTransform = GetHostingSceneComponentTransform(Pairs.Key);
				PivotLocation += ComponentTransform.TransformPosition(Transform.GetLocation());
				++NumSelected;
			}
		}
	}	
	if (NumSelected > 0)
	{
		PivotLocation /= (NumSelected);
		return PivotLocation;
	}

	return FEdMode::GetWidgetLocation();
}

bool FTLLRN_ControlRigEditMode::GetPivotForOrbit(FVector& OutPivot) const
{
	if (UsesTransformWidget())
	{
		OutPivot = GetWidgetLocation();
		return true;
	}
	return FEdMode::GetPivotForOrbit(OutPivot);
}

bool FTLLRN_ControlRigEditMode::GetCustomDrawingCoordinateSystem(FMatrix& OutMatrix, void* InData)
{
	//since we strip translation just want the first one
	for (const auto& Pairs : TLLRN_ControlRigShapeActors)
	{
		if (AreTLLRN_RigElementSelectedAndMovable(Pairs.Key))
		{
			if (const FTransform* PivotTransform = PivotTransforms.Find(Pairs.Key))
			{
				// check that the cached pivot is up-to-date and update it if needed
				FTransform Transform = *PivotTransform;
				UpdatePivotTransformsIfNeeded(Pairs.Key, Transform);
				OutMatrix = Transform.ToMatrixNoScale().RemoveTranslation();
				return true;
			}
		}
	}
	return false;
}

bool FTLLRN_ControlRigEditMode::GetCustomInputCoordinateSystem(FMatrix& OutMatrix, void* InData)
{
	return GetCustomDrawingCoordinateSystem(OutMatrix, InData);
}

bool FTLLRN_ControlRigEditMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click)
{
	const bool bClickSelectThroughGizmo = CVarClickSelectThroughGizmo.GetValueOnGameThread();
	//if Control is down we act like we are selecting an axis so don't do this check
	//if doing control else we can't do control selection anymore, see FMouseDeltaTracker::DetermineCurrentAxis(
	if (Click.IsControlDown() == false && bClickSelectThroughGizmo == false)
	{
		const EAxisList::Type CurrentAxis = InViewportClient->GetCurrentWidgetAxis();
		//if we are hitting a widget, besides arcball then bail saying we are handling it
		if (CurrentAxis != EAxisList::None)
		{
			return true;
		}
	}

	InteractionType = GetInteractionType(InViewportClient);

	if(HActor* ActorHitProxy = HitProxyCast<HActor>(HitProxy))
	{
		if(ActorHitProxy->Actor)
		{
			if (ActorHitProxy->Actor->IsA<ATLLRN_ControlRigShapeActor>())
			{
				ATLLRN_ControlRigShapeActor* ShapeActor = CastChecked<ATLLRN_ControlRigShapeActor>(ActorHitProxy->Actor);
				if (ShapeActor->IsSelectable() && ShapeActor->TLLRN_ControlRig.IsValid())
				{
					FScopedTransaction ScopedTransaction(LOCTEXT("SelectControlTransaction", "Select Control"), !AreEditingTLLRN_ControlRigDirectly() && !GIsTransacting);

					// temporarily disable the interaction scope
					FTLLRN_ControlRigInteractionScope** InteractionScope = InteractionScopes.Find(ShapeActor->TLLRN_ControlRig.Get());
					const bool bInteractionScopePresent = InteractionScope != nullptr;
					if (bInteractionScopePresent)
					{
						delete* InteractionScope;
						InteractionScopes.Remove(ShapeActor->TLLRN_ControlRig.Get());
					}
					
					const FName& ControlName = ShapeActor->ControlName;
					if (Click.IsShiftDown()) //guess we just select
					{
						SetTLLRN_RigElementSelection(ShapeActor->TLLRN_ControlRig.Get(),ETLLRN_RigElementType::Control, ControlName, true);
					}
					else if(Click.IsControlDown()) //if ctrl we toggle selection
					{
						if (UTLLRN_ControlRig* TLLRN_ControlRig = ShapeActor->TLLRN_ControlRig.Get())
						{
							bool bIsSelected = TLLRN_ControlRig->IsControlSelected(ControlName);
							SetTLLRN_RigElementSelection(TLLRN_ControlRig, ETLLRN_RigElementType::Control, ControlName, !bIsSelected);
						}
					}
					else
					{
						//also need to clear actor selection. Sequencer will handle this automatically if done in Sequencder UI but not if done by clicking
						if (!AreEditingTLLRN_ControlRigDirectly())
						{
							if (GEditor && GEditor->GetSelectedActorCount())
							{
								GEditor->SelectNone(false, true);
								GEditor->NoteSelectionChange();
							}
							//also need to clear explicitly in sequencer
							if (WeakSequencer.IsValid())
							{
								if (ISequencer* SequencerPtr = WeakSequencer.Pin().Get())
								{
									SequencerPtr->GetViewModel()->GetSelection()->Empty();
								}
							}
						}
						ClearTLLRN_RigElementSelection(ValidControlTypeMask());
						SetTLLRN_RigElementSelection(ShapeActor->TLLRN_ControlRig.Get(),ETLLRN_RigElementType::Control, ControlName, true);
					}

					if (bInteractionScopePresent)
					{
						*InteractionScope = new FTLLRN_ControlRigInteractionScope(ShapeActor->TLLRN_ControlRig.Get());
						InteractionScopes.Add(ShapeActor->TLLRN_ControlRig.Get(), *InteractionScope);

					}

					// for now we show this menu all the time if body is selected
					// if we want some global menu, we'll have to move this
					if (Click.GetKey() == EKeys::RightMouseButton)
					{
						OpenContextMenu(InViewportClient);
					}
	
					return true;
				}

				return true;
			}
			else 
			{ 
				for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
				{
					if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
					{

						//if we have an additive or fk control rig active select the control based upon the selected bone.
						UAdditiveTLLRN_ControlRig* AdditiveTLLRN_ControlRig = Cast<UAdditiveTLLRN_ControlRig>(TLLRN_ControlRig);
						UFKTLLRN_ControlRig* FKTLLRN_ControlRig = Cast<UFKTLLRN_ControlRig>(TLLRN_ControlRig);

						if ((AdditiveTLLRN_ControlRig || FKTLLRN_ControlRig) && TLLRN_ControlRig->GetObjectBinding().IsValid())
						{
							if (USkeletalMeshComponent* RigMeshComp = Cast<USkeletalMeshComponent>(TLLRN_ControlRig->GetObjectBinding()->GetBoundObject()))
							{
								const USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(ActorHitProxy->PrimComponent);

								if (SkelComp == RigMeshComp)
								{
									FHitResult Result(1.0f);
									bool bHit = RigMeshComp->LineTraceComponent(Result, Click.GetOrigin(), Click.GetOrigin() + Click.GetDirection() * TLLRN_ControlRigSelectionConstants::BodyTraceDistance, FCollisionQueryParams(NAME_None, FCollisionQueryParams::GetUnknownStatId(), true));

									if (bHit)
									{
										FName ControlName(*(Result.BoneName.ToString() + TEXT("_CONTROL")));
										if (TLLRN_ControlRig->FindControl(ControlName))
										{
											FScopedTransaction ScopedTransaction(LOCTEXT("SelectControlTransaction", "Select Control"), !AreEditingTLLRN_ControlRigDirectly() && !GIsTransacting);

											if (Click.IsShiftDown()) //guess we just select
											{
												SetTLLRN_RigElementSelection(TLLRN_ControlRig,ETLLRN_RigElementType::Control, ControlName, true);
											}
											else if (Click.IsControlDown()) //if ctrl we toggle selection
											{
												bool bIsSelected = TLLRN_ControlRig->IsControlSelected(ControlName);
												SetTLLRN_RigElementSelection(TLLRN_ControlRig, ETLLRN_RigElementType::Control, ControlName, !bIsSelected);
											}
											else
											{
												ClearTLLRN_RigElementSelection(ValidControlTypeMask());
												SetTLLRN_RigElementSelection(TLLRN_ControlRig,ETLLRN_RigElementType::Control, ControlName, true);
											}
											return true;
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
	else if (HFKTLLRN_RigBoneProxy* FKBoneProxy = HitProxyCast<HFKTLLRN_RigBoneProxy>(HitProxy))
	{
		FName ControlName(*(FKBoneProxy->BoneName.ToString() + TEXT("_CONTROL")));
		if (FKBoneProxy->TLLRN_ControlRig->FindControl(ControlName))
		{
			FScopedTransaction ScopedTransaction(LOCTEXT("SelectControlTransaction", "Select Control"), !AreEditingTLLRN_ControlRigDirectly() && !GIsTransacting);

			if (Click.IsShiftDown()) //guess we just select
			{
				SetTLLRN_RigElementSelection(FKBoneProxy->TLLRN_ControlRig,ETLLRN_RigElementType::Control, ControlName, true);
			}
			else if (Click.IsControlDown()) //if ctrl we toggle selection
			{
				for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
				{
					if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
					{
						bool bIsSelected = TLLRN_ControlRig->IsControlSelected(ControlName);
						SetTLLRN_RigElementSelection(FKBoneProxy->TLLRN_ControlRig, ETLLRN_RigElementType::Control, ControlName, !bIsSelected);
					}
				}
			}
			else
			{
				ClearTLLRN_RigElementSelection(ValidControlTypeMask());
				SetTLLRN_RigElementSelection(FKBoneProxy->TLLRN_ControlRig,ETLLRN_RigElementType::Control, ControlName, true);
			}
			return true;
		}
	}
	else if (HPersonaBoneHitProxy* BoneHitProxy = HitProxyCast<HPersonaBoneHitProxy>(HitProxy))
	{
		if (RuntimeTLLRN_ControlRigs.Num() > 0)
		{
			if (UTLLRN_ControlRig* DebuggedTLLRN_ControlRig = RuntimeTLLRN_ControlRigs[0].Get())
			{
				UTLLRN_RigHierarchy* Hierarchy = DebuggedTLLRN_ControlRig->GetHierarchy();

				// Cache mapping?
				for (int32 Index = 0; Index < Hierarchy->Num(); Index++)
				{
					const FTLLRN_RigElementKey ElementToSelect = Hierarchy->GetKey(Index);
					if (ElementToSelect.Type == ETLLRN_RigElementType::Bone && ElementToSelect.Name == BoneHitProxy->BoneName)
					{
						if (FSlateApplication::Get().GetModifierKeys().IsShiftDown())
						{
							Hierarchy->GetController()->SelectElement(ElementToSelect, true);
						}
						else if (FSlateApplication::Get().GetModifierKeys().IsControlDown())
						{
							const bool bSelect = !Hierarchy->IsSelected(ElementToSelect);
							Hierarchy->GetController()->SelectElement(ElementToSelect, bSelect);
						}
						else
						{
							TArray<FTLLRN_RigElementKey> NewSelection;
							NewSelection.Add(ElementToSelect);
							Hierarchy->GetController()->SetSelection(NewSelection);
						}
						return true;
					}
				}
			}
		}
	}
	else
	{
		InteractionType = (uint8)ETLLRN_ControlRigInteractionType::None;
	}

	// for now we show this menu all the time if body is selected
	// if we want some global menu, we'll have to move this
	if (Click.GetKey() == EKeys::RightMouseButton)
	{
		OpenContextMenu(InViewportClient);
		return true;
	}

	
	// clear selected controls
	if (Click.IsShiftDown() ==false && Click.IsControlDown() == false)
	{
		FScopedTransaction ScopedTransaction(LOCTEXT("SelectControlTransaction", "Select Control"), !AreEditingTLLRN_ControlRigDirectly() && !GIsTransacting);
		ClearTLLRN_RigElementSelection(FTLLRN_RigElementTypeHelper::ToMask(ETLLRN_RigElementType::All));
	}

	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();

	if (Settings && Settings->bOnlySelectTLLRN_RigControls)
	{
		return true;
	}
	/*
	if(!InViewportClient->IsLevelEditorClient() && !InViewportClient->IsSimulateInEditorViewport())
	{
		bool bHandled = false;
		const bool bSelectingSections = GetAnimPreviewScene().AllowMeshHitProxies();

		USkeletalMeshComponent* MeshComponent = GetAnimPreviewScene().GetPreviewMeshComponent();

		if ( HitProxy )
		{
			if ( HitProxy->IsA( HPersonaBoneProxy::StaticGetType() ) )
			{			
				SetTLLRN_RigElementSelection(ETLLRN_RigElementType::Bone, static_cast<HPersonaBoneProxy*>(HitProxy)->BoneName, true);
				bHandled = true;
			}
		}
		
		if ( !bHandled && !bSelectingSections )
		{
			// Cast for phys bodies if we didn't get any hit proxies
			FHitResult Result(1.0f);
			UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene().GetPreviewMeshComponent();
			bool bHit = PreviewMeshComponent->LineTraceComponent(Result, Click.GetOrigin(), Click.GetOrigin() + Click.GetDirection() * 10000.0f, FCollisionQueryParams(NAME_None, FCollisionQueryParams::GetUnknownStatId(),true));
			
			if(bHit)
			{
				SetTLLRN_RigElementSelection(ETLLRN_RigElementType::Bone, Result.BoneName, true);
				bHandled = true;
			}
		}
	}
	*/
	
	return FEdMode::HandleClick(InViewportClient, HitProxy, Click);
}

void FTLLRN_ControlRigEditMode::OpenContextMenu(FEditorViewportClient* InViewportClient)
{
	TSharedPtr<FUICommandList> Commands = CommandBindings;
	if (OnContextMenuCommandsDelegate.IsBound())
	{
		Commands = OnContextMenuCommandsDelegate.Execute();
	}

	if (OnGetContextMenuDelegate.IsBound())
	{
		TSharedPtr<SWidget> MenuWidget = SNullWidget::NullWidget;
		
		if (UToolMenu* ContextMenu = OnGetContextMenuDelegate.Execute())
		{
			UToolMenus* ToolMenus = UToolMenus::Get();
			MenuWidget = ToolMenus->GenerateWidget(ContextMenu);
		}

		TSharedPtr<SWidget> ParentWidget = InViewportClient->GetEditorViewportWidget();

		if (MenuWidget.IsValid() && ParentWidget.IsValid())
		{
			const FVector2D MouseCursorLocation = FSlateApplication::Get().GetCursorPos();

			FSlateApplication::Get().PushMenu(
				ParentWidget.ToSharedRef(),
				FWidgetPath(),
				MenuWidget.ToSharedRef(),
				MouseCursorLocation,
				FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
			);
		}
	}
}

bool FTLLRN_ControlRigEditMode::IntersectSelect(bool InSelect, const TFunctionRef<bool(const ATLLRN_ControlRigShapeActor*, const FTransform&)>& Intersects)
{
	bool bSelected = false;

	for (auto& Pairs : TLLRN_ControlRigShapeActors)
	{
		FTransform ComponentTransform = GetHostingSceneComponentTransform(Pairs.Key);
		for (ATLLRN_ControlRigShapeActor* ShapeActor : Pairs.Value)
		{
			if (ShapeActor->IsHiddenEd())
			{
				continue;
			}

			const FTransform ControlTransform = ShapeActor->GetGlobalTransform() * ComponentTransform;
			if (Intersects(ShapeActor, ControlTransform))
			{
				SetTLLRN_RigElementSelection(Pairs.Key,ETLLRN_RigElementType::Control, ShapeActor->ControlName, InSelect);
				bSelected = true;
			}
		}
	}

	return bSelected;
}

static FConvexVolume GetVolumeFromBox(const FBox& InBox)
{
	FConvexVolume ConvexVolume;
	ConvexVolume.Planes.Empty(6);

	ConvexVolume.Planes.Add(FPlane(FVector::LeftVector, -InBox.Min.Y));
	ConvexVolume.Planes.Add(FPlane(FVector::RightVector, InBox.Max.Y));
	ConvexVolume.Planes.Add(FPlane(FVector::UpVector, InBox.Max.Z));
	ConvexVolume.Planes.Add(FPlane(FVector::DownVector, -InBox.Min.Z));
	ConvexVolume.Planes.Add(FPlane(FVector::ForwardVector, InBox.Max.X));
	ConvexVolume.Planes.Add(FPlane(FVector::BackwardVector, -InBox.Min.X));

	ConvexVolume.Init();

	return ConvexVolume;
}

bool IntersectsBox( AActor& InActor, const FBox& InBox, FLevelEditorViewportClient* LevelViewportClient, bool bUseStrictSelection )
{
	bool bActorHitByBox = false;
	if (InActor.IsHiddenEd())
	{
		return false;
	}

	const TArray<FName>& HiddenLayers = LevelViewportClient->ViewHiddenLayers;
	bool bActorIsVisible = true;
	for ( auto Layer : InActor.Layers )
	{
		// Check the actor isn't in one of the layers hidden from this viewport.
		if( HiddenLayers.Contains( Layer ) )
		{
			return false;
		}
	}

	// Iterate over all actor components, selecting out primitive components
	for (UActorComponent* Component : InActor.GetComponents())
	{
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
		if (PrimitiveComponent && PrimitiveComponent->IsRegistered() && PrimitiveComponent->IsVisibleInEditor())
		{
			if (PrimitiveComponent->IsShown(LevelViewportClient->EngineShowFlags) && PrimitiveComponent->ComponentIsTouchingSelectionBox(InBox, false, bUseStrictSelection))
			{
				return true;
			}
		}
	}
	
	return false;
}

bool FTLLRN_ControlRigEditMode::BoxSelect(FBox& InBox, bool InSelect)
{

	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
	FLevelEditorViewportClient* LevelViewportClient = GCurrentLevelEditingViewportClient;
	if (LevelViewportClient->IsInGameView() == true || Settings->bHideControlShapes)
	{
		return  FEdMode::BoxSelect(InBox, InSelect);
	}
	const bool bStrictDragSelection = GetDefault<ULevelEditorViewportSettings>()->bStrictBoxSelection;

	FScopedTransaction ScopedTransaction(LOCTEXT("SelectControlTransaction", "Select Control"), !AreEditingTLLRN_ControlRigDirectly() && !GIsTransacting);
	const bool bShiftDown = LevelViewportClient->Viewport->KeyState(EKeys::LeftShift) || LevelViewportClient->Viewport->KeyState(EKeys::RightShift);
	if (!bShiftDown)
	{
		ClearTLLRN_RigElementSelection(ValidControlTypeMask());
	}

	// Select all actors that are within the selection box area.  Be aware that certain modes do special processing below.	
	bool bSomethingSelected = false;
	UWorld* IteratorWorld = GWorld;
	for( FActorIterator It(IteratorWorld); It; ++It )
	{
		AActor* Actor = *It;

		if (!Actor->IsA<ATLLRN_ControlRigShapeActor>())
		{
			continue;
		}

		ATLLRN_ControlRigShapeActor* ShapeActor = CastChecked<ATLLRN_ControlRigShapeActor>(Actor);
		if (!ShapeActor->IsSelectable() || ShapeActor->TLLRN_ControlRig.IsValid () == false || ShapeActor->TLLRN_ControlRig->GetControlsVisible() == false)
		{
			continue;
		}

		if (IntersectsBox(*Actor, InBox, LevelViewportClient, bStrictDragSelection))
		{
			bSomethingSelected = true;
			const FName& ControlName = ShapeActor->ControlName;
			SetTLLRN_RigElementSelection(ShapeActor->TLLRN_ControlRig.Get(),ETLLRN_RigElementType::Control, ControlName, true);

			if (bShiftDown)
			{
			}
			else
			{
				SetTLLRN_RigElementSelection(ShapeActor->TLLRN_ControlRig.Get(),ETLLRN_RigElementType::Control, ControlName, true);
			}
		}
	}
	if (bSomethingSelected == true)
	{
		return true;
	}	
	ScopedTransaction.Cancel();
	//if only selecting controls return true to stop any more selections
	if (Settings && Settings->bOnlySelectTLLRN_RigControls)
	{
		return true;
	}
	return FEdMode::BoxSelect(InBox, InSelect);
}

bool FTLLRN_ControlRigEditMode::FrustumSelect(const FConvexVolume& InFrustum, FEditorViewportClient* InViewportClient, bool InSelect)
{
	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
	if (!Settings)
	{
		return false;
	}
	
	//need to check for a zero frustum since ComponentIsTouchingSelectionFrustum will return true, selecting everything, when this is the case
	// cf. FDragTool_ActorFrustumSelect::CalculateFrustum 
	const bool bAreTopBottomMalformed = InFrustum.Planes[0].IsNearlyZero() && InFrustum.Planes[2].IsNearlyZero();
	const bool bAreRightLeftMalformed = InFrustum.Planes[1].IsNearlyZero() && InFrustum.Planes[3].IsNearlyZero();
	const bool bMalformedFrustum = bAreTopBottomMalformed || bAreRightLeftMalformed;
	if (bMalformedFrustum || InViewportClient->IsInGameView() == true || Settings->bHideControlShapes)
	{
		return Settings->bOnlySelectTLLRN_RigControls;
	}

	FScopedTransaction ScopedTransaction(LOCTEXT("SelectControlTransaction", "Select Control"), !AreEditingTLLRN_ControlRigDirectly() && !GIsTransacting);
	bool bSomethingSelected(false);
	const bool bShiftDown = InViewportClient->Viewport->KeyState(EKeys::LeftShift) || InViewportClient->Viewport->KeyState(EKeys::RightShift);
	if (!bShiftDown)
	{
		ClearTLLRN_RigElementSelection(ValidControlTypeMask());
	}

	for (auto& Pairs : TLLRN_ControlRigShapeActors)
	{
		for (ATLLRN_ControlRigShapeActor* ShapeActor : Pairs.Value)
		{
			const bool bTreatShape = ShapeActor && ShapeActor->TLLRN_ControlRig.IsValid() && ShapeActor->TLLRN_ControlRig->GetControlsVisible() &&
				ShapeActor->IsSelectable() && !ShapeActor->IsTemporarilyHiddenInEditor();
			if (bTreatShape)
			{
				for (UActorComponent* Component : ShapeActor->GetComponents())
				{
					UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
					if (PrimitiveComponent && PrimitiveComponent->IsRegistered() && PrimitiveComponent->IsVisibleInEditor())
					{
						if (PrimitiveComponent->IsShown(InViewportClient->EngineShowFlags) && PrimitiveComponent->ComponentIsTouchingSelectionFrustum(InFrustum, false /*only bsp*/, false/*encompass entire*/))
						{
							bSomethingSelected = true;
							const FName& ControlName = ShapeActor->ControlName;
							SetTLLRN_RigElementSelection(Pairs.Key,ETLLRN_RigElementType::Control, ControlName, true);
						}
					}
				}
			}
		}
	}

	EWorldType::Type WorldType = InViewportClient->GetWorld()->WorldType;
	const bool bIsAssetEditor =
		(WorldType == EWorldType::Editor || WorldType == EWorldType::EditorPreview) &&
			!InViewportClient->IsLevelEditorClient();

	if (bIsAssetEditor)
	{
		float BoneRadius = 1;
		EBoneDrawMode::Type BoneDrawMode = EBoneDrawMode::None;
		if (const FAnimationViewportClient* AnimViewportClient = static_cast<FAnimationViewportClient*>(InViewportClient))
		{
			BoneDrawMode = AnimViewportClient->GetBoneDrawMode();
			BoneRadius = AnimViewportClient->GetBoneDrawSize();
		}

		if(BoneDrawMode != EBoneDrawMode::None)
		{
			for(TWeakObjectPtr<UTLLRN_ControlRig> WeakTLLRN_ControlRig : RuntimeTLLRN_ControlRigs)
			{
				if(UTLLRN_ControlRig* TLLRN_ControlRig = WeakTLLRN_ControlRig.Get())
				{
					if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
					{
						TArray<FTLLRN_RigBoneElement*> Bones = Hierarchy->GetBones();
						for(int32 Index = 0; Index < Bones.Num(); Index++)
						{
							const int32 BoneIndex = Bones[Index]->GetIndex();
							const TArray<int32> Children = Hierarchy->GetChildren(BoneIndex);

							const FVector Start = Hierarchy->GetGlobalTransform(BoneIndex).GetLocation();

							if(InFrustum.IntersectSphere(Start, 0.1f * BoneRadius))
							{
								bSomethingSelected = true;
								SetTLLRN_RigElementSelection(TLLRN_ControlRig, ETLLRN_RigElementType::Bone, Bones[Index]->GetFName(), true);
								continue;
							}

							bool bSelectedBone = false;
							for(int32 ChildIndex : Children)
							{
								if(Hierarchy->Get(ChildIndex)->GetType() != ETLLRN_RigElementType::Bone)
								{
									continue;
								}
								
								const FVector End = Hierarchy->GetGlobalTransform(ChildIndex).GetLocation();

								const float BoneLength = (End - Start).Size();
								const float Radius = FMath::Max(BoneLength * 0.05f, 0.1f) * BoneRadius;
								const int32 Steps = FMath::CeilToInt(BoneLength / (Radius * 1.5f) + 0.5);
								const FVector Step = (End - Start) / FVector::FReal(Steps - 1);

								// intersect segment-wise along the bone
								FVector Position = Start;
								for(int32 StepIndex = 0; StepIndex < Steps; StepIndex++)
								{
									if(InFrustum.IntersectSphere(Position, Radius))
									{
										bSomethingSelected = true;
										bSelectedBone = true;
										SetTLLRN_RigElementSelection(TLLRN_ControlRig, ETLLRN_RigElementType::Bone, Bones[Index]->GetFName(), true);
										break;
									}
									Position += Step;
								}

								if(bSelectedBone)
								{
									break;
								}
							}
						}
					}
				}
			}
		}

		for(TWeakObjectPtr<UTLLRN_ControlRig> WeakTLLRN_ControlRig : RuntimeTLLRN_ControlRigs)
		{
			if(UTLLRN_ControlRig* TLLRN_ControlRig = WeakTLLRN_ControlRig.Get())
			{
				if (Settings->bDisplayNulls || TLLRN_ControlRig->IsConstructionModeEnabled())
				{
					if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
					{
						TArray<FTLLRN_RigNullElement*> Nulls = Hierarchy->GetNulls();
						for(int32 Index = 0; Index < Nulls.Num(); Index++)
						{
							const int32 NullIndex = Nulls[Index]->GetIndex();

							const FTransform Transform = Hierarchy->GetGlobalTransform(NullIndex);
							const FVector Origin = Transform.GetLocation();
							const float MaxScale = Transform.GetMaximumAxisScale();

							if(InFrustum.IntersectSphere(Origin, MaxScale * Settings->AxisScale))
							{
								bSomethingSelected = true;
								SetTLLRN_RigElementSelection(TLLRN_ControlRig, ETLLRN_RigElementType::Null, Nulls[Index]->GetFName(), true);
							}
						}
					}
				}
			}
		}
	}

	if (bSomethingSelected == true)
	{
		return true;
	}
	
	ScopedTransaction.Cancel();
	//if only selecting controls return true to stop any more selections
	if (Settings->bOnlySelectTLLRN_RigControls)
	{
		return true;
	}
	return FEdMode::FrustumSelect(InFrustum, InViewportClient, InSelect);
}

void FTLLRN_ControlRigEditMode::SelectNone()
{
	ClearTLLRN_RigElementSelection(FTLLRN_RigElementTypeHelper::ToMask(ETLLRN_RigElementType::All));

	FEdMode::SelectNone();
}

bool FTLLRN_ControlRigEditMode::IsMovingCamera(const FViewport* InViewport) const
{
	const bool LeftMouseButtonDown = InViewport->KeyState(EKeys::LeftMouseButton);
	const bool bIsAltKeyDown = InViewport->KeyState(EKeys::LeftAlt) || InViewport->KeyState(EKeys::RightAlt);
	return LeftMouseButtonDown && bIsAltKeyDown;
}

bool FTLLRN_ControlRigEditMode::IsDoingDrag(const FViewport* InViewport) const
{
	if(!UTLLRN_ControlRigEditorSettings::Get()->bLeftMouseDragDoesMarquee)
	{
		return false;
	}

	if (Owner && Owner->GetInteractiveToolsContext()->InputRouter->HasActiveMouseCapture())
	{
		// don't start dragging if the ITF handled tracking event first   
		return false;
	}
	
	const bool LeftMouseButtonDown = InViewport->KeyState(EKeys::LeftMouseButton);
	const bool bIsCtrlKeyDown = InViewport->KeyState(EKeys::LeftControl) || InViewport->KeyState(EKeys::RightControl);
	const bool bIsAltKeyDown = InViewport->KeyState(EKeys::LeftAlt) || InViewport->KeyState(EKeys::RightAlt);
	const EAxisList::Type CurrentAxis = GetCurrentWidgetAxis();
	
	//if shift is down we still want to drag
	return LeftMouseButtonDown && (CurrentAxis == EAxisList::None) && !bIsCtrlKeyDown && !bIsAltKeyDown;
}

bool FTLLRN_ControlRigEditMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (IsDragAnimSliderToolPressed(InViewport)) //this is needed to make sure we get all of the processed mouse events, for some reason the above may not return true
	{
		//handled by processed mouse clicks
		return true;
	}

	if (IsDoingDrag(InViewport))
	{
		return DragToolHandler.InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
	}

	FVector Drag = InDrag;
	FRotator Rot = InRot;
	FVector Scale = InScale;

	const bool bCtrlDown = InViewport->KeyState(EKeys::LeftControl) || InViewport->KeyState(EKeys::RightControl);
	const bool bShiftDown = InViewport->KeyState(EKeys::LeftShift) || InViewport->KeyState(EKeys::RightShift);

	//button down if left and ctrl and right is down, needed for indirect posting

	// enable MMB with the new TRS gizmos
	const bool bEnableMMB = UEditorInteractiveGizmoManager::UsesNewTRSGizmos();
	
	const bool bMouseButtonDown =
		InViewport->KeyState(EKeys::LeftMouseButton) ||
		(bCtrlDown && InViewport->KeyState(EKeys::RightMouseButton)) ||
		bEnableMMB;

	const UE::Widget::EWidgetMode WidgetMode = InViewportClient->GetWidgetMode();
	const EAxisList::Type CurrentAxis = InViewportClient->GetCurrentWidgetAxis();
	const ECoordSystem CoordSystem = InViewportClient->GetWidgetCoordSystemSpace();

	const bool bDoRotation = !Rot.IsZero() && (WidgetMode == UE::Widget::WM_Rotate || WidgetMode == UE::Widget::WM_TranslateRotateZ);
	const bool bDoTranslation = !Drag.IsZero() && (WidgetMode == UE::Widget::WM_Translate || WidgetMode == UE::Widget::WM_TranslateRotateZ);
	const bool bDoScale = !Scale.IsZero() && WidgetMode == UE::Widget::WM_Scale;


	if (InteractionScopes.Num() > 0 && bMouseButtonDown && CurrentAxis != EAxisList::None
		&& (bDoRotation || bDoTranslation || bDoScale))
	{
		for (auto& Pairs : TLLRN_ControlRigShapeActors)
		{
			if (AreTLLRN_RigElementsSelected(ValidControlTypeMask(), Pairs.Key))
			{
				FTransform ComponentTransform = GetHostingSceneComponentTransform(Pairs.Key);

				if (bIsChangingControlShapeTransform)
				{
					for (ATLLRN_ControlRigShapeActor* ShapeActor : Pairs.Value)
					{
						if (ShapeActor->IsSelected())
						{
							if (bManipulatorMadeChange == false)
							{
								GEditor->BeginTransaction(LOCTEXT("ChangeControlShapeTransaction", "Change Control Shape Transform"));
							}

							ChangeControlShapeTransform(ShapeActor, bDoTranslation, InDrag, bDoRotation, InRot, bDoScale, InScale, ComponentTransform);
							bManipulatorMadeChange = true;

							// break here since we only support changing shape transform of a single control at a time
							break;
						}
					}
				}
				else
				{
					const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
					bool bDoLocal = (CoordSystem == ECoordSystem::COORD_Local && Settings && Settings->bLocalTransformsInEachLocalSpace);
					bool bUseLocal = false;
					bool bCalcLocal = bDoLocal;
					bool bFirstTime = true;
					FTransform InOutLocal = FTransform::Identity;
					
					bool const bJustStartedManipulation = !bManipulatorMadeChange;
					bool bAnyAdditiveRig = false;
					for (ATLLRN_ControlRigShapeActor* ShapeActor : Pairs.Value)
					{
						if (ShapeActor->TLLRN_ControlRig.IsValid())
						{
							if (ShapeActor->TLLRN_ControlRig->IsAdditive())
							{
								bAnyAdditiveRig = true;
								break;
							}
						}
					}

					for (ATLLRN_ControlRigShapeActor* ShapeActor : Pairs.Value)
					{
						if (ShapeActor->IsEnabled() && ShapeActor->IsSelected())
						{
							// test local vs global
							if (bManipulatorMadeChange == false)
							{
								GEditor->BeginTransaction(LOCTEXT("MoveControlTransaction", "Move Control"));
							}

							// Cannot benefit of same local transform when applying to additive rigs
							if (!bAnyAdditiveRig)
							{
								if (bFirstTime)
								{
									bFirstTime = false;
								}
								else
								{
									if (bDoLocal)
									{
										bUseLocal = true;
										bDoLocal = false;
									}
								}
							}

							if(bJustStartedManipulation)
							{
								if(const FTLLRN_RigControlElement* ControlElement = Pairs.Key->FindControl(ShapeActor->ControlName))
								{
									ShapeActor->OffsetTransform = Pairs.Key->GetHierarchy()->GetGlobalControlOffsetTransform(ControlElement->GetKey(), false);
								}
							}

							MoveControlShape(ShapeActor, bDoTranslation, InDrag, bDoRotation, InRot, bDoScale, InScale, ComponentTransform,
								bUseLocal, bDoLocal, InOutLocal);
							bManipulatorMadeChange = true;
						}
					}
				}
			}
			else if (AreTLLRN_RigElementSelectedAndMovable(Pairs.Key))
			{
				FTransform ComponentTransform = GetHostingSceneComponentTransform(Pairs.Key);

				// set Bone transform
				// that will set initial Bone transform
				TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(Pairs.Key);

				for (int32 Index = 0; Index < SelectedTLLRN_RigElements.Num(); ++Index)
				{
					const ETLLRN_RigElementType SelectedTLLRN_RigElementType = SelectedTLLRN_RigElements[Index].Type;

					if (SelectedTLLRN_RigElementType == ETLLRN_RigElementType::Control)
					{
						FTransform NewWorldTransform = OnGetTLLRN_RigElementTransformDelegate.Execute(SelectedTLLRN_RigElements[Index], false, true) * ComponentTransform;
						bool bTransformChanged = false;
						if (bDoRotation)
						{
							FQuat CurrentRotation = NewWorldTransform.GetRotation();
							CurrentRotation = (Rot.Quaternion() * CurrentRotation);
							NewWorldTransform.SetRotation(CurrentRotation);
							bTransformChanged = true;
						}

						if (bDoTranslation)
						{
							FVector CurrentLocation = NewWorldTransform.GetLocation();
							CurrentLocation = CurrentLocation + Drag;
							NewWorldTransform.SetLocation(CurrentLocation);
							bTransformChanged = true;
						}

						if (bDoScale)
						{
							FVector CurrentScale = NewWorldTransform.GetScale3D();
							CurrentScale = CurrentScale + Scale;
							NewWorldTransform.SetScale3D(CurrentScale);
							bTransformChanged = true;
						}

						if (bTransformChanged)
						{
							if (bManipulatorMadeChange == false)
							{
								GEditor->BeginTransaction(LOCTEXT("MoveControlTransaction", "Move Control"));
							}
							FTransform NewComponentTransform = NewWorldTransform.GetRelativeTransform(ComponentTransform);
							OnSetTLLRN_RigElementTransformDelegate.Execute(SelectedTLLRN_RigElements[Index], NewComponentTransform, false);
							bManipulatorMadeChange = true;
						}
					}
				}
			}
		}
	}

	UpdatePivotTransforms();

	if (bManipulatorMadeChange)
	{
		TickManipulatableObjects(0.f);
	}
	//if in level editor we want to move other things also
	return IsInLevelEditor() ? false :bManipulatorMadeChange;
}

bool FTLLRN_ControlRigEditMode::ShouldDrawWidget() const
{
	for (const TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			if (AreTLLRN_RigElementSelectedAndMovable(TLLRN_ControlRig))
			{
				return true;
			}
		}
	}
	return FEdMode::ShouldDrawWidget();
}

bool FTLLRN_ControlRigEditMode::IsCompatibleWith(FEditorModeID OtherModeID) const
{
	return OtherModeID == FName(TEXT("EM_SequencerMode"), FNAME_Find) || OtherModeID == FName(TEXT("MotionTrailEditorMode"), FNAME_Find); /*|| OtherModeID == FName(TEXT("EditMode.TLLRN_ControlRigEditor"), FNAME_Find);*/
}

void FTLLRN_ControlRigEditMode::AddReferencedObjects( FReferenceCollector& Collector )
{
	for (auto& ShapeActors : TLLRN_ControlRigShapeActors)
	{
		for (auto& ShapeActor : ShapeActors.Value)
		{		
			Collector.AddReferencedObject(ShapeActor);
		}
	}
	if (ControlProxy)
	{
		Collector.AddReferencedObject(ControlProxy);
	}
}

void FTLLRN_ControlRigEditMode::ClearTLLRN_RigElementSelection(uint32 InTypes)
{
	for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			if (!AreEditingTLLRN_ControlRigDirectly())
			{
				if (UTLLRN_RigHierarchyController* Controller = TLLRN_ControlRig->GetHierarchy()->GetController())
				{
					Controller->ClearSelection();
				}
			}
			else if (UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(TLLRN_ControlRig->GetClass()->ClassGeneratedBy))
			{
				Blueprint->GetHierarchyController()->ClearSelection();
			}
		}
	}
}

// internal private function that doesn't use guarding.
void FTLLRN_ControlRigEditMode::SetTLLRN_RigElementSelectionInternal(UTLLRN_ControlRig* TLLRN_ControlRig, ETLLRN_RigElementType Type, const FName& InTLLRN_RigElementName, bool bSelected)
{
	if(UTLLRN_RigHierarchyController* Controller = TLLRN_ControlRig->GetHierarchy()->GetController())
	{
		Controller->SelectElement(FTLLRN_RigElementKey(InTLLRN_RigElementName, Type), bSelected);
	}
}

void FTLLRN_ControlRigEditMode::SetTLLRN_RigElementSelection(UTLLRN_ControlRig* TLLRN_ControlRig, ETLLRN_RigElementType Type, const FName& InTLLRN_RigElementName, bool bSelected)
{
	if (!bSelecting)
	{
		TGuardValue<bool> ReentrantGuard(bSelecting, true);

		SetTLLRN_RigElementSelectionInternal(TLLRN_ControlRig,Type, InTLLRN_RigElementName, bSelected);

		HandleSelectionChanged();
	}
}

void FTLLRN_ControlRigEditMode::SetTLLRN_RigElementSelection(UTLLRN_ControlRig* TLLRN_ControlRig, ETLLRN_RigElementType Type, const TArray<FName>& InTLLRN_RigElementNames, bool bSelected)
{
	if (!bSelecting)
	{
		TGuardValue<bool> ReentrantGuard(bSelecting, true);

		for (const FName& ElementName : InTLLRN_RigElementNames)
		{
			SetTLLRN_RigElementSelectionInternal(TLLRN_ControlRig, Type, ElementName, bSelected);
		}

		HandleSelectionChanged();
	}
}

TArray<FTLLRN_RigElementKey> FTLLRN_ControlRigEditMode::GetSelectedTLLRN_RigElements(UTLLRN_ControlRig* TLLRN_ControlRig) const
{
	if (TLLRN_ControlRig == nullptr && GetTLLRN_ControlRigs().Num() > 0)
	{
		TLLRN_ControlRig = GetTLLRN_ControlRigs()[0].Get();
	}

	TArray<FTLLRN_RigElementKey> SelectedKeys;

	if (TLLRN_ControlRig->GetHierarchy())
	{
		SelectedKeys = TLLRN_ControlRig->GetHierarchy()->GetSelectedKeys();
	}

	// currently only 1 transient control is allowed at a time
	// Transient Control's bSelected flag is never set to true, probably to avoid confusing other parts of the system
	// But since Edit Mode directly deals with transient controls, its selection status is given special treatment here.
	// So basically, whenever a bone is selected, and there is a transient control present, we consider both selected.
	if (SelectedKeys.Num() == 1)
	{
		if (SelectedKeys[0].Type == ETLLRN_RigElementType::Bone || SelectedKeys[0].Type == ETLLRN_RigElementType::Null)
		{
			const FName ControlName = UTLLRN_ControlRig::GetNameForTransientControl(SelectedKeys[0]);
			const FTLLRN_RigElementKey TransientControlKey = FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control);
			if(TLLRN_ControlRig->GetHierarchy()->Contains(TransientControlKey))
			{
				SelectedKeys.Add(TransientControlKey);
			}

		}
	}
	else
	{
		// check if there is a pin value transient control active
		// when a pin control is active, all existing selection should have been cleared
		TArray<FTLLRN_RigControlElement*> TransientControls = TLLRN_ControlRig->GetHierarchy()->GetTransientControls();

		if (TransientControls.Num() > 0)
		{
			if (ensure(SelectedKeys.Num() == 0))
			{
				SelectedKeys.Add(TransientControls[0]->GetKey());
			}
		}
	}
	return SelectedKeys;
}

bool FTLLRN_ControlRigEditMode::AreTLLRN_RigElementsSelected(uint32 InTypes, UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(InTLLRN_ControlRig);

	for (const FTLLRN_RigElementKey& Ele : SelectedTLLRN_RigElements)
	{
		if (FTLLRN_RigElementTypeHelper::DoesHave(InTypes, Ele.Type))
		{
			return true;
		}
	}

	return false;
}

int32 FTLLRN_ControlRigEditMode::GetNumSelectedTLLRN_RigElements(uint32 InTypes, UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(InTLLRN_ControlRig);
	if (FTLLRN_RigElementTypeHelper::DoesHave(InTypes, ETLLRN_RigElementType::All))
	{
		return SelectedTLLRN_RigElements.Num();
	}
	else
	{
		int32 NumSelected = 0;
		for (const FTLLRN_RigElementKey& Ele : SelectedTLLRN_RigElements)
		{
			if (FTLLRN_RigElementTypeHelper::DoesHave(InTypes, Ele.Type))
			{
				++NumSelected;
			}
		}

		return NumSelected;
	}

	return 0;
}

void FTLLRN_ControlRigEditMode::RefreshObjects()
{
	SetObjects_Internal();
}

bool FTLLRN_ControlRigEditMode::CanRemoveFromPreviewScene(const USceneComponent* InComponent)
{
	for (auto& ShapeActors : TLLRN_ControlRigShapeActors)
	{
		for (auto& ShapeActor : ShapeActors.Value)
		{
			TInlineComponentArray<USceneComponent*> SceneComponents;
			ShapeActor->GetComponents(SceneComponents, true);
			if (SceneComponents.Contains(InComponent))
			{
				return false;
			}
		}
	}

	// we don't need it 
	return true;
}

ECoordSystem FTLLRN_ControlRigEditMode::GetCoordSystemSpace() const
{
	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
	if (Settings && Settings->bCoordSystemPerWidgetMode)
	{
		const int32 WidgetMode = static_cast<int32>(GetModeManager()->GetWidgetMode());
		if (CoordSystemPerWidgetMode.IsValidIndex(WidgetMode))
		{
			return CoordSystemPerWidgetMode[WidgetMode];
		}
	}

	return GetModeManager()->GetCoordSystem();	
}

bool FTLLRN_ControlRigEditMode::ComputePivotFromEditedShape(UTLLRN_ControlRig* InTLLRN_ControlRig, FTransform& OutTransform) const
{
	const UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig ? InTLLRN_ControlRig->GetHierarchy() : nullptr;
	if (!Hierarchy)
	{
		return false;
	}

	if (!ensure(bIsChangingControlShapeTransform))
	{
		return false;
	}
	
	OutTransform = FTransform::Identity;
	
	if (auto* ShapeActors = TLLRN_ControlRigShapeActors.Find(InTLLRN_ControlRig))
	{
		// we just want to change the shape transform of one single control.
		const int32 Index = ShapeActors->IndexOfByPredicate([](const TObjectPtr<ATLLRN_ControlRigShapeActor>& ShapeActor)
		{
			return IsValid(ShapeActor) && ShapeActor->IsSelected();
		});

		if (Index != INDEX_NONE)
		{
			if (FTLLRN_RigControlElement* ControlElement = InTLLRN_ControlRig->FindControl((*ShapeActors)[Index]->ControlName))
			{
				OutTransform = Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal);
			}				
		}
	}
	
	return true;
}

bool FTLLRN_ControlRigEditMode::ComputePivotFromShapeActors(UTLLRN_ControlRig* InTLLRN_ControlRig, const bool bEachLocalSpace, const bool bIsParentSpace, FTransform& OutTransform) const
{
	if (!ensure(!bIsChangingControlShapeTransform))
	{
		return false;
	}
	
	const UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig ? InTLLRN_ControlRig->GetHierarchy() : nullptr;
	if (!Hierarchy)
	{
		return false;
	}
	const FTransform ComponentTransform = GetHostingSceneComponentTransform(InTLLRN_ControlRig);

	FTransform LastTransform = FTransform::Identity, PivotTransform = FTransform::Identity;

	if (const auto* ShapeActors = TLLRN_ControlRigShapeActors.Find(InTLLRN_ControlRig))
	{
		// if in local just use the first selected actor transform
		// otherwise, compute the average location as pivot location
		
		int32 NumSelectedControls = 0;
		FVector PivotLocation = FVector::ZeroVector;
		for (const TObjectPtr<ATLLRN_ControlRigShapeActor>& ShapeActor : *ShapeActors)
		{
			if (IsValid(ShapeActor) && ShapeActor->IsSelected())
			{
				const FTLLRN_RigElementKey ControlKey = ShapeActor->GetElementKey();
				bool bGetParentTransform = bIsParentSpace && Hierarchy->GetNumberOfParents(ControlKey);
	
				const FTransform ShapeTransform = ShapeActor->GetActorTransform().GetRelativeTransform(ComponentTransform);
				LastTransform = bGetParentTransform ? Hierarchy->GetParentTransform(ControlKey) : ShapeTransform;
				PivotLocation += ShapeTransform.GetLocation();
				
				++NumSelectedControls;
				if (bEachLocalSpace)
				{
					break;
				}
			}
		}

		if (NumSelectedControls > 1)
		{
			PivotLocation /= static_cast<double>(NumSelectedControls);
		}
		PivotTransform.SetLocation(PivotLocation);
	}

	// Use the last transform's rotation as pivot rotation
	const FTransform WorldTransform = LastTransform * ComponentTransform;
	PivotTransform.SetRotation(WorldTransform.GetRotation());
	
	OutTransform = PivotTransform;
	
	return true;
}

bool FTLLRN_ControlRigEditMode::ComputePivotFromElements(UTLLRN_ControlRig* InTLLRN_ControlRig, FTransform& OutTransform) const
{
	if (!ensure(!bIsChangingControlShapeTransform))
	{
		return false;
	}
	
	const UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig ? InTLLRN_ControlRig->GetHierarchy() : nullptr;
	if (!Hierarchy)
	{
		return false;
	}
	
	const FTransform ComponentTransform = GetHostingSceneComponentTransform(InTLLRN_ControlRig);
	
	int32 NumSelection = 0;
	FTransform LastTransform = FTransform::Identity, PivotTransform = FTransform::Identity;
	FVector PivotLocation = FVector::ZeroVector;
	const TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(InTLLRN_ControlRig);
	
	for (int32 Index = 0; Index < SelectedTLLRN_RigElements.Num(); ++Index)
	{
		if (SelectedTLLRN_RigElements[Index].Type == ETLLRN_RigElementType::Control)
		{
			LastTransform = OnGetTLLRN_RigElementTransformDelegate.Execute(SelectedTLLRN_RigElements[Index], false, true);
			PivotLocation += LastTransform.GetLocation();
			++NumSelection;
		}
	}

	if (NumSelection == 1)
	{
		// A single control just uses its own transform
		const FTransform WorldTransform = LastTransform * ComponentTransform;
		PivotTransform.SetRotation(WorldTransform.GetRotation());
	}
	else if (NumSelection > 1)
	{
		PivotLocation /= static_cast<double>(NumSelection);
		PivotTransform.SetRotation(ComponentTransform.GetRotation());
	}
		
	PivotTransform.SetLocation(PivotLocation);
	OutTransform = PivotTransform;

	return true;
}

void FTLLRN_ControlRigEditMode::UpdatePivotTransforms()
{
	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
	const bool bEachLocalSpace = Settings && Settings->bLocalTransformsInEachLocalSpace;
	const bool bIsParentSpace = GetCoordSystemSpace() == COORD_Parent;

	PivotTransforms.Reset();

	for (const TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			bool bAdd = false;
			FTransform Pivot = FTransform::Identity;
			if (AreTLLRN_RigElementsSelected(ValidControlTypeMask(), TLLRN_ControlRig))
			{
				if (bIsChangingControlShapeTransform)
				{
					bAdd = ComputePivotFromEditedShape(TLLRN_ControlRig, Pivot);
				}
				else
				{
					bAdd = ComputePivotFromShapeActors(TLLRN_ControlRig, bEachLocalSpace, bIsParentSpace, Pivot);			
				}
			}
			else if (AreTLLRN_RigElementSelectedAndMovable(TLLRN_ControlRig))
			{
				// do we even get in here ?!
				// we will enter the if first as AreTLLRN_RigElementsSelected will return true before AreTLLRN_RigElementSelectedAndMovable does...
				bAdd = ComputePivotFromElements(TLLRN_ControlRig, Pivot);
			}
			if (bAdd)
			{
				PivotTransforms.Add(TLLRN_ControlRig, MoveTemp(Pivot));
			}
		}
	}

	bPivotsNeedUpdate = false;

	//If in level editor and the transforms changed we need to force hit proxy invalidate so widget hit testing 
	//doesn't work off of it's last transform.  Similar to what sequencer does on re-evaluation but do to how edit modes and widget ticks happen
	//it doesn't work for control rig gizmo's
	if (IsInLevelEditor())
	{
		if (HasPivotTransformsChanged())
		{
			for (FEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients())
			{
				if (LevelVC)
				{
					if (!LevelVC->IsRealtime())
					{
						LevelVC->RequestRealTimeFrames(1);
					}

					if (LevelVC->Viewport)
					{
						LevelVC->Viewport->InvalidateHitProxy();
					}
				}
			}
		}
		LastPivotTransforms = PivotTransforms;
	}
}

void FTLLRN_ControlRigEditMode::RequestTransformWidgetMode(UE::Widget::EWidgetMode InWidgetMode)
{
	RequestedWidgetModes.Add(InWidgetMode);
}

bool FTLLRN_ControlRigEditMode::HasPivotTransformsChanged() const
{
	if (PivotTransforms.Num() != LastPivotTransforms.Num())
	{
		return true;
	}
	for (const TPair<UTLLRN_ControlRig*, FTransform>& Transform : PivotTransforms)
	{
		if (const FTransform* LastTransform = LastPivotTransforms.Find(Transform.Key))
		{
			if (Transform.Value.Equals(*LastTransform, 1e-4f) == false)
			{
				return true;
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

void FTLLRN_ControlRigEditMode::UpdatePivotTransformsIfNeeded(UTLLRN_ControlRig* InTLLRN_ControlRig, FTransform& InOutTransform) const
{
	if (!bPivotsNeedUpdate)
	{
		return;
	}

	if (!InTLLRN_ControlRig)
	{
		return;
	}

	// Update shape actors transforms
	if (auto* ShapeActors = TLLRN_ControlRigShapeActors.Find(InTLLRN_ControlRig))
	{
		FTransform ComponentTransform = FTransform::Identity;
		if (!AreEditingTLLRN_ControlRigDirectly())
		{
			ComponentTransform = GetHostingSceneComponentTransform(InTLLRN_ControlRig);
		}
		for (ATLLRN_ControlRigShapeActor* ShapeActor : *ShapeActors)
		{
			const FTransform Transform = InTLLRN_ControlRig->GetControlGlobalTransform(ShapeActor->ControlName);
			ShapeActor->SetActorTransform(Transform * ComponentTransform);
		}
	}

	// Update pivot
	if (AreTLLRN_RigElementsSelected(ValidControlTypeMask(), InTLLRN_ControlRig))
	{
		if (bIsChangingControlShapeTransform)
		{
			ComputePivotFromEditedShape(InTLLRN_ControlRig, InOutTransform);
		}
		else
		{
			const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
			const bool bEachLocalSpace = Settings && Settings->bLocalTransformsInEachLocalSpace;
			const bool bIsParentSpace = GetCoordSystemSpace() == COORD_Parent;
			ComputePivotFromShapeActors(InTLLRN_ControlRig, bEachLocalSpace, bIsParentSpace, InOutTransform);			
		}
	}
	else if (AreTLLRN_RigElementSelectedAndMovable(InTLLRN_ControlRig))
	{
		ComputePivotFromElements(InTLLRN_ControlRig, InOutTransform);
	}
}

void FTLLRN_ControlRigEditMode::HandleSelectionChanged()
{
	for (const auto& ShapeActors : TLLRN_ControlRigShapeActors)
	{
		for (const TObjectPtr<ATLLRN_ControlRigShapeActor>& ShapeActor : ShapeActors.Value)
		{
			TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
			ShapeActor->GetComponents(PrimitiveComponents, true);
			for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
			{
				PrimitiveComponent->PushSelectionToProxy();
			}
		}
	}

	// automatically exit shape transform edit mode if there is no shape selected
	if (bIsChangingControlShapeTransform)
	{
		if (!CanChangeControlShapeTransform())
		{
			bIsChangingControlShapeTransform = false;
		}
	}

	// update the pivot transform of our selected objects (they could be animating)
	UpdatePivotTransforms();
	
	//need to force the redraw also
	if (!AreEditingTLLRN_ControlRigDirectly())
	{
		GEditor->RedrawLevelEditingViewports(true);
	}
}

void FTLLRN_ControlRigEditMode::BindCommands()
{
	const FTLLRN_ControlRigEditModeCommands& Commands = FTLLRN_ControlRigEditModeCommands::Get();

	CommandBindings->MapAction(
		Commands.ToggleManipulators,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::ToggleManipulators));
	CommandBindings->MapAction(
		Commands.ToggleAllManipulators,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::ToggleAllManipulators));
	CommandBindings->MapAction(
		Commands.ZeroTransforms,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::ZeroTransforms, true));
	CommandBindings->MapAction(
		Commands.ZeroAllTransforms,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::ZeroTransforms, false));
	CommandBindings->MapAction(
		Commands.InvertTransforms,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::InvertInputPose, true));
	CommandBindings->MapAction(
		Commands.InvertAllTransforms,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::InvertInputPose, false));
	CommandBindings->MapAction(
		Commands.ClearSelection,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::ClearSelection));

	CommandBindings->MapAction(
		Commands.ChangeAnimSliderTool,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::ChangeAnimSliderTool),
		FCanExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::CanChangeAnimSliderTool)
	);

	CommandBindings->MapAction(
		Commands.FrameSelection,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::FrameSelection),
		FCanExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::CanFrameSelection)
	);

	CommandBindings->MapAction(
		Commands.IncreaseControlShapeSize,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::IncreaseShapeSize));

	CommandBindings->MapAction(
		Commands.DecreaseControlShapeSize,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::DecreaseShapeSize));

	CommandBindings->MapAction(
		Commands.ResetControlShapeSize,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::ResetControlShapeSize));

	CommandBindings->MapAction(
		Commands.ToggleControlShapeTransformEdit,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::ToggleControlShapeTransformEdit));

	CommandBindings->MapAction(
		Commands.SetTLLRN_AnimLayerPassthroughKey,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::SetTLLRN_AnimLayerPassthroughKey));

	CommandBindings->MapAction(
		Commands.OpenSpacePickerWidget,
		FExecuteAction::CreateRaw(this, &FTLLRN_ControlRigEditMode::OpenSpacePickerWidget));
}

bool FTLLRN_ControlRigEditMode::IsControlSelected() const
{
	static uint32 TypeFlag = (uint32)ETLLRN_RigElementType::Control;
	for (const TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			if (AreTLLRN_RigElementsSelected(TypeFlag,TLLRN_ControlRig))
			{
				return true;
			}
		}
	}
	return false;
}

bool FTLLRN_ControlRigEditMode::CanFrameSelection()
{
	for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			if (GetSelectedTLLRN_RigElements(TLLRN_ControlRig).Num() > 0)
			{
				return true;
			}
		}
	}
	return  false;;
}

void FTLLRN_ControlRigEditMode::ClearSelection()
{
	ClearTLLRN_RigElementSelection(FTLLRN_RigElementTypeHelper::ToMask(ETLLRN_RigElementType::All));
	if (GEditor)
	{
		GEditor->Exec(GetWorld(), TEXT("SELECT NONE"));
	}
}

void FTLLRN_ControlRigEditMode::FrameSelection()
{
	if(CurrentViewportClient)
	{
		FSphere Sphere(EForceInit::ForceInit);
		if(GetCameraTarget(Sphere))
		{
			FBox Bounds(EForceInit::ForceInit);
			Bounds += Sphere.Center;
			Bounds += Sphere.Center + FVector::OneVector * Sphere.W;
			Bounds += Sphere.Center - FVector::OneVector * Sphere.W;
			CurrentViewportClient->FocusViewportOnBox(Bounds);
			return;
		}
    }

	TArray<AActor*> Actors;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(TLLRN_ControlRig);
			for (const FTLLRN_RigElementKey& SelectedKey : SelectedTLLRN_RigElements)
			{
				if (SelectedKey.Type == ETLLRN_RigElementType::Control)
				{
					ATLLRN_ControlRigShapeActor* ShapeActor = GetControlShapeFromControlName(TLLRN_ControlRig,SelectedKey.Name);
					if (ShapeActor)
					{
						Actors.Add(ShapeActor);
					}
				}
			}
		}
	}

	if (Actors.Num())
	{
		TArray<UPrimitiveComponent*> SelectedComponents;
		GEditor->MoveViewportCamerasToActor(Actors, SelectedComponents, true);
	}
}

void FTLLRN_ControlRigEditMode::FrameItems(const TArray<FTLLRN_RigElementKey>& InItems)
{
	if(!OnGetTLLRN_RigElementTransformDelegate.IsBound())
	{
		return;
	}

	if(CurrentViewportClient == nullptr)
	{
		DeferredItemsToFrame = InItems;
		return;
	}

	FBox Box(ForceInit);

	for (int32 Index = 0; Index < InItems.Num(); ++Index)
	{
		static const float Radius = 20.f;
		if (InItems[Index].Type == ETLLRN_RigElementType::Bone || InItems[Index].Type == ETLLRN_RigElementType::Null)
		{
			FTransform Transform = OnGetTLLRN_RigElementTransformDelegate.Execute(InItems[Index], false, true);
			Box += Transform.TransformPosition(FVector::OneVector * Radius);
			Box += Transform.TransformPosition(FVector::OneVector * -Radius);
		}
		else if (InItems[Index].Type == ETLLRN_RigElementType::Control)
		{
			FTransform Transform = OnGetTLLRN_RigElementTransformDelegate.Execute(InItems[Index], false, true);
			Box += Transform.TransformPosition(FVector::OneVector * Radius);
			Box += Transform.TransformPosition(FVector::OneVector * -Radius);
		}
	}

	if(Box.IsValid)
	{
		CurrentViewportClient->FocusViewportOnBox(Box);
	}
}

void FTLLRN_ControlRigEditMode::IncreaseShapeSize()
{
	UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
	Settings->GizmoScale += 0.1f;
	GetModeManager()->SetWidgetScale(Settings->GizmoScale);
}

void FTLLRN_ControlRigEditMode::DecreaseShapeSize()
{
	UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
	Settings->GizmoScale -= 0.1f;
	GetModeManager()->SetWidgetScale(Settings->GizmoScale);
}

void FTLLRN_ControlRigEditMode::ResetControlShapeSize()
{
	UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
	GetModeManager()->SetWidgetScale(PreviousGizmoScale);
}

uint8 FTLLRN_ControlRigEditMode::GetInteractionType(const FEditorViewportClient* InViewportClient)
{
	ETLLRN_ControlRigInteractionType Result = ETLLRN_ControlRigInteractionType::None;
	if (InViewportClient->IsMovingCamera())
	{
		return static_cast<uint8>(Result);
	}
	
	switch (InViewportClient->GetWidgetMode())
	{
		case UE::Widget::WM_Translate:
			EnumAddFlags(Result, ETLLRN_ControlRigInteractionType::Translate);
			break;
		case UE::Widget::WM_TranslateRotateZ:
			EnumAddFlags(Result, ETLLRN_ControlRigInteractionType::Translate);
			EnumAddFlags(Result, ETLLRN_ControlRigInteractionType::Rotate);
			break;
		case UE::Widget::WM_Rotate:
			EnumAddFlags(Result, ETLLRN_ControlRigInteractionType::Rotate);
			break;
		case UE::Widget::WM_Scale:
			EnumAddFlags(Result, ETLLRN_ControlRigInteractionType::Scale);
			break;
		default:
			break;
	}
	return static_cast<uint8>(Result);
}

void FTLLRN_ControlRigEditMode::ToggleControlShapeTransformEdit()
{ 
	if (bIsChangingControlShapeTransform)
	{
		bIsChangingControlShapeTransform = false;
	}
	else if (CanChangeControlShapeTransform())
	{
		bIsChangingControlShapeTransform = true;
	}
}

void FTLLRN_ControlRigEditMode::GetAllSelectedControls(TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>>& OutSelectedControls) const
{
	for (const TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			if (const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
			{
				TArray<FTLLRN_RigElementKey> SelectedControls = Hierarchy->GetSelectedKeys(ETLLRN_RigElementType::Control);
				if (SelectedControls.Num() > 0)
				{
					OutSelectedControls.Add(TLLRN_ControlRig, SelectedControls);
				}
			}
		}
	}
}

/** If Anim Slider is open, got to the next tool*/
void FTLLRN_ControlRigEditMode::ChangeAnimSliderTool()
{
	if (Toolkit.IsValid())
	{
		StaticCastSharedPtr<FTLLRN_ControlRigEditModeToolkit>(Toolkit)->GetToNextActiveSlider();
	}
}

/** If Anim Slider is open, then can drag*/
bool FTLLRN_ControlRigEditMode::CanChangeAnimSliderTool() const
{
	if (Toolkit.IsValid())
	{
		return (StaticCastSharedPtr<FTLLRN_ControlRigEditModeToolkit>(Toolkit)->CanChangeAnimSliderTool());
	}
	return false;
}

/** If Anim Slider is open, drag the tool*/
void FTLLRN_ControlRigEditMode::DragAnimSliderTool(double IncrementVal)
{
	if (Toolkit.IsValid())
	{
		StaticCastSharedPtr<FTLLRN_ControlRigEditModeToolkit>(Toolkit)->DragAnimSliderTool(IncrementVal);
	}
}

void FTLLRN_ControlRigEditMode::StartAnimSliderTool(int32 InX)
{
	if (Toolkit.IsValid())
	{
		bisTrackingAnimToolDrag = true;
		GEditor->BeginTransaction(LOCTEXT("AnimSliderBlend", "AnimSlider Blend"));
		StartXValue = InX;
		StaticCastSharedPtr<FTLLRN_ControlRigEditModeToolkit>(Toolkit)->StartAnimSliderTool();
	}
}

/** Reset and stop user the anim slider tool*/
void FTLLRN_ControlRigEditMode::ResetAnimSlider()
{
	if (Toolkit.IsValid())
	{
		GEditor->EndTransaction();
		bisTrackingAnimToolDrag = false;
		StartXValue.Reset();
		StaticCastSharedPtr<FTLLRN_ControlRigEditModeToolkit>(Toolkit)->ResetAnimSlider();
	}
}

/** If the Drag Anim Slider Tool is pressed*/
bool FTLLRN_ControlRigEditMode::IsDragAnimSliderToolPressed(FViewport* InViewport)
{
	if (IsInLevelEditor() && Toolkit.IsValid() &&  CanChangeAnimSliderTool())
	{
		bool bIsMovingSlider = false;
		const FTLLRN_ControlRigEditModeCommands& Commands = FTLLRN_ControlRigEditModeCommands::Get();
		// Need to iterate through primary and secondary to make sure they are all pressed.
		for (uint32 i = 0; i < static_cast<uint8>(EMultipleKeyBindingIndex::NumChords); ++i)
		{
			EMultipleKeyBindingIndex ChordIndex = static_cast<EMultipleKeyBindingIndex>(i);
			const FInputChord& Chord = *Commands.DragAnimSliderTool->GetActiveChord(ChordIndex);
			bIsMovingSlider |= Chord.IsValidChord() && InViewport->KeyState(Chord.Key);
		}
		return bIsMovingSlider;
	}
	return false;
}

void FTLLRN_ControlRigEditMode::SetTLLRN_AnimLayerPassthroughKey()
{
	ISequencer* Sequencer = WeakSequencer.Pin().Get();
	if (Sequencer)
	{
		if (UTLLRN_TLLRN_AnimLayers* TLLRN_TLLRN_AnimLayers = UTLLRN_TLLRN_AnimLayers::GetTLLRN_TLLRN_AnimLayers(Sequencer))
		{
			const FScopedTransaction Transaction(LOCTEXT("SetPassthroughKey_Transaction", "Set Passthrough Key"), !GIsTransacting);
			for (UTLLRN_AnimLayer* TLLRN_AnimLayer : TLLRN_TLLRN_AnimLayers->TLLRN_TLLRN_AnimLayers)
			{
				if (TLLRN_AnimLayer->GetSelectedInList())
				{
					int32 Index = TLLRN_TLLRN_AnimLayers->GetTLLRN_AnimLayerIndex(TLLRN_AnimLayer);
					if (Index != INDEX_NONE)
					{
						TLLRN_TLLRN_AnimLayers->SetPassthroughKey(Sequencer, Index);
					}
				}
			}
		}
	}
}

void FTLLRN_ControlRigEditMode::OpenSpacePickerWidget()
{
	TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> SelectedTLLRN_ControlRigsAndControls;
	GetAllSelectedControls(SelectedTLLRN_ControlRigsAndControls);

	if (SelectedTLLRN_ControlRigsAndControls.Num() < 1)
	{
		return;
	}

	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
	TArray<TArray<FTLLRN_RigElementKey>> AllSelectedControls;
	SelectedTLLRN_ControlRigsAndControls.GenerateKeyArray(TLLRN_ControlRigs);
	SelectedTLLRN_ControlRigsAndControls.GenerateValueArray(AllSelectedControls);


	//mz todo handle multiple control rigs with space picker
	UTLLRN_ControlRig* RuntimeRig = TLLRN_ControlRigs[0];
	TArray<FTLLRN_RigElementKey>& SelectedControls = AllSelectedControls[0];

	UTLLRN_RigHierarchy* Hierarchy = RuntimeRig->GetHierarchy();

	TSharedRef<STLLRN_RigSpacePickerWidget> PickerWidget =
	SNew(STLLRN_RigSpacePickerWidget)
	.Hierarchy(Hierarchy)
	.Controls(SelectedControls)
	.Title(LOCTEXT("PickSpace", "Pick Space"))
	.AllowDelete(AreEditingTLLRN_ControlRigDirectly())
	.AllowReorder(AreEditingTLLRN_ControlRigDirectly())
	.AllowAdd(AreEditingTLLRN_ControlRigDirectly())
	.GetControlCustomization_Lambda([this, RuntimeRig](UTLLRN_RigHierarchy*, const FTLLRN_RigElementKey& InControlKey)
	{
		return RuntimeRig->GetControlCustomization(InControlKey);
	})
	.OnActiveSpaceChanged_Lambda([this, SelectedControls, RuntimeRig](UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey, const FTLLRN_RigElementKey& InSpaceKey)
	{
		check(SelectedControls.Contains(InControlKey));
		if (!AreEditingTLLRN_ControlRigDirectly())
		{
			if (WeakSequencer.IsValid())
			{
				if (const FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(InControlKey))
				{
					ISequencer* Sequencer = WeakSequencer.Pin().Get();
					if (Sequencer)
					{
						FScopedTransaction Transaction(LOCTEXT("KeyTLLRN_ControlTLLRN_RigSpace", "Key Control Rig Space"));
						FSpaceChannelAndSection SpaceChannelAndSection = FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::FindSpaceChannelAndSectionForControl(RuntimeRig, InControlKey.Name, Sequencer, true /*bCreateIfNeeded*/);
						if (SpaceChannelAndSection.SpaceChannel)
						{
							const FFrameRate TickResolution = Sequencer->GetFocusedTickResolution();
							const FFrameTime FrameTime = Sequencer->GetLocalTime().ConvertTo(TickResolution);
							FFrameNumber CurrentTime = FrameTime.GetFrame();
							FTLLRN_ControlTLLRN_RigSpaceChannelHelpers::SequencerKeyTLLRN_ControlTLLRN_RigSpaceChannel(RuntimeRig, Sequencer, SpaceChannelAndSection.SpaceChannel, SpaceChannelAndSection.SectionToKey, CurrentTime, InHierarchy, InControlKey, InSpaceKey);
						}
					}
				}
			}
		}
		else
		{
			if (RuntimeRig->IsAdditive())
			{
				const FTransform Transform = RuntimeRig->GetControlGlobalTransform(InControlKey.Name);
			   RuntimeRig->SwitchToParent(InControlKey, InSpaceKey, false, true);
			   RuntimeRig->Evaluate_AnyThread();
			   FTLLRN_RigControlValue ControlValue = RuntimeRig->GetControlValueFromGlobalTransform(InControlKey.Name, Transform, ETLLRN_RigTransformType::CurrentGlobal);
			   RuntimeRig->SetControlValue(InControlKey.Name, ControlValue);
			   RuntimeRig->Evaluate_AnyThread();
			}
			else
			{
				const FTransform Transform = InHierarchy->GetGlobalTransform(InControlKey);
				UTLLRN_RigHierarchy::TElementDependencyMap Dependencies = InHierarchy->GetDependenciesForVM(RuntimeRig->GetVM());
				FString OutFailureReason;
				if (InHierarchy->SwitchToParent(InControlKey, InSpaceKey, false, true, Dependencies, &OutFailureReason))
				{
					InHierarchy->SetGlobalTransform(InControlKey, Transform);
				}
				else
				{
					if(UTLLRN_RigHierarchyController* Controller = InHierarchy->GetController())
					{
						static constexpr TCHAR MessageFormat[] = TEXT("Could not switch %s to parent %s: %s");
						Controller->ReportAndNotifyErrorf(MessageFormat,
							*InControlKey.Name.ToString(),
							*InSpaceKey.Name.ToString(),
							*OutFailureReason);
					}
				}
			}
		}
		
	})
	.OnSpaceListChanged_Lambda([this, SelectedControls, RuntimeRig](UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey, const TArray<FTLLRN_RigElementKey>& InSpaceList)
	{
		check(SelectedControls.Contains(InControlKey));

		// check if we are in the control rig editor or in the level
		if(AreEditingTLLRN_ControlRigDirectly())
		{
			if (UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(RuntimeRig->GetClass()->ClassGeneratedBy))
			{
				if(UTLLRN_RigHierarchy* Hierarchy = Blueprint->Hierarchy)
				{
					// update the settings in the control element
					if(FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(InControlKey))
					{
						Blueprint->Modify();
						FScopedTransaction Transaction(LOCTEXT("ControlChangeAvailableSpaces", "Edit Available Spaces"));

						ControlElement->Settings.Customization.AvailableSpaces = InSpaceList;
						Hierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
					}

					// also update the debugged instance
					if(Hierarchy != InHierarchy)
					{
						if(FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(InControlKey))
						{
							ControlElement->Settings.Customization.AvailableSpaces = InSpaceList;
							InHierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
						}
					}
				}
			}
		}
		else
		{
			// update the settings in the control element
			if(FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(InControlKey))
			{
				FScopedTransaction Transaction(LOCTEXT("ControlChangeAvailableSpaces", "Edit Available Spaces"));

				InHierarchy->Modify();

				FTLLRN_RigControlElementCustomization ControlCustomization = *RuntimeRig->GetControlCustomization(InControlKey);	
				ControlCustomization.AvailableSpaces = InSpaceList;
				ControlCustomization.RemovedSpaces.Reset();

				// remember  the elements which are in the asset's available list but removed by the user
				for(const FTLLRN_RigElementKey& AvailableSpace : ControlElement->Settings.Customization.AvailableSpaces)
				{
					if(!ControlCustomization.AvailableSpaces.Contains(AvailableSpace))
					{
						ControlCustomization.RemovedSpaces.Add(AvailableSpace);
					}
				}

				RuntimeRig->SetControlCustomization(InControlKey, ControlCustomization);
				InHierarchy->Notify(ETLLRN_RigHierarchyNotification::ControlSettingChanged, ControlElement);
			}
		}

	});
	// todo: implement GetAdditionalSpacesDelegate to pull spaces from sequencer

	PickerWidget->OpenDialog(false);
}

FText FTLLRN_ControlRigEditMode::GetToggleControlShapeTransformEditHotKey() const
{
	const FTLLRN_ControlRigEditModeCommands& Commands = FTLLRN_ControlRigEditModeCommands::Get();
	return Commands.ToggleControlShapeTransformEdit->GetInputText();
}

void FTLLRN_ControlRigEditMode::ToggleManipulators()
{
	if (!AreEditingTLLRN_ControlRigDirectly())
	{
		TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> SelectedControls;
		GetAllSelectedControls(SelectedControls);
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
		SelectedControls.GenerateKeyArray(TLLRN_ControlRigs);
		for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
		{
			if (TLLRN_ControlRig)
			{
				FScopedTransaction ScopedTransaction(LOCTEXT("ToggleControlsVisibility", "Toggle Controls Visibility"),!GIsTransacting);
				TLLRN_ControlRig->Modify();
				TLLRN_ControlRig->ToggleControlsVisible();
			}
		}
	}
	else
	{
		UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
		Settings->bHideControlShapes = !Settings->bHideControlShapes;
	}
}

void FTLLRN_ControlRigEditMode::ToggleAllManipulators()
{	
	UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
	Settings->bHideControlShapes = !Settings->bHideControlShapes;

	//turn on all if in level editor in case any where off
	if (!AreEditingTLLRN_ControlRigDirectly() && Settings->bHideControlShapes)
	{
		for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
			{
				TLLRN_ControlRig->SetControlsVisible(true);
			}
		}
	}
}

bool FTLLRN_ControlRigEditMode::AreControlsVisible() const
{
	if (!AreEditingTLLRN_ControlRigDirectly())
	{
		TMap<UTLLRN_ControlRig*, TArray<FTLLRN_RigElementKey>> SelectedControls;
		GetAllSelectedControls(SelectedControls);
		TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
		SelectedControls.GenerateKeyArray(TLLRN_ControlRigs);
		for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
		{
			if (TLLRN_ControlRig)
			{
				if (!TLLRN_ControlRig->bControlsVisible)
				{
					return false;
				}
			}
		}
		return true;
	}
	
	UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
	return !Settings->bHideControlShapes;
}

void FTLLRN_ControlRigEditMode::ZeroTransforms(bool bSelectionOnly)
{
	// Gather up the control rigs for the selected controls
	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			if (!bSelectionOnly || TLLRN_ControlRig->CurrentControlSelection().Num() > 0)
			{
				TLLRN_ControlRigs.Add(TLLRN_ControlRig);
			}
		}
	}
	if (TLLRN_ControlRigs.Num() == 0)
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("HierarchyZeroTransforms", "Zero Transforms"));
	FTLLRN_RigControlModifiedContext Context;
	Context.SetKey = ETLLRN_ControlRigSetKey::DoNotCare;

	for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(TLLRN_ControlRig);
		if (TLLRN_ControlRig->IsAdditive())
		{
			// For additive rigs, ignore boolean controls
			SelectedTLLRN_RigElements = SelectedTLLRN_RigElements.FilterByPredicate([TLLRN_ControlRig](const FTLLRN_RigElementKey& Key)
			{
				if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(Key.Name))
				{
					return Element->CanTreatAsAdditive();
				}
				return true;
			});
		}
		TArray<FTLLRN_RigElementKey> ControlsToReset = SelectedTLLRN_RigElements;
		TArray<FTLLRN_RigElementKey> ControlsInteracting = SelectedTLLRN_RigElements;
		TArray<FTLLRN_RigElementKey> TransformElementsToReset = SelectedTLLRN_RigElements;
		if (!bSelectionOnly)
		{
			TArray<FTLLRN_RigBaseElement*> Elements = TLLRN_ControlRig->GetHierarchy()->GetElementsOfType<FTLLRN_RigBaseElement>(true);
			TransformElementsToReset.Reset();
			TransformElementsToReset.Reserve(Elements.Num());
			for (const FTLLRN_RigBaseElement* Element : Elements)
			{
				// For additive rigs, ignore non-additive controls
				if (const FTLLRN_RigControlElement* Control = Cast<FTLLRN_RigControlElement>(Element))
				{
					if (TLLRN_ControlRig->IsAdditive() && !Control->CanTreatAsAdditive())
					{
						continue;
					}
				}
				TransformElementsToReset.Add(Element->GetKey());
			}
			
			TArray<FTLLRN_RigControlElement*> Controls;
			TLLRN_ControlRig->GetControlsInOrder(Controls);
			ControlsToReset.SetNum(0);
			ControlsInteracting.SetNum(0);
			for (const FTLLRN_RigControlElement* Control : Controls)
			{
				// For additive rigs, ignore boolean controls
				if (TLLRN_ControlRig->IsAdditive() && Control->Settings.ControlType == ETLLRN_RigControlType::Bool)
				{
					continue;
				}
				ControlsToReset.Add(Control->GetKey());
				if(Control->Settings.AnimationType == ETLLRN_RigControlAnimationType::AnimationControl ||
					Control->IsAnimationChannel())
				{
					ControlsInteracting.Add(Control->GetKey());
				}
			}
		}
		bool bHasNonDefaultParent = false;
		TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> Parents;
		for (const FTLLRN_RigElementKey& Key : TransformElementsToReset)
		{
			FTLLRN_RigElementKey SpaceKey = TLLRN_ControlRig->GetHierarchy()->GetActiveParent(Key);
			Parents.Add(Key, SpaceKey);
			if (!bHasNonDefaultParent && SpaceKey != TLLRN_ControlRig->GetHierarchy()->GetDefaultParentKey())
			{
				bHasNonDefaultParent = true;
			}
		}

		FTLLRN_ControlRigInteractionScope InteractionScope(TLLRN_ControlRig, ControlsInteracting);
		for (const FTLLRN_RigElementKey& ElementToReset : TransformElementsToReset)
		{
			FTLLRN_RigControlElement* ControlElement = nullptr;
			if (ElementToReset.Type == ETLLRN_RigElementType::Control)
			{
				ControlElement = TLLRN_ControlRig->FindControl(ElementToReset.Name);
				if (ControlElement->Settings.bIsTransientControl)
				{
					if(UTLLRN_ControlRig::GetNodeNameFromTransientControl(ControlElement->GetKey()).IsEmpty())
					{
						ControlElement = nullptr;
					}
				}
			}
			
			const FTransform InitialLocalTransform = TLLRN_ControlRig->GetInitialLocalTransform(ElementToReset);
			TLLRN_ControlRig->Modify();
			if (bHasNonDefaultParent == true) //possibly not at default parent so switch to it
			{
				TLLRN_ControlRig->GetHierarchy()->SwitchToDefaultParent(ElementToReset);
			}
			if (ControlElement)
			{
				TLLRN_ControlRig->SetControlLocalTransform(ElementToReset.Name, InitialLocalTransform, true, Context, true, true);
				const FVector InitialAngles = TLLRN_ControlRig->GetHierarchy()->GetControlPreferredEulerAngles(ControlElement, ControlElement->Settings.PreferredRotationOrder, true);
				TLLRN_ControlRig->GetHierarchy()->SetControlPreferredEulerAngles(ControlElement, InitialAngles, ControlElement->Settings.PreferredRotationOrder);
				NotifyDrivenControls(TLLRN_ControlRig, ElementToReset, Context);

				if (bHasNonDefaultParent == false)
				{
					TLLRN_ControlRig->ControlModified().Broadcast(TLLRN_ControlRig, ControlElement, ETLLRN_ControlRigSetKey::DoNotCare);
				}
			}
			else
			{
				TLLRN_ControlRig->GetHierarchy()->SetLocalTransform(ElementToReset, InitialLocalTransform, false, true, true);
			}

			//@helge not sure what to do if the non-default parent
			if (UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(TLLRN_ControlRig->GetClass()->ClassGeneratedBy))
			{
				Blueprint->Hierarchy->SetLocalTransform(ElementToReset, InitialLocalTransform);
			}
		}

		if (bHasNonDefaultParent == true) //now we have the initial pose setup we need to get the global transforms as specified now then set them in the current parent space
		{
			TLLRN_ControlRig->Evaluate_AnyThread();

			//get global transforms
			TMap<FTLLRN_RigElementKey, FTransform> GlobalTransforms;
			for (const FTLLRN_RigElementKey& ElementToReset : TransformElementsToReset)
			{
				if (ElementToReset.IsTypeOf(ETLLRN_RigElementType::Control))
				{
					FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ElementToReset.Name);
					if (ControlElement && !ControlElement->Settings.bIsTransientControl)
					{
						FTransform GlobalTransform = TLLRN_ControlRig->GetHierarchy()->GetGlobalTransform(ElementToReset);
						GlobalTransforms.Add(ElementToReset, GlobalTransform);
					}
					NotifyDrivenControls(TLLRN_ControlRig, ElementToReset,Context);
				}
				else
				{
					FTransform GlobalTransform = TLLRN_ControlRig->GetHierarchy()->GetGlobalTransform(ElementToReset);
					GlobalTransforms.Add(ElementToReset, GlobalTransform);
				}
			}
			//switch back to original parent space
			for (const FTLLRN_RigElementKey& ElementToReset : TransformElementsToReset)
			{
				if (const FTLLRN_RigElementKey* SpaceKey = Parents.Find(ElementToReset))
				{
					if (ElementToReset.IsTypeOf(ETLLRN_RigElementType::Control))
					{
						FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ElementToReset.Name);
						if (ControlElement && !ControlElement->Settings.bIsTransientControl)
						{
							TLLRN_ControlRig->GetHierarchy()->SwitchToParent(ElementToReset, *SpaceKey);
						}
					}
					else
					{
						TLLRN_ControlRig->GetHierarchy()->SwitchToParent(ElementToReset, *SpaceKey);
					}
				}
			}
			//set global transforms in this space // do it twice since ControlsInOrder is not really always in order
			for (int32 SetHack = 0; SetHack < 2; ++SetHack)
			{
				TLLRN_ControlRig->Evaluate_AnyThread();
				for (const FTLLRN_RigElementKey& ElementToReset : TransformElementsToReset)
				{
					if (const FTransform* GlobalTransform = GlobalTransforms.Find(ElementToReset))
					{
						if (ElementToReset.IsTypeOf(ETLLRN_RigElementType::Control))
						{
							FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ElementToReset.Name);
							if (ControlElement && !ControlElement->Settings.bIsTransientControl)
							{
								TLLRN_ControlRig->SetControlGlobalTransform(ElementToReset.Name, *GlobalTransform, true);
								TLLRN_ControlRig->Evaluate_AnyThread();
								NotifyDrivenControls(TLLRN_ControlRig, ElementToReset, Context);
							}
						}
						else
						{
							TLLRN_ControlRig->GetHierarchy()->SetGlobalTransform(ElementToReset, *GlobalTransform, false, true, true);
						}
					}
				}
			}
			//send notifies

			for (const FTLLRN_RigElementKey& ControlToReset : ControlsToReset)
			{
				FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ControlToReset.Name);
				if (ControlElement && !ControlElement->Settings.bIsTransientControl)
				{
					TLLRN_ControlRig->ControlModified().Broadcast(TLLRN_ControlRig, ControlElement, ETLLRN_ControlRigSetKey::DoNotCare);
				}
			}
		}
		else
		{
			// we have to insert the interaction event before we run current events
			TArray<FName> NewEventQueue = {FTLLRN_RigUnit_InteractionExecution::EventName};
			NewEventQueue.Append(TLLRN_ControlRig->EventQueue);
			TGuardValue<TArray<FName>> EventGuard(TLLRN_ControlRig->EventQueue, NewEventQueue);
			TLLRN_ControlRig->Evaluate_AnyThread();
			for (const FTLLRN_RigElementKey& ControlToReset : ControlsToReset)
			{
				NotifyDrivenControls(TLLRN_ControlRig, ControlToReset, Context);
			}
		}
	}
}

void FTLLRN_ControlRigEditMode::InvertInputPose(bool bSelectionOnly)
{
	// Gather up the control rigs for the selected controls
	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigs;
	for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			if (!bSelectionOnly || TLLRN_ControlRig->CurrentControlSelection().Num() > 0)
			{
				TLLRN_ControlRigs.Add(TLLRN_ControlRig);
			}
		}
	}
	if (TLLRN_ControlRigs.Num() == 0)
	{
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("HierarchyInvertTransformsToRestPose", "Invert Transforms to Rest Pose"));

	for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigs)
	{
		if (!TLLRN_ControlRig->IsAdditive())
		{
			ZeroTransforms(bSelectionOnly);
			continue;
		}

		TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements;
		if (bSelectionOnly)
		{
			SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(TLLRN_ControlRig);
			SelectedTLLRN_RigElements = SelectedTLLRN_RigElements.FilterByPredicate([TLLRN_ControlRig](const FTLLRN_RigElementKey& Key)
			{
				if (FTLLRN_RigControlElement* Element = TLLRN_ControlRig->FindControl(Key.Name))
				{
					return Element->Settings.ControlType != ETLLRN_RigControlType::Bool;
				}
				return true;
			});
		}

		const TArray<FTLLRN_RigControlElement*> ModifiedElements = TLLRN_ControlRig->InvertInputPose(SelectedTLLRN_RigElements, ETLLRN_ControlRigSetKey::Never);
		TLLRN_ControlRig->Evaluate_AnyThread();

		for (FTLLRN_RigControlElement* ControlElement : ModifiedElements)
		{
			TLLRN_ControlRig->ControlModified().Broadcast(TLLRN_ControlRig, ControlElement, ETLLRN_ControlRigSetKey::DoNotCare);
		}
	}
}

bool FTLLRN_ControlRigEditMode::MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InX, int32 InY)
{
	// avoid hit proxy cast as much as possible
	// NOTE: with synthesized mouse moves, this is being called a lot sadly so playing in sequencer with the mouse over the viewport leads to fps drop
	auto HasAnyHoverableShapeActor = [this, InViewportClient]()
	{
		if (!InViewportClient || InViewportClient->IsInGameView())
		{
			return false;
		}
		
		for (auto&[TLLRN_ControlRig, ShapeActors]: TLLRN_ControlRigShapeActors)
		{
			for (const ATLLRN_ControlRigShapeActor* ShapeActor : ShapeActors)
			{
				if (ShapeActor && ShapeActor->IsSelectable() && !ShapeActor->IsTemporarilyHiddenInEditor())
				{
					return true;
				}
			}
		}
		return false;
	};
	
	if (HasAnyHoverableShapeActor())
	{	
		const HActor* ActorHitProxy = HitProxyCast<HActor>(InViewport->GetHitProxy(InX, InY));
		const ATLLRN_ControlRigShapeActor* HitShape = ActorHitProxy && ActorHitProxy->Actor ? Cast<ATLLRN_ControlRigShapeActor>(ActorHitProxy->Actor) : nullptr;
		auto IsHovered = [HitShape](const ATLLRN_ControlRigShapeActor* InShapeActor)
		{
			return HitShape ? InShapeActor == HitShape : false;
		};

		for (const auto& [TLLRN_ControlRig, Shapes] : TLLRN_ControlRigShapeActors)
		{
			for (ATLLRN_ControlRigShapeActor* ShapeActor : Shapes)
			{
				if (ShapeActor)
				{
					ShapeActor->SetHovered(IsHovered(ShapeActor));
				}
			}
		}
	}

	return false;
}

bool FTLLRN_ControlRigEditMode::MouseEnter(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InX, int32 InY)
{
	if (PendingFocus.IsEnabled() && InViewportClient)
	{
		const FEditorModeTools* ModeTools = GetModeManager();  
		if (ModeTools && ModeTools == &GLevelEditorModeTools())
		{
			FEditorViewportClient* HoveredVPC = ModeTools->GetHoveredViewportClient();
			if (HoveredVPC == InViewportClient)
			{
				const TWeakPtr<SViewport> ViewportWidget = InViewportClient->GetEditorViewportWidget()->GetSceneViewport()->GetViewportWidget();
				if (ViewportWidget.IsValid())
				{
					PendingFocus.SetPendingFocusIfNeeded(ViewportWidget.Pin()->GetContent());
				}
			}
		}
	}

	return IPersonaEditMode::MouseEnter(InViewportClient, InViewport, InX, InY);
}

bool FTLLRN_ControlRigEditMode::MouseLeave(FEditorViewportClient* /*InViewportClient*/, FViewport* /*InViewport*/)
{
	PendingFocus.ResetPendingFocus();
	
	for (auto& ShapeActors : TLLRN_ControlRigShapeActors)
	{
		for (ATLLRN_ControlRigShapeActor* ShapeActor : ShapeActors.Value)
		{
			ShapeActor->SetHovered(false);
		}
	}

	return false;
}

void FTLLRN_ControlRigEditMode::RegisterPendingFocusMode()
{
	if (!IsInLevelEditor())
	{
		return;
	}
	
	static IConsoleVariable* UseFocusMode = CRModeLocals::GetFocusModeVariable();
	if (ensure(UseFocusMode))
	{
		auto OnFocusModeChanged = [this](IConsoleVariable*)
		{
			PendingFocus.Enable(CRModeLocals::bFocusMode);
			if (WeakSequencer.IsValid())
			{
				TSharedPtr<ISequencer> PreviousSequencer = WeakSequencer.Pin();
				TSharedRef<SSequencer> PreviousSequencerWidget = StaticCastSharedRef<SSequencer>(PreviousSequencer->GetSequencerWidget());
				PreviousSequencerWidget->EnablePendingFocusOnHovering(CRModeLocals::bFocusMode);
			}
		};
		if (!PendingFocusHandle.IsValid())
		{
			PendingFocusHandle = UseFocusMode->OnChangedDelegate().AddLambda(OnFocusModeChanged);
		}		
		OnFocusModeChanged(UseFocusMode);
	}
}

void FTLLRN_ControlRigEditMode::UnregisterPendingFocusMode()
{
	static constexpr bool bDisable = false;
	if (WeakSequencer.IsValid())
	{
		TSharedRef<SSequencer> SequencerWidget = StaticCastSharedRef<SSequencer>(WeakSequencer.Pin()->GetSequencerWidget());
		SequencerWidget->EnablePendingFocusOnHovering(bDisable);
	}

	PendingFocus.Enable(bDisable);
	
	if (PendingFocusHandle.IsValid())
	{
		static IConsoleVariable* UseFocusMode = CRModeLocals::GetFocusModeVariable();
		if (ensure(UseFocusMode))
		{
			UseFocusMode->OnChangedDelegate().Remove(PendingFocusHandle);
		}
		PendingFocusHandle.Reset();
	}
}

bool FTLLRN_ControlRigEditMode::CheckMovieSceneSig()
{
	bool bSomethingChanged = false;
	if (WeakSequencer.IsValid())
	{
		TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
		if (Sequencer->GetFocusedMovieSceneSequence() && Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene())
		{
			FGuid CurrentMovieSceneSig = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetSignature();
			if (LastMovieSceneSig != CurrentMovieSceneSig)
			{
				if (ULevelSequence* LevelSequence = Cast<ULevelSequence>(Sequencer->GetFocusedMovieSceneSequence()))
				{
					TArray<TWeakObjectPtr<UTLLRN_ControlRig>> CurrentTLLRN_ControlRigs;
					TArray<FTLLRN_ControlRigSequencerBindingProxy> Proxies = UTLLRN_ControlRigSequencerEditorLibrary::GetTLLRN_ControlRigs(LevelSequence);
					for (FTLLRN_ControlRigSequencerBindingProxy& Proxy : Proxies)
					{
						if (UTLLRN_ControlRig* TLLRN_ControlRig = Proxy.TLLRN_ControlRig.Get())
						{
							CurrentTLLRN_ControlRigs.Add(TLLRN_ControlRig);
							if (RuntimeTLLRN_ControlRigs.Contains(TLLRN_ControlRig) == false)
							{
								AddTLLRN_ControlRigInternal(TLLRN_ControlRig);
								bSomethingChanged = true;
							}
						}
					}
					TArray<TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigsToRemove;
					for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
					{
						if (CurrentTLLRN_ControlRigs.Contains(RuntimeRigPtr) == false)
						{
							TLLRN_ControlRigsToRemove.Add(RuntimeRigPtr);
						}
					}
					for (TWeakObjectPtr<UTLLRN_ControlRig>& OldRuntimeRigPtr : TLLRN_ControlRigsToRemove)
					{
						RemoveTLLRN_ControlRig(OldRuntimeRigPtr.Get());
					}
				}
				LastMovieSceneSig = CurrentMovieSceneSig;
				if (bSomethingChanged)
				{
					SetObjects_Internal();
				}
				DetailKeyFrameCache->ResetCachedData();
			}
		}
	}
	return bSomethingChanged;
}

void FTLLRN_ControlRigEditMode::PostUndo()
{
	bool bInvalidateViewport = false;
	if (WeakSequencer.IsValid())
	{
		bool bHaveInvalidTLLRN_ControlRig = false;
		for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
		{
			if (RuntimeRigPtr.IsValid() == false)
			{				
				bHaveInvalidTLLRN_ControlRig = bInvalidateViewport = true;
				break;
			}
		}
		//if one is invalid we need to clear everything,since no longer have ptr to selectively delete
		if (bHaveInvalidTLLRN_ControlRig == true)
		{
			TArray<TWeakObjectPtr<UTLLRN_ControlRig>> PreviousRuntimeRigs = RuntimeTLLRN_ControlRigs;
			for (int32 PreviousRuntimeRigIndex = 0; PreviousRuntimeRigIndex < PreviousRuntimeRigs.Num(); PreviousRuntimeRigIndex++)
			{
				if (PreviousRuntimeRigs[PreviousRuntimeRigIndex].IsValid())
				{
					RemoveTLLRN_ControlRig(PreviousRuntimeRigs[PreviousRuntimeRigIndex].Get());
				}
			}
			RuntimeTLLRN_ControlRigs.Reset();
			DestroyShapesActors(nullptr);
			DelegateHelpers.Reset();
			RuntimeTLLRN_ControlRigs.Reset();
		}
		TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
		if (ULevelSequence* LevelSequence = Cast<ULevelSequence>(Sequencer->GetFocusedMovieSceneSequence()))
		{
			bool bSomethingAdded = false;
			TArray<FTLLRN_ControlRigSequencerBindingProxy> Proxies = UTLLRN_ControlRigSequencerEditorLibrary::GetTLLRN_ControlRigs(LevelSequence);
			for (FTLLRN_ControlRigSequencerBindingProxy& Proxy : Proxies)
			{
				if (UTLLRN_ControlRig* TLLRN_ControlRig = Proxy.TLLRN_ControlRig.Get())
				{
					if (RuntimeTLLRN_ControlRigs.Contains(TLLRN_ControlRig) == false)
					{
						AddTLLRN_ControlRigInternal(TLLRN_ControlRig);
						bSomethingAdded = true;

					}
				}
			}
			if (bSomethingAdded)
			{
				Sequencer->ForceEvaluate();
				SetObjects_Internal();
				bInvalidateViewport = true;
			}
		}
	}
	else
	{
		for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
		{
			if (RuntimeRigPtr.IsValid() == false)
			{
				DestroyShapesActors(RuntimeRigPtr.Get());
				bInvalidateViewport = true;
			}
		}
	}

	//normal actor undo will force the redraw, so we need to do the same for our transients/controls.
	if (!AreEditingTLLRN_ControlRigDirectly() && (bInvalidateViewport || UsesTransformWidget()))
	{
		GEditor->GetTimerManager()->SetTimerForNextTick([this]()
		{
			//due to tick ordering need to manually make sure we get everything done in correct order.
			PostPoseUpdate();
			UpdatePivotTransforms();
			GEditor->RedrawLevelEditingViewports(true);
		});
	}

}

void FTLLRN_ControlRigEditMode::RequestToRecreateControlShapeActors(UTLLRN_ControlRig* TLLRN_ControlRig)
{ 
	if (TLLRN_ControlRig)
	{
		if (RecreateControlShapesRequired != ERecreateTLLRN_ControlRigShape::RecreateAll)
		{
			RecreateControlShapesRequired = ERecreateTLLRN_ControlRigShape::RecreateSpecified;
			if (TLLRN_ControlRigsToRecreate.Find(TLLRN_ControlRig) == INDEX_NONE)
			{
				TLLRN_ControlRigsToRecreate.Add(TLLRN_ControlRig);
			}
		}
	}
	else
	{
		RecreateControlShapesRequired = ERecreateTLLRN_ControlRigShape::RecreateAll;
	}
}

// temporarily we just support following types of gizmo
static bool IsSupportedControlType(const ETLLRN_RigControlType ControlType)
{
	switch (ControlType)
	{
	case ETLLRN_RigControlType::Float:
	case ETLLRN_RigControlType::ScaleFloat:
	case ETLLRN_RigControlType::Integer:
	case ETLLRN_RigControlType::Vector2D:
	case ETLLRN_RigControlType::Position:
	case ETLLRN_RigControlType::Scale:
	case ETLLRN_RigControlType::Rotator:
	case ETLLRN_RigControlType::Transform:
	case ETLLRN_RigControlType::TransformNoScale:
	case ETLLRN_RigControlType::EulerTransform:
	{
		return true;
	}
	default:
	{
		break;
	}
	}

	return false;
}

bool FTLLRN_ControlRigEditMode::TryUpdatingControlsShapes(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	using namespace TLLRN_ControlRigEditMode::Shapes;
	
	const UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig ? InTLLRN_ControlRig->GetHierarchy() : nullptr;
	if (!Hierarchy)
	{
		return false;
	}

	const auto* ShapeActors = TLLRN_ControlRigShapeActors.Find(InTLLRN_ControlRig);
	if (!ShapeActors)
	{
		// create the shapes if they don't already exist
		CreateShapeActors(InTLLRN_ControlRig);
		return true;
	}
	
	// get controls which need shapes
	TArray<FTLLRN_RigControlElement*> Controls;
	GetControlsEligibleForShapes(InTLLRN_ControlRig, Controls);

	if (Controls.IsEmpty())
	{
		// not control needing shape so clear the shape actors
		DestroyShapesActors(InTLLRN_ControlRig);
		return true;
	}
	
	const TArray<TObjectPtr<ATLLRN_ControlRigShapeActor>>& Shapes = *ShapeActors;
	const int32 NumShapes = Shapes.Num();

	TArray<FTLLRN_RigControlElement*> ControlPerShapeActor;
	ControlPerShapeActor.SetNumZeroed(NumShapes);

	if (Controls.Num() == NumShapes)
	{
		//unfortunately n*n-ish but this should be very rare and much faster than recreating them
		for (int32 ShapeActorIndex = 0; ShapeActorIndex < NumShapes; ShapeActorIndex++)
		{
			if (const ATLLRN_ControlRigShapeActor* Actor = Shapes[ShapeActorIndex].Get())
			{
				const int32 ControlIndex = Controls.IndexOfByPredicate([Actor](const FTLLRN_RigControlElement* Control)
				{
					return Control && Control->GetFName() == Actor->ControlName;
				});
				if (ControlIndex != INDEX_NONE)
				{
					ControlPerShapeActor[ShapeActorIndex] = Controls[ControlIndex];
					Controls.RemoveAtSwap(ControlIndex);
				}
			}
			else //no actor just recreate
			{
				return false;
			}
		}
	}

	// Some controls don't have associated shape so recreate them 
	if (!Controls.IsEmpty())
	{
		return false;
	}

	// we have matching controls - we should at least sync their settings.
	// PostPoseUpdate / TickControlShape is going to take care of color, visibility etc.
	// MeshTransform has to be handled here.
	const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>>& ShapeLibraries = InTLLRN_ControlRig->GetShapeLibraries();
	for (int32 ShapeActorIndex = 0; ShapeActorIndex < NumShapes; ShapeActorIndex++)
	{
		const ATLLRN_ControlRigShapeActor* ShapeActor = Shapes[ShapeActorIndex].Get();
		FTLLRN_RigControlElement* ControlElement = ControlPerShapeActor[ShapeActorIndex];
		if (ShapeActor && ControlElement)
		{
			const FTransform ShapeTransform = Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
			if (const FTLLRN_ControlRigShapeDefinition* ShapeDef = UTLLRN_ControlRigShapeLibrary::GetShapeByName(ControlElement->Settings.ShapeName, ShapeLibraries, InTLLRN_ControlRig->ShapeLibraryNameMap))
			{
				const FTransform& MeshTransform = ShapeDef->Transform;
				if (UStaticMesh* ShapeMesh = ShapeDef->StaticMesh.LoadSynchronous())
				{
					if(ShapeActor->StaticMeshComponent->GetStaticMesh() != ShapeMesh)
					{
						ShapeActor->StaticMeshComponent->SetStaticMesh(ShapeMesh);
					}
				}
				ShapeActor->StaticMeshComponent->SetRelativeTransform(MeshTransform * ShapeTransform);
			}
			else
			{
				ShapeActor->StaticMeshComponent->SetRelativeTransform(ShapeTransform);
			}
		}
	}

	// equivalent to PostPoseUpdate for those shapes only
	FTransform ComponentTransform = FTransform::Identity;
	if (!AreEditingTLLRN_ControlRigDirectly())
	{
		ComponentTransform = GetHostingSceneComponentTransform(InTLLRN_ControlRig);
	}
		
	const FShapeUpdateParams Params(InTLLRN_ControlRig, ComponentTransform, IsTLLRN_ControlRigSkelMeshVisible(InTLLRN_ControlRig));
	for (int32 ShapeActorIndex = 0; ShapeActorIndex < NumShapes; ShapeActorIndex++)
	{
		ATLLRN_ControlRigShapeActor* ShapeActor = Shapes[ShapeActorIndex].Get();
		FTLLRN_RigControlElement* ControlElement = ControlPerShapeActor[ShapeActorIndex];
		if (ShapeActor && ControlElement)
		{
			UpdateControlShape(ShapeActor, ControlElement, Params);
		}

		// workaround for UE-225122, FPrimitiveSceneProxy currently lazily updates the transform, but due to a thread sync issue,
		// if we are setting the transform to 0 at tick 1 and setting it to the correct value like 100 at tick2, depending on
		// the value of the cached transform, only one of the two sets would be committed. This call clears the cached transform to 0 such that
		// set to 0(here) is always ignored and set to 100(TickControlShape) is always accepted.
		ShapeActor->MarkComponentsRenderStateDirty();
	}

	return true;
}

void FTLLRN_ControlRigEditMode::RecreateControlShapeActors()
{
	if (RecreateControlShapesRequired == ERecreateTLLRN_ControlRigShape::RecreateAll)
	{
		// recreate all control rigs shape actors
		for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
		{
			if (UTLLRN_ControlRig* RuntimeTLLRN_ControlRig = RuntimeRigPtr.Get())
			{
				DestroyShapesActors(RuntimeTLLRN_ControlRig);
				CreateShapeActors(RuntimeTLLRN_ControlRig);
			}
		}
		RecreateControlShapesRequired = ERecreateTLLRN_ControlRigShape::RecreateNone;
		return;
	}

	if (TLLRN_ControlRigsToRecreate.IsEmpty())
	{
		// nothing to update
		return;
	}
	
	// update or recreate all control rigs in TLLRN_ControlRigsToRecreate
	TArray<UTLLRN_ControlRig*> TLLRN_ControlRigsCopy = TLLRN_ControlRigsToRecreate;
	for (UTLLRN_ControlRig* TLLRN_ControlRig : TLLRN_ControlRigsCopy)
	{
		const bool bUpdated = TryUpdatingControlsShapes(TLLRN_ControlRig);
		if (!bUpdated)
		{
			DestroyShapesActors(TLLRN_ControlRig);
			CreateShapeActors(TLLRN_ControlRig);
		}
	}
	RecreateControlShapesRequired = ERecreateTLLRN_ControlRigShape::RecreateNone;
	TLLRN_ControlRigsToRecreate.SetNum(0);
}

void FTLLRN_ControlRigEditMode::CreateShapeActors(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	using namespace TLLRN_ControlRigEditMode::Shapes;
	
	if(bShowControlsAsOverlay)
	{
		// enable translucent selection
		GetMutableDefault<UEditorPerProjectUserSettings>()->bAllowSelectTranslucent = true;
	}

	const TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>> ShapeLibraries = InTLLRN_ControlRig->GetShapeLibraries();

	const int32 TLLRN_ControlRigIndex = RuntimeTLLRN_ControlRigs.Find(InTLLRN_ControlRig);
	const UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig->GetHierarchy();

	// get controls for which shapes are needed in the editor
	TArray<FTLLRN_RigControlElement*> Controls;
	GetControlsEligibleForShapes(InTLLRN_ControlRig, Controls);

	// new shape actors to be created
	TArray<ATLLRN_ControlRigShapeActor*> NewShapeActors;
	NewShapeActors.Reserve(Controls.Num());

	for (FTLLRN_RigControlElement* ControlElement : Controls)
	{
		const FTLLRN_RigControlSettings& ControlSettings = ControlElement->Settings;
		
		FTLLRN_ControlShapeActorCreationParam Param;
		Param.ManipObj = InTLLRN_ControlRig;
		Param.TLLRN_ControlRigIndex = TLLRN_ControlRigIndex;
		Param.TLLRN_ControlRig = InTLLRN_ControlRig;
		Param.ControlName = ControlElement->GetFName();
		Param.ShapeName = ControlSettings.ShapeName;
		Param.SpawnTransform = InTLLRN_ControlRig->GetControlGlobalTransform(ControlElement->GetFName());
		Param.ShapeTransform = Hierarchy->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
		Param.bSelectable = ControlSettings.IsSelectable(false);

		if (const FTLLRN_ControlRigShapeDefinition* ShapeDef = UTLLRN_ControlRigShapeLibrary::GetShapeByName(ControlSettings.ShapeName, ShapeLibraries, InTLLRN_ControlRig->ShapeLibraryNameMap))
		{
			Param.MeshTransform = ShapeDef->Transform;
			Param.StaticMesh = ShapeDef->StaticMesh;
			Param.Material = ShapeDef->Library->DefaultMaterial;
			if (bShowControlsAsOverlay)
			{
				TSoftObjectPtr<UMaterial> XRayMaterial = ShapeDef->Library->XRayMaterial;
				if (XRayMaterial.IsPending())
				{
					XRayMaterial.LoadSynchronous();
				}
				if (XRayMaterial.IsValid())
				{
					Param.Material = XRayMaterial;
				}
			}
			Param.ColorParameterName = ShapeDef->Library->MaterialColorParameter;
		}

		Param.Color = ControlSettings.ShapeColor;

		// create a new shape actor that will represent that control in the editor
		ATLLRN_ControlRigShapeActor* NewShapeActor = FTLLRN_ControlRigShapeHelper::CreateDefaultShapeActor(WorldPtr, Param);
		if (NewShapeActor)
		{
			//not drawn in game or in game view.
			NewShapeActor->SetActorHiddenInGame(true);
			NewShapeActors.Add(NewShapeActor);
		}
	}

	// add or replace shape actors
	auto* ShapeActors = TLLRN_ControlRigShapeActors.Find(InTLLRN_ControlRig);
	if (ShapeActors)
	{
		// this shouldn't happen but make sure we destroy any existing shape
		DestroyShapesActorsFromWorld(*ShapeActors);
		*ShapeActors = NewShapeActors;
	}
	else
	{
		ShapeActors = &TLLRN_ControlRigShapeActors.Emplace(InTLLRN_ControlRig, ObjectPtrWrap(NewShapeActors));
	}

	// setup shape actors
	if(ensure(ShapeActors))
	{
		const USceneComponent* Component = GetHostingSceneComponent(InTLLRN_ControlRig);
		if (AActor* PreviewActor = Component ? Component->GetOwner() : nullptr)
		{
			for (ATLLRN_ControlRigShapeActor* ShapeActor : *ShapeActors)
			{
				// attach to preview actor, so that we can communicate via relative transform from the preview actor
				ShapeActor->AttachToActor(PreviewActor, FAttachmentTransformRules::KeepWorldTransform);

				TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
				ShapeActor->GetComponents(PrimitiveComponents, true);
				for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
				{
					PrimitiveComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateRaw(this, &FTLLRN_ControlRigEditMode::ShapeSelectionOverride);
					PrimitiveComponent->PushSelectionToProxy();
				}
			}
		}
	}

	if (!AreEditingTLLRN_ControlRigDirectly())
	{
		if (ControlProxy)
		{
			ControlProxy->RecreateAllProxies(InTLLRN_ControlRig);
		}
	}
}

FTLLRN_ControlRigEditMode* FTLLRN_ControlRigEditMode::GetEditModeFromWorldContext(UWorld* InWorldContext)
{
	return nullptr;
}

bool FTLLRN_ControlRigEditMode::ShapeSelectionOverride(const UPrimitiveComponent* InComponent) const
{
    //Think we only want to do this in regular editor, in the level editor we are driving selection
	if (AreEditingTLLRN_ControlRigDirectly())
	{
	    ATLLRN_ControlRigShapeActor* OwnerActor = Cast<ATLLRN_ControlRigShapeActor>(InComponent->GetOwner());
	    if (OwnerActor)
	    {
		    // See if the actor is in a selected unit proxy
		    return OwnerActor->IsSelected();
	    }
	}

	return false;
}

void FTLLRN_ControlRigEditMode::OnObjectsReplaced(const TMap<UObject*, UObject*>& OldToNewInstanceMap)
{
	for (int32 RigIndex = 0; RigIndex < RuntimeTLLRN_ControlRigs.Num(); RigIndex++)
	{
		UObject* OldObject = RuntimeTLLRN_ControlRigs[RigIndex].Get();
		UObject* NewObject = OldToNewInstanceMap.FindRef(OldObject);
		if (NewObject)
		{
			TArray<TWeakObjectPtr<UTLLRN_ControlRig>> PreviousRuntimeRigs = RuntimeTLLRN_ControlRigs;
			for (int32 PreviousRuntimeRigIndex = 0; PreviousRuntimeRigIndex < PreviousRuntimeRigs.Num(); PreviousRuntimeRigIndex++)
			{
				if (PreviousRuntimeRigs[PreviousRuntimeRigIndex].IsValid())
				{
					RemoveTLLRN_ControlRig(PreviousRuntimeRigs[PreviousRuntimeRigIndex].Get());
				}
			}
			RuntimeTLLRN_ControlRigs.Reset();

			UTLLRN_ControlRig* NewRig = Cast<UTLLRN_ControlRig>(NewObject);
			AddTLLRN_ControlRigInternal(NewRig);

			NewRig->Initialize();

			SetObjects_Internal();
		}
	}
}

bool FTLLRN_ControlRigEditMode::IsTransformDelegateAvailable() const
{
	return (OnGetTLLRN_RigElementTransformDelegate.IsBound() && OnSetTLLRN_RigElementTransformDelegate.IsBound());
}

bool FTLLRN_ControlRigEditMode::AreTLLRN_RigElementSelectedAndMovable(UTLLRN_ControlRig* TLLRN_ControlRig) const
{
	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();

	if (!Settings || !TLLRN_ControlRig)
	{
		return false;
	}

	auto IsAnySelectedControlMovable = [this, TLLRN_ControlRig]()
	{
		UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
		if (!Hierarchy)
		{
			return false;
		}
		
		const TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(TLLRN_ControlRig);
		return SelectedTLLRN_RigElements.ContainsByPredicate([Hierarchy](const FTLLRN_RigElementKey& Element)
		{
			if (!FTLLRN_RigElementTypeHelper::DoesHave(ValidControlTypeMask(), Element.Type))
			{
				return false;
			}
			const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(Element);
			if (!ControlElement)
			{
				return false;
			}
			// can a control non selectable in the viewport be movable?  
			return ControlElement->Settings.IsSelectable();
		});
	};
	
	if (!IsAnySelectedControlMovable())
	{
		return false;
	}

	//when in sequencer/level we don't have that delegate so don't check.
	if (AreEditingTLLRN_ControlRigDirectly())
	{
		if (!IsTransformDelegateAvailable())
		{
			return false;
		}
	}
	else //do check for the binding though
	{
		// if (GetHostingSceneComponent(TLLRN_ControlRig) == nullptr)
		// {
		// 	return false;
		// }
	}

	return true;
}

void FTLLRN_ControlRigEditMode::ReplaceTLLRN_ControlRig(UTLLRN_ControlRig* OldTLLRN_ControlRig, UTLLRN_ControlRig* NewTLLRN_ControlRig)
{
	if (OldTLLRN_ControlRig != nullptr)
	{
		RemoveTLLRN_ControlRig(OldTLLRN_ControlRig);
	}
	AddTLLRN_ControlRigInternal(NewTLLRN_ControlRig);
	SetObjects_Internal();
	RequestToRecreateControlShapeActors(NewTLLRN_ControlRig);

}
void FTLLRN_ControlRigEditMode::OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	if(bSuspendHierarchyNotifs || InElement == nullptr)
	{
		return;
	}

	check(InElement);
	switch(InNotif)
	{
		case ETLLRN_RigHierarchyNotification::ElementAdded:
		case ETLLRN_RigHierarchyNotification::ElementRemoved:
		case ETLLRN_RigHierarchyNotification::ElementRenamed:
		case ETLLRN_RigHierarchyNotification::ElementReordered:
		case ETLLRN_RigHierarchyNotification::HierarchyReset:
		{
			UTLLRN_ControlRig* TLLRN_ControlRig = InHierarchy->GetTypedOuter<UTLLRN_ControlRig>();
			RequestToRecreateControlShapeActors(TLLRN_ControlRig);
			break;
		}
		case ETLLRN_RigHierarchyNotification::ControlSettingChanged:
		case ETLLRN_RigHierarchyNotification::ControlVisibilityChanged:
		case ETLLRN_RigHierarchyNotification::ControlShapeTransformChanged:
		{
			const FTLLRN_RigElementKey Key = InElement->GetKey();
			UTLLRN_ControlRig* TLLRN_ControlRig = InHierarchy->GetTypedOuter<UTLLRN_ControlRig>();
			if (Key.Type == ETLLRN_RigElementType::Control)
			{
				if (const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(InElement))
				{
					if (ATLLRN_ControlRigShapeActor* ShapeActor = GetControlShapeFromControlName(TLLRN_ControlRig,Key.Name))
					{
						// try to lazily apply the changes to the actor
						const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
						if (ShapeActor->UpdateControlSettings(InNotif, TLLRN_ControlRig, ControlElement, Settings->bHideControlShapes, !AreEditingTLLRN_ControlRigDirectly()))
						{
							break;
						}
					}
				}
			}

			if(TLLRN_ControlRig != nullptr)
			{
				// if we can't deal with this lazily, let's fall back to recreating all control shape actors
				RequestToRecreateControlShapeActors(TLLRN_ControlRig);
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::ControlDrivenListChanged:
		{
			if (!AreEditingTLLRN_ControlRigDirectly())
			{
				// to synchronize the selection between the viewport / editmode and the details panel / sequencer
				// we re-select the control. during deselection we recover the previously set driven list
				// and then select the control again with the up2date list. this makes sure that the tracks
				// are correctly selected in sequencer to match what the proxy control is driving.
				if (FTLLRN_RigControlElement* ControlElement = InHierarchy->Find<FTLLRN_RigControlElement>(InElement->GetKey()))
				{
					UTLLRN_ControlRig* TLLRN_ControlRig = InHierarchy->GetTypedOuter<UTLLRN_ControlRig>();
					if(ControlProxy->IsSelected(TLLRN_ControlRig, ControlElement))
					{
						// reselect the control - to affect the details panel / sequencer
						if(UTLLRN_RigHierarchyController* Controller = InHierarchy->GetController())
						{
							const FTLLRN_RigElementKey Key = ControlElement->GetKey();
							{
								// Restore the previously selected driven elements
								// so that we can deselect them accordingly.
								TGuardValue<TArray<FTLLRN_RigElementKey>> DrivenGuard(
									ControlElement->Settings.DrivenControls,
									ControlElement->Settings.PreviouslyDrivenControls);
								
								Controller->DeselectElement(Key);
							}

							// now select the proxy control again given the new driven list
							Controller->SelectElement(Key);
						}
					}
				}
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementSelected:
		case ETLLRN_RigHierarchyNotification::ElementDeselected:
		{
			const FTLLRN_RigElementKey Key = InElement->GetKey();

			switch (InElement->GetType())
			{
				case ETLLRN_RigElementType::Bone:
            	case ETLLRN_RigElementType::Null:
            	case ETLLRN_RigElementType::Curve:
            	case ETLLRN_RigElementType::Control:
            	case ETLLRN_RigElementType::Physics:
            	case ETLLRN_RigElementType::Reference:
            	case ETLLRN_RigElementType::Connector:
            	case ETLLRN_RigElementType::Socket:
				{
					const bool bSelected = InNotif == ETLLRN_RigHierarchyNotification::ElementSelected;
					// users may select gizmo and control rig units, so we have to let them go through both of them if they do
						// first go through gizmo actor
					UTLLRN_ControlRig* TLLRN_ControlRig = InHierarchy->GetTypedOuter<UTLLRN_ControlRig>();
					if (TLLRN_ControlRig == nullptr)
					{
						if (RuntimeTLLRN_ControlRigs.Num() > 0)
						{
							TLLRN_ControlRig = RuntimeTLLRN_ControlRigs[0].Get();
						}
					}
					if (TLLRN_ControlRig)
					{
						OnTLLRN_ControlRigSelectedDelegate.Broadcast(TLLRN_ControlRig, Key, bSelected);
					}
					// if it's control
					if (Key.Type == ETLLRN_RigElementType::Control)
					{
						FScopedTransaction ScopedTransaction(LOCTEXT("SelectControlTransaction", "Select Control"), !AreEditingTLLRN_ControlRigDirectly() && !GIsTransacting);
						if (!AreEditingTLLRN_ControlRigDirectly())
						{
							ControlProxy->Modify();
						}
						
						ATLLRN_ControlRigShapeActor* ShapeActor = GetControlShapeFromControlName(TLLRN_ControlRig,Key.Name);
						if (ShapeActor)
						{
							ShapeActor->SetSelected(bSelected);

						}
						if (!AreEditingTLLRN_ControlRigDirectly())
						{
							if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->GetHierarchy()->Find<FTLLRN_RigControlElement>(Key))
							{
								ControlProxy->SelectProxy(TLLRN_ControlRig, ControlElement, bSelected);

								if(ControlElement->CanDriveControls())
								{
									const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();

									const TArray<FTLLRN_RigElementKey>& DrivenKeys = ControlElement->Settings.DrivenControls;
									for(const FTLLRN_RigElementKey& DrivenKey : DrivenKeys)
									{
										if (FTLLRN_RigControlElement* DrivenControl = TLLRN_ControlRig->GetHierarchy()->Find<FTLLRN_RigControlElement>(DrivenKey))
										{
											ControlProxy->SelectProxy(TLLRN_ControlRig, DrivenControl, bSelected);

											if (ATLLRN_ControlRigShapeActor* DrivenShapeActor = GetControlShapeFromControlName(TLLRN_ControlRig,DrivenControl->GetFName()))
											{
												if(bSelected)
												{
													DrivenShapeActor->OverrideColor = Settings->DrivenControlColor;
												}
												else
												{
													DrivenShapeActor->OverrideColor = FLinearColor(0, 0, 0, 0);
												}
											}
										}
									}

									TLLRN_ControlRig->GetHierarchy()->ForEach<FTLLRN_RigControlElement>(
										[this, TLLRN_ControlRig, ControlElement, DrivenKeys, bSelected](FTLLRN_RigControlElement* AnimationChannelControl) -> bool
										{
											if(AnimationChannelControl->IsAnimationChannel())
											{
												if(const FTLLRN_RigControlElement* ParentControlElement =
													Cast<FTLLRN_RigControlElement>(TLLRN_ControlRig->GetHierarchy()->GetFirstParent(AnimationChannelControl)))
												{
													if(DrivenKeys.Contains(ParentControlElement->GetKey()) ||
														ParentControlElement->GetKey() == ControlElement->GetKey())
													{
														ControlProxy->SelectProxy(TLLRN_ControlRig, AnimationChannelControl, bSelected);
														return true;
													}
												}
												if(AnimationChannelControl->Settings.Customization.AvailableSpaces.Contains(ControlElement->GetKey()))
												{
													ControlProxy->SelectProxy(TLLRN_ControlRig, AnimationChannelControl, bSelected);
													return true;
												}
											}
											return true;
										}
									);
								}
							}
						}

					}
					bSelectionChanged = true;
		
					break;
				}
				default:
				{
					ensureMsgf(false, TEXT("Unsupported Type of TLLRN_RigElement: %s"), *Key.ToString());
					break;
				}
			}
		}
		case ETLLRN_RigHierarchyNotification::ParentWeightsChanged:
		{
			// TickManipulatableObjects(0.f);
			break;
		}
		case ETLLRN_RigHierarchyNotification::InteractionBracketOpened:
		case ETLLRN_RigHierarchyNotification::InteractionBracketClosed:
		default:
		{
			break;
		}
	}
}

void FTLLRN_ControlRigEditMode::OnHierarchyModified_AnyThread(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	if(bSuspendHierarchyNotifs)
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
		return;
	}

	if (InNotif != ETLLRN_RigHierarchyNotification::ControlSettingChanged &&
		InNotif != ETLLRN_RigHierarchyNotification::ControlVisibilityChanged &&
		InNotif != ETLLRN_RigHierarchyNotification::ControlDrivenListChanged &&
		InNotif != ETLLRN_RigHierarchyNotification::ControlShapeTransformChanged &&
		InNotif != ETLLRN_RigHierarchyNotification::ElementSelected &&
		InNotif != ETLLRN_RigHierarchyNotification::ElementDeselected)
	{
		OnHierarchyModified(InNotif, InHierarchy, InElement);
		return;
	}
	
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

void FTLLRN_ControlRigEditMode::OnControlModified(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControlElement, const FTLLRN_RigControlModifiedContext& Context)
{
	//this makes sure the details panel ui get's updated, don't remove
	const bool bModify = Context.SetKey != ETLLRN_ControlRigSetKey::Never;
	ControlProxy->ProxyChanged(Subject, InControlElement, bModify);

	bPivotsNeedUpdate = true;
	
	/*
	FScopedTransaction ScopedTransaction(LOCTEXT("ModifyControlTransaction", "Modify Control"),!GIsTransacting && Context.SetKey != ETLLRN_ControlRigSetKey::Never);
	ControlProxy->Modify();
	RecalcPivotTransform();

	if (UTLLRN_ControlRig* TLLRN_ControlRig = static_cast<UTLLRN_ControlRig*>(Subject))
	{
		FTransform ComponentTransform = GetHostingSceneComponentTransform();
		if (ATLLRN_ControlRigShapeActor* const* Actor = GizmoToControlMap.FindKey(InControl.Index))
		{
			TickControlShape(*Actor, ComponentTransform);
		}
	}
	*/
}

void FTLLRN_ControlRigEditMode::OnPreConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName)
{
	bIsConstructionEventRunning = true;
}

void FTLLRN_ControlRigEditMode::OnPostConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName)
{
	bIsConstructionEventRunning = false;

	const int32 RigIndex = RuntimeTLLRN_ControlRigs.Find(InRig);
	if(!LastHierarchyHash.IsValidIndex(RigIndex) || !LastShapeLibraryHash.IsValidIndex(RigIndex))
	{
		return;
	}
	
	const int32 HierarchyHash = InRig->GetHierarchy()->GetTopologyHash(false, true);
	const int32 ShapeLibraryHash = InRig->GetShapeLibraryHash();
	if((LastHierarchyHash[RigIndex] != HierarchyHash) ||
		(LastShapeLibraryHash[RigIndex] != ShapeLibraryHash))
	{
		LastHierarchyHash[RigIndex] = HierarchyHash;
		LastShapeLibraryHash[RigIndex] = ShapeLibraryHash;

		auto Task = [this, InRig]()
		{
			RequestToRecreateControlShapeActors(InRig);
			RecreateControlShapeActors();
			HandleSelectionChanged();
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

void FTLLRN_ControlRigEditMode::OnWidgetModeChanged(UE::Widget::EWidgetMode InWidgetMode)
{
	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
	if (Settings && Settings->bCoordSystemPerWidgetMode)
	{
		TGuardValue<bool> ReentrantGuardSelf(bIsChangingCoordSystem, true);

		FEditorModeTools* ModeManager = GetModeManager();
		int32 WidgetMode = (int32)ModeManager->GetWidgetMode();
		if (WidgetMode >= 0 && WidgetMode < CoordSystemPerWidgetMode.Num())
		{
			ModeManager->SetCoordSystem(CoordSystemPerWidgetMode[WidgetMode]);
		}
	}
}

void FTLLRN_ControlRigEditMode::OnCoordSystemChanged(ECoordSystem InCoordSystem)
{
	TGuardValue<bool> ReentrantGuardSelf(bIsChangingCoordSystem, true);

	FEditorModeTools* ModeManager = GetModeManager();
	int32 WidgetMode = (int32)ModeManager->GetWidgetMode();
	ECoordSystem CoordSystem = ModeManager->GetCoordSystem();
	if (WidgetMode >= 0 && WidgetMode < CoordSystemPerWidgetMode.Num())
	{
		CoordSystemPerWidgetMode[WidgetMode] = CoordSystem;
	}
}

bool FTLLRN_ControlRigEditMode::CanChangeControlShapeTransform()
{
	if (AreEditingTLLRN_ControlRigDirectly())
	{
		for (TWeakObjectPtr<UTLLRN_ControlRig> RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
			{
				TArray<FTLLRN_RigElementKey> SelectedTLLRN_RigElements = GetSelectedTLLRN_RigElements(TLLRN_ControlRig);
				// do not allow multi-select
				if (SelectedTLLRN_RigElements.Num() == 1)
				{
					if (AreTLLRN_RigElementsSelected(ValidControlTypeMask(),TLLRN_ControlRig))
					{
						// only enable for a Control with Gizmo enabled and visible
						if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->GetHierarchy()->Find<FTLLRN_RigControlElement>(SelectedTLLRN_RigElements[0]))
						{
							if (ControlElement->Settings.IsVisible())
							{
								if (ATLLRN_ControlRigShapeActor* ShapeActor = GetControlShapeFromControlName(TLLRN_ControlRig,SelectedTLLRN_RigElements[0].Name))
								{
									if (ensure(ShapeActor->IsSelected()))
									{
										return true;
									}
								}
							}
						}
						
					}
				}
			}
		}
	}

	return false;
}

void FTLLRN_ControlRigEditMode::OnSettingsChanged(const UTLLRN_ControlRigEditModeSettings* InSettings)
{
	if (!InSettings)
	{
		return;
	}
	
	// check if the settings for xray rendering are different for any of the control shape actors
	if(bShowControlsAsOverlay != InSettings->bShowControlsAsOverlay)
	{
		bShowControlsAsOverlay = InSettings->bShowControlsAsOverlay;
		for (TWeakObjectPtr<UTLLRN_ControlRig>& RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
		{
			if (UTLLRN_ControlRig* RuntimeTLLRN_ControlRig = RuntimeRigPtr.Get())
			{
				UpdateSelectabilityOnSkeletalMeshes(RuntimeTLLRN_ControlRig, !bShowControlsAsOverlay);
			}
		}
		RequestToRecreateControlShapeActors();
	}
}

void FTLLRN_ControlRigEditMode::SetControlShapeTransform(
	const ATLLRN_ControlRigShapeActor* InShapeActor,
	const FTransform& InGlobalTransform,
	const FTransform& InToWorldTransform,
	const FTLLRN_RigControlModifiedContext& InContext,
	const bool bPrintPython,
	const bool bFixEulerFlips) const
{
	UTLLRN_ControlRig* TLLRN_ControlRig = InShapeActor->TLLRN_ControlRig.Get();
	if (!TLLRN_ControlRig)
	{
		return;
	}

	static constexpr bool bNotify = true, bUndo = true;
	if (AreEditingTLLRN_ControlRigDirectly())
	{
		// assumes it's attached to actor
		TLLRN_ControlRig->SetControlGlobalTransform(
			InShapeActor->ControlName, InGlobalTransform, bNotify, InContext, bUndo, bPrintPython, bFixEulerFlips);
		return;
	}

	auto EvaluateRigIfAdditive = [TLLRN_ControlRig]()
	{
		// skip compensation and evaluate the rig to force notifications: auto-key and constraints updates (among others) are based on
		// UTLLRN_ControlRig::OnControlModified being broadcast but this only happens on evaluation for additive rigs.
		// constraint compensation is disabled while manipulating in that case to avoid re-entrant evaluations 
		if (TLLRN_ControlRig->IsAdditive())
		{
			TGuardValue<bool> CompensateGuard(FMovieSceneConstraintChannelHelper::bDoNotCompensate, true);
			TLLRN_ControlRig->Evaluate_AnyThread();
		}
	};
	
	// find the last constraint in the stack (this could be cached on mouse press)
	TArray< TWeakObjectPtr<UTickableConstraint> > Constraints;
	FTransformConstraintUtils::GetParentConstraints(TLLRN_ControlRig->GetWorld(), InShapeActor, Constraints);

	const int32 LastActiveIndex = FTransformConstraintUtils::GetLastActiveConstraintIndex(Constraints);
	const bool bNeedsConstraintPostProcess = Constraints.IsValidIndex(LastActiveIndex);
	
	// set the global space, assumes it's attached to actor
	// no need to compensate for constraints here, this will be done after when setting the control in the constraint space
	{
		TGuardValue<bool> CompensateGuard(FMovieSceneConstraintChannelHelper::bDoNotCompensate, true);
		TLLRN_ControlRig->SetControlGlobalTransform(
			InShapeActor->ControlName, InGlobalTransform, bNotify, InContext, bUndo, bPrintPython, bFixEulerFlips);
		EvaluateRigIfAdditive();
	}

	if (!bNeedsConstraintPostProcess)
	{
		return;
	}
	
	// switch to constraint space
	const FTransform WorldTransform = InGlobalTransform * InToWorldTransform;
	FTransform LocalTransform = TLLRN_ControlRig->GetControlLocalTransform(InShapeActor->ControlName);

	const TOptional<FTransform> RelativeTransform =
		FTransformConstraintUtils::GetConstraintsRelativeTransform(Constraints, LocalTransform, WorldTransform);
	if (RelativeTransform)
	{
		LocalTransform = *RelativeTransform; 
	}

	FTLLRN_RigControlModifiedContext Context = InContext;
	Context.bConstraintUpdate = false;
	
	TLLRN_ControlRig->SetControlLocalTransform(InShapeActor->ControlName, LocalTransform, bNotify, Context, bUndo, bFixEulerFlips);
	EvaluateRigIfAdditive();
	
	const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(TLLRN_ControlRig->GetWorld());
	Controller.EvaluateAllConstraints();
}

FTransform FTLLRN_ControlRigEditMode::GetControlShapeTransform(const ATLLRN_ControlRigShapeActor* ShapeActor)
{
	if (const UTLLRN_ControlRig* TLLRN_ControlRig = ShapeActor->TLLRN_ControlRig.Get())
	{
		return TLLRN_ControlRig->GetControlGlobalTransform(ShapeActor->ControlName);
	}
	return FTransform::Identity;
}

void FTLLRN_ControlRigEditMode::MoveControlShape(ATLLRN_ControlRigShapeActor* ShapeActor, const bool bTranslation, FVector& InDrag,
	const bool bRotation, FRotator& InRot, const bool bScale, FVector& InScale, const FTransform& ToWorldTransform,
	bool bUseLocal, bool bCalcLocal, FTransform& InOutLocal)
{
	UTLLRN_ControlRig* TLLRN_ControlRig = ShapeActor ? ShapeActor->TLLRN_ControlRig.Get() : nullptr;
	if (!ensure(TLLRN_ControlRig))
	{
		return;
	}

	bool bTransformChanged = false;

	// In case for some reason the shape actor was detached, make sure to attach it again to the scene component
	if (!ShapeActor->GetAttachParentActor())
	{
		if (USceneComponent* SceneComponent = GetHostingSceneComponent(TLLRN_ControlRig))
		{
			if (AActor* OwnerActor = SceneComponent->GetOwner())
			{
				ShapeActor->AttachToActor(OwnerActor, FAttachmentTransformRules::KeepWorldTransform);
			}
		}
	}
	
	//first case is where we do all controls by the local diff.
	if (bUseLocal)
	{
		FTransform CurrentLocalTransform = TLLRN_ControlRig->GetControlLocalTransform(ShapeActor->ControlName);

		if (bRotation)
		{
			FQuat CurrentRotation = CurrentLocalTransform.GetRotation();
			CurrentRotation = (CurrentRotation * InOutLocal.GetRotation());
			CurrentLocalTransform.SetRotation(CurrentRotation);
			bTransformChanged = true;
		}

		if (bTranslation)
		{
			FVector CurrentLocation = CurrentLocalTransform.GetLocation();
			CurrentLocation = CurrentLocation + InOutLocal.GetLocation();
			CurrentLocalTransform.SetLocation(CurrentLocation);
			bTransformChanged = true;
		}

		if (bTransformChanged)
		{
			TLLRN_ControlRig->InteractionType = InteractionType;
			TLLRN_ControlRig->ElementsBeingInteracted.AddUnique(ShapeActor->GetElementKey());
			
			TLLRN_ControlRig->SetControlLocalTransform(ShapeActor->ControlName, CurrentLocalTransform,true, FTLLRN_RigControlModifiedContext(), true, /*fix eulers*/ true);

			FTransform CurrentTransform  = TLLRN_ControlRig->GetControlGlobalTransform(ShapeActor->ControlName);			// assumes it's attached to actor
			CurrentTransform = ToWorldTransform * CurrentTransform;

			// make the transform relative to the offset transform again.
			// first we'll make it relative to the offset used at the time of starting the drag
			// and then we'll make it absolute again based on the current offset. these two can be
			// different if we are interacting on a control on an animated character
			CurrentTransform = CurrentTransform.GetRelativeTransform(ShapeActor->OffsetTransform);
			CurrentTransform = CurrentTransform * TLLRN_ControlRig->GetHierarchy()->GetGlobalControlOffsetTransform(ShapeActor->GetElementKey(), false);
			
			ShapeActor->SetGlobalTransform(CurrentTransform);

			TLLRN_ControlRig->Evaluate_AnyThread();

			return;
		}
	}


	//not local or doing scale.
	FTransform CurrentTransform = GetControlShapeTransform(ShapeActor) * ToWorldTransform;

	if (bRotation)
	{
		FQuat CurrentRotation = CurrentTransform.GetRotation();
		CurrentRotation = (InRot.Quaternion() * CurrentRotation);
		CurrentTransform.SetRotation(CurrentRotation);
		bTransformChanged = true;
	}

	if (bTranslation)
	{
		FVector CurrentLocation = CurrentTransform.GetLocation();
		CurrentLocation = CurrentLocation + InDrag;
		CurrentTransform.SetLocation(CurrentLocation);
		bTransformChanged = true;
	}

	if (bScale)
	{
		FVector CurrentScale = CurrentTransform.GetScale3D();
		CurrentScale = CurrentScale + InScale;
		CurrentTransform.SetScale3D(CurrentScale);
		bTransformChanged = true;
	}

	if (bTransformChanged)
	{
		TLLRN_ControlRig->InteractionType = InteractionType;
		TLLRN_ControlRig->ElementsBeingInteracted.AddUnique(ShapeActor->GetElementKey());

		FTransform NewTransform = CurrentTransform.GetRelativeTransform(ToWorldTransform);
		
		FTLLRN_RigControlModifiedContext Context;
		Context.EventName = FTLLRN_RigUnit_BeginExecution::EventName;
		Context.bConstraintUpdate = true;

		if (bCalcLocal)
		{
			InOutLocal = TLLRN_ControlRig->GetControlLocalTransform(ShapeActor->ControlName);
		}

		const UWorld* World = TLLRN_ControlRig->GetWorld();
		const bool bPrintPythonCommands = World ? World->IsPreviewWorld() : false;

		const FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ShapeActor->ControlName);
		const bool bIsTransientControl = ControlElement ? ControlElement->Settings.bIsTransientControl : false;

		// if we are operating on a PIE instance which is playing we need to reapply the input pose
		// since the hierarchy will also have been brought into the solved pose. by reapplying the
		// input pose we avoid double transformation / double forward solve results.
		if(bIsTransientControl && World && World->IsPlayInEditor() && !World->IsPaused())
		{
			TLLRN_ControlRig->GetHierarchy()->SetPose(TLLRN_ControlRig->InputPoseOnDebuggedRig);
		}
		
		TLLRN_ControlRig->Evaluate_AnyThread();
		//fix flips and do rotation orders only if not additive
		const bool bFixEulerFlips = !TLLRN_ControlRig->IsAdditive();
		SetControlShapeTransform(ShapeActor, NewTransform, ToWorldTransform, Context, bPrintPythonCommands, bFixEulerFlips);
		NotifyDrivenControls(TLLRN_ControlRig, ShapeActor->GetElementKey(),Context);

		if (ControlElement && !bIsTransientControl)
		{
			TLLRN_ControlRig->Evaluate_AnyThread();
		}
		
		ShapeActor->SetGlobalTransform(CurrentTransform);

		if (bCalcLocal)
		{
			FTransform NewLocal = TLLRN_ControlRig->GetControlLocalTransform(ShapeActor->ControlName);
			InOutLocal = NewLocal.GetRelativeTransform(InOutLocal);
		}
	}
}

void FTLLRN_ControlRigEditMode::ChangeControlShapeTransform(ATLLRN_ControlRigShapeActor* ShapeActor, const bool bTranslation, FVector& InDrag,
	const bool bRotation, FRotator& InRot, const bool bScale, FVector& InScale, const FTransform& ToWorldTransform)
{
	bool bTransformChanged = false; 

	FTransform CurrentTransform = FTransform::Identity;

	if (UTLLRN_ControlRig* TLLRN_ControlRig = ShapeActor->TLLRN_ControlRig.Get())
	{
		if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->GetHierarchy()->Find<FTLLRN_RigControlElement>(ShapeActor->GetElementKey()))
		{
			CurrentTransform = TLLRN_ControlRig->GetHierarchy()->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentGlobal);
			CurrentTransform = CurrentTransform * ToWorldTransform;
		}
	}

	if (bRotation)
	{
		FQuat CurrentRotation = CurrentTransform.GetRotation();
		CurrentRotation = (InRot.Quaternion() * CurrentRotation);
		CurrentTransform.SetRotation(CurrentRotation);
		bTransformChanged = true;
	}

	if (bTranslation)
	{
		FVector CurrentLocation = CurrentTransform.GetLocation();
		CurrentLocation = CurrentLocation + InDrag;
		CurrentTransform.SetLocation(CurrentLocation);
		bTransformChanged = true;
	}

	if (bScale)
	{
		FVector CurrentScale = CurrentTransform.GetScale3D();
		CurrentScale = CurrentScale + InScale;
		CurrentTransform.SetScale3D(CurrentScale);
		bTransformChanged = true;
	}

	if (bTransformChanged)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = ShapeActor->TLLRN_ControlRig.Get())
		{

			FTransform NewTransform = CurrentTransform.GetRelativeTransform(ToWorldTransform);
			FTLLRN_RigControlModifiedContext Context;

			if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->GetHierarchy()->Find<FTLLRN_RigControlElement>(ShapeActor->GetElementKey()))
			{
				// do not setup undo for this first step since it is just used to calculate the local transform
				TLLRN_ControlRig->GetHierarchy()->SetControlShapeTransform(ControlElement, NewTransform, ETLLRN_RigTransformType::CurrentGlobal, false);
				FTransform CurrentLocalShapeTransform = TLLRN_ControlRig->GetHierarchy()->GetControlShapeTransform(ControlElement, ETLLRN_RigTransformType::CurrentLocal);
				// this call should trigger an instance-to-BP update in TLLRN_ControlRigEditor
				TLLRN_ControlRig->GetHierarchy()->SetControlShapeTransform(ControlElement, CurrentLocalShapeTransform, ETLLRN_RigTransformType::InitialLocal, true);

				FTransform MeshTransform = FTransform::Identity;
				FTransform ShapeTransform = CurrentLocalShapeTransform;
				if (const FTLLRN_ControlRigShapeDefinition* Gizmo = UTLLRN_ControlRigShapeLibrary::GetShapeByName(ControlElement->Settings.ShapeName, TLLRN_ControlRig->GetShapeLibraries(), TLLRN_ControlRig->ShapeLibraryNameMap))
				{
					MeshTransform = Gizmo->Transform;
				}
				ShapeActor->StaticMeshComponent->SetRelativeTransform(MeshTransform * ShapeTransform);
			}
		}
	} 
}


bool FTLLRN_ControlRigEditMode::ModeSupportedByShapeActor(const ATLLRN_ControlRigShapeActor* ShapeActor, UE::Widget::EWidgetMode InMode) const
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = ShapeActor->TLLRN_ControlRig.Get())
	{
		const FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ShapeActor->ControlName);
		if (ControlElement)
		{
			if (bIsChangingControlShapeTransform)
			{
				return true;
			}

			if (IsSupportedControlType(ControlElement->Settings.ControlType))
			{
				switch (InMode)
				{
					case UE::Widget::WM_None:
						return true;
					case UE::Widget::WM_Rotate:
					{
						switch (ControlElement->Settings.ControlType)
						{
							case ETLLRN_RigControlType::Rotator:
							case ETLLRN_RigControlType::Transform:
							case ETLLRN_RigControlType::TransformNoScale:
							case ETLLRN_RigControlType::EulerTransform:
							{
								return true;
							}
							default:
							{
								break;
							}
						}
						break;
					}
					case UE::Widget::WM_Translate:
					{
						switch (ControlElement->Settings.ControlType)
						{
							case ETLLRN_RigControlType::Float:
							case ETLLRN_RigControlType::Integer:
							case ETLLRN_RigControlType::Vector2D:
							case ETLLRN_RigControlType::Position:
							case ETLLRN_RigControlType::Transform:
							case ETLLRN_RigControlType::TransformNoScale:
							case ETLLRN_RigControlType::EulerTransform:
							{
								return true;
							}
							default:
							{
								break;
							}
						}
						break;
					}
					case UE::Widget::WM_Scale:
					{
						switch (ControlElement->Settings.ControlType)
						{
							case ETLLRN_RigControlType::Scale:
							case ETLLRN_RigControlType::ScaleFloat:
							case ETLLRN_RigControlType::Transform:
							case ETLLRN_RigControlType::EulerTransform:
							{
								return true;
							}
							default:
							{
								break;
							}
						}
						break;
					}
					case UE::Widget::WM_TranslateRotateZ:
					{
						switch (ControlElement->Settings.ControlType)
						{
							case ETLLRN_RigControlType::Transform:
							case ETLLRN_RigControlType::TransformNoScale:
							case ETLLRN_RigControlType::EulerTransform:
							{
								return true;
							}
							default:
							{
								break;
							}
						}
						break;
					}
				}
			}
		}
	}
	return false;
}

bool FTLLRN_ControlRigEditMode::IsTLLRN_ControlRigSkelMeshVisible(const UTLLRN_ControlRig* InTLLRN_ControlRig) const
{
	if (IsInLevelEditor())
	{
		if (InTLLRN_ControlRig)
		{
			if (const USceneComponent* SceneComponent = GetHostingSceneComponent(InTLLRN_ControlRig))
			{
				const AActor* Actor = SceneComponent->GetTypedOuter<AActor>();
				return Actor ? (Actor->IsHiddenEd() == false && SceneComponent->IsVisibleInEditor()) : SceneComponent->IsVisibleInEditor();
			}
		}
		return false;
	}
	return true;
}

void FTLLRN_ControlRigEditMode::TickControlShape(ATLLRN_ControlRigShapeActor* ShapeActor, const FTransform& ComponentTransform) const
{
	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
	if (ShapeActor)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = ShapeActor->TLLRN_ControlRig.Get())
		{
			const FTransform Transform = TLLRN_ControlRig->GetControlGlobalTransform(ShapeActor->ControlName);
			ShapeActor->SetActorTransform(Transform * ComponentTransform);

			if (FTLLRN_RigControlElement* ControlElement = TLLRN_ControlRig->FindControl(ShapeActor->ControlName))
			{
				const bool bControlsHiddenInViewport = Settings->bHideControlShapes || !TLLRN_ControlRig->GetControlsVisible()
					|| (IsTLLRN_ControlRigSkelMeshVisible(TLLRN_ControlRig) == false);

				bool bIsVisible = ControlElement->Settings.IsVisible();
				bool bRespectVisibilityForSelection = IsInLevelEditor() ? false: true;

				if(!bControlsHiddenInViewport)
				{
					if(ControlElement->Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl)
					{					
						bRespectVisibilityForSelection = false;
						if(Settings->bShowAllProxyControls)
						{
							bIsVisible = true;
						}
					}
				}
				
				ShapeActor->SetShapeColor(ShapeActor->OverrideColor.A < SMALL_NUMBER ?
					ControlElement->Settings.ShapeColor : ShapeActor->OverrideColor);
				ShapeActor->SetIsTemporarilyHiddenInEditor(
					!bIsVisible ||
					bControlsHiddenInViewport);
				
				ShapeActor->SetSelectable(
					ControlElement->Settings.IsSelectable(bRespectVisibilityForSelection));
			}
		}
	}
}

ATLLRN_ControlRigShapeActor* FTLLRN_ControlRigEditMode::GetControlShapeFromControlName(UTLLRN_ControlRig* InTLLRN_ControlRig,const FName& ControlName) const
{
	const auto* ShapeActors = TLLRN_ControlRigShapeActors.Find(InTLLRN_ControlRig);
	if (ShapeActors)
	{
		for (ATLLRN_ControlRigShapeActor* ShapeActor : *ShapeActors)
		{
			if (ShapeActor->ControlName == ControlName)
			{
				return ShapeActor;
			}
		}
	}

	return nullptr;
}

void FTLLRN_ControlRigEditMode::AddTLLRN_ControlRigInternal(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	RuntimeTLLRN_ControlRigs.AddUnique(InTLLRN_ControlRig);
	LastHierarchyHash.Add(INDEX_NONE);
	LastShapeLibraryHash.Add(INDEX_NONE);

	InTLLRN_ControlRig->SetControlsVisible(true);
	InTLLRN_ControlRig->PostInitInstanceIfRequired();
	InTLLRN_ControlRig->GetHierarchy()->OnModified().RemoveAll(this);
	InTLLRN_ControlRig->GetHierarchy()->OnModified().AddSP(this, &FTLLRN_ControlRigEditMode::OnHierarchyModified_AnyThread);
	InTLLRN_ControlRig->OnPostConstruction_AnyThread().AddSP(this, &FTLLRN_ControlRigEditMode::OnPostConstruction_AnyThread);

	//needed for the control rig track editor delegates to get hooked up
	if (WeakSequencer.IsValid())
	{
		TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
		Sequencer->ObjectImplicitlyAdded(InTLLRN_ControlRig);
	}
	OnTLLRN_ControlRigAddedOrRemovedDelegate.Broadcast(InTLLRN_ControlRig, true);

	UpdateSelectabilityOnSkeletalMeshes(InTLLRN_ControlRig, !bShowControlsAsOverlay);
}

TArrayView<const TWeakObjectPtr<UTLLRN_ControlRig>> FTLLRN_ControlRigEditMode::GetTLLRN_ControlRigs() const
{
	return MakeArrayView(RuntimeTLLRN_ControlRigs);
}

TArrayView<TWeakObjectPtr<UTLLRN_ControlRig>> FTLLRN_ControlRigEditMode::GetTLLRN_ControlRigs() 
{
	return MakeArrayView(RuntimeTLLRN_ControlRigs);
}

TArray<UTLLRN_ControlRig*> FTLLRN_ControlRigEditMode::GetTLLRN_ControlRigsArray(bool bIsVisible)
{
	TArray < UTLLRN_ControlRig*> TLLRN_ControlRigs;
	for (TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (TLLRN_ControlRigPtr.IsValid() && TLLRN_ControlRigPtr.Get() != nullptr && (bIsVisible == false ||TLLRN_ControlRigPtr.Get()->GetControlsVisible()))
		{
			TLLRN_ControlRigs.Add(TLLRN_ControlRigPtr.Get());
		}
	}
	return TLLRN_ControlRigs;
}

TArray<const UTLLRN_ControlRig*> FTLLRN_ControlRigEditMode::GetTLLRN_ControlRigsArray(bool bIsVisible) const
{
	TArray<const UTLLRN_ControlRig*> TLLRN_ControlRigs;
	for (const TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (TLLRN_ControlRigPtr.IsValid() && TLLRN_ControlRigPtr.Get() != nullptr && (bIsVisible == false || TLLRN_ControlRigPtr.Get()->GetControlsVisible()))
		{
			TLLRN_ControlRigs.Add(TLLRN_ControlRigPtr.Get());
		}
	}
	return TLLRN_ControlRigs;
}

void FTLLRN_ControlRigEditMode::RemoveTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if (InTLLRN_ControlRig == nullptr)
	{
		return;
	}
	InTLLRN_ControlRig->ControlModified().RemoveAll(this);
	InTLLRN_ControlRig->GetHierarchy()->OnModified().RemoveAll(this);
	InTLLRN_ControlRig->OnPreConstructionForUI_AnyThread().RemoveAll(this);
	InTLLRN_ControlRig->OnPostConstruction_AnyThread().RemoveAll(this);
	
	int32 Index = RuntimeTLLRN_ControlRigs.Find(InTLLRN_ControlRig);
	TStrongObjectPtr<UTLLRN_ControlRigEditModeDelegateHelper>* DelegateHelper = DelegateHelpers.Find(InTLLRN_ControlRig);
	if (DelegateHelper && DelegateHelper->IsValid())
	{
		DelegateHelper->Get()->RemoveDelegates();
		DelegateHelper->Reset();
		DelegateHelpers.Remove(InTLLRN_ControlRig);
	}
	DestroyShapesActors(InTLLRN_ControlRig);
	if (RuntimeTLLRN_ControlRigs.IsValidIndex(Index))
	{
		RuntimeTLLRN_ControlRigs.RemoveAt(Index);
	}
	if (LastHierarchyHash.IsValidIndex(Index))
	{
		LastHierarchyHash.RemoveAt(Index);
	}
	if (LastShapeLibraryHash.IsValidIndex(Index))
	{
		LastShapeLibraryHash.RemoveAt(Index);
	}

	//needed for the control rig track editor delegates to get removed
	if (WeakSequencer.IsValid())
	{
		TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
		Sequencer->ObjectImplicitlyRemoved(InTLLRN_ControlRig);
	}
	OnTLLRN_ControlRigAddedOrRemovedDelegate.Broadcast(InTLLRN_ControlRig, false);
	
	UpdateSelectabilityOnSkeletalMeshes(InTLLRN_ControlRig, true);
}

void FTLLRN_ControlRigEditMode::TickManipulatableObjects(float DeltaTime)
{
	for (TWeakObjectPtr<UTLLRN_ControlRig> RuntimeRigPtr : RuntimeTLLRN_ControlRigs)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = RuntimeRigPtr.Get())
		{
			// tick skeletalmeshcomponent, that's how they update their transform from rig change
			USceneComponent* SceneComponent = GetHostingSceneComponent(TLLRN_ControlRig);
			if (UTLLRN_ControlRigComponent* TLLRN_ControlRigComponent = Cast<UTLLRN_ControlRigComponent>(SceneComponent))
			{
				TLLRN_ControlRigComponent->Update();
			}
			else if (USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(SceneComponent))
			{
				// NOTE: we have to update/tick ALL children skeletal mesh components here because user can attach
				// additional skeletal meshes via the "Copy Pose from Mesh" node.
				//
				// If this is left up to the viewport tick(), the attached meshes will render before they get the latest
				// parent bone transforms resulting in a visible lag on all attached components.

				// get hierarchically ordered list of ALL child skeletal mesh components (recursive)
				const AActor* ThisActor = MeshComponent->GetOwner();
				TArray<USceneComponent*> ChildrenComponents;
				MeshComponent->GetChildrenComponents(true, ChildrenComponents);
				TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshesToUpdate;
				SkeletalMeshesToUpdate.Add(MeshComponent);
				for (USceneComponent* ChildComponent : ChildrenComponents)
				{
					if (USkeletalMeshComponent* ChildMeshComponent = Cast<USkeletalMeshComponent>(ChildComponent))
					{
						if (ThisActor == ChildMeshComponent->GetOwner())
						{
							SkeletalMeshesToUpdate.Add(ChildMeshComponent);
						}
					}
				}

				// update pose of all children skeletal meshes in this actor
				for (USkeletalMeshComponent* SkeletalMeshToUpdate : SkeletalMeshesToUpdate)
				{
					// "Copy Pose from Mesh" requires AnimInstance::PreUpdate() to copy the parent bone transforms.
					// have to TickAnimation() to ensure that PreUpdate() is called on all anim instances
					SkeletalMeshToUpdate->TickAnimation(0.0f, false);
					SkeletalMeshToUpdate->RefreshBoneTransforms();
					SkeletalMeshToUpdate->RefreshFollowerComponents	();
					SkeletalMeshToUpdate->UpdateComponentToWorld();
					SkeletalMeshToUpdate->FinalizeBoneTransform();
					SkeletalMeshToUpdate->MarkRenderTransformDirty();
					SkeletalMeshToUpdate->MarkRenderDynamicDataDirty();
				}
			}
		}
	}
	PostPoseUpdate();
}


void FTLLRN_ControlRigEditMode::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	// if world gets cleaned up first, we destroy gizmo actors
	if (WorldPtr == World)
	{
		DestroyShapesActors(nullptr);
	}
}

void FTLLRN_ControlRigEditMode::OnEditorClosed()
{
	TLLRN_ControlRigShapeActors.Reset();
	TLLRN_ControlRigsToRecreate.Reset();
}

FTLLRN_ControlRigEditMode::FMarqueeDragTool::FMarqueeDragTool()
	: bIsDeletingDragTool(false)
{
}

bool FTLLRN_ControlRigEditMode::FMarqueeDragTool::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	return (DragTool.IsValid() && InViewportClient->GetCurrentWidgetAxis() == EAxisList::None);
}

bool FTLLRN_ControlRigEditMode::FMarqueeDragTool::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (!bIsDeletingDragTool)
	{
		// Ending the drag tool may pop up a modal dialog which can cause unwanted reentrancy - protect against this.
		TGuardValue<bool> RecursionGuard(bIsDeletingDragTool, true);

		// Delete the drag tool if one exists.
		if (DragTool.IsValid())
		{
			if (DragTool->IsDragging())
			{
				DragTool->EndDrag();
			}
			DragTool.Reset();
			return true;
		}
	}
	
	return false;
}

void FTLLRN_ControlRigEditMode::FMarqueeDragTool::MakeDragTool(FEditorViewportClient* InViewportClient)
{
	DragTool.Reset();
	if (InViewportClient->IsOrtho())
	{
		DragTool = MakeShareable( new FDragTool_ActorBoxSelect(InViewportClient) );
	}
	else
	{
		DragTool = MakeShareable( new FDragTool_ActorFrustumSelect(InViewportClient) );
	}
}

bool FTLLRN_ControlRigEditMode::FMarqueeDragTool::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (DragTool.IsValid() == false || InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
	{
		return false;
	}
	if (DragTool->IsDragging() == false)
	{
		int32 InX = InViewport->GetMouseX();
		int32 InY = InViewport->GetMouseY();
		FVector2D Start(InX, InY);

		DragTool->StartDrag(InViewportClient, GEditor->ClickLocation,Start);
	}
	const bool bUsingDragTool = UsingDragTool();
	if (bUsingDragTool == false)
	{
		return false;
	}

	DragTool->AddDelta(InDrag);
	return true;
}

bool FTLLRN_ControlRigEditMode::FMarqueeDragTool::UsingDragTool() const
{
	return DragTool.IsValid() && DragTool->IsDragging();
}

void FTLLRN_ControlRigEditMode::FMarqueeDragTool::Render3DDragTool(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (DragTool.IsValid())
	{
		DragTool->Render3D(View, PDI);
	}
}

void FTLLRN_ControlRigEditMode::FMarqueeDragTool::RenderDragTool(const FSceneView* View, FCanvas* Canvas)
{
	if (DragTool.IsValid())
	{
		DragTool->Render(View, Canvas);
	}
}

void FTLLRN_ControlRigEditMode::DestroyShapesActors(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	using namespace TLLRN_ControlRigEditMode::Shapes;
	
	if (!InTLLRN_ControlRig)
	{
		// destroy all control rigs shape actors
		for(auto& ShapeActors: TLLRN_ControlRigShapeActors)
		{
			DestroyShapesActorsFromWorld(ShapeActors.Value);
		}
		
		TLLRN_ControlRigShapeActors.Reset();
		TLLRN_ControlRigsToRecreate.Reset();
		
		if (OnWorldCleanupHandle.IsValid())
		{
			FWorldDelegates::OnWorldCleanup.Remove(OnWorldCleanupHandle);
		}
		
		return;
	}

	// only destroy control rigs shape actors related to InTLLRN_ControlRig
	TLLRN_ControlRigsToRecreate.Remove(InTLLRN_ControlRig);
	if (const auto* ShapeActors = TLLRN_ControlRigShapeActors.Find(InTLLRN_ControlRig))
	{
		DestroyShapesActorsFromWorld(*ShapeActors);
		TLLRN_ControlRigShapeActors.Remove(InTLLRN_ControlRig);
	}
}

USceneComponent* FTLLRN_ControlRigEditMode::GetHostingSceneComponent(const UTLLRN_ControlRig* TLLRN_ControlRig) const
{
	if (TLLRN_ControlRig == nullptr && GetTLLRN_ControlRigs().Num() > 0)
	{
		TLLRN_ControlRig = GetTLLRN_ControlRigs()[0].Get();
	}
	if (TLLRN_ControlRig)
	{
		TSharedPtr<ITLLRN_ControlRigObjectBinding> ObjectBinding = TLLRN_ControlRig->GetObjectBinding();
		if (ObjectBinding.IsValid())
		{
			if (USceneComponent* BoundSceneComponent = Cast<USceneComponent>(ObjectBinding->GetBoundObject()))
			{
				return BoundSceneComponent;
			}
			else if (USkeleton* BoundSkeleton = Cast<USkeleton>(ObjectBinding->GetBoundObject()))
			{
				// Bound to a Skeleton means we are previewing an Animation Sequence
				if (WorldPtr)
				{
					TObjectPtr<AActor>* PreviewActor = WorldPtr->PersistentLevel->Actors.FindByPredicate([](TObjectPtr<AActor> Actor)
					{
						return Actor && Actor->GetClass() == AAnimationEditorPreviewActor::StaticClass();
					});

					if(PreviewActor)
					{
						if (UDebugSkelMeshComponent* DebugComponent = (*PreviewActor)->FindComponentByClass<UDebugSkelMeshComponent>())
						{
							return DebugComponent;
						}
					}
				}
			}			
		}
		
	}	

	return nullptr;
}

FTransform FTLLRN_ControlRigEditMode::GetHostingSceneComponentTransform(const UTLLRN_ControlRig* TLLRN_ControlRig) const
{
	// we care about this transform only in the level,
	// since in the control rig editor the debug skeletal mesh component
	// is set at identity anyway.
	if(IsInLevelEditor())
	{
		if (TLLRN_ControlRig == nullptr && GetTLLRN_ControlRigs().Num() > 0)
		{
			TLLRN_ControlRig = GetTLLRN_ControlRigs()[0].Get();
		}

		const USceneComponent* HostingComponent = GetHostingSceneComponent(TLLRN_ControlRig);
		return HostingComponent ? HostingComponent->GetComponentTransform() : FTransform::Identity;
	}
	return FTransform::Identity;
}

void FTLLRN_ControlRigEditMode::OnPoseInitialized()
{
	OnAnimSystemInitializedDelegate.Broadcast();
}

void FTLLRN_ControlRigEditMode::PostPoseUpdate() const
{
	for (auto& ShapeActors : TLLRN_ControlRigShapeActors)
	{
		FTransform ComponentTransform = FTransform::Identity;
		if (!AreEditingTLLRN_ControlRigDirectly())
		{
			ComponentTransform = GetHostingSceneComponentTransform(ShapeActors.Key);
		}
		for (ATLLRN_ControlRigShapeActor* ShapeActor : ShapeActors.Value)
		{
			TickControlShape(ShapeActor, ComponentTransform);	
		}
	}

}

void FTLLRN_ControlRigEditMode::NotifyDrivenControls(UTLLRN_ControlRig* InTLLRN_ControlRig, const FTLLRN_RigElementKey& InKey, const FTLLRN_RigControlModifiedContext& InContext)
{
	// if we are changing a proxy control - we also need to notify the change for the driven controls
	if (FTLLRN_RigControlElement* ControlElement = InTLLRN_ControlRig->GetHierarchy()->Find<FTLLRN_RigControlElement>(InKey))
	{
		if(ControlElement->CanDriveControls())
		{
			const bool bFixEulerFlips = InTLLRN_ControlRig->IsAdditive() == false ? true : false;
			FTLLRN_RigControlModifiedContext Context(InContext);
			Context.EventName = FTLLRN_RigUnit_BeginExecution::EventName;

			for(const FTLLRN_RigElementKey& DrivenKey : ControlElement->Settings.DrivenControls)
			{
				if(DrivenKey.Type == ETLLRN_RigElementType::Control)
				{
					const FTransform DrivenTransform = InTLLRN_ControlRig->GetControlLocalTransform(DrivenKey.Name);
					InTLLRN_ControlRig->SetControlLocalTransform(DrivenKey.Name, DrivenTransform, true, Context, false /*undo*/, bFixEulerFlips);
				}
			}
		}
	}
}

void FTLLRN_ControlRigEditMode::UpdateSelectabilityOnSkeletalMeshes(UTLLRN_ControlRig* InTLLRN_ControlRig, bool bEnabled)
{
	if(const USceneComponent* HostingComponent = GetHostingSceneComponent(InTLLRN_ControlRig))
	{
		if(const AActor* HostingOwner = HostingComponent->GetOwner())
		{
			for(UActorComponent* ActorComponent : HostingOwner->GetComponents())
			{
				if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ActorComponent))
				{
					SkeletalMeshComponent->bSelectable = bEnabled;
					SkeletalMeshComponent->MarkRenderStateDirty();
				}
				else if(UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(ActorComponent))
				{
					StaticMeshComponent->bSelectable = bEnabled;
					StaticMeshComponent->MarkRenderStateDirty();
				}
			}
		}
	}
}

void FTLLRN_ControlRigEditMode::SetOnlySelectTLLRN_RigControls(bool Val)
{
	UTLLRN_ControlRigEditModeSettings* Settings = GetMutableDefault<UTLLRN_ControlRigEditModeSettings>();
	Settings->bOnlySelectTLLRN_RigControls = Val;
}

bool FTLLRN_ControlRigEditMode::GetOnlySelectTLLRN_RigControls()const
{
	const UTLLRN_ControlRigEditModeSettings* Settings = GetDefault<UTLLRN_ControlRigEditModeSettings>();
	return Settings->bOnlySelectTLLRN_RigControls;
}

/**
* FDetailKeyFrameCacheAndHandler
*/

bool FDetailKeyFrameCacheAndHandler::IsPropertyKeyable(const UClass* InObjectClass, const IPropertyHandle& InPropertyHandle) const
{
	TArray<UObject*> OuterObjects;
	InPropertyHandle.GetOuterObjects(OuterObjects);
	if (OuterObjects.Num() == 1)
	{
		if (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = Cast< UTLLRN_ControlTLLRN_RigControlsProxy>(OuterObjects[0]))
		{
			for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : Proxy->TLLRN_ControlRigItems)
			{
				if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
				{
					for (const FName& CName : Items.Value.ControlElements)
					{
						if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
						{
							if (!TLLRN_ControlRig->GetHierarchy()->IsAnimatable(ControlElement))
							{
								return false;
							}
						}
					}
				}
			}
		}
	}
	

	if (InObjectClass
		&& InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyTransform::StaticClass())
		&& InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyLocation::StaticClass())
		&& InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyRotation::StaticClass())
		&& InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyScale::StaticClass())
		&& InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyVector2D::StaticClass())
		&& InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyFloat::StaticClass())
		&& InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyBool::StaticClass())
		&& InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyInteger::StaticClass())
		)
	{
		return true;
	}

	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimLayer::StaticClass()))
	{
		return true;
	}

	if ((InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyTransform::StaticClass()) && InPropertyHandle.GetProperty())
		&& (InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Location) ||
		InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Rotation) ||
		InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyTransform, Scale)))
	{
		return true;
	}


	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyLocation::StaticClass()) && InPropertyHandle.GetProperty()
		&& InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyLocation, Location))
	{
		return true;
	}

	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyRotation::StaticClass()) && InPropertyHandle.GetProperty()
		&& InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyRotation, Rotation))
	{
		return true;
	}

	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyScale::StaticClass()) && InPropertyHandle.GetProperty()
		&& InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyScale, Scale))
	{
		return true;
	}

	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyVector2D::StaticClass()) && InPropertyHandle.GetProperty()
		&& InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyVector2D, Vector2D))
	{
		return true;
	}

	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyInteger::StaticClass()) && InPropertyHandle.GetProperty()
		&& InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyInteger, Integer))
	{
		return true;
	}

	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyBool::StaticClass()) && InPropertyHandle.GetProperty()
		&& InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyBool, Bool))
	{
		return true;
	}

	if (InObjectClass && InObjectClass->IsChildOf(UTLLRN_AnimDetailControlsProxyFloat::StaticClass()) && InPropertyHandle.GetProperty()
		&& InPropertyHandle.GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_AnimDetailControlsProxyFloat, Float))
	{
		return true;
	}
	FCanKeyPropertyParams CanKeyPropertyParams(InObjectClass, InPropertyHandle);
	if (WeakSequencer.IsValid() && WeakSequencer.Pin()->CanKeyProperty(CanKeyPropertyParams))
	{
		return true;
	}

	return false;
}

bool FDetailKeyFrameCacheAndHandler::IsPropertyKeyingEnabled() const
{
	if (WeakSequencer.IsValid() &&  WeakSequencer.Pin()->GetFocusedMovieSceneSequence())
	{
		return true;
	}

	return false;
}

bool FDetailKeyFrameCacheAndHandler::IsPropertyAnimated(const IPropertyHandle& PropertyHandle, UObject* ParentObject) const
{
	if (WeakSequencer.IsValid() && WeakSequencer.Pin()->GetFocusedMovieSceneSequence())
	{
		TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
		constexpr bool bCreateHandleIfMissing = false;
		FGuid ObjectHandle = Sequencer->GetHandleToObject(ParentObject, bCreateHandleIfMissing);
		if (ObjectHandle.IsValid())
		{
			UMovieScene* MovieScene = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
			FProperty* Property = PropertyHandle.GetProperty();
			TSharedRef<FPropertyPath> PropertyPath = FPropertyPath::CreateEmpty();
			PropertyPath->AddProperty(FPropertyInfo(Property));
			FName PropertyName(*PropertyPath->ToString(TEXT(".")));
			TSubclassOf<UMovieSceneTrack> TrackClass; //use empty @todo find way to get the UMovieSceneTrack from the Property type.
			return MovieScene->FindTrack(TrackClass, ObjectHandle, PropertyName) != nullptr;
		}
	}
	return false;
}

void FDetailKeyFrameCacheAndHandler::OnKeyPropertyClicked(const IPropertyHandle& KeyedPropertyHandle)
{
	if (WeakSequencer.IsValid() && !WeakSequencer.Pin()->IsAllowedToChange())
	{
		return;
	}
	FScopedTransaction ScopedTransaction(LOCTEXT("KeyAttribute", "Key Attribute"), !GIsTransacting);
	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();

	TArray<UObject*> Objects;
	KeyedPropertyHandle.GetOuterObjects(Objects);
	for (UObject* Object : Objects)
	{
		if (Object)
		{
			if (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = Cast< UTLLRN_ControlTLLRN_RigControlsProxy>(Object))
			{
				Proxy->SetKey(SequencerPtr, KeyedPropertyHandle);
			}
			else if (UTLLRN_AnimLayer* TLLRN_AnimLayer = (Object->GetTypedOuter<UTLLRN_AnimLayer>()))
			{
				TLLRN_AnimLayer->SetKey(SequencerPtr, KeyedPropertyHandle);
			}
		}
	}
}

EPropertyKeyedStatus FDetailKeyFrameCacheAndHandler::GetPropertyKeyedStatus(const IPropertyHandle& PropertyHandle) const
{
	if (WeakSequencer.IsValid() == false)
	{
		return EPropertyKeyedStatus::NotKeyed;
	}		

	if (const EPropertyKeyedStatus* ExistingKeyedStatus = CachedPropertyKeyedStatusMap.Find(&PropertyHandle))
	{
		return *ExistingKeyedStatus;
	}
	//hack so we can get the reset cache state updated, use ToggleEditable state
	{
		IPropertyHandle* NotConst = const_cast<IPropertyHandle*>(&PropertyHandle);
		NotConst->NotifyPostChange(EPropertyChangeType::ToggleEditable);
	}

	TSharedPtr<ISequencer> SequencerPtr = WeakSequencer.Pin();
	UMovieSceneSequence* Sequence = SequencerPtr->GetFocusedMovieSceneSequence();
	EPropertyKeyedStatus KeyedStatus = EPropertyKeyedStatus::NotKeyed;

	UMovieScene* MovieScene = Sequence ? Sequence->GetMovieScene() : nullptr;
	if (!MovieScene)
	{
		return KeyedStatus;
	}

	TArray<UObject*> OuterObjects;
	PropertyHandle.GetOuterObjects(OuterObjects);
	if (OuterObjects.IsEmpty())
	{
		return EPropertyKeyedStatus::NotKeyed;
	}
	
	for (UObject* Object : OuterObjects)
	{
		if (Object)
		{
			if (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy = Cast< UTLLRN_ControlTLLRN_RigControlsProxy>(Object))
			{
				KeyedStatus = Proxy->GetPropertyKeyedStatus(SequencerPtr, PropertyHandle);
			}
			else if (UTLLRN_AnimLayer* TLLRN_AnimLayer = (Object->GetTypedOuter<UTLLRN_AnimLayer>()))
			{
				KeyedStatus = TLLRN_AnimLayer->GetPropertyKeyedStatus(SequencerPtr, PropertyHandle);
			}
		}
		//else check to see if it's in sequencer
	}
	CachedPropertyKeyedStatusMap.Add(&PropertyHandle, KeyedStatus);

	return KeyedStatus;
}

void FDetailKeyFrameCacheAndHandler::SetDelegates(TWeakPtr<ISequencer>& InWeakSequencer, FTLLRN_ControlRigEditMode* InEditMode)
{
	WeakSequencer = InWeakSequencer;
	EditMode = InEditMode;
	if (TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin())
	{
		Sequencer->OnMovieSceneDataChanged().AddRaw(this, &FDetailKeyFrameCacheAndHandler::OnMovieSceneDataChanged);
		Sequencer->OnGlobalTimeChanged().AddRaw(this, &FDetailKeyFrameCacheAndHandler::OnGlobalTimeChanged);
		Sequencer->OnEndScrubbingEvent().AddRaw(this, &FDetailKeyFrameCacheAndHandler::ResetCachedData);
		Sequencer->OnChannelChanged().AddRaw(this, &FDetailKeyFrameCacheAndHandler::OnChannelChanged);
		Sequencer->OnStopEvent().AddRaw(this, &FDetailKeyFrameCacheAndHandler::ResetCachedData);
	}
}

void FDetailKeyFrameCacheAndHandler::UnsetDelegates()
{
	if (TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin())
	{
		Sequencer->OnMovieSceneDataChanged().RemoveAll(this);
		Sequencer->OnGlobalTimeChanged().RemoveAll(this);
		Sequencer->OnEndScrubbingEvent().RemoveAll(this);
		Sequencer->OnChannelChanged().RemoveAll(this);
		Sequencer->OnStopEvent().RemoveAll(this);
	}
}

void FDetailKeyFrameCacheAndHandler::OnGlobalTimeChanged()
{
	// Only reset cached data when not playing
	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	if (Sequencer.IsValid() && Sequencer->GetPlaybackStatus() != EMovieScenePlayerStatus::Playing)
	{
		ResetCachedData();
	}
}

void FDetailKeyFrameCacheAndHandler::OnMovieSceneDataChanged(EMovieSceneDataChangeType DataChangeType)
{
	if (DataChangeType == EMovieSceneDataChangeType::MovieSceneStructureItemAdded
		|| DataChangeType == EMovieSceneDataChangeType::MovieSceneStructureItemRemoved
		|| DataChangeType == EMovieSceneDataChangeType::MovieSceneStructureItemsChanged
		|| DataChangeType == EMovieSceneDataChangeType::ActiveMovieSceneChanged
		|| DataChangeType == EMovieSceneDataChangeType::RefreshAllImmediately)
	{
		ResetCachedData();
	}
}

void FDetailKeyFrameCacheAndHandler::OnChannelChanged(const FMovieSceneChannelMetaData*, UMovieSceneSection*)
{
	ResetCachedData();
}

void FDetailKeyFrameCacheAndHandler::ResetCachedData()
{
	CachedPropertyKeyedStatusMap.Reset();
	bValuesDirty = true;
}

void FDetailKeyFrameCacheAndHandler::UpdateIfDirty()
{
	if (bValuesDirty == true)
	{
		if (FMovieSceneConstraintChannelHelper::bDoNotCompensate == false) //if compensating don't reset this.
		{
			if (EditMode && EditMode->GetControlProxy())
			{
				EditMode->GetControlProxy()->ValuesChanged();
			}
			bValuesDirty = false;
		}
	}
}

namespace TLLRN_ControlRigEditMode::Shapes
{

void GetControlsEligibleForShapes(UTLLRN_ControlRig* InTLLRN_ControlRig, TArray<FTLLRN_RigControlElement*>& OutControls)
{
	OutControls.Reset();

	const UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig ? InTLLRN_ControlRig->GetHierarchy() : nullptr;
	if (!Hierarchy)
	{
		return;
	}

	OutControls = Hierarchy->GetFilteredElements<FTLLRN_RigControlElement>([](const FTLLRN_RigControlElement* ControlElement)
	{
		const FTLLRN_RigControlSettings& ControlSettings = ControlElement->Settings;
		return ControlSettings.SupportsShape() && IsSupportedControlType(ControlSettings.ControlType);
	});
}

void DestroyShapesActorsFromWorld(const TArray<TObjectPtr<ATLLRN_ControlRigShapeActor>>& InShapeActorsToDestroy)
{
	// NOTE: should UWorld::EditorDestroyActor really modify the level when removing the shapes?
	// kept for legacy but I guess this should be set to false
	static constexpr bool bShouldModifyLevel = true;

	for (const TObjectPtr<ATLLRN_ControlRigShapeActor>& ShapeActorPtr: InShapeActorsToDestroy)
	{
		if (ATLLRN_ControlRigShapeActor* ShapeActor = ShapeActorPtr.Get())
		{
			if (UWorld* World = ShapeActor->GetWorld())
			{
				if (ShapeActor->GetAttachParentActor())
				{
					ShapeActor->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
				}
				World->EditorDestroyActor(ShapeActor,bShouldModifyLevel);
			}			
		}
	}
}

FShapeUpdateParams::FShapeUpdateParams(const UTLLRN_ControlRig* InTLLRN_ControlRig, const FTransform& InComponentTransform, const bool InSkeletalMeshVisible)
	: TLLRN_ControlRig(InTLLRN_ControlRig)
	, Hierarchy(InTLLRN_ControlRig->GetHierarchy())
	, Settings(GetDefault<UTLLRN_ControlRigEditModeSettings>())
	, ComponentTransform(InComponentTransform)
	, bIsSkeletalMeshVisible(InSkeletalMeshVisible)
{}

bool FShapeUpdateParams::IsValid() const
{
	return TLLRN_ControlRig && Hierarchy && Settings;
}

void UpdateControlShape(ATLLRN_ControlRigShapeActor* InShapeActor, FTLLRN_RigControlElement* InControlElement, const FShapeUpdateParams& InUpdateParams)
{
	if (!InShapeActor || !InControlElement || !InUpdateParams.IsValid())
	{
		return;
	}

	// update transform
	const FTransform Transform = InUpdateParams.Hierarchy->GetTransform(InControlElement, ETLLRN_RigTransformType::CurrentGlobal);
	InShapeActor->SetActorTransform(Transform * InUpdateParams.ComponentTransform);

	const FTLLRN_RigControlSettings& ControlSettings = InControlElement->Settings;

	// update visibility & color
	bool bIsVisible = ControlSettings.IsVisible();
	bool bRespectVisibilityForSelection = true; 

	const bool bControlsHiddenInViewport =
		InUpdateParams.Settings->bHideControlShapes ||
		!InUpdateParams.TLLRN_ControlRig->GetControlsVisible() ||
		!InUpdateParams.bIsSkeletalMeshVisible;
	
	if (!bControlsHiddenInViewport)
	{
		if (ControlSettings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl)
		{					
			bRespectVisibilityForSelection = false;
			if (InUpdateParams.Settings->bShowAllProxyControls)
			{
				bIsVisible = true;
			}
		}
	}

	InShapeActor->SetIsTemporarilyHiddenInEditor(!bIsVisible || bControlsHiddenInViewport);

	// update color
	InShapeActor->SetShapeColor(InShapeActor->OverrideColor.A < SMALL_NUMBER ?
		ControlSettings.ShapeColor : InShapeActor->OverrideColor);
	
	// update selectability
	InShapeActor->SetSelectable( ControlSettings.IsSelectable(bRespectVisibilityForSelection) );
}
	
}

#undef LOCTEXT_NAMESPACE
