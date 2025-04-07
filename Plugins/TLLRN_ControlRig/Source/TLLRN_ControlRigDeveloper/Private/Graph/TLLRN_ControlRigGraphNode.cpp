// Copyright Epic Games, Inc. All Rights Reserved.

#include "Graph/TLLRN_ControlRigGraphNode.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "ITLLRN_ControlRigEditorModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigGraphNode)

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigGraphNode"

UTLLRN_ControlRigGraphNode::UTLLRN_ControlRigGraphNode()
: URigVMEdGraphNode()
{
}

#if WITH_EDITOR

void UTLLRN_ControlRigGraphNode::AddPinSearchMetaDataInfo(const UEdGraphPin* Pin, TArray<FSearchTagDataPair>& OutTaggedMetaData) const
{
	Super::AddPinSearchMetaDataInfo(Pin, OutTaggedMetaData);

	if(const URigVMPin* ModelPin = FindModelPinFromGraphPin(Pin))
	{
		if(ModelPin->GetCPPTypeObject() == FTLLRN_RigElementKey::StaticStruct())
		{
			const FString DefaultValue = ModelPin->GetDefaultValue();
			if(!DefaultValue.IsEmpty())
			{
				FString TLLRN_RigElementKeys;
				if(ModelPin->IsArray())
				{
					TLLRN_RigElementKeys = DefaultValue;
				}
				else
				{
					TLLRN_RigElementKeys = FString::Printf(TEXT("(%s)"), *DefaultValue);
				}
				if(!TLLRN_RigElementKeys.IsEmpty())
				{
					TLLRN_RigElementKeys.ReplaceInline(TEXT("="), TEXT(","));
					TLLRN_RigElementKeys.ReplaceInline(TEXT("\""), TEXT(""));
					OutTaggedMetaData.Emplace(FText::FromString(TEXT("Rig Items")), FText::FromString(TLLRN_RigElementKeys));
				}
			}
		}
	}
}

#endif

#undef LOCTEXT_NAMESPACE

