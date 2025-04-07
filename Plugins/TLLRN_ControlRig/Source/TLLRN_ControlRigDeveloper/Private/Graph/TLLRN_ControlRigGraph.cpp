// Copyright Epic Games, Inc. All Rights Reserved.

#include "Graph/TLLRN_ControlRigGraph.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlRig.h"
#include "RigVMModel/RigVMGraph.h"
#include "Units/TLLRN_RigUnit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigGraph)

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigGraph"

UTLLRN_ControlRigGraph::UTLLRN_ControlRigGraph()
{
	bSuspendModelNotifications = false;
	bIsTemporaryGraphForCopyPaste = false;
	LastHierarchyTopologyVersion = INDEX_NONE;
	bIsFunctionDefinition = false;
}

void UTLLRN_ControlRigGraph::InitializeFromBlueprint(URigVMBlueprint* InBlueprint)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	Super::InitializeFromBlueprint(InBlueprint);

	const UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = CastChecked<UTLLRN_ControlRigBlueprint>(InBlueprint);
	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBlueprint->Hierarchy;

	if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(TLLRN_ControlRigBlueprint->GetObjectBeingDebugged()))
	{
		Hierarchy = TLLRN_ControlRig->GetHierarchy();
	}

	if(Hierarchy)
	{
		CacheNameLists(Hierarchy, &TLLRN_ControlRigBlueprint->DrawContainer, TLLRN_ControlRigBlueprint->ShapeLibraries);
	}
}

#if WITH_EDITOR

TArray<TSharedPtr<FRigVMStringWithTag>> UTLLRN_ControlRigGraph::EmptyElementNameList;

const TArray<TSharedPtr<FRigVMStringWithTag>>* UTLLRN_ControlRigGraph::GetNameListForWidget(const FString& InWidgetName) const
{
	if (InWidgetName == TEXT("BoneName"))
	{
		return GetBoneNameList();
	}
	if (InWidgetName == TEXT("ControlName"))
	{
		return GetControlNameListWithoutAnimationChannels();
	}
	if (InWidgetName == TEXT("SpaceName"))
	{
		return GetNullNameList();
	}
	if (InWidgetName == TEXT("CurveName"))
	{
		return GetCurveNameList();
	}
	return nullptr;
}

