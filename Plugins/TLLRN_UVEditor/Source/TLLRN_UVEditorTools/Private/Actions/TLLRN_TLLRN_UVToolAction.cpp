// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actions/TLLRN_TLLRN_UVToolAction.h"

#include "ContextObjectStore.h"
#include "Selection/TLLRN_UVToolSelectionAPI.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_TLLRN_UVToolAction)

#define LOCTEXT_NAMESPACE "UTLLRN_TLLRN_UVToolAction"

using namespace UE::Geometry;

void UTLLRN_TLLRN_UVToolAction::Setup(UInteractiveToolManager* ToolManagerIn)
{
	ToolManager = ToolManagerIn;

	UContextObjectStore* ContextStore = GetToolManager()->GetContextObjectStore();
	SelectionAPI = ContextStore->FindContext<UTLLRN_UVToolSelectionAPI>();
	checkSlow(SelectionAPI);
	EmitChangeAPI = ContextStore->FindContext<UTLLRN_UVToolEmitChangeAPI>();
}

void UTLLRN_TLLRN_UVToolAction::Shutdown()
{
	SelectionAPI = nullptr;
	EmitChangeAPI = nullptr;
}

#undef LOCTEXT_NAMESPACE
