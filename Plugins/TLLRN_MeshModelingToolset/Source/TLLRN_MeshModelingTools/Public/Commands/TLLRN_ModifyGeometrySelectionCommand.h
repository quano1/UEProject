// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Selection/TLLRN_SelectionEditInteractiveCommand.h"
#include "TLLRN_ModifyGeometrySelectionCommand.generated.h"


/**
 * UTLLRN_ModifyGeometrySelectionCommand updates/edits the current selection in various ways.
 * Default operation is to Select All.
 * Subclasses below can be used in situations where specific per-modification types are needed.
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ModifyGeometrySelectionCommand : public UTLLRN_GeometrySelectionEditCommand
{
	GENERATED_BODY()
public:

	enum class EModificationType
	{
		SelectAll = 0,
		ExpandToConnected = 1,

		Invert = 10,
		InvertConnected = 11,

		Expand = 20,
		Contract = 21
	};
	virtual EModificationType GetModificationType() const { return EModificationType::SelectAll; }

	virtual bool AllowEmptySelection() const override { return GetModificationType() == EModificationType::SelectAll || GetModificationType() == EModificationType::Invert; }

	virtual bool IsModifySelectionCommand() const override { return true; }
	virtual FText GetCommandShortString() const override;

	virtual bool CanExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* Arguments) override;
	virtual void ExecuteCommandForSelection(UTLLRN_GeometrySelectionEditCommandArguments* Arguments, UInteractiveCommandResult** Result) override;
};


/**
 * Command to Invert the current Selection
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ModifyGeometrySelectionCommand_Invert : public UTLLRN_ModifyGeometrySelectionCommand
{
	GENERATED_BODY()
public:
	virtual EModificationType GetModificationType() const override { return EModificationType::Invert; }

};

/**
 * Command to Expand the current Selection to all connected geometry
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ModifyGeometrySelectionCommand_ExpandToConnected : public UTLLRN_ModifyGeometrySelectionCommand
{
	GENERATED_BODY()
public:
	virtual EModificationType GetModificationType() const override { return EModificationType::ExpandToConnected; }

};

/**
 * Command to Invert the current Selection, only considering connected geometry
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ModifyGeometrySelectionCommand_InvertConnected : public UTLLRN_ModifyGeometrySelectionCommand
{
	GENERATED_BODY()
public:
	virtual EModificationType GetModificationType() const override { return EModificationType::InvertConnected; }
};

/**
 * Command to Expand the current Selection by a one-ring
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ModifyGeometrySelectionCommand_Expand : public UTLLRN_ModifyGeometrySelectionCommand
{
	GENERATED_BODY()
public:
	virtual EModificationType GetModificationType() const override { return EModificationType::Expand; }
};

/**
 * Command to Contract the current Selection by a one-ring
 */
UCLASS()
class TLLRN_MESHMODELINGTOOLS_API UTLLRN_ModifyGeometrySelectionCommand_Contract : public UTLLRN_ModifyGeometrySelectionCommand
{
	GENERATED_BODY()
public:
	virtual EModificationType GetModificationType() const override { return EModificationType::Contract; }
};