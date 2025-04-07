// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_ControlRigDefines.h"
#include "RigVMCore/RigVMStruct.h"
#include "RigVMCore/RigVMRegistry.h"
#include "TLLRN_RigUnitContext.h"
#include "RigVMFunctions/RigVMFunctionDefines.h"

#if WITH_EDITOR
#include "RigVMModel/Nodes/RigVMUnitNode.h"
#endif

#include "TLLRN_RigUnit.generated.h"

struct FTLLRN_RigUnitContext;
struct FRigDirectManipulationInfo;

#if WITH_EDITOR

struct TLLRN_CONTROLRIG_API FRigDirectManipulationTarget
{
	FRigDirectManipulationTarget()
		: Name()
		, ControlType(ETLLRN_RigControlType::EulerTransform)
	{
	}

	FRigDirectManipulationTarget(const FString& InName, ETLLRN_RigControlType InControlType)
		: Name(InName)
		, ControlType(InControlType)
	{
	}

	bool operator == (const FRigDirectManipulationTarget& InOther) const
	{
		return Name.Equals(InOther.Name, ESearchCase::CaseSensitive);
	}

	bool operator > (const FRigDirectManipulationTarget& InOther) const
	{
		return Name > InOther.Name;
	}

	bool operator < (const FRigDirectManipulationTarget& InOther) const
	{
		return Name < InOther.Name;
	}

	FString Name;
	ETLLRN_RigControlType ControlType;
};

#endif

/** Base class for all rig units */
USTRUCT(BlueprintType, meta=(Abstract, NodeColor = "0.1 0.1 0.1", ExecuteContext="FTLLRN_ControlRigExecuteContext"))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnit : public FRigVMStruct
{
	GENERATED_BODY()

public:

	FTLLRN_RigUnit()
	{}

	/** Virtual destructor */
	virtual ~FTLLRN_RigUnit() {}

	virtual FTLLRN_RigElementKey DetermineSpaceForPin(const FString& InPinPath, void* InUserContext) const { return FTLLRN_RigElementKey(); }
	
	virtual FTransform DetermineOffsetTransformForPin(const FString& InPinPath, void* InUserContext) const { return FTransform::Identity; }
	
	/** The name of the method used within each rig unit */
	static FName GetMethodName()
	{
		static const FLazyName MethodName = FRigVMStruct::ExecuteName;
		return MethodName;
	}

#if WITH_EDITOR
	/** Returns the targets for viewport posing */
	virtual bool GetDirectManipulationTargets(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, UTLLRN_RigHierarchy* InHierarchy, TArray<FRigDirectManipulationTarget>& InOutTargets, FString* OutFailureReason) const;

	/** Optionally configures a control's settings and value for a given target */
	virtual void ConfigureDirectManipulationControl(const URigVMUnitNode* InNode, TSharedPtr<FRigDirectManipulationInfo> InInfo, FTLLRN_RigControlSettings& InOutSettings, FTLLRN_RigControlValue& InOutValue) const;

	/** Sets a control's pose to represent this viewport pose target */ 
	virtual bool UpdateHierarchyForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo);

	/** Sets the values on this node based on a viewport pose */
	virtual bool UpdateDirectManipulationFromHierarchy(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo);

	/** returns a list of pins affected by the viewport pose */
	virtual TArray<const URigVMPin*> GetPinsForDirectManipulation(const URigVMUnitNode* InNode, const FRigDirectManipulationTarget& InTarget) const;

	/** Allows the node to draw debug drawing during a manipulation */
	virtual void PerformDebugDrawingForDirectManipulation(const URigVMUnitNode* InNode, TSharedPtr<FStructOnScope> InInstance, FTLLRN_ControlRigExecuteContext& InContext, TSharedPtr<FRigDirectManipulationInfo> InInfo) const;

private:
	
	static bool AddDirectManipulationTarget_Internal(TArray<FRigDirectManipulationTarget>& InOutTargets, const URigVMPin* InPin, const UScriptStruct* InScriptStruct);
	static TTuple<const FStructProperty*, uint8*> FindStructPropertyAndTargetMemory(TSharedPtr<FStructOnScope> InInstance, const UScriptStruct* InStruct, const FString& InPinPath); 
#endif
};

/** Base class for all rig units that can change data */
USTRUCT(BlueprintType, meta = (Abstract))
struct TLLRN_CONTROLRIG_API FTLLRN_RigUnitMutable : public FTLLRN_RigUnit
{
	GENERATED_BODY()

	FTLLRN_RigUnitMutable()
	: FTLLRN_RigUnit()
	{}

	/*
	 * This property is used to chain multiple mutable units together
	 */
	UPROPERTY(DisplayName = "Execute", Transient, meta = (Input, Output))
	FTLLRN_ControlRigExecuteContext ExecuteContext;
};

#if WITH_EDITOR

struct TLLRN_CONTROLRIG_API FRigDirectManipulationInfo
{
	FRigDirectManipulationInfo()
		: bInitialized(false)
		, Target()
		, ControlKey(NAME_None, ETLLRN_RigElementType::Control)
		, OffsetTransform(FTransform::Identity)
	{
		Reset();
	}

	void Reset()
	{
		bInitialized = false;
		OffsetTransform = FTransform::Identity;
		Transforms.Reset();
		Transforms.Add(FTransform::Identity);
	}

	bool bInitialized;
	FRigDirectManipulationTarget Target;
	FTLLRN_RigElementKey ControlKey;
	FTransform OffsetTransform;
	TArray<FTransform> Transforms;
	TWeakObjectPtr<const URigVMUnitNode> Node;
};

#endif