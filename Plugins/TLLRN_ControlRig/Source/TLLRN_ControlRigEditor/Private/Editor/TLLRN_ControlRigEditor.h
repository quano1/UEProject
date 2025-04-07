// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITLLRN_ControlRigEditor.h"
#include "Editor/TLLRN_ControlRigEditorEditMode.h"
#include "AssetEditorModeManager.h"
#include "DragAndDrop/GraphNodeDragDropOp.h"
#include "TLLRN_ControlRigDefines.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "IPersonaViewport.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "RigVMModel/RigVMGraph.h"
#include "RigVMModel/Nodes/RigVMLibraryNode.h"
#include "RigVMCore/RigVM.h"
#include "Widgets/SRigVMGraphPinNameListValueWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "Styling/SlateTypes.h"
#include "AnimPreviewInstance.h"
#include "ScopedTransaction.h"
#include "Graph/TLLRN_ControlRigGraphNode.h"
#include "RigVMModel/RigVMController.h"
#include "Editor/RigVMDetailsViewWrapperObject.h"
#include "TLLRN_ControlRigTestData.h"
#include "TLLRN_ModularTLLRN_RigController.h"
#include "RigVMHost.h"
#include "SchematicGraphPanel/SSchematicGraphPanel.h"
#include "Units/TLLRN_RigUnit.h"
#include "TLLRN_ControlRigSchematicModel.h"

class UTLLRN_ControlRigBlueprint;
class IPersonaToolkit;
class SWidget;
class SBorder;
class USkeletalMesh;
class FStructOnScope;
class UToolMenu;
class FTLLRN_ControlRigEditor;

struct FTLLRN_ControlRigEditorModes
{
	// Mode constants
	static const FName TLLRN_ControlRigEditorMode;
	static FText GetLocalizedMode(const FName InMode)
	{
		static TMap< FName, FText > LocModes;

		if (LocModes.Num() == 0)
		{
			LocModes.Add(TLLRN_ControlRigEditorMode, NSLOCTEXT("TLLRN_ControlRigEditorModes", "TLLRN_ControlRigEditorMode", "Rigging"));
		}

		check(InMode != NAME_None);
		const FText* OutDesc = LocModes.Find(InMode);
		check(OutDesc);
		return *OutDesc;
	}
private:
	FTLLRN_ControlRigEditorModes() {}
};

class FTLLRN_ControlRigEditor : public ITLLRN_ControlRigEditor
{
public:

	virtual void InitRigVMEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class URigVMBlueprint* InRigVMBlueprint) override;

	virtual const FName GetEditorAppName() const override;
	virtual const FName GetEditorModeName() const override;
	virtual TSharedPtr<FApplicationMode> CreateEditorMode() override;
	virtual const FSlateBrush* GetDefaultTabIcon() const override;

public:
	FTLLRN_ControlRigEditor();
	virtual ~FTLLRN_ControlRigEditor();

	virtual UObject* GetOuterForHost() const override;

	// FRigVMEditor interface
	virtual UClass* GetDetailWrapperClass() const;
	virtual void Compile() override;
	virtual void HandleModifiedEvent(ERigVMGraphNotifType InNotifType, URigVMGraph* InGraph, UObject* InSubject) override;
	virtual void OnCreateGraphEditorCommands(TSharedPtr<FUICommandList> GraphEditorCommandsList) override;
	UE_DEPRECATED(5.4, "Please use HandleVMCompiledEvent with ExtendedExecuteContext parameter.")
	virtual void HandleVMCompiledEvent(UObject* InCompiledObject, URigVM* InVM) override {}
	virtual void HandleVMCompiledEvent(UObject* InCompiledObject, URigVM* InVM, FRigVMExtendedExecuteContext& InContext) override;
	virtual bool ShouldOpenGraphByDefault() const override { return !IsTLLRN_ModularRig(); }
	virtual FReply OnViewportDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	// allows the editor to fill an empty graph
	virtual void CreateEmptyGraphContent(URigVMController* InController) override;

	int32 GetTLLRN_RigHierarchyTabCount() const { return TLLRN_RigHierarchyTabCount; }
	int32 GetTLLRN_ModularTLLRN_RigHierarchyTabCount() const { return TLLRN_ModularTLLRN_RigHierarchyTabCount; }

	bool IsTLLRN_ModularRig() const;
	bool IsTLLRN_RigModule() const;

