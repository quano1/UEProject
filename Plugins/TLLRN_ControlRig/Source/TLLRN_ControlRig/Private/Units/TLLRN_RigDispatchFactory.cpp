// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/TLLRN_RigDispatchFactory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigDispatchFactory)

#if WITH_EDITOR

FString FTLLRN_RigDispatchFactory::GetArgumentDefaultValue(const FName& InArgumentName, TRigVMTypeIndex InTypeIndex) const
{
	if(InTypeIndex == FRigVMRegistry::Get().GetTypeIndex<FEulerTransform>())
	{
		static FString DefaultValueString = GetDefaultValueForStruct(FEulerTransform::Identity); 
		return DefaultValueString;
	}
	if(InTypeIndex == FRigVMRegistry::Get().GetTypeIndex<FTLLRN_RigElementKey>())
	{
		static FString DefaultValueString = GetDefaultValueForStruct(FTLLRN_RigElementKey(NAME_None, ETLLRN_RigElementType::Bone)); 
		return DefaultValueString;
	}
	return FRigVMDispatchFactory::GetArgumentDefaultValue(InArgumentName, InTypeIndex);
}

#endif
