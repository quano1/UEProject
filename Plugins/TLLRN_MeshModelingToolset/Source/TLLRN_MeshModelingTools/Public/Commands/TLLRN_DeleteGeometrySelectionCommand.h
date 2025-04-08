// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Selection/TLLRN_SelectionEditInteractiveCommand.h"
#include "TLLRN_DeleteGeometrySelectionCommand.generated.h"


/**
 * UTLLRN_DeleteGeometrySelectionCommand deletes the geometric elements identified by the Selection.
 * Currently only supports mesh selections (Triangle and Polygroup types)
 * Deletes selected faces, or faces connected to selected edges, or faces connected to selected vertices.
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_DeleteGeometrySelectionCommand : public UTLLRN_GeometrySelectionEditCommand
{
	GENERATED_BODY()
public:

	virtual FText GetCommandShortString() const override;

	virtual bool CanExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* Arguments) override;
	virtual void ExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* Arguments, UInteractiveCommandResult** Result) override;
};