public:
	
	// IToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FString GetDocumentationLink() const override
	{
		return FString(TEXT("Engine/Animation/TLLRN_ControlRig"));
	}

	// BlueprintEditor interface
	virtual FReply OnSpawnGraphNodeByShortcut(FInputChord InChord, const FVector2D& InPosition, UEdGraph* InGraph) override;
	virtual bool IsSectionVisible(NodeSectionID::Type InSectionID) const override;
	virtual bool NewDocument_IsVisibleForType(ECreatedDocumentType GraphType) const override;

	virtual void PostTransaction(bool bSuccess, const FTransaction* Transaction, bool bIsRedo) override;

	void EnsureValidTLLRN_RigElementsInDetailPanel();

	//  FTickableEditorObject Interface
	virtual void Tick(float DeltaTime) override;

	// Gets the Control Rig Blueprint being edited/viewed
	UTLLRN_ControlRigBlueprint* GetTLLRN_ControlRigBlueprint() const;

	// returns the hierarchy being debugged
	UTLLRN_ControlRig* GetTLLRN_ControlRig() const;

	// returns the hierarchy being debugged
	UTLLRN_RigHierarchy* GetHierarchyBeingDebugged() const;

	void SetDetailViewForTLLRN_RigElements();
	void SetDetailViewForTLLRN_RigElements(const TArray<FTLLRN_RigElementKey>& InKeys);
	bool DetailViewShowsAnyTLLRN_RigElement() const;
	bool DetailViewShowsTLLRN_RigElement(FTLLRN_RigElementKey InKey) const;
	TArray<FTLLRN_RigElementKey> GetSelectedTLLRN_RigElementsFromDetailView() const;

	void SetDetailViewForTLLRN_RigModules();
	void SetDetailViewForTLLRN_RigModules(const TArray<FString> InKeys);
	bool DetailViewShowsAnyTLLRN_RigModule() const;
	bool DetailViewShowsTLLRN_RigModule(FString InKey) const;
	TArray<FString> ModulesSelected;

	virtual void SetDetailObjects(const TArray<UObject*>& InObjects) override;
	virtual void RefreshDetailView() override;

	void CreatePersonaToolKitIfRequired();

public:

	/** Get the persona toolkit */
	TSharedRef<IPersonaToolkit> GetPersonaToolkit() const { return PersonaToolkit.ToSharedRef(); }

	/** Get the edit mode */
	FTLLRN_ControlRigEditorEditMode* GetEditMode() const;

	// this changes everytime you compile, so don't cache it expecting it will last. 
	UTLLRN_ControlRig* GetInstanceRig() const { return GetTLLRN_ControlRig();  }

	void OnCurveContainerChanged();

	void OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);
	void OnHierarchyModified_AnyThread(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);

	void HandleRigTypeChanged(UTLLRN_ControlRigBlueprint* InBlueprint);

	void HandleTLLRN_ModularRigModified(ETLLRN_ModularRigNotification InNotification, const FTLLRN_RigModuleReference* InModule);
	void HandlePostCompileTLLRN_ModularRigs(URigVMBlueprint* InBlueprint);
	void SwapModuleWithinAsset();
	void SwapModuleAcrossProject();

	const FName TLLRN_RigHierarchyToGraphDragAndDropMenuName = TEXT("TLLRN_ControlRigEditor.TLLRN_RigHierarchyToGraphDragAndDropMenu");
	void CreateTLLRN_RigHierarchyToGraphDragAndDropMenu() const;
	void OnGraphNodeDropToPerform(TSharedPtr<FGraphNodeDragDropOp> InDragDropOp, UEdGraph* InGraph, const FVector2D& InNodePosition, const FVector2D& InScreenPosition);

	FPersonaViewportKeyDownDelegate& GetKeyDownDelegate() { return OnKeyDownDelegate; }

	FOnGetContextMenu& OnGetViewportContextMenu() { return OnGetViewportContextMenuDelegate; }
	FNewMenuCommandsDelegate& OnViewportContextMenuCommands() { return OnViewportContextMenuCommandsDelegate; }

	// DirectManipulation functionality
	void HandleRequestDirectManipulationPosition() const { (void)HandleRequestDirectManipulation(ETLLRN_RigControlType::Position); }
	void HandleRequestDirectManipulationRotation() const { (void)HandleRequestDirectManipulation(ETLLRN_RigControlType::Rotator); }
	void HandleRequestDirectManipulationScale() const { (void)HandleRequestDirectManipulation(ETLLRN_RigControlType::Scale); }
	bool HandleRequestDirectManipulation(ETLLRN_RigControlType InControlType) const;
	bool SetDirectionManipulationSubject(const URigVMUnitNode* InNode);
	bool IsDirectManipulationEnabled() const;
	EVisibility GetDirectManipulationVisibility() const;
	FText GetDirectionManipulationText() const;
	void OnDirectManipulationChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	const TArray<FRigDirectManipulationTarget> GetDirectManipulationTargets() const;
	const TArray<TSharedPtr<FString>>& GetDirectManipulationTargetTextList() const;
	bool ClearDirectManipulationSubject() { return SetDirectionManipulationSubject(nullptr); }
	void RefreshDirectManipulationTextList();

	// Rig connector functionality
	EVisibility GetConnectorWarningVisibility() const;
	FText GetConnectorWarningText() const;
	FReply OnNavigateToConnectorWarning() const;
	FSimpleMulticastDelegate& OnRequestNavigateToConnectorWarning() { return RequestNavigateToConnectorWarningDelegate; }

	FVector2D ComputePersonaProjectedScreenPos(const FVector& InWorldPos, bool bClampToScreenRectangle = false);

	void FindReferencesOfItem(const FTLLRN_RigElementKey& InKey);
	
