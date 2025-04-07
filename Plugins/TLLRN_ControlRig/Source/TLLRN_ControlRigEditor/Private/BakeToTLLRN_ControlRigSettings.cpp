// Copyright Epic Games, Inc. All Rights Reserved.

#include "BakeToTLLRN_ControlRigSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BakeToTLLRN_ControlRigSettings)

UBakeToTLLRN_ControlRigSettings::UBakeToTLLRN_ControlRigSettings(const FObjectInitializer& Initializer)
	: Super(Initializer)
{}

void UBakeToTLLRN_ControlRigSettings::Reset()
{
	bReduceKeys = false;
	SmartReduce.Reset();
}

