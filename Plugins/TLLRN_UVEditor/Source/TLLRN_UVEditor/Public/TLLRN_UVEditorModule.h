// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

#include "TLLRN_UVEditorModularFeature.h"

class FLayoutExtender;

/**
 * Besides the normal module things, the module class is also responsible for hooking the 
 * UV editor into existing menus.
 */
class TLLRN_UVEDITOR_API FTLLRN_UVEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	DECLARE_EVENT_OneParam(FTLLRN_UVEditorModule, FOnRegisterLayoutExtensions, FLayoutExtender&);
	virtual FOnRegisterLayoutExtensions& OnRegisterLayoutExtensions() { return RegisterLayoutExtensions; }

protected:
	void RegisterMenus();

private:
	FOnRegisterLayoutExtensions	RegisterLayoutExtensions;

	/** StaticClass is not safe on shutdown, so we cache the name, and use this to unregister on shut down */
	TArray<FName> ClassesToUnregisterOnShutdown;

	TSharedPtr<UE::Geometry::FTLLRN_UVEditorModularFeature> TLLRN_UVEditorAssetEditor;
};
