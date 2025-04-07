// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigNumericalValidationPass.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigNumericalValidationPass)

////////////////////////////////////////////////////////////////////////////////
// UTLLRN_ControlRigNumericalValidationPass
////////////////////////////////////////////////////////////////////////////////

UTLLRN_ControlRigNumericalValidationPass::UTLLRN_ControlRigNumericalValidationPass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bCheckControls(true)
	, bCheckBones(true)
	, bCheckCurves(true)
	, TranslationPrecision(0.01f)
	, RotationPrecision(0.01f)
	, ScalePrecision(0.001f)
	, CurvePrecision(0.01f)
{
}

void UTLLRN_ControlRigNumericalValidationPass::OnSubjectChanged(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_ControlRigValidationContext* InContext)
{
	Pose.Reset();

	TArray<FName> EventNames = InTLLRN_ControlRig->GetEventQueue();
	if (EventNames.Num() == 2)
	{
		EventNameA = EventNames[0];
		EventNameB = EventNames[1];
	}
	else
	{
		EventNameA = EventNameB = NAME_None;
	}
}

void UTLLRN_ControlRigNumericalValidationPass::OnInitialize(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_ControlRigValidationContext* InContext)
{
	OnSubjectChanged(InTLLRN_ControlRig, InContext);
}

void UTLLRN_ControlRigNumericalValidationPass::OnEvent(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InEventName, FTLLRN_ControlRigValidationContext* InContext)
{
	if (InEventName == EventNameA)
	{
		Pose = InTLLRN_ControlRig->GetHierarchy()->GetPose();
	}
	else if (InEventName == EventNameB)
	{
		for(FTLLRN_RigPoseElement& PoseElement : Pose)
		{
			if (!PoseElement.Index.UpdateCache(InTLLRN_ControlRig->GetHierarchy()))
			{
				continue;
			}

			FTLLRN_RigElementKey Key = PoseElement.Index.GetKey();
			int32 ElementIndex = PoseElement.Index.GetIndex();

			if(Key.Type == ETLLRN_RigElementType::Control)
			{
				if (!bCheckControls)
				{
					continue;
				}
			}
			else if(Key.Type == ETLLRN_RigElementType::Bone)
			{
				if (!bCheckBones)
				{
					continue;
				}
			}
			else if(Key.Type != ETLLRN_RigElementType::Curve)
			{
				continue;
			}
			
			if(Key.Type == ETLLRN_RigElementType::Curve)
			{
				float A = PoseElement.CurveValue;
				float B = InTLLRN_ControlRig->GetHierarchy()->GetCurveValue(ElementIndex);

				if (FMath::Abs(A - B) > CurvePrecision + SMALL_NUMBER)
				{
					FString EventNameADisplayString = InContext->GetDisplayNameForEvent(EventNameA);
					FString EventNameBDisplayString = InContext->GetDisplayNameForEvent(EventNameB);
					FString Message = FString::Printf(TEXT("Values don't match between %s and %s."), *EventNameADisplayString, *EventNameBDisplayString);
					InContext->Report(EMessageSeverity::Warning, Key, Message);
				}
				continue;
			}

			FTransform A = PoseElement.GlobalTransform;
			FTransform B = InTLLRN_ControlRig->GetHierarchy()->GetGlobalTransform(PoseElement.Index);

			FQuat RotA = A.GetRotation().GetNormalized();
			FQuat RotB = B.GetRotation().GetNormalized();

			bool bPosesMatch = true;
			if (FMath::Abs(A.GetLocation().X - B.GetLocation().X) > TranslationPrecision + SMALL_NUMBER)
			{
				bPosesMatch = false;
			}
			if (FMath::Abs(A.GetLocation().Y - B.GetLocation().Y) > TranslationPrecision + SMALL_NUMBER)
			{
				bPosesMatch = false;
			}
			if (FMath::Abs(A.GetLocation().Z - B.GetLocation().Z) > TranslationPrecision + SMALL_NUMBER)
			{
				bPosesMatch = false;
			}
			if (FMath::Abs(RotA.GetAngle() - RotB.GetAngle()) > RotationPrecision + SMALL_NUMBER)
			{
				bPosesMatch = false;
			}
			if (FMath::Abs(RotA.GetAngle()) > SMALL_NUMBER || FMath::Abs(RotB.GetAngle()) > SMALL_NUMBER)
			{
				if (FMath::Abs(RotA.GetRotationAxis().X - RotB.GetRotationAxis().X) > RotationPrecision + SMALL_NUMBER)
				{
					bPosesMatch = false;
				}
				if (FMath::Abs(RotA.GetRotationAxis().Y - RotB.GetRotationAxis().Y) > RotationPrecision + SMALL_NUMBER)
				{
					bPosesMatch = false;
				}
				if (FMath::Abs(RotA.GetRotationAxis().Z - RotB.GetRotationAxis().Z) > RotationPrecision + SMALL_NUMBER)
				{
					bPosesMatch = false;
				}
			}
			if (FMath::Abs(A.GetScale3D().X - B.GetScale3D().X) > TranslationPrecision + SMALL_NUMBER)
			{
				bPosesMatch = false;
			}
			if (FMath::Abs(A.GetScale3D().Y - B.GetScale3D().Y) > TranslationPrecision + SMALL_NUMBER)
			{
				bPosesMatch = false;
			}
			if (FMath::Abs(A.GetScale3D().Z - B.GetScale3D().Z) > TranslationPrecision + SMALL_NUMBER)
			{
				bPosesMatch = false;
			}

			if (!bPosesMatch)
			{
				FString EventNameADisplayString = InContext->GetDisplayNameForEvent(EventNameA);
				FString EventNameBDisplayString = InContext->GetDisplayNameForEvent(EventNameB);
				FString Message = FString::Printf(TEXT("Poses don't match between %s and %s."), *EventNameADisplayString, *EventNameBDisplayString);
				InContext->Report(EMessageSeverity::Warning, Key, Message);

				if (InTLLRN_ControlRig->GetHierarchy()->IsSelected(Key))
				{
					if (FRigVMDrawInterface* DrawInterface = InContext->GetDrawInterface())
					{
						DrawInterface->DrawAxes(FTransform::Identity, A, 50.f);
						DrawInterface->DrawAxes(FTransform::Identity, B, 50.f);
					}
				}
			}
		}
	}
	else
	{
		InContext->Report(EMessageSeverity::Info, TEXT("Numerical validation only works when running 'Backwards and Forwards'"));
	}
}

