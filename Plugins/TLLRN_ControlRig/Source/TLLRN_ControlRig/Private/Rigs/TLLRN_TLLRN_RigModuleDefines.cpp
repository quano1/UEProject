// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_TLLRN_RigModuleDefines.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_TLLRN_RigModuleDefines)

bool FTLLRN_RigModuleConnector::operator==(const FTLLRN_RigModuleConnector& Other) const
{
	return Name == Other.Name && GetTypeHash(Settings) == GetTypeHash(Other.Settings);
}