void UTLLRN_ControlRigGraph::CacheNameLists(UTLLRN_RigHierarchy* InHierarchy, const FRigVMDrawContainer* DrawContainer, TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>> ShapeLibraries)
{
	if (UTLLRN_ControlRigGraph* OuterGraph = Cast<UTLLRN_ControlRigGraph>(GetOuter()))
	{
		return;
	}

	check(InHierarchy);
	check(DrawContainer);

	UTLLRN_ControlRig* TLLRN_ControlRig = InHierarchy->GetTypedOuter<UTLLRN_ControlRig>();

	if(LastHierarchyTopologyVersion != InHierarchy->GetTopologyVersion())
	{
		ElementNameLists.FindOrAdd(ETLLRN_RigElementType::All);
		ElementNameLists.FindOrAdd(ETLLRN_RigElementType::Bone);
		ElementNameLists.FindOrAdd(ETLLRN_RigElementType::Null);
		ElementNameLists.FindOrAdd(ETLLRN_RigElementType::Control);
		ElementNameLists.FindOrAdd(ETLLRN_RigElementType::Curve);
		ElementNameLists.FindOrAdd(ETLLRN_RigElementType::Physics);
		ElementNameLists.FindOrAdd(ETLLRN_RigElementType::Reference);
		ElementNameLists.FindOrAdd(ETLLRN_RigElementType::Connector);
		ElementNameLists.FindOrAdd(ETLLRN_RigElementType::Socket);

		TArray<TSharedPtr<FRigVMStringWithTag>>& AllNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::All);
		TArray<TSharedPtr<FRigVMStringWithTag>>& BoneNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::Bone);
		TArray<TSharedPtr<FRigVMStringWithTag>>& NullNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::Null);
		TArray<TSharedPtr<FRigVMStringWithTag>>& ControlNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::Control);
		TArray<TSharedPtr<FRigVMStringWithTag>>& CurveNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::Curve);
		TArray<TSharedPtr<FRigVMStringWithTag>>& PhysicsNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::Physics);
		TArray<TSharedPtr<FRigVMStringWithTag>>& ReferenceNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::Reference);
		TArray<TSharedPtr<FRigVMStringWithTag>>& ConnectorNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::Connector);
		TArray<TSharedPtr<FRigVMStringWithTag>>& SocketNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::Socket);
		
		CacheNameListForHierarchy<FTLLRN_RigBaseElement>(TLLRN_ControlRig, InHierarchy, AllNameList, false);
		CacheNameListForHierarchy<FTLLRN_RigBoneElement>(TLLRN_ControlRig, InHierarchy, BoneNameList, false);
		CacheNameListForHierarchy<FTLLRN_RigNullElement>(TLLRN_ControlRig, InHierarchy, NullNameList, false);
		CacheNameListForHierarchy<FTLLRN_RigControlElement>(TLLRN_ControlRig, InHierarchy, ControlNameList, false);
		CacheNameListForHierarchy<FTLLRN_RigControlElement>(TLLRN_ControlRig, InHierarchy, ControlNameListWithoutAnimationChannels, true);
		CacheNameListForHierarchy<FTLLRN_RigCurveElement>(TLLRN_ControlRig, InHierarchy, CurveNameList, false);
		CacheNameListForHierarchy<FTLLRN_RigPhysicsElement>(TLLRN_ControlRig, InHierarchy, PhysicsNameList, false);
		CacheNameListForHierarchy<FTLLRN_RigReferenceElement>(TLLRN_ControlRig, InHierarchy, ReferenceNameList, false);
		CacheNameListForHierarchy<FTLLRN_RigConnectorElement>(TLLRN_ControlRig, InHierarchy, ConnectorNameList, false);
		CacheNameListForHierarchy<FTLLRN_RigSocketElement>(TLLRN_ControlRig, InHierarchy, SocketNameList, false);

		LastHierarchyTopologyVersion = InHierarchy->GetTopologyVersion();
	}
	CacheNameList<FRigVMDrawContainer>(*DrawContainer, DrawingNameList);

	// always update the connector name list since the connector may have been re-resolved
	TArray<TSharedPtr<FRigVMStringWithTag>>& ConnectorNameList = ElementNameLists.FindChecked(ETLLRN_RigElementType::Connector);
	CacheNameListForHierarchy<FTLLRN_RigConnectorElement>(TLLRN_ControlRig, InHierarchy, ConnectorNameList, false);

	ShapeNameList.Reset();
	ShapeNameList.Add(MakeShared<FRigVMStringWithTag>(FName(NAME_None).ToString()));

	TMap<FString, FString> LibraryNameMap;
	if(TLLRN_ControlRig)
	{
		LibraryNameMap = TLLRN_ControlRig->ShapeLibraryNameMap;
	}

	for(const TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>& ShapeLibrary : ShapeLibraries)
	{
		if(ShapeLibrary.IsNull() || !ShapeLibrary.IsValid())
		{
			ShapeLibrary.LoadSynchronous();
		}

		if(ShapeLibrary.IsNull() || !ShapeLibrary.IsValid())
		{
			continue;
		}

		const bool bUseNameSpace = ShapeLibraries.Num() > 1;
		FString LibraryName = ShapeLibrary->GetName();
		if(const FString* RemappedName = LibraryNameMap.Find(LibraryName))
		{
			LibraryName = *RemappedName;
		}
		
		const FString NameSpace = bUseNameSpace ? LibraryName + TEXT(".") : FString();
		ShapeNameList.Add(MakeShared<FRigVMStringWithTag>(UTLLRN_ControlRigShapeLibrary::GetShapeName(ShapeLibrary.Get(), bUseNameSpace, LibraryNameMap, ShapeLibrary->DefaultShape)));
		for (const FTLLRN_ControlRigShapeDefinition& Shape : ShapeLibrary->Shapes)
		{
			ShapeNameList.Add(MakeShared<FRigVMStringWithTag>(UTLLRN_ControlRigShapeLibrary::GetShapeName(ShapeLibrary.Get(), bUseNameSpace, LibraryNameMap, Shape)));
		}
	}
}

