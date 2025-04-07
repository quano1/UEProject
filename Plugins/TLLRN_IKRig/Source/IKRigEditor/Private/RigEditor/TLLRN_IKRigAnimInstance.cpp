// Copyright Epic Games, Inc. All Rights Reserved.

#include "RigEditor/TLLRN_IKRigAnimInstance.h"
#include "RigEditor/TLLRN_IKRigAnimInstanceProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_IKRigAnimInstance)

UTLLRN_IKRigAnimInstance::UTLLRN_IKRigAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseMultiThreadedAnimationUpdate = false;
}

void UTLLRN_IKRigAnimInstance::SetIKRigAsset(UTLLRN_IKRigDefinition* InIKRigAsset)
{
	FTLLRN_IKRigAnimInstanceProxy& Proxy = GetProxyOnGameThread<FTLLRN_IKRigAnimInstanceProxy>();
	Proxy.SetIKRigAsset(InIKRigAsset);
}

void UTLLRN_IKRigAnimInstance::SetProcessorNeedsInitialized()
{
	IKRigNode.SetProcessorNeedsInitialized();
}

UTLLRN_IKRigProcessor* UTLLRN_IKRigAnimInstance::GetCurrentlyRunningProcessor()
{
	IKRigNode.CreateTLLRN_IKRigProcessorIfNeeded(this);
	return IKRigNode.GetTLLRN_IKRigProcessor();
}

FAnimInstanceProxy* UTLLRN_IKRigAnimInstance::CreateAnimInstanceProxy()
{
	return new FTLLRN_IKRigAnimInstanceProxy(this, &IKRigNode);
}

