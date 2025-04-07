// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "AnimNode_TLLRN_ControlRig.h"
#include "AnimNode_TLLRN_ControlRig_Library.generated.h"

USTRUCT(BlueprintType)
struct FTLLRN_ControlRigReference : public FAnimNodeReference
{
	GENERATED_BODY()

	typedef FAnimNode_TLLRN_ControlRig FInternalNodeType;
};

// Exposes operations to be performed on a control rig anim node
UCLASS(Experimental, MinimalAPI)
class UAnimNodeTLLRN_ControlRigLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Get a control rig context from an anim node context */
	UFUNCTION(BlueprintCallable, Category = "Animation|TLLRN_ControlRig", meta = (BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static TLLRN_CONTROLRIG_API FTLLRN_ControlRigReference ConvertToTLLRN_ControlRig(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result);

	/** Get a control rig context from an anim node context (pure) */
	UFUNCTION(BlueprintPure, Category = "Animation|TLLRN_ControlRig", meta = (BlueprintThreadSafe, DisplayName = "Convert to Sequence Player"))
	static void ConvertToTLLRN_ControlRigPure(const FAnimNodeReference& Node, FTLLRN_ControlRigReference& TLLRN_ControlRig, bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		TLLRN_ControlRig = ConvertToTLLRN_ControlRig(Node, ConversionResult);
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}

	/** Set the control rig class on the node */
	UFUNCTION(BlueprintCallable, Category = "Animation|TLLRN_ControlRig", meta = (BlueprintThreadSafe))
	static TLLRN_CONTROLRIG_API FTLLRN_ControlRigReference SetTLLRN_ControlRigClass(const FTLLRN_ControlRigReference& Node, TSubclassOf<UTLLRN_ControlRig> TLLRN_ControlRigClass);
};