const TArray<TSharedPtr<FRigVMStringWithTag>>* UTLLRN_ControlRigGraph::GetElementNameList(ETLLRN_RigElementType InElementType) const
{
	if (UTLLRN_ControlRigGraph* OuterGraph = Cast<UTLLRN_ControlRigGraph>(GetOuter()))
	{
		return OuterGraph->GetElementNameList(InElementType);
	}
	
	if(InElementType == ETLLRN_RigElementType::None)
	{
		return &EmptyElementNameList;
	}
	
	if(!ElementNameLists.Contains(InElementType))
	{
		const UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(GetBlueprint());
		if(Blueprint == nullptr)
		{
			return &EmptyElementNameList;
		}

		UTLLRN_ControlRigGraph* MutableThis = (UTLLRN_ControlRigGraph*)this;
		UTLLRN_RigHierarchy* Hierarchy = Blueprint->Hierarchy;
		if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(Blueprint->GetObjectBeingDebugged()))
		{
			Hierarchy = TLLRN_ControlRig->GetHierarchy();
		}	
			
		MutableThis->CacheNameLists(Hierarchy, &Blueprint->DrawContainer, Blueprint->ShapeLibraries);
	}
	return &ElementNameLists.FindChecked(InElementType);
}

const TArray<TSharedPtr<FRigVMStringWithTag>>* UTLLRN_ControlRigGraph::GetElementNameList(URigVMPin* InPin) const
{
	if (InPin)
	{
		if (URigVMPin* ParentPin = InPin->GetParentPin())
		{
			if (ParentPin->GetCPPTypeObject() == FTLLRN_RigElementKey::StaticStruct())
			{
				if (URigVMPin* TypePin = ParentPin->FindSubPin(TEXT("Type")))
				{
					FString DefaultValue = TypePin->GetDefaultValue();
					if (!DefaultValue.IsEmpty())
					{
						ETLLRN_RigElementType Type = (ETLLRN_RigElementType)StaticEnum<ETLLRN_RigElementType>()->GetValueByNameString(DefaultValue);
						return GetElementNameList(Type);
					}
				}
			}
		}
	}

	return GetBoneNameList(nullptr);
}

const TArray<TSharedPtr<FRigVMStringWithTag>> UTLLRN_ControlRigGraph::GetSelectedElementsNameList() const
{
	TArray<TSharedPtr<FRigVMStringWithTag>> Result;
	if (UTLLRN_ControlRigGraph* OuterGraph = Cast<UTLLRN_ControlRigGraph>(GetOuter()))
	{
		return OuterGraph->GetSelectedElementsNameList();
	}
	
	const UTLLRN_ControlRigBlueprint* Blueprint = CastChecked<UTLLRN_ControlRigBlueprint>(GetBlueprint());
	if(Blueprint == nullptr)
	{
		return Result;
	}
	
	TArray<FTLLRN_RigElementKey> Keys = Blueprint->Hierarchy->GetSelectedKeys();
	for (const FTLLRN_RigElementKey& Key : Keys)
	{
		FString ValueStr;
		FTLLRN_RigElementKey::StaticStruct()->ExportText(ValueStr, &Key, nullptr, nullptr, PPF_None, nullptr);
		Result.Add(MakeShared<FRigVMStringWithTag>(ValueStr));
	}
	
	return Result;
}

