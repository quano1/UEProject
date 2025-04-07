// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TLLRN_ControlRigBlueprint.h"
#include "EdGraph/EdGraph.h"
#include "Graph/TLLRN_ControlRigGraphNode.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "RigVMModel/RigVMGraph.h"
#include "RigVMCore/RigVM.h"
#include "EdGraph/RigVMEdGraph.h"
#include "TLLRN_ControlRigGraph.generated.h"

class UTLLRN_ControlRigBlueprint;
class UTLLRN_ControlRigGraphSchema;
class UTLLRN_ControlRig;
class URigVMController;
struct FTLLRN_TLLRN_RigCurveContainer;

UCLASS()
class TLLRN_CONTROLRIGDEVELOPER_API UTLLRN_ControlRigGraph : public URigVMEdGraph
{
	GENERATED_BODY()

public:
	UTLLRN_ControlRigGraph();

	/** Set up this graph */
	virtual void InitializeFromBlueprint(URigVMBlueprint* InBlueprint) override;

	virtual bool HandleModifiedEvent_Internal(ERigVMGraphNotifType InNotifType, URigVMGraph* InGraph, UObject* InSubject) override;

#if WITH_EDITOR

	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetBoneNameList(URigVMPin* InPin = nullptr) const
	{
		return GetElementNameList(ETLLRN_RigElementType::Bone);
	}
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetControlNameList(URigVMPin* InPin = nullptr) const
	{
		return GetElementNameList(ETLLRN_RigElementType::Control);
	}
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetControlNameListWithoutAnimationChannels(URigVMPin* InPin = nullptr) const
	{
		return &ControlNameListWithoutAnimationChannels;
	}
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetNullNameList(URigVMPin* InPin = nullptr) const
	{
		return GetElementNameList(ETLLRN_RigElementType::Null);
	}
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetCurveNameList(URigVMPin* InPin = nullptr) const
	{
		return GetElementNameList(ETLLRN_RigElementType::Curve);
	}
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetConnectorNameList(URigVMPin* InPin = nullptr) const
	{
		return GetElementNameList(ETLLRN_RigElementType::Connector);
	}
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetSocketNameList(URigVMPin* InPin = nullptr) const
	{
		return GetElementNameList(ETLLRN_RigElementType::Socket);
	}

	virtual const TArray<TSharedPtr<FRigVMStringWithTag>>* GetNameListForWidget(const FString& InWidgetName) const override;

	void CacheNameLists(UTLLRN_RigHierarchy* InHierarchy, const FRigVMDrawContainer* DrawContainer, TArray<TSoftObjectPtr<UTLLRN_ControlRigShapeLibrary>> ShapeLibraries);
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetElementNameList(ETLLRN_RigElementType InElementType = ETLLRN_RigElementType::Bone) const;
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetElementNameList(URigVMPin* InPin = nullptr) const;
	const TArray<TSharedPtr<FRigVMStringWithTag>> GetSelectedElementsNameList() const;
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetDrawingNameList(URigVMPin* InPin = nullptr) const;
	const TArray<TSharedPtr<FRigVMStringWithTag>>* GetShapeNameList(URigVMPin* InPin = nullptr) const;

	FReply HandleGetSelectedClicked(URigVMEdGraph* InEdGraph, URigVMPin* InPin, FString InDefaultValue);
	FReply HandleBrowseClicked(URigVMEdGraph* InEdGraph, URigVMPin* InPin, FString InDefaultValue);

private:

	template<class T>
	static bool IncludeElementInNameList(const T* InElement)
	{
		return true;
	}

	template<class T>
	void CacheNameListForHierarchy(UTLLRN_ControlRig* InTLLRN_ControlRig, UTLLRN_RigHierarchy* InHierarchy, TArray<TSharedPtr<FRigVMStringWithTag>>& OutNameList, bool bFilter = true)
	{
        TArray<FRigVMStringWithTag> Names;
		for (auto Element : *InHierarchy)
		{
			if(Element->IsA<T>())
			{
				if(!bFilter || IncludeElementInNameList<T>(Cast<T>(Element)))
				{
					static const FLinearColor Color(FLinearColor(0.0, 112.f/255.f, 224.f/255.f));
					FRigVMStringTag Tag;
					
					if(InTLLRN_ControlRig)
					{
						if(const FTLLRN_RigElementKey* SourceKey = InTLLRN_ControlRig->GetElementKeyRedirector().FindReverse(Element->GetKey()))
						{
							Tag = FRigVMStringTag(Element->GetKey().Name, Color);
							Names.Emplace(SourceKey->Name.ToString(), Tag);
							continue;
						}

						// look up the resolved name
						if(Element->GetType() == ETLLRN_RigElementType::Connector)
						{
							if(const FTLLRN_CachedTLLRN_RigElement* Cache = InTLLRN_ControlRig->GetElementKeyRedirector().Find(Element->GetKey()))
							{
								Tag = FRigVMStringTag(Cache->GetKey().Name, Color);
							}
						}
					}

					Names.Emplace(Element->GetName(), Tag);
				}
			}
		}
		Names.Sort();

		OutNameList.Reset();
		OutNameList.Add(MakeShared<FRigVMStringWithTag>(FName(NAME_None).ToString()));
		for (const FRigVMStringWithTag& Name : Names)
		{
			OutNameList.Add(MakeShared<FRigVMStringWithTag>(Name));
		}
	}

	template<class T>
	void CacheNameList(const T& ElementList, TArray<TSharedPtr<FRigVMStringWithTag>>& OutNameList)
	{
		TArray<FString> Names;
		for (auto Element : ElementList)
		{
			Names.Add(Element.Name.ToString());
		}
		Names.Sort();

		OutNameList.Reset();
		OutNameList.Add(MakeShared<FRigVMStringWithTag>(FName(NAME_None).ToString()));
		for (const FString& Name : Names)
		{
			OutNameList.Add(MakeShared<FRigVMStringWithTag>(Name));
		}
	}

	TMap<ETLLRN_RigElementType, TArray<TSharedPtr<FRigVMStringWithTag>>> ElementNameLists;
	TArray<TSharedPtr<FRigVMStringWithTag>>	ControlNameListWithoutAnimationChannels;
	TArray<TSharedPtr<FRigVMStringWithTag>> DrawingNameList;
	TArray<TSharedPtr<FRigVMStringWithTag>> ShapeNameList;
	int32 LastHierarchyTopologyVersion;

	static TArray<TSharedPtr<FRigVMStringWithTag>> EmptyElementNameList;

#endif

	friend class UTLLRN_ControlRigGraphNode;
	friend class FTLLRN_ControlRigEditor;
	friend class SRigVMGraphNode;
	friend class UTLLRN_ControlRigBlueprint;
};

template<>
inline bool UTLLRN_ControlRigGraph::IncludeElementInNameList<FTLLRN_RigControlElement>(const FTLLRN_RigControlElement* InElement)
{
	return !InElement->IsAnimationChannel();
}
