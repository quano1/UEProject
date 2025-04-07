// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Delegates/IDelegateInstance.h"
#include "ITLLRN_ControlTLLRN_RigModule.h"
#include "Materials/Material.h"

class FTLLRN_ControlTLLRN_RigModule : public ITLLRN_ControlTLLRN_RigModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	UMaterial* ManipulatorMaterial;
	
private:
	FDelegateHandle OnCreateMovieSceneObjectSpawnerHandle;

	void RegisterTransformableCustomization() const;
};