const TArray<TSharedPtr<FRigVMStringWithTag>>* UTLLRN_ControlRigGraph::GetDrawingNameList(URigVMPin* InPin) const
{
	if (UTLLRN_ControlRigGraph* OuterGraph = Cast<UTLLRN_ControlRigGraph>(GetOuter()))
	{
		return OuterGraph->GetDrawingNameList(InPin);
	}
	return &DrawingNameList;
}

const TArray<TSharedPtr<FRigVMStringWithTag>>* UTLLRN_ControlRigGraph::GetShapeNameList(URigVMPin* InPin) const
{
	if (UTLLRN_ControlRigGraph* OuterGraph = Cast<UTLLRN_ControlRigGraph>(GetOuter()))
	{
		return OuterGraph->GetShapeNameList(InPin);
	}
	return &ShapeNameList;
}

FReply UTLLRN_ControlRigGraph::HandleGetSelectedClicked(URigVMEdGraph* InEdGraph, URigVMPin* InPin, FString InDefaultValue)
{
	UTLLRN_ControlRigBlueprint* RigVMBlueprint = CastChecked<UTLLRN_ControlRigBlueprint>(InEdGraph->GetBlueprint());
	if(InPin->GetCustomWidgetName() == TEXT("ElementName"))
	{
		if (URigVMPin* ParentPin = InPin->GetParentPin())
		{
			InEdGraph->GetController()->SetPinDefaultValue(ParentPin->GetPinPath(), InDefaultValue, true, true, false, true);
			return FReply::Handled();
		}
	}

	else if (InPin->GetCustomWidgetName() == TEXT("BoneName"))
	{
		UTLLRN_RigHierarchy* Hierarchy = RigVMBlueprint->Hierarchy;
		TArray<FTLLRN_RigElementKey> Keys = Hierarchy->GetSelectedKeys();
		FTLLRN_RigBaseElement* Element = Hierarchy->FindChecked(Keys[0]);
		if (Element->GetType() == ETLLRN_RigElementType::Bone)
		{
			InEdGraph->GetController()->SetPinDefaultValue(InPin->GetPinPath(), Keys[0].Name.ToString(), true, true, false, true);
			return FReply::Handled();
		}
	}

	// if we don't have a key pin - this is just a plain name.
	// let's derive the type of element this node deals with from its name.
	// there's nothing better in place for now.
	else if(const URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(InPin->GetNode()))
	{
		const int32 LastIndex = StaticEnum<ETLLRN_RigElementType>()->GetIndexByName(TEXT("Last")); 
		const FString UnitName = UnitNode->GetScriptStruct()->GetStructCPPName();
		for(int32 EnumIndex = 0; EnumIndex < LastIndex; EnumIndex++)
		{
			const FString EnumDisplayName = StaticEnum<ETLLRN_RigElementType>()->GetDisplayNameTextByIndex(EnumIndex).ToString();
			if(UnitName.Contains(EnumDisplayName))
			{
				const ETLLRN_RigElementType ElementType = (ETLLRN_RigElementType)StaticEnum<ETLLRN_RigElementType>()->GetValueByIndex(EnumIndex);

				FTLLRN_RigElementKey Key;
				FTLLRN_RigElementKey::StaticStruct()->ImportText(*InDefaultValue, &Key, nullptr, EPropertyPortFlags::PPF_None, nullptr, FTLLRN_RigElementKey::StaticStruct()->GetName(), true);
				if (Key.IsValid())
				{
					if(Key.Type == ElementType)
					{
						InEdGraph->GetController()->SetPinDefaultValue(InPin->GetPinPath(), Key.Name.ToString(), true, true, false, true);
						return FReply::Handled();
					}
				}
				break;
			}
		}
	}
	return FReply::Unhandled();
}

