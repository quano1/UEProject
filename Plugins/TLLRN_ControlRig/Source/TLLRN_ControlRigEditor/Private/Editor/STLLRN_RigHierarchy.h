// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Engine/SkeletalMesh.h"
#include "Editor/STLLRN_RigHierarchyTreeView.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Editor/RigVMEditor.h"
#include "TLLRN_ControlRigSchematicModel.h"
#include "TLLRN_ControlRigDragOps.h"
#include "STLLRN_RigHierarchy.generated.h"

class STLLRN_RigHierarchy;
class FTLLRN_ControlRigEditor;
class SSearchBox;
class FUICommandList;
class URigVMBlueprint;
class UTLLRN_ControlRig;
struct FAssetData;
class FMenuBuilder;
class UToolMenu;
struct FToolMenuContext;

USTRUCT()
struct FTLLRN_RigHierarchyImportSettings
{
	GENERATED_BODY()

	FTLLRN_RigHierarchyImportSettings()
	: Mesh(nullptr)
	{}

	UPROPERTY(EditAnywhere, Category = "Hierachy Import")
	TObjectPtr<USkeletalMesh> Mesh;
};

/** Widget allowing editing of a control rig's structure */
class STLLRN_RigHierarchy : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(STLLRN_RigHierarchy) {}
	SLATE_END_ARGS()

	~STLLRN_RigHierarchy();

	void Construct(const FArguments& InArgs, TSharedRef<FTLLRN_ControlRigEditor> InTLLRN_ControlRigEditor);

	FTLLRN_ControlRigEditor* GetTLLRN_ControlRigEditor() const
	{
		if(TLLRN_ControlRigEditor.IsValid())
		{
			return TLLRN_ControlRigEditor.Pin().Get();
		}
		return nullptr;
	}

