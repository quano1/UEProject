// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ModelingToolsHostCustomizationAPI.h"

#include "ContextObjectStore.h"
#include "InteractiveToolsContext.h"
#include "TLLRN_ModelingToolsEditorModeToolkit.h"

bool UTLLRN_ModelingToolsHostCustomizationAPI::RequestAcceptCancelButtonOverride(FAcceptCancelButtonOverrideParams& Params)
{
	if (TSharedPtr<FTLLRN_ModelingToolsEditorModeToolkit> ToolkitPinned = Toolkit.Pin())
	{
		return ToolkitPinned->RequestAcceptCancelButtonOverride(Params);
	}
	return false;
}

bool UTLLRN_ModelingToolsHostCustomizationAPI::RequestCompleteButtonOverride(FCompleteButtonOverrideParams& Params)
{
	if (TSharedPtr<FTLLRN_ModelingToolsEditorModeToolkit> ToolkitPinned = Toolkit.Pin())
	{
		return ToolkitPinned->RequestCompleteButtonOverride(Params);
	}
	return false;
}

void UTLLRN_ModelingToolsHostCustomizationAPI::ClearButtonOverrides()
{
	if (TSharedPtr<FTLLRN_ModelingToolsEditorModeToolkit> ToolkitPinned = Toolkit.Pin())
	{
		ToolkitPinned->ClearButtonOverrides();
	}
}

UTLLRN_ModelingToolsHostCustomizationAPI* UTLLRN_ModelingToolsHostCustomizationAPI::Register(
	UInteractiveToolsContext* ToolsContext, TSharedRef<FTLLRN_ModelingToolsEditorModeToolkit> Toolkit)
{
	if (!ensure(ToolsContext && ToolsContext->ContextObjectStore))
	{
		return nullptr;
	}

	UTLLRN_ModelingToolsHostCustomizationAPI* Found = ToolsContext->ContextObjectStore->FindContext<UTLLRN_ModelingToolsHostCustomizationAPI>();
	if (Found)
	{
		if (!ensureMsgf(Found->Toolkit == Toolkit,
			TEXT("UTLLRN_ModelingToolsHostCustomizationAPI already registered, but with different toolkit. "
				"Do not expect multiple toolkits to provide tool overlays to customize.")))
		{
			Found->Toolkit = Toolkit;
		}
		return Found;
	}
	else
	{
		UTLLRN_ModelingToolsHostCustomizationAPI* Instance = NewObject<UTLLRN_ModelingToolsHostCustomizationAPI>();
		Instance->Toolkit = Toolkit;
		ensure(ToolsContext->ContextObjectStore->AddContextObject(Instance));
		return Instance;
	}
}

bool UTLLRN_ModelingToolsHostCustomizationAPI::Deregister(UInteractiveToolsContext* ToolsContext)
{
	if (!ensure(ToolsContext && ToolsContext->ContextObjectStore))
	{
		return false;
	}

	ToolsContext->ContextObjectStore->RemoveContextObjectsOfType(UTLLRN_ModelingToolsHostCustomizationAPI::StaticClass());
	return true;
}
