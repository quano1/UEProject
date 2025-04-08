// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Selection/TLLRN_SelectionEditInteractiveCommand.h"
#include "TLLRN_DisconnectGeometrySelectionCommand.generated.h"


/**
 * UTLLRN_DisconnectGeometrySelectionCommand disconnects the geometric elements identified by the Selection.
 * Currently only supports mesh selections (Triangle and Polygroup types)
 * Disconnects selected faces, or faces connected to selected edges, or faces connected to selected vertices.
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_DisconnectGeometrySelectionCommand : public UTLLRN_GeometrySelectionEditCommand
{
	GENERATED_BODY()
public:

	virtual FText GetCommandShortString() const override;

	virtual bool CanExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* Arguments) override;
	virtual void ExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* Arguments, UInteractiveCommandResult** Result) override;
};