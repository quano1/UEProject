// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/TLLRN_ControlRigNodeWorkflow.h"
#include "RigVMCore/RigVMStruct.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigNodeWorkflow)

#if WITH_EDITOR
#include "RigVMModel/RigVMNode.h"
#include "RigVMModel/RigVMController.h"
#endif

bool UTLLRN_ControlRigWorkflowOptions::EnsureAtLeastOneTLLRN_RigElementSelected() const
{
	if(Selection.IsEmpty())
	{
		static constexpr TCHAR SelectAtLeastOneTLLRN_RigElementMessage[] = TEXT("Please select at least one element in the hierarchy!");
		Reportf(EMessageSeverity::Error, SelectAtLeastOneTLLRN_RigElementMessage);
		return false;
	}
	return true;
}

// provides the default workflows for any pin
TArray<FRigVMUserWorkflow> UTLLRN_ControlTLLRN_RigTransformWorkflowOptions::ProvideWorkflows(const UObject* InSubject)
{
	TArray<FRigVMUserWorkflow> Workflows;
#if WITH_EDITOR
	if(const URigVMPin* Pin = Cast<URigVMPin>(InSubject))
	{
		if(!Pin->IsArray())
		{
			if(Pin->GetCPPType() == RigVMTypeUtils::GetUniqueStructTypeName(TBaseStructure<FTransform>::Get()))
			{
				Workflows.Emplace(
					TEXT("Set from hierarchy"),
					TEXT("Sets the pin to match the global transform of the selected element in the hierarchy"),
					ERigVMUserWorkflowType::PinContext,
					FRigVMPerformUserWorkflowDelegate::CreateStatic(&UTLLRN_ControlTLLRN_RigTransformWorkflowOptions::PerformTransformWorkflow),
					StaticClass()
				);
			}
		}
	}
#endif
	return Workflows;
}

#if WITH_EDITOR

bool UTLLRN_ControlTLLRN_RigTransformWorkflowOptions::PerformTransformWorkflow(const URigVMUserWorkflowOptions* InOptions,
	UObject* InController)
{
	URigVMController* Controller = CastChecked<URigVMController>(InController);
	
	if(const UTLLRN_ControlTLLRN_RigTransformWorkflowOptions* Options = Cast<UTLLRN_ControlTLLRN_RigTransformWorkflowOptions>(InOptions))
	{
		if(Options->EnsureAtLeastOneTLLRN_RigElementSelected())
		{
			const FTLLRN_RigElementKey& Key = Options->Selection[0];
			if(FTLLRN_RigTransformElement* TransformElement = (FTLLRN_RigTransformElement*)Options->Hierarchy->Find<FTLLRN_RigTransformElement>(Key))
			{
				const FTransform Transform = Options->Hierarchy->GetTransform(TransformElement, Options->TransformType);

				return Controller->SetPinDefaultValue(
					InOptions->GetSubject<URigVMPin>()->GetPinPath(),
					FRigVMStruct::ExportToFullyQualifiedText<FTransform>(Transform)
				);
			}
		}
	}
	
	return false;
}
#endif