private:

	void OnEditorClose(const FRigVMEditor* InEditor, URigVMBlueprint* InBlueprint);

	/** Bind commands that this widget handles */
	void BindCommands();

	/** SWidget interface */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/** Rebuild the tree view */
	void RefreshTreeView(bool bRebuildContent = true);

	/** Return all selected keys */
	TArray<FTLLRN_RigElementKey> GetSelectedKeys() const;

	/** Check whether we can deleting the selected item(s) */
	bool CanDeleteItem() const;

	/** Delete Item */
	void HandleDeleteItem();

	/** Create a new item */
	void HandleNewItem(ETLLRN_RigElementType InElementType, bool bIsAnimationChannel);

	/** Check we can find the references of an item */
	bool CanFindReferencesOfItem() const;

	/** Find all references of an item */
	void HandleFindReferencesOfItem();

	/** Check whether we can deleting the selected item(s) */
	bool CanDuplicateItem() const;

	/** Duplicate Item */
	void HandleDuplicateItem();

	/** Mirror Item */
	void HandleMirrorItem();

	/** Check whether we can deleting the selected item(s) */
	bool CanRenameItem() const;

	/** Rename Item */
	void HandleRenameItem();

	bool CanPasteItems() const;
	bool CanCopyOrPasteItems() const;
	void HandleCopyItems();
	void HandlePasteItems();
	void HandlePasteLocalTransforms();
	void HandlePasteGlobalTransforms();
	void HandlePasteTransforms(ETLLRN_RigTransformType::Type InTransformType, bool bAffectChildren);

	/** Set Selection Changed */
	void OnSelectionChanged(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo);

	TSharedRef< SWidget > CreateFilterMenu();
	TSharedPtr< SWidget > CreateContextMenuWidget();
	void OnItemClicked(TSharedPtr<FRigTreeElement> InItem);
	void OnItemDoubleClicked(TSharedPtr<FRigTreeElement> InItem);
	void OnSetExpansionRecursive(TSharedPtr<FRigTreeElement> InItem, bool bShouldBeExpanded);
	TOptional<FText> OnGetItemTooltip(const FTLLRN_RigElementKey& InKey) const;

	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	// reply to a drag operation
	FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	TOptional<EItemDropZone> OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FRigTreeElement> TargetItem);
	FReply OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FRigTreeElement> TargetItem);
	void OnElementKeyTagDragDetected(const FTLLRN_RigElementKey& InDraggedTag);
	void UpdateConnectorMatchesOnDrag(const TArray<FTLLRN_RigElementKey>& InDraggedKeys);

	static const FName ContextMenuName;
	static void CreateContextMenu();
	UToolMenu* GetContextMenu();
	TSharedPtr<FUICommandList> GetContextMenuCommands() const;
	
	static const FName DragDropMenuName;
	static void CreateDragDropMenu();
	UToolMenu* GetDragDropMenu(const TArray<FTLLRN_RigElementKey>& DraggedKeys, FTLLRN_RigElementKey TargetKey);

	/** Our owning control rig editor */
	TWeakPtr<FTLLRN_ControlRigEditor> TLLRN_ControlRigEditor;

	FRigTreeDisplaySettings DisplaySettings;
	const FRigTreeDisplaySettings& GetDisplaySettings() const { return DisplaySettings; } 

	/** Search box widget */
	TSharedPtr<SSearchBox> FilterBox;

	EVisibility IsToolbarVisible() const;
	EVisibility IsSearchbarVisible() const;
	FReply OnImportSkeletonClicked();
	FText GetImportHierarchyText() const;
	bool IsImportHierarchyEnabled() const;
	void OnFilterTextChanged(const FText& SearchText);

	/** Tree view widget */
	TSharedPtr<STLLRN_RigHierarchyTreeView> TreeView;

	TWeakObjectPtr<UTLLRN_ControlRigBlueprint> TLLRN_ControlRigBlueprint;
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRigBeingDebuggedPtr;
	
	/** Command list we bind to */
	TSharedPtr<FUICommandList> CommandList;

	bool IsMultiSelected(bool bIncludeProcedural) const;
	bool IsSingleSelected(bool bIncludeProcedural) const;
	bool IsSingleBoneSelected(bool bIncludeProcedural) const;
	bool IsSingleNullSelected(bool bIncludeProcedural) const;
	bool IsControlSelected(bool bIncludeProcedural) const;
	bool IsControlOrNullSelected(bool bIncludeProcedural) const;
	bool IsProceduralElementSelected() const;
	bool IsNonProceduralElementSelected() const;
	bool CanAddElement(const ETLLRN_RigElementType ElementType) const;
	bool CanAddAnimationChannel() const;

	UTLLRN_RigHierarchy* GetHierarchy() const;
	UTLLRN_RigHierarchy* GetDefaultHierarchy() const;
	const UTLLRN_RigHierarchy* GetHierarchyForTreeView() const { return GetHierarchy(); }
	FTLLRN_RigElementKey OnGetResolvedKey(const FTLLRN_RigElementKey& InKey);
	void OnRequestDetailsInspection(const FTLLRN_RigElementKey& InKey);
	
	void ImportHierarchy(const FAssetData& InAssetData);
	void CreateImportMenu(FMenuBuilder& MenuBuilder);
	void CreateRefreshMenu(FMenuBuilder& MenuBuilder);
	void CreateResetCurvesMenu(FMenuBuilder& MenuBuilder);
	bool ShouldFilterOnImport(const FAssetData& AssetData) const;
	void RefreshHierarchy(const FAssetData& InAssetData, bool bOnlyResetCurves);
	void UpdateMesh(USkeletalMesh* InMesh, const bool bImport) const;

	void HandleResetTransform(bool bSelectionOnly);
	void HandleResetInitialTransform();
	void HandleSetInitialTransformFromCurrentTransform();
	void HandleSetInitialTransformFromClosestBone();
	void HandleSetShapeTransformFromCurrent();
	void HandleFrameSelection();
	void HandleControlBoneOrSpaceTransform();
	void HandleUnparent();
	bool FindClosestBone(const FVector& Point, FName& OutTLLRN_RigElementName, FTransform& OutGlobalTransform) const;
	void HandleTestSpaceSwitching();

	void HandleParent(const FToolMenuContext& Context);
	void HandleAlign(const FToolMenuContext& Context);
	FReply ReparentOrMatchTransform(const TArray<FTLLRN_RigElementKey>& DraggedKeys, FTLLRN_RigElementKey TargetKey, bool bReparentItems, int32 LocalIndex = INDEX_NONE);
	FReply ResolveConnector(const FTLLRN_RigElementKey& DraggedKey, const FTLLRN_RigElementKey& TargetKey);
	
	FName CreateUniqueName(const FName& InBaseName, ETLLRN_RigElementType InElementType) const;

	void ClearDetailPanel() const;

	bool bIsChangingTLLRN_RigHierarchy;
	void OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);
	void OnHierarchyModified_AnyThread(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);
	void OnTLLRN_ModularRigModified(ETLLRN_ModularRigNotification InNotif, const FTLLRN_RigModuleReference* InModule);
	void HandleRefreshEditorFromBlueprint(URigVMBlueprint* InBlueprint);
	void HandleSetObjectBeingDebugged(UObject* InObject);
	void OnPreConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName);
	void OnPostConstruction_AnyThread(UTLLRN_ControlRig* InRig, const FName& InEventName);
	void OnNavigateToFirstConnectorWarning();

	bool bIsConstructionEventRunning;
	uint32 LastHierarchyHash;
	TArray<FTLLRN_RigElementKey> SelectionBeforeConstruction;
	TMap<FTLLRN_RigElementKey, FTLLRN_ModularRigResolveResult> DragRigResolveResults;

public:
	FName HandleRenameElement(const FTLLRN_RigElementKey& OldKey, const FString& NewName);
	bool HandleVerifyNameChanged(const FTLLRN_RigElementKey& OldKey, const FString& NewName, FText& OutErrorMessage);

	// SWidget Overrides
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	friend class FRigTreeElement;
	friend class STLLRN_RigHierarchyItem;
	friend class UTLLRN_ControlRigBlueprintEditorLibrary;
};