protected:

	virtual void BindCommands() override;
	virtual FMenuBuilder GenerateBulkEditMenu() override;

	void OnHierarchyChanged();

	void SynchronizeViewportBoneSelection();

	virtual void SaveAsset_Execute() override;
	virtual void SaveAssetAs_Execute() override;

	// update the cached modification value
	void UpdateBoneModification(FName BoneName, const FTransform& Transform);

	// remove a single bone modification across all instance
	void RemoveBoneModification(FName BoneName);

	// reset all bone modification across all instance
	void ResetAllBoneModification();

	virtual void HandleVMExecutedEvent(URigVMHost* InHost, const FName& InEventName) override;

	// FBaseToolKit overrides
	virtual void CreateEditorModeManager() override;

private:
	/** Fill the toolbar with content */
	virtual void FillToolbar(FToolBarBuilder& ToolbarBuilder, bool bEndSection = true) override;

	virtual TArray<FName> GetDefaultEventQueue() const override;
	virtual void SetEventQueue(TArray<FName> InEventQueue, bool bCompile) override;
	virtual int32 GetEventQueueComboValue() const override;
	virtual FText GetEventQueueLabel() const override;
	virtual FSlateIcon GetEventQueueIcon(const TArray<FName>& InEventQueue) const override;
	virtual void HandleSetObjectBeingDebugged(UObject* InObject) override;

	/** Handle preview scene setup */
	void HandlePreviewSceneCreated(const TSharedRef<IPersonaPreviewScene>& InPersonaPreviewScene);
	void HandleViewportCreated(const TSharedRef<class IPersonaViewport>& InViewport);
	void HandleToggleControlVisibility();
	bool AreControlsVisible() const;
	void HandleToggleControlsAsOverlay();
	bool AreControlsAsOverlay() const;
	bool IsToolbarDrawNullsEnabled() const;
	bool GetToolbarDrawNulls() const;
	void HandleToggleToolbarDrawNulls();
	bool IsToolbarDrawSocketsEnabled() const;
	bool GetToolbarDrawSockets() const;
	void HandleToggleToolbarDrawSockets();
	bool GetToolbarDrawAxesOnSelection() const;
	void HandleToggleToolbarDrawAxesOnSelection();
	TOptional<float> GetToolbarAxesScale() const;
	void OnToolbarAxesScaleChanged(float InValue);
	void HandleToggleSchematicViewport();
	bool IsSchematicViewportActive() const;
	EVisibility GetSchematicOverlayVisibility() const;

		/** Handle switching skeletal meshes */
	void HandlePreviewMeshChanged(USkeletalMesh* InOldSkeletalMesh, USkeletalMesh* InNewSkeletalMesh);

	/** Push a newly compiled/opened control rig to the edit mode */
	virtual void UpdateRigVMHost() override;
	virtual void UpdateRigVMHost_PreClearOldHost(URigVMHost* InPreviousHost) override;

	/** Update the name lists for use in name combo boxes */
	virtual void CacheNameLists() override;

	/** Rebind our anim instance to the preview's skeletal mesh component */
	void RebindToSkeletalMeshComponent();

	virtual void GenerateEventQueueMenuContent(FMenuBuilder& MenuBuilder) override;

	enum ETLLRN_RigElementGetterSetterType
	{
		ETLLRN_RigElementGetterSetterType_Transform,
		ETLLRN_RigElementGetterSetterType_Rotation,
		ETLLRN_RigElementGetterSetterType_Translation,
		ETLLRN_RigElementGetterSetterType_Initial,
		ETLLRN_RigElementGetterSetterType_Relative,
		ETLLRN_RigElementGetterSetterType_Offset,
		ETLLRN_RigElementGetterSetterType_Name
	};

	void FilterDraggedKeys(TArray<FTLLRN_RigElementKey>& Keys, bool bRemoveNameSpace);
	void HandleMakeElementGetterSetter(ETLLRN_RigElementGetterSetterType Type, bool bIsGetter, TArray<FTLLRN_RigElementKey> Keys, UEdGraph* Graph, FVector2D NodePosition);

	void HandleOnControlModified(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context);

	virtual void HandleRefreshEditorFromBlueprint(URigVMBlueprint* InBlueprint) override;

	FText GetTestAssetName() const;
	FText GetTestAssetTooltip() const;
	bool SetTestAssetPath(const FString& InAssetPath);
	TSharedRef<SWidget> GenerateTestAssetModeMenuContent();
	TSharedRef<SWidget> GenerateTestAssetRecordMenuContent();
	bool RecordTestData(double InRecordingDuration);
	void ToggleTestData();

