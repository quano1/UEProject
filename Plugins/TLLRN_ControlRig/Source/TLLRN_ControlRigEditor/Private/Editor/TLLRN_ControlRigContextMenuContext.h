// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "Rigs/TLLRN_RigHierarchyDefines.h"

#include "TLLRN_ControlRigContextMenuContext.generated.h"

class UTLLRN_ControlRig;
class UTLLRN_ControlRigBlueprint;
class FTLLRN_ControlRigEditor;
class URigVMGraph;
class URigVMNode;
class URigVMPin;
class STLLRN_RigHierarchy;
class STLLRN_ModularRigModel;

USTRUCT(BlueprintType)
struct FTLLRN_ControlRigTLLRN_RigHierarchyDragAndDropContext
{
	GENERATED_BODY()
	
	FTLLRN_ControlRigTLLRN_RigHierarchyDragAndDropContext() = default;
	
	FTLLRN_ControlRigTLLRN_RigHierarchyDragAndDropContext(const TArray<FTLLRN_RigElementKey> InDraggedElementKeys, FTLLRN_RigElementKey InTargetElementKey)
		: DraggedElementKeys(InDraggedElementKeys)
		, TargetElementKey(InTargetElementKey)
	{
	}
	
	UPROPERTY(BlueprintReadOnly, Category = TLLRN_ControlRigEditorExtensions)
	TArray<FTLLRN_RigElementKey> DraggedElementKeys;

	UPROPERTY(BlueprintReadOnly, Category = TLLRN_ControlRigEditorExtensions)
	FTLLRN_RigElementKey TargetElementKey;
};

USTRUCT(BlueprintType)
struct FTLLRN_ControlRigGraphNodeContextMenuContext
{
	GENERATED_BODY()

	FTLLRN_ControlRigGraphNodeContextMenuContext()
		: Graph(nullptr)
		, Node(nullptr)
		, Pin(nullptr)
	{
	}
	
	FTLLRN_ControlRigGraphNodeContextMenuContext(TObjectPtr<const URigVMGraph> InGraph, TObjectPtr<const URigVMNode> InNode, TObjectPtr<const URigVMPin> InPin)
		: Graph(InGraph)
		, Node(InNode)
		, Pin(InPin)
	{
	}
	
	/** The graph associated with this context. */
	UPROPERTY(BlueprintReadOnly, Category = TLLRN_ControlRigEditorExtensions)
	TObjectPtr<const URigVMGraph> Graph;

	/** The node associated with this context. */
	UPROPERTY(BlueprintReadOnly, Category = TLLRN_ControlRigEditorExtensions)
	TObjectPtr<const URigVMNode> Node;

	/** The pin associated with this context; may be NULL when over a node. */
	UPROPERTY(BlueprintReadOnly, Category = TLLRN_ControlRigEditorExtensions)
	TObjectPtr<const URigVMPin> Pin;
};

USTRUCT(BlueprintType)
struct FTLLRN_ControlRigTLLRN_RigHierarchyToGraphDragAndDropContext
{
	GENERATED_BODY()

	FTLLRN_ControlRigTLLRN_RigHierarchyToGraphDragAndDropContext() = default;
	
	FTLLRN_ControlRigTLLRN_RigHierarchyToGraphDragAndDropContext(const TArray<FTLLRN_RigElementKey>& InDraggedElementKeys, UEdGraph* InGraph, const FVector2D& InNodePosition)
		:DraggedElementKeys(InDraggedElementKeys)
		,Graph(InGraph)
		,NodePosition(InNodePosition)
	{ };

	UPROPERTY(BlueprintReadOnly, Category = TLLRN_ControlRigEditorExtensions)
	TArray<FTLLRN_RigElementKey> DraggedElementKeys;

	TWeakObjectPtr<UEdGraph> Graph;

	FVector2D NodePosition;

	FString GetSectionTitle() const;
};

struct FTLLRN_ControlRigMenuSpecificContext
{
	TWeakPtr<STLLRN_RigHierarchy> TLLRN_RigHierarchyPanel;
	
	FTLLRN_ControlRigTLLRN_RigHierarchyDragAndDropContext TLLRN_RigHierarchyDragAndDropContext;

	TWeakPtr<STLLRN_ModularRigModel> TLLRN_ModularRigModelPanel;
	
	FTLLRN_ControlRigGraphNodeContextMenuContext GraphNodeContextMenuContext;

	FTLLRN_ControlRigTLLRN_RigHierarchyToGraphDragAndDropContext TLLRN_RigHierarchyToGraphDragAndDropContext;
};

UCLASS(BlueprintType)
class UTLLRN_ControlRigContextMenuContext : public UObject
{
	GENERATED_BODY()

public:
	/**
	 *	Initialize the Context
	 * @param InTLLRN_ControlRigEditor 	    The Control Rig Editor hosting the menus
	 * @param InMenuSpecificContext 	Additional context for specific menus
	*/
	void Init(TWeakPtr<FTLLRN_ControlRigEditor> InTLLRN_ControlRigEditor, const FTLLRN_ControlRigMenuSpecificContext& InMenuSpecificContext = FTLLRN_ControlRigMenuSpecificContext());

	/** Get the control rig blueprint that we are editing */
	UFUNCTION(BlueprintCallable, Category = TLLRN_ControlRigEditorExtensions)
    UTLLRN_ControlRigBlueprint* GetTLLRN_ControlRigBlueprint() const;
	
	/** Get the active control rig instance in the viewport */
	UFUNCTION(BlueprintCallable, Category = TLLRN_ControlRigEditorExtensions)
    UTLLRN_ControlRig* GetTLLRN_ControlRig() const;

	/** Returns true if either alt key is down */
	UFUNCTION(BlueprintCallable, Category = TLLRN_ControlRigEditorExtensions)
	bool IsAltDown() const;
	
	/** Returns context for a drag & drop action that contains source and target element keys */ 
	UFUNCTION(BlueprintCallable, Category = TLLRN_ControlRigEditorExtensions)
	FTLLRN_ControlRigTLLRN_RigHierarchyDragAndDropContext GetTLLRN_RigHierarchyDragAndDropContext();

	/** Returns context for graph node context menu */
	UFUNCTION(BlueprintCallable, Category = TLLRN_ControlRigEditorExtensions)
	FTLLRN_ControlRigGraphNodeContextMenuContext GetGraphNodeContextMenuContext();
	
	/** Returns context for the menu that shows up when drag and drop from Rig Hierarchy to the Rig Graph*/
	UFUNCTION(BlueprintCallable, Category = TLLRN_ControlRigEditorExtensions)
    FTLLRN_ControlRigTLLRN_RigHierarchyToGraphDragAndDropContext GetTLLRN_RigHierarchyToGraphDragAndDropContext();

	STLLRN_RigHierarchy* GetTLLRN_RigHierarchyPanel() const;

	STLLRN_ModularRigModel* GetTLLRN_ModularRigModelPanel() const;
	
	FTLLRN_ControlRigEditor* GetTLLRN_ControlRigEditor() const;

private:
	TWeakPtr<FTLLRN_ControlRigEditor> WeakTLLRN_ControlRigEditor;

	FTLLRN_ControlRigMenuSpecificContext MenuSpecificContext;
};