// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Selection/TLLRN_SelectionEditInteractiveCommand.h"
#include "TLLRN_RetriangulateGeometrySelectionCommand.generated.h"


/**
 * UTLLRN_RetriangulateGeometrySelectionCommand 
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_RetriangulateGeometrySelectionCommand : public UTLLRN_GeometrySelectionEditCommand
{
	GENERATED_BODY()
public:

	virtual bool AllowEmptySelection() const override { return true; }
	virtual FText GetCommandShortString() const override;

	virtual bool CanExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* Arguments) override;
	virtual void ExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* Arguments, UInteractiveCommandResult** Result) override;
};