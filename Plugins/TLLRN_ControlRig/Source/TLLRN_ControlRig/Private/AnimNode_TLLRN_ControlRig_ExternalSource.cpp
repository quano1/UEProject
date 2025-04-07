// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNode_TLLRN_ControlRig_ExternalSource.h"
#include "TLLRN_ControlRig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_TLLRN_ControlRig_ExternalSource)

FAnimNode_TLLRN_ControlRig_ExternalSource::FAnimNode_TLLRN_ControlRig_ExternalSource()
{
	bTLLRN_ControlRigRequiresInitialization = false;
}

void FAnimNode_TLLRN_ControlRig_ExternalSource::SetTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	TLLRN_ControlRig = InTLLRN_ControlRig;
	// requires initializing animation system
}

UTLLRN_ControlRig* FAnimNode_TLLRN_ControlRig_ExternalSource::GetTLLRN_ControlRig() const
{
	return (TLLRN_ControlRig.IsValid()? TLLRN_ControlRig.Get() : nullptr);
}

TSubclassOf<UTLLRN_ControlRig> FAnimNode_TLLRN_ControlRig_ExternalSource::GetTLLRN_ControlRigClass() const
{
	if(UTLLRN_ControlRig* CR = GetTLLRN_ControlRig())
	{
		return CR->GetClass();
	}
	return nullptr;
}

void FAnimNode_TLLRN_ControlRig_ExternalSource::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_TLLRN_ControlRigBase::Initialize_AnyThread(Context);

	if (TLLRN_ControlRig.IsValid())
	{
		//Don't Initialize the Control Rig here, the owner of the rig is in charge of initializing
		SetTargetInstance(TLLRN_ControlRig.Get());
	}
	else
	{
		SetTargetInstance(nullptr);
	}
}


