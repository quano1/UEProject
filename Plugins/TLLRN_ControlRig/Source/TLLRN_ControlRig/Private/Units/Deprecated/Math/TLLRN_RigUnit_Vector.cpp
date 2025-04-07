// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Deprecated/Math/TLLRN_RigUnit_Vector.h"
#include "RigVMFunctions/Math/RigVMFunction_MathVector.h"
#include "Math/TLLRN_ControlRigMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_Vector)

FTLLRN_RigUnit_Multiply_VectorVector_Execute()
{
	Result = FRigMathLibrary::Multiply(Argument0, Argument1);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_Multiply_VectorVector::GetUpgradeInfo() const
{
	FRigVMFunction_MathVectorMul NewNode;
	NewNode.A = Argument0;
	NewNode.B = Argument1;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Argument0"), TEXT("A"));
	Info.AddRemappedPin(TEXT("Argument1"), TEXT("B"));
	return Info;
}

FTLLRN_RigUnit_Add_VectorVector_Execute()
{
	Result = FRigMathLibrary::Add(Argument0, Argument1);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_Add_VectorVector::GetUpgradeInfo() const
{
	FRigVMFunction_MathVectorAdd NewNode;
	NewNode.A = Argument0;
	NewNode.B = Argument1;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Argument0"), TEXT("A"));
	Info.AddRemappedPin(TEXT("Argument1"), TEXT("B"));
	return Info;
}

FTLLRN_RigUnit_Subtract_VectorVector_Execute()
{
	Result = FRigMathLibrary::Subtract(Argument0, Argument1);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_Subtract_VectorVector::GetUpgradeInfo() const
{
	FRigVMFunction_MathVectorSub NewNode;
	NewNode.A = Argument0;
	NewNode.B = Argument1;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Argument0"), TEXT("A"));
	Info.AddRemappedPin(TEXT("Argument1"), TEXT("B"));
	return Info;
}

FTLLRN_RigUnit_Divide_VectorVector_Execute()
{
	Result = FRigMathLibrary::Divide(Argument0, Argument1);
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_Divide_VectorVector::GetUpgradeInfo() const
{
	FRigVMFunction_MathVectorDiv NewNode;
	NewNode.A = Argument0;
	NewNode.B = Argument1;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Argument0"), TEXT("A"));
	Info.AddRemappedPin(TEXT("Argument1"), TEXT("B"));
	return Info;
}

FTLLRN_RigUnit_Distance_VectorVector_Execute()
{
	Result = (Argument0 - Argument1).Size();
}

FRigVMStructUpgradeInfo FTLLRN_RigUnit_Distance_VectorVector::GetUpgradeInfo() const
{
	FRigVMFunction_MathVectorDistance NewNode;
	NewNode.A = Argument0;
	NewNode.B = Argument1;

	FRigVMStructUpgradeInfo Info(*this, NewNode);
	Info.AddRemappedPin(TEXT("Argument0"), TEXT("A"));
	Info.AddRemappedPin(TEXT("Argument1"), TEXT("B"));
	return Info;
}