FReply UTLLRN_ControlRigGraph::HandleBrowseClicked(URigVMEdGraph* InEdGraph, URigVMPin* InPin, FString InDefaultValue)
{
	UTLLRN_ControlRigBlueprint* RigVMBlueprint = CastChecked<UTLLRN_ControlRigBlueprint>(InEdGraph->GetBlueprint());
	URigVMPin* KeyPin = InPin->GetParentPin();
	if(KeyPin && KeyPin->GetCPPTypeObject() == FTLLRN_RigElementKey::StaticStruct())
	{
		// browse to rig element key
		FString DefaultValue = InPin->GetParentPin()->GetDefaultValue();
		if (!DefaultValue.IsEmpty())
		{
			FTLLRN_RigElementKey Key;
			FTLLRN_RigElementKey::StaticStruct()->ImportText(*DefaultValue, &Key, nullptr, EPropertyPortFlags::PPF_None, nullptr, FTLLRN_RigElementKey::StaticStruct()->GetName(), true);
			if (Key.IsValid())
			{
				RigVMBlueprint->GetHierarchyController()->SetSelection({Key});
			}
		}
	}
	else if (InPin->GetCustomWidgetName() == TEXT("BoneName"))
	{
		// browse to named bone
		const FString DefaultValue = InPin->GetDefaultValue();
		FTLLRN_RigElementKey Key(*DefaultValue, ETLLRN_RigElementType::Bone);
		RigVMBlueprint->GetHierarchyController()->SetSelection({Key});
	}
	else
	{
		// if we don't have a key pin - this is just a plain name.
		// let's derive the type of element this node deals with from its name.
		// there's nothing better in place for now.
		if(const URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(InPin->GetNode()))
		{
			const int32 LastIndex = StaticEnum<ETLLRN_RigElementType>()->GetIndexByName(TEXT("Last")); 
			const FString UnitName = UnitNode->GetScriptStruct()->GetStructCPPName();
			for(int32 EnumIndex = 0; EnumIndex < LastIndex; EnumIndex++)
			{
				const FString EnumDisplayName = StaticEnum<ETLLRN_RigElementType>()->GetDisplayNameTextByIndex(EnumIndex).ToString();
				if(UnitName.Contains(EnumDisplayName))
				{
					const FString DefaultValue = InPin->GetDefaultValue();
					const ETLLRN_RigElementType ElementType = (ETLLRN_RigElementType)StaticEnum<ETLLRN_RigElementType>()->GetValueByIndex(EnumIndex);
					FTLLRN_RigElementKey Key(*DefaultValue, ElementType);
					RigVMBlueprint->GetHierarchyController()->SetSelection({Key});
					break;
				}
			}
		}
	}
	return FReply::Handled();
}

#endif

bool UTLLRN_ControlRigGraph::HandleModifiedEvent_Internal(ERigVMGraphNotifType InNotifType, URigVMGraph* InGraph, UObject* InSubject)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_FUNC()

	if(!Super::HandleModifiedEvent_Internal(InNotifType, InGraph, InSubject))
	{
		return false;
	}

	switch (InNotifType)
	{
		case ERigVMGraphNotifType::PinDefaultValueChanged:
		{
			if (URigVMPin* ModelPin = Cast<URigVMPin>(InSubject))
			{
				if (UTLLRN_ControlRigGraphNode* RigNode = Cast<UTLLRN_ControlRigGraphNode>(FindNodeForModelNodeName(ModelPin->GetNode()->GetFName())))
				{
					if (Cast<URigVMUnitNode>(ModelPin->GetNode()))
					{
						// if the node contains a rig element key - invalidate the node
						if(RigNode->GetAllPins().ContainsByPredicate([](UEdGraphPin* Pin) -> bool
						{
							return Pin->PinType.PinSubCategoryObject == FTLLRN_RigElementKey::StaticStruct();
						}))
						{
							// we do this to enforce the refresh of the element name widgets
							RigNode->SynchronizeGraphPinValueWithModelPin(ModelPin);
						}
					}
				}
			}
			break;
		}
		default:
		{
			break;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

