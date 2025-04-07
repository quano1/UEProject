// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigBlueprintGeneratedClass.h"
#include "Units/Control/TLLRN_RigUnit_Control.h"
#include "TLLRN_ControlRigObjectVersion.h"
#include "TLLRN_ControlRig.h"
#include "UObject/Package.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigBlueprintGeneratedClass)

UTLLRN_ControlRigBlueprintGeneratedClass::UTLLRN_ControlRigBlueprintGeneratedClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTLLRN_ControlRigBlueprintGeneratedClass::Serialize(FArchive& Ar)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	// don't use URigVMBlueprintGeneratedClass
	// to avoid backwards compat issues.
	UBlueprintGeneratedClass::Serialize(Ar);

	Ar.UsingCustomVersion(FTLLRN_ControlRigObjectVersion::GUID);

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::SwitchedToRigVM)
	{
		return;
	}

	// for debugging purposes we'll give this VM a name that's useful.
	static TAtomic<uint32> NumVMs{ 0 };
	static constexpr TCHAR Format[] = TEXT("%s_VM_%u");
	const FString VMDebugName = FString::Printf(Format, *GetName(), uint32(++NumVMs));
	URigVM* VM = NewObject<URigVM>(GetTransientPackage(), *VMDebugName);

	if (UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(GetDefaultObject(true)))
	{
		if (Ar.IsSaving() && CDO->VM)
		{
			VM->CopyDataForSerialization(CDO->VM);
		}
	}
	
	VM->Serialize(Ar);

	if (UTLLRN_ControlRig* CDO = Cast<UTLLRN_ControlRig>(GetDefaultObject(false)))
	{
		if (Ar.IsLoading() && CDO->VM)
		{
			CDO->VM->CopyDataForSerialization(VM);
		}
	}

	if (Ar.CustomVer(FTLLRN_ControlRigObjectVersion::GUID) < FTLLRN_ControlRigObjectVersion::StoreFunctionsInGeneratedClass)
	{
		return;
	}
	
	Ar << GraphFunctionStore;
}

