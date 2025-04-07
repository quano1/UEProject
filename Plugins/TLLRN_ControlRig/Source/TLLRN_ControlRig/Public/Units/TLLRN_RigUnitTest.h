// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/AutomationTest.h"
#include "TLLRN_RigUnit.h"
#include "TLLRN_RigUnitContext.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "RigVMCore/RigVMStructTest.h"

class FTLLRN_ControlTLLRN_RigUnitTestBase : public FRigVMStructTestBase<FTLLRN_ControlRigExecuteContext>
{
public:
	FTLLRN_ControlTLLRN_RigUnitTestBase(const FString& InName, bool bIsComplex)
		: FRigVMStructTestBase<FTLLRN_ControlRigExecuteContext>(InName, bIsComplex)
		, Hierarchy(nullptr)
		, Controller(nullptr)
	{
	}

	~FTLLRN_ControlTLLRN_RigUnitTestBase()
	{
		if (Hierarchy.IsValid())
		{
			// we no longer add/remove the controller to/from root since controller is now part of the hierarchy
			Hierarchy->RemoveFromRoot();
		}
	}

	virtual void Initialize() override
	{
		FRigVMStructTestBase<FTLLRN_ControlRigExecuteContext>::Initialize();
		
		if (!Hierarchy)
		{
			Hierarchy = NewObject<UTLLRN_RigHierarchy>();
			Controller = Hierarchy->GetController(true);

			Hierarchy->AddToRoot();
			// we no longer add the controller to root since controller is now part of the hierarchy
		}
		ExecuteContext.Hierarchy = Hierarchy.Get();
	}

	TSoftObjectPtr<UTLLRN_RigHierarchy> Hierarchy;
	TSoftObjectPtr<UTLLRN_RigHierarchyController> Controller;
};

#define TLLRN_CONTROLRIG_RIGUNIT_STRINGIFY(Content) #Content
#define IMPLEMENT_RIGUNIT_AUTOMATION_TEST(TUnitStruct) \
	class TUnitStruct##Test : public FTLLRN_ControlTLLRN_RigUnitTestBase \
	{ \
	public: \
		TUnitStruct##Test( const FString& InName ) \
		:FTLLRN_ControlTLLRN_RigUnitTestBase( InName, false ) {\
		} \
		virtual EAutomationTestFlags GetTestFlags() const override { return EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter; } \
		virtual bool IsStressTest() const { return false; } \
		virtual uint32 GetRequiredDeviceNum() const override { return 1; } \
		virtual FString GetTestSourceFileName() const override { return __FILE__; } \
		virtual int32 GetTestSourceFileLine() const override { return __LINE__; } \
	protected: \
		virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const override \
		{ \
			OutBeautifiedNames.Add(TEXT(TLLRN_CONTROLRIG_RIGUNIT_STRINGIFY(TLLRN_ControlRig.Units.TUnitStruct))); \
			OutTestCommands.Add(FString()); \
		} \
		TUnitStruct Unit; \
		virtual bool RunTest(const FString& Parameters) override \
		{ \
			Initialize(); \
			Hierarchy->Reset(); \
			Unit = TUnitStruct(); \
			return RunTLLRN_ControlTLLRN_RigUnitTest(Parameters); \
		} \
		virtual bool RunTLLRN_ControlTLLRN_RigUnitTest(const FString& Parameters); \
		virtual FString GetBeautifiedTestName() const override { return TEXT(TLLRN_CONTROLRIG_RIGUNIT_STRINGIFY(TLLRN_ControlRig.Units.TUnitStruct)); } \
		void InitAndExecute() \
		{ \
			Unit.Initialize(); \
			Unit.Execute(ExecuteContext); \
		} \
		void Execute() \
		{ \
			Unit.Execute(ExecuteContext); \
		} \
	}; \
	namespace\
	{\
		TUnitStruct##Test TUnitStruct##AutomationTestInstance(TEXT(TLLRN_CONTROLRIG_RIGUNIT_STRINGIFY(TUnitStruct##Test))); \
	} \
	bool TUnitStruct##Test::RunTLLRN_ControlTLLRN_RigUnitTest(const FString& Parameters)