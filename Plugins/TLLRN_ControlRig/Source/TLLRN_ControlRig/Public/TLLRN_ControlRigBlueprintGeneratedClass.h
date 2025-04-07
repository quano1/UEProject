// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "TLLRN_ControlRigDefines.h"
#include "RigVMCore/RigVM.h"
#include "RigVMCore/RigVMGraphFunctionHost.h"
#include "RigVMBlueprintGeneratedClass.h"
#include "TLLRN_ControlRigBlueprintGeneratedClass.generated.h"

UCLASS()
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigBlueprintGeneratedClass : public URigVMBlueprintGeneratedClass
{
	GENERATED_UCLASS_BODY()

public:

	// UObject interface
	void Serialize(FArchive& Ar);
};