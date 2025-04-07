// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/TLLRN_ControlRigSequenceObjectReference.h"
#include "TLLRN_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigSequenceObjectReference)

FTLLRN_ControlRigSequenceObjectReference FTLLRN_ControlRigSequenceObjectReference::Create(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	check(InTLLRN_ControlRig);

	FTLLRN_ControlRigSequenceObjectReference NewReference;
	NewReference.TLLRN_ControlRigClass = InTLLRN_ControlRig->GetClass();

	return NewReference;
}

bool FTLLRN_ControlRigSequenceObjectReferenceMap::HasBinding(const FGuid& ObjectId) const
{
	return BindingIds.Contains(ObjectId);
}

void FTLLRN_ControlRigSequenceObjectReferenceMap::RemoveBinding(const FGuid& ObjectId)
{
	int32 Index = BindingIds.IndexOfByKey(ObjectId);
	if (Index != INDEX_NONE)
	{
		BindingIds.RemoveAtSwap(Index, EAllowShrinking::No);
		References.RemoveAtSwap(Index, EAllowShrinking::No);
	}
}

void FTLLRN_ControlRigSequenceObjectReferenceMap::CreateBinding(const FGuid& ObjectId, const FTLLRN_ControlRigSequenceObjectReference& ObjectReference)
{
	int32 ExistingIndex = BindingIds.IndexOfByKey(ObjectId);
	if (ExistingIndex == INDEX_NONE)
	{
		ExistingIndex = BindingIds.Num();

		BindingIds.Add(ObjectId);
		References.AddDefaulted();
	}

	References[ExistingIndex].Array.AddUnique(ObjectReference);
}

