// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Deprecated/Math/TLLRN_RigUnit_Converter.h"
#include "RigVMFunctions/Math/RigVMFunction_MathTransform.h"
#include "RigVMFunctions/Math/RigVMFunction_MathQuaternion.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_Converter)

FTLLRN_RigUnit_ConvertTransform_Execute()
{
	Result.FromFTransform(Input);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ConvertTransform::GetUpgradeInfo() const
{
	FRigVMFunction_MathTransformToEulerTransform NewNode;
	NewNode.Value = Input;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Input"), TEXT("Value"));
	return Info;
}

FTLLRN_RigUnit_ConvertEulerTransform_Execute()
{
	Result = Input.ToFTransform();
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ConvertEulerTransform::GetUpgradeInfo() const
{
	FRigVMFunction_MathTransformFromEulerTransform NewNode;
	NewNode.EulerTransform = Input;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Input"), TEXT("EulerTransform"));
	return Info;
}

FTLLRN_RigUnit_ConvertRotation_Execute()
{
	Result = Input.Quaternion();
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ConvertRotation::GetUpgradeInfo() const
{
	FRigVMFunction_MathQuaternionFromRotator NewNode;
	NewNode.Rotator = Input;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Input"), TEXT("Rotator"));
	return Info;
}

FTLLRN_RigUnit_ConvertQuaternion_Execute()
{
	Result = Input.Rotator();
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ConvertQuaternion::GetUpgradeInfo() const
{
	FRigVMFunction_MathQuaternionToRotator NewNode;
	NewNode.Value = Input;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Input"), TEXT("Value"));
	return Info;
}

FTLLRN_RigUnit_ConvertVectorToRotation_Execute()
{
	Result = Input.Rotation();
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ConvertVectorToRotation::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_ConvertVectorToQuaternion_Execute()
{
	Result = Input.Rotation().Quaternion();
	Result.Normalize();
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ConvertVectorToQuaternion::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_ConvertRotationToVector_Execute()
{
	Result = Input.RotateVector(FVector(1.f, 0.f, 0.f));
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ConvertRotationToVector::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_ConvertQuaternionToVector_Execute()
{
	Result = Input.RotateVector(FVector(1.f, 0.f, 0.f));
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ConvertQuaternionToVector::GetUpgradeInfo() const
{
	// this node is no longer supported
	return FRigVMStructUpgradeInfo();
}

FTLLRN_RigUnit_ToSwingAndTwist_Execute()
{
	if (!TwistAxis.IsZero())
	{
		FVector NormalizedAxis = TwistAxis.GetSafeNormal();
		Input.ToSwingTwist(TwistAxis, Swing, Twist);
	}
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_ToSwingAndTwist::GetUpgradeInfo() const
{
	FRigVMFunction_MathQuaternionSwingTwist NewNode;
	NewNode.Input = Input;
	NewNode.TwistAxis = TwistAxis;

	return FRigVMStructUpgradeInfo(*this, NewNode);
}

