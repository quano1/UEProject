// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/RigVMDetailsViewWrapperObject.h"
#include "TLLRN_ControlRigWrapperObject.generated.h"

UCLASS()
class TLLRN_CONTROLRIGEDITOR_API UTLLRN_ControlRigWrapperObject : public URigVMDetailsViewWrapperObject
{
public:
	GENERATED_BODY()

	virtual UClass* GetClassForStruct(UScriptStruct* InStruct, bool bCreateIfNeeded) const override;
};
