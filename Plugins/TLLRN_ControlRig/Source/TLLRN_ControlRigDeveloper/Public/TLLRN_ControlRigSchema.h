// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RigVMModel/RigVMSchema.h"
#include "TLLRN_ControlRigSchema.generated.h"

UCLASS(BlueprintType)
class TLLRN_CONTROLRIGDEVELOPER_API UTLLRN_ControlRigSchema : public URigVMSchema
{
	GENERATED_UCLASS_BODY()

public:

	virtual bool ShouldUnfoldStruct(URigVMController* InController, const UStruct* InStruct) const override;

	virtual bool SupportsUnitFunction(URigVMController* InController, const FRigVMFunction* InUnitFunction) const override;
};