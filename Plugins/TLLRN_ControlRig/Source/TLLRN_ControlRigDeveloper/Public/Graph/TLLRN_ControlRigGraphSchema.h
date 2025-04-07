// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_ControlRigGraphNode.h"
#include "GraphEditorDragDropAction.h"
#include "RigVMModel/RigVMGraph.h"
#include "EdGraphSchema_K2_Actions.h"
#include "EdGraph/RigVMEdGraphSchema.h"

#include "TLLRN_ControlRigGraphSchema.generated.h"

class UTLLRN_ControlRigBlueprint;
class UTLLRN_ControlRigGraph;
class UTLLRN_ControlRigGraphNode;
class UTLLRN_ControlRigGraphNode_Unit;
class UTLLRN_ControlRigGraphNode_Property;

UCLASS()
class TLLRN_CONTROLRIGDEVELOPER_API UTLLRN_ControlRigGraphSchema : public URigVMEdGraphSchema
{
	GENERATED_BODY()

public:
	/** Name constants */
	static inline const FLazyName GraphName_TLLRN_ControlRig = FLazyName(TEXT("Rig"));

public:
	UTLLRN_ControlRigGraphSchema();

	// UEdGraphSchema interface
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;

	// URigVMEdGraphSchema interface
	virtual const FLazyName& GetRootGraphName() const override { return GraphName_TLLRN_ControlRig; }
	virtual bool IsRigVMDefaultEvent(const FName& InEventName) const override;
};
