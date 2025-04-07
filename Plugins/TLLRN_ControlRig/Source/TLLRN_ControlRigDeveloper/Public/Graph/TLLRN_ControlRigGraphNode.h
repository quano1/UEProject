// Copyright Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "K2Node.h"
#include "UObject/SoftObjectPath.h"
#include "EdGraphSchema_K2.h"
#include "RigVMModel/RigVMGraph.h"
#include "EdGraph/RigVMEdGraphNode.h"
#include "TLLRN_ControlRigGraphNode.generated.h"

class FBlueprintActionDatabaseRegistrar;
class UEdGraph;
struct FSlateIcon;
class UTLLRN_ControlRigBlueprint;

/** Base class for animation TLLRN_ControlRig-related nodes */
UCLASS()
class TLLRN_CONTROLRIGDEVELOPER_API UTLLRN_ControlRigGraphNode : public URigVMEdGraphNode
{
	GENERATED_BODY()

	friend class FTLLRN_ControlRigGraphNodeDetailsCustomization;
	friend class FTLLRN_ControlRigBlueprintCompilerContext;
	friend class UTLLRN_ControlRigGraph;
	friend class UTLLRN_ControlRigGraphSchema;
	friend class UTLLRN_ControlRigBlueprint;
	friend class FTLLRN_ControlRigGraphTraverser;
	friend class FTLLRN_ControlRigGraphPanelPinFactory;
	friend class FTLLRN_ControlRigEditor;
	friend class STLLRN_ControlRigGraphPinCurveFloat;

public:

	UTLLRN_ControlRigGraphNode();

#if WITH_EDITOR
	virtual void AddPinSearchMetaDataInfo(const UEdGraphPin* Pin, TArray<FSearchTagDataPair>& OutTaggedMetaData) const override;
#endif

private:

	friend class SRigVMGraphNode;
	friend class FTLLRN_ControlRigArgumentLayout;
	friend class FTLLRN_ControlRigGraphDetails;
	friend class UTLLRN_ControlRigTemplateNodeSpawner;
	friend class UTLLRN_ControlRigArrayNodeSpawner;
	friend class UTLLRN_ControlRigIfNodeSpawner;
	friend class UTLLRN_ControlRigSelectNodeSpawner;
};
