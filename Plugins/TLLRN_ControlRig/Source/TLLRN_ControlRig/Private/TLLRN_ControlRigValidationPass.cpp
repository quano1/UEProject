// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigValidationPass.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"
#include "UObject/Package.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigValidationPass)

////////////////////////////////////////////////////////////////////////////////
// FTLLRN_ControlRigValidationContext
////////////////////////////////////////////////////////////////////////////////

FTLLRN_ControlRigValidationContext::FTLLRN_ControlRigValidationContext()
	: DrawInterface(nullptr)
{
}

void FTLLRN_ControlRigValidationContext::Clear()
{
	ClearDelegate.ExecuteIfBound();
}

void FTLLRN_ControlRigValidationContext::Report(EMessageSeverity::Type InSeverity, const FString& InMessage)
{
	Report(InSeverity, FTLLRN_RigElementKey(), FLT_MAX, InMessage);
}

void FTLLRN_ControlRigValidationContext::Report(EMessageSeverity::Type InSeverity, const FTLLRN_RigElementKey& InKey, const FString& InMessage)
{
	Report(InSeverity, InKey, FLT_MAX, InMessage);
}

void FTLLRN_ControlRigValidationContext::Report(EMessageSeverity::Type InSeverity, const FTLLRN_RigElementKey& InKey, float InQuality, const FString& InMessage)
{
	ReportDelegate.ExecuteIfBound(InSeverity, InKey, InQuality, InMessage);
}

FString FTLLRN_ControlRigValidationContext::GetDisplayNameForEvent(const FName& InEventName) const
{
	FString DisplayName = InEventName.ToString();

#if WITH_EDITOR
	if(InEventName == FTLLRN_RigUnit_BeginExecution::EventName)
	{
		FTLLRN_RigUnit_BeginExecution::StaticStruct()->GetStringMetaDataHierarchical(FRigVMStruct::DisplayNameMetaName, &DisplayName);
	}
	else if (InEventName == FTLLRN_RigUnit_InverseExecution::EventName)
	{
		FTLLRN_RigUnit_InverseExecution::StaticStruct()->GetStringMetaDataHierarchical(FRigVMStruct::DisplayNameMetaName, &DisplayName);
	}
	else if (InEventName == FTLLRN_RigUnit_PrepareForExecution::EventName)
	{
		FTLLRN_RigUnit_PrepareForExecution::StaticStruct()->GetStringMetaDataHierarchical(FRigVMStruct::DisplayNameMetaName, &DisplayName);
	}
#endif

	return DisplayName;
}

////////////////////////////////////////////////////////////////////////////////
// UTLLRN_ControlRigValidator
////////////////////////////////////////////////////////////////////////////////

UTLLRN_ControlRigValidator::UTLLRN_ControlRigValidator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UTLLRN_ControlRigValidationPass* UTLLRN_ControlRigValidator::FindPass(UClass* InClass) const
{
	for (UTLLRN_ControlRigValidationPass* Pass : Passes)
	{
		if (Pass->GetClass() == InClass)
		{
			return Pass;
		}
	}
	return nullptr;
}

UTLLRN_ControlRigValidationPass* UTLLRN_ControlRigValidator::AddPass(UClass* InClass)
{
	check(InClass);

	if (UTLLRN_ControlRigValidationPass* ExistingPass = FindPass(InClass))
	{
		return ExistingPass;
	}

	UTLLRN_ControlRigValidationPass* NewPass = NewObject<UTLLRN_ControlRigValidationPass>(this, InClass);
	Passes.Add(NewPass);

	NewPass->OnSubjectChanged(GetTLLRN_ControlRig(), &ValidationContext);
	NewPass->OnInitialize(GetTLLRN_ControlRig(), &ValidationContext);

	return NewPass;
}

void UTLLRN_ControlRigValidator::RemovePass(UClass* InClass)
{
	for (UTLLRN_ControlRigValidationPass* Pass : Passes)
	{
		if (Pass->GetClass() == InClass)
		{
			Passes.Remove(Pass);
			Pass->Rename(nullptr, GetTransientPackage());
			return;
		}
	}
}

void UTLLRN_ControlRigValidator::SetTLLRN_ControlRig(UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = WeakTLLRN_ControlRig.Get())
	{
		TLLRN_ControlRig->OnInitialized_AnyThread().RemoveAll(this);
		TLLRN_ControlRig->OnExecuted_AnyThread().RemoveAll(this);
	}

	ValidationContext.DrawInterface = nullptr;
	WeakTLLRN_ControlRig = InTLLRN_ControlRig;

	if (UTLLRN_ControlRig* TLLRN_ControlRig = WeakTLLRN_ControlRig.Get())
	{
		OnTLLRN_ControlRigInitialized(TLLRN_ControlRig, FTLLRN_RigUnit_BeginExecution::EventName);
		TLLRN_ControlRig->OnInitialized_AnyThread().AddUObject(this, &UTLLRN_ControlRigValidator::OnTLLRN_ControlRigInitialized);
		TLLRN_ControlRig->OnExecuted_AnyThread().AddUObject(this, &UTLLRN_ControlRigValidator::OnTLLRN_ControlRigExecuted);
		ValidationContext.DrawInterface = &TLLRN_ControlRig->DrawInterface;
	}
}

void UTLLRN_ControlRigValidator::OnTLLRN_ControlRigInitialized(URigVMHost* Subject, const FName& EventName)
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = WeakTLLRN_ControlRig.Get())
	{
		if (TLLRN_ControlRig != Subject)
		{
			return;
		}

		ValidationContext.Clear();

		for (int32 PassIndex = 0; PassIndex < Passes.Num(); PassIndex++)
		{
			Passes[PassIndex]->OnInitialize(TLLRN_ControlRig, &ValidationContext);
		}
	}
}

void UTLLRN_ControlRigValidator::OnTLLRN_ControlRigExecuted(URigVMHost* Subject, const FName& EventName)
{
	if (UTLLRN_ControlRig* TLLRN_ControlRig = WeakTLLRN_ControlRig.Get())
	{
		if (TLLRN_ControlRig != Subject)
		{
			return;
		}

		for (int32 PassIndex = 0; PassIndex < Passes.Num(); PassIndex++)
		{
			Passes[PassIndex]->OnEvent(TLLRN_ControlRig, EventName, &ValidationContext);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// UTLLRN_ControlRigValidationPass
////////////////////////////////////////////////////////////////////////////////

UTLLRN_ControlRigValidationPass::UTLLRN_ControlRigValidationPass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

