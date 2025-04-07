// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DragAndDrop/GraphNodeDragDropOp.h"
#include "Rigs/TLLRN_RigHierarchyDefines.h"

class STLLRN_RigHierarchyTagWidget;

class FTLLRN_RigElementHierarchyDragDropOp : public FGraphNodeDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FTLLRN_RigElementHierarchyDragDropOp, FGraphNodeDragDropOp)

	static TSharedRef<FTLLRN_RigElementHierarchyDragDropOp> New(const TArray<FTLLRN_RigElementKey>& InElements);

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;

	/** @return true if this drag operation contains property paths */
	bool HasElements() const
	{
		return Elements.Num() > 0;
	}

	/** @return The property paths from this drag operation */
	const TArray<FTLLRN_RigElementKey>& GetElements() const
	{
		return Elements;
	}

	FString GetJoinedElementNames() const;

	bool IsDraggingSingleConnector() const;
	bool IsDraggingSingleSocket() const;

private:

	/** Data for the property paths this item represents */
	TArray<FTLLRN_RigElementKey> Elements;
};

class FTLLRN_RigHierarchyTagDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FTLLRN_RigHierarchyTagDragDropOp, FDecoratedDragDropOp)

	static TSharedRef<FTLLRN_RigHierarchyTagDragDropOp> New(TSharedPtr<STLLRN_RigHierarchyTagWidget> InTagWidget);

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;

	/** @return The identifier being dragged */
	const FString& GetIdentifier() const
	{
		return Identifier;
	}

private:

	FText Text;
	FString Identifier;
};

class FTLLRN_ModularTLLRN_RigModuleDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FTLLRN_ModularTLLRN_RigModuleDragDropOp, FDragDropOperation)

	static TSharedRef<FTLLRN_ModularTLLRN_RigModuleDragDropOp> New(const TArray<FString>& InElements);

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;

	/** @return true if this drag operation contains property paths */
	bool HasElements() const
	{
		return Elements.Num() > 0;
	}

	/** @return The property paths from this drag operation */
	const TArray<FString>& GetElements() const
	{
		return Elements;
	}

	FString GetJoinedElementNames() const;

private:

	/** Data for the property paths this item represents */
	TArray<FString> Elements;
};