protected:

	/** Persona toolkit used to support skeletal mesh preview */
	TSharedPtr<IPersonaToolkit> PersonaToolkit;

	/** Preview instance inspector widget */
	TSharedPtr<IPersonaViewport> PreviewViewport;

	/** preview scene */
	TSharedPtr<IPersonaPreviewScene> PreviewScene;

	/** preview animation instance */
	UAnimPreviewInstance* PreviewInstance;

	/** Model for the schematic views */
	FTLLRN_ControlRigSchematicModel SchematicModel;

	/** Delegate to deal with key down evens in the viewport / editor */
	FPersonaViewportKeyDownDelegate OnKeyDownDelegate;

	/** Delgate to build the context menu for the viewport */
	FOnGetContextMenu OnGetViewportContextMenuDelegate;
	UToolMenu* HandleOnGetViewportContextMenuDelegate();
	FNewMenuCommandsDelegate OnViewportContextMenuCommandsDelegate;
	TSharedPtr<FUICommandList> HandleOnViewportContextMenuCommandsDelegate();

	/** Bone Selection related */
	FTransform GetTLLRN_RigElementTransform(const FTLLRN_RigElementKey& InElement, bool bLocal, bool bOnDebugInstance) const;
	void SetTLLRN_RigElementTransform(const FTLLRN_RigElementKey& InElement, const FTransform& InTransform, bool bLocal);
	
	/** delegate for changing property */
	virtual void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void OnWrappedPropertyChangedChainEvent(URigVMDetailsViewWrapperObject* InWrapperObject, const FString& InPropertyPath, FPropertyChangedChainEvent& InPropertyChangedChainEvent) override;

	URigVMController* ActiveController;

	/** Currently executing TLLRN_ControlRig or not - later maybe this will change to enum for whatever different mode*/
	bool bExecutionTLLRN_ControlRig;

	void OnAnimInitialized();

	bool IsConstructionModeEnabled() const;
	bool IsDebuggingExternalTLLRN_ControlRig(const UTLLRN_ControlRig* InTLLRN_ControlRig = nullptr) const;
	bool ShouldExecuteTLLRN_ControlRig(const UTLLRN_ControlRig* InTLLRN_ControlRig = nullptr) const;

	int32 TLLRN_RigHierarchyTabCount;
	int32 TLLRN_ModularTLLRN_RigHierarchyTabCount;
	TWeakObjectPtr<AStaticMeshActor> WeakGroundActorPtr;

	void OnPreForwardsSolve_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName);
	void OnPreConstructionForUI_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName);
	void OnPreConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName);
	void OnPostConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName);
	FTLLRN_RigPose PreConstructionPose;
	TArray<FTLLRN_RigSocketState> SocketStates;
	TArray<FTLLRN_RigConnectorState> ConnectorStates;

	bool bIsConstructionEventRunning;
	uint32 LastHierarchyHash;

	TStrongObjectPtr<UTLLRN_ControlRigTestData> TestDataStrongPtr;

	TWeakObjectPtr<const URigVMUnitNode> DirectManipulationSubject;
	mutable TArray<TSharedPtr<FString>> DirectManipulationTextList;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> DirectManipulationCombo;
	bool bRefreshDirectionManipulationTargetsRequired;
	FSimpleMulticastDelegate RequestNavigateToConnectorWarningDelegate;
	TSharedPtr<SSchematicGraphPanel> SchematicViewport;
	bool bSchematicViewPortIsHidden;

	static const TArray<FName> ForwardsSolveEventQueue;
	static const TArray<FName> BackwardsSolveEventQueue;
	static const TArray<FName> ConstructionEventQueue;
	static const TArray<FName> BackwardsAndForwardsSolveEventQueue;

	friend class FTLLRN_ControlRigEditorMode;
	friend class FTLLRN_ModularRigEditorMode;
	friend class STLLRN_ControlRigStackView;
	friend class STLLRN_RigHierarchy;
	friend class STLLRN_ModularRigModel;
	friend struct FTLLRN_RigHierarchyTabSummoner;
	friend struct FTLLRN_ModularRigModelTabSummoner;
};
