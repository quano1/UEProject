// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Rigs/TLLRN_RigHierarchy.h"
#include "RigVMCore/RigVMUserWorkflow.h"
#include "TLLRN_ControlRigNodeWorkflow.generated.h"

UCLASS(BlueprintType)
class TLLRN_CONTROLRIG_API UTLLRN_ControlRigWorkflowOptions : public URigVMUserWorkflowOptions
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Options)
	TObjectPtr<const UTLLRN_RigHierarchy> Hierarchy;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Options)
	TArray<FTLLRN_RigElementKey> Selection;

	UFUNCTION(BlueprintCallable, Category = "Options")
	bool EnsureAtLeastOneTLLRN_RigElementSelected() const;
};

UCLASS(BlueprintType)
class TLLRN_CONTROLRIG_API UTLLRN_ControlTLLRN_RigTransformWorkflowOptions : public UTLLRN_ControlRigWorkflowOptions
{
	GENERATED_BODY()

public:

	// The type of transform to retrieve from the hierarchy
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Options)
	TEnumAsByte<ETLLRN_RigTransformType::Type> TransformType = ETLLRN_RigTransformType::CurrentGlobal;

	UFUNCTION()
	TArray<FRigVMUserWorkflow> ProvideWorkflows(const UObject* InSubject);

protected:

#if WITH_EDITOR
	static bool PerformTransformWorkflow(const URigVMUserWorkflowOptions* InOptions, UObject* InController);
#endif
};
