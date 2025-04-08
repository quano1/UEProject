// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FTLLRN_ModelingComponentsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// registered in StartupModule() with FCoreDelegates::OnPostEngineInit and called once
	void OnPostEngineInit();
};
