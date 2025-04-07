// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Actions/TLLRN_TLLRN_UVToolAction.h"

#include "TLLRN_UnsetUVsAction.generated.h"

//~ Action that unsets UVs, intended for testing other tools. For now, this is
//~  in the editor module (instead of TLLRN_UVEditorTools) so that we can keep it
//~  unexposed.
UCLASS()
class UTLLRN_UnsetUVsAction : public UTLLRN_TLLRN_UVToolAction
{
	GENERATED_BODY()

public:
	virtual bool CanExecuteAction() const override;
	virtual bool ExecuteAction() override;
};