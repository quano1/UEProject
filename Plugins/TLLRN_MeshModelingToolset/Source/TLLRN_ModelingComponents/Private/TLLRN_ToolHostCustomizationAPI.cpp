// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ToolHostCustomizationAPI.h"

#include "ContextObjectStore.h"
#include "InteractiveToolManager.h"
#include "UObject/Object.h"

TScriptInterface<ITLLRN_ToolHostCustomizationAPI> ITLLRN_ToolHostCustomizationAPI::Find(UInteractiveToolManager* ToolManager)
{
	if (!ensure(ToolManager))
	{
		return nullptr;
	}

	UContextObjectStore* ContextObjectStore = ToolManager->GetContextObjectStore();
	if (!ensure(ContextObjectStore))
	{
		return nullptr;
	}

	return ContextObjectStore->FindContextByClass(UTLLRN_ToolHostCustomizationAPI::StaticClass());
}