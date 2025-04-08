// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InteractiveCommand.h"
#include "Selection/GeometrySelector.h"    // for FGeometrySelectionHandle
#include "TLLRN_SelectionEditInteractiveCommand.generated.h"

/**
 * Arguments for a UTLLRN_GeometrySelectionEditCommand
 */
UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_GeometrySelectionEditCommandArguments : public UInteractiveCommandArguments
{
	GENERATED_BODY()
public:
	FGeometrySelectionHandle SelectionHandle;
	UE::Geometry::EGeometryElementType ElementType;
	UE::Geometry::EGeometryTopologyType TopologyMode;

	bool IsSelectionEmpty() const
	{
		return SelectionHandle.Selection == nullptr || SelectionHandle.Selection->IsEmpty();
	}

	bool IsMatchingType(FGeometryIdentifier::ETargetType TargetType, FGeometryIdentifier::EObjectType EngineType) const
	{
		return SelectionHandle.Identifier.TargetType == TargetType
			|| SelectionHandle.Identifier.ObjectType == EngineType;
	}
};


UCLASS()
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_GeometrySelectionEditCommandResult : public UInteractiveCommandResult
{
	GENERATED_BODY()
public:
	FGeometrySelectionHandle SourceHandle;

	UE::Geometry::FGeometrySelection OutputSelection;
};


/**
 * UTLLRN_GeometrySelectionEditCommand is a command that edits geometry based on a selection.
 * Requires a UTLLRN_GeometrySelectionEditCommandArguments
 */
UCLASS(Abstract)
class TLLRN_MODELINGCOMPONENTS_API UTLLRN_GeometrySelectionEditCommand : public UInteractiveCommand
{
	GENERATED_BODY()
public:
	virtual bool AllowEmptySelection() const { return false; }
	virtual bool IsModifySelectionCommand() const { return false; }

	virtual bool CanExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* SelectionArgs)
	{
		return false;
	}
	
	virtual void ExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* Arguments, UInteractiveCommandResult** Result = nullptr)
	{
	}


public:

	// UInteractiveCommand API

	virtual bool CanExecuteCommand(UInteractiveCommandArguments* Arguments) override
	{
		UTLLRN_GeometrySelectionEditCommandArguments* SelectionArgs = Cast<UTLLRN_GeometrySelectionEditCommandArguments>(Arguments);
		if  (SelectionArgs && (SelectionArgs->IsSelectionEmpty() == false || AllowEmptySelection()) )
		{
			return CanExecuteCommandForSelection(SelectionArgs);
		}
		return false;
	}

	virtual void ExecuteCommand(UInteractiveCommandArguments* Arguments, UInteractiveCommandResult** Result = nullptr) override
	{
		UTLLRN_GeometrySelectionEditCommandArguments* SelectionArgs = Cast<UTLLRN_GeometrySelectionEditCommandArguments>(Arguments);
		if ( SelectionArgs && (SelectionArgs->IsSelectionEmpty() == false || AllowEmptySelection()) )
		{
			ExecuteCommandForSelection(SelectionArgs, Result);
		}
	}


};
