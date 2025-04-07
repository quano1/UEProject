// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class TLLRN_IKRIGDEVELOPER_API FTLLRN_IKRigDeveloperModule : public IModuleInterface
{
public:
	void StartupModule() override;
	void ShutdownModule() override;
};