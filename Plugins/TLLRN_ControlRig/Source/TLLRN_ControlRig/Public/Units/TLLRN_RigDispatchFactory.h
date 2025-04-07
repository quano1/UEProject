// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_ControlRigDefines.h"
#include "RigVMCore/RigVMDispatchFactory.h"
#include "TLLRN_RigUnitContext.h"
#include "TLLRN_RigDispatchFactory.generated.h"

/** Base class for all rig dispatch factories */
USTRUCT(BlueprintType, meta=(Abstract, ExecuteContextType=FTLLRN_ControlRigExecuteContext))
struct TLLRN_CONTROLRIG_API FTLLRN_RigDispatchFactory : public FRigVMDispatchFactory
{
	GENERATED_BODY()

	virtual UScriptStruct* GetExecuteContextStruct() const override
	{
		return FTLLRN_ControlRigExecuteContext::StaticStruct();
	}

	virtual void RegisterDependencyTypes_NoLock(FRigVMRegistry_NoLock& InRegistry) const override
	{
		InRegistry.FindOrAddType_NoLock(FTLLRN_ControlRigExecuteContext::StaticStruct());
		InRegistry.FindOrAddType_NoLock(FTLLRN_RigElementKey::StaticStruct());
    	InRegistry.FindOrAddType_NoLock(FTLLRN_CachedTLLRN_RigElement::StaticStruct());
	}

#if WITH_EDITOR

	virtual FString GetArgumentDefaultValue(const FName& InArgumentName, TRigVMTypeIndex InTypeIndex) const override;

#endif

	static const FTLLRN_RigUnitContext& GetTLLRN_RigUnitContext(const FRigVMExtendedExecuteContext& InContext)
	{
		return InContext.GetPublicData<FTLLRN_ControlRigExecuteContext>().UnitContext;
	}
};

