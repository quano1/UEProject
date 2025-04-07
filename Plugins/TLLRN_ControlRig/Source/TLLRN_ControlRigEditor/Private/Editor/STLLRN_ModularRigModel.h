// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Editor/STLLRN_ModularRigTreeView.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "DragAndDrop/GraphNodeDragDropOp.h"
#include "Editor/RigVMEditor.h"
#include "TLLRN_ControlRigDragOps.h"

class STLLRN_ModularRigModel;
class FTLLRN_ControlRigEditor;
class SSearchBox;
class FUICommandList;
class URigVMBlueprint;
class UTLLRN_ControlRig;
struct FAssetData;
class FMenuBuilder;
class UToolMenu;
struct FToolMenuContext;

/** Widget allowing editing of a control rig's structure */
class STLLRN_ModularRigModel : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(STLLRN_ModularRigModel) {}
	SLATE_END_ARGS()

	~STLLRN_ModularRigModel();

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
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/** Rebuild the tree view */
	void RefreshTreeView(bool bRebuildContent = true);

	/** Returns all selected items */
	TArray<TSharedPtr<FTLLRN_ModularRigTreeElement>> GetSelectedItems() const;

	/** Return all selected keys */
	TArray<FString> GetSelectedKeys() const;

	/** Create a new item */
	void HandleNewItem();
	void HandleNewItem(UClass* InClass, const FString& InParentPath);

	/** Rename item */
	bool CanRenameModule() const;
	void HandleRenameModule();
	FName HandleRenameModule(const FString& InOldPath, const FName& InNewName);
	bool HandleVerifyNameChanged(const FString& InOldPath, const FName& InNewName, FText& OutErrorMessage);

	/** Delete items */
	void HandleDeleteModules();
	void HandleDeleteModules(const TArray<FString>& InPaths);

	/** Reparent items */
	void HandleReparentModules(const TArray<FString>& InPaths, const FString& InParentPath);

	/** Mirror items */
	void HandleMirrorModules();
	void HandleMirrorModules(const TArray<FString>& InPaths);

	/** Reresolve items */
	void HandleReresolveModules();
	void HandleReresolveModules(const TArray<FString>& InPaths);

	/** Swap module class for items */
	bool CanSwapModules() const;
	void HandleSwapClassForModules();
	void HandleSwapClassForModules(const TArray<FString>& InPaths);

	/** Resolve connector */
	void HandleConnectorResolved(const FTLLRN_RigElementKey& InConnector, const FTLLRN_RigElementKey& InTarget);

	/** UnResolve connector */
	void HandleConnectorDisconnect(const FTLLRN_RigElementKey& InConnector);

	/** Set Selection Changed */
	void HandleSelectionChanged(TSharedPtr<FTLLRN_ModularRigTreeElement> Selection, ESelectInfo::Type SelectInfo);

	TSharedPtr< SWidget > CreateContextMenuWidget();
	void OnItemClicked(TSharedPtr<FTLLRN_ModularRigTreeElement> InItem);
	void OnItemDoubleClicked(TSharedPtr<FTLLRN_ModularRigTreeElement> InItem);
	
	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	// reply to a drag operation
	FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	// reply to a drop operation on item
	TOptional<EItemDropZone> OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FTLLRN_ModularRigTreeElement> TargetItem);
	FReply OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FTLLRN_ModularRigTreeElement> TargetItem);

	// SWidget Overrides
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	static const FName ContextMenuName;
	static void CreateContextMenu();
	UToolMenu* GetContextMenu();
	TSharedPtr<FUICommandList> GetContextMenuCommands() const;
	
	/** Our owning control rig editor */
	TWeakPtr<FTLLRN_ControlRigEditor> TLLRN_ControlRigEditor;

	/** Tree view widget */
	TSharedPtr<STLLRN_ModularRigTreeView> TreeView;
	TSharedPtr<SHeaderRow> HeaderRowWidget;

	TWeakObjectPtr<UTLLRN_ControlRigBlueprint> TLLRN_ControlRigBlueprint;
	TWeakObjectPtr<UTLLRN_ModularRig> TLLRN_ControlRigBeingDebuggedPtr;
	
	/** Command list we bind to */
	TSharedPtr<FUICommandList> CommandList;

	bool IsSingleSelected() const;
	
	UTLLRN_ModularRig* GetTLLRN_ModularRig() const;
	UTLLRN_ModularRig* GetDefaultTLLRN_ModularRig() const;
	const UTLLRN_ModularRig* GetTLLRN_ModularRigForTreeView() const { return GetTLLRN_ModularRig(); }
	void OnRequestDetailsInspection(const FString& InKey);
	void HandlePreCompileTLLRN_ModularRigs(URigVMBlueprint* InBlueprint);
	void HandlePostCompileTLLRN_ModularRigs(URigVMBlueprint* InBlueprint);
	void OnTLLRN_ModularRigModified(ETLLRN_ModularRigNotification InNotif, const FTLLRN_RigModuleReference* InModule);
	void OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);

	void HandleRefreshEditorFromBlueprint(URigVMBlueprint* InBlueprint);
	void HandleSetObjectBeingDebugged(UObject* InObject);

	TSharedRef<SWidget> OnGetOptionsMenu();
	void OnFilterTextChanged(const FText& SearchText);

	bool bShowSecondaryConnectors;
	bool bShowOptionalConnectors;
	bool bShowUnresolvedConnectors;
	FText FilterText;

	TSharedPtr<SSearchBox> FilterBox;
	bool bIsPerformingSelection;

public:

	friend class FTLLRN_ModularRigTreeElement;
	friend class STLLRN_ModularRigModelItem;
	friend class UTLLRN_ControlRigBlueprintEditorLibrary;
};

