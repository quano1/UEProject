// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNode_TLLRN_ControlRig_Library.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_TLLRN_ControlRig_Library)

FTLLRN_ControlRigReference UAnimNodeTLLRN_ControlRigLibrary::ConvertToTLLRN_ControlRig(const FAnimNodeReference& Node, EAnimNodeReferenceConversionResult& Result)
{
	return FAnimNodeReference::ConvertToType<FTLLRN_ControlRigReference>(Node, Result);
}

FTLLRN_ControlRigReference UAnimNodeTLLRN_ControlRigLibrary::SetTLLRN_ControlRigClass(const FTLLRN_ControlRigReference& Node, TSubclassOf<UTLLRN_ControlRig> TLLRN_ControlRigClass)
{
	Node.CallAnimNodeFunction<FAnimNode_TLLRN_ControlRig>(
	TEXT("SetSequence"),
	[TLLRN_ControlRigClass](FAnimNode_TLLRN_ControlRig& InTLLRN_ControlRigNode)
	{
		InTLLRN_ControlRigNode.SetTLLRN_ControlRigClass(TLLRN_ControlRigClass);
	});

	return Node;
}