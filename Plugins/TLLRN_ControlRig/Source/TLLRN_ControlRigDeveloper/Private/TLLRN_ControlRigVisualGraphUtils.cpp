// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigVisualGraphUtils.h"

#include "TLLRN_ControlRig.h"
#include "VisualGraphUtilsModule.h"

#if WITH_EDITOR

#include "HAL/PlatformApplicationMisc.h"
#include "RigVMModel/RigVMNode.h"
#include "Units/Execution/TLLRN_RigUnit_BeginExecution.h"
#include "Units/Execution/TLLRN_RigUnit_PrepareForExecution.h"
#include "Units/Execution/TLLRN_RigUnit_InverseExecution.h"

FAutoConsoleCommandWithWorldAndArgs FCmdTLLRN_ControlRigVisualGraphUtilsDumpHierarchy
(
	TEXT("VisualGraphUtils.TLLRN_ControlRig.TraverseHierarchy"),
	TEXT("Traverses the hierarchy for a given control rig"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InParams, UWorld* InWorld)
	{
		const FName BeginExecuteEventName = FTLLRN_RigUnit_BeginExecution::EventName;
		const FName PrepareForExecuteEventName = FTLLRN_RigUnit_PrepareForExecution::EventName;
		const FName InverseExecuteEventName = FTLLRN_RigUnit_InverseExecution::EventName;

		if(InParams.Num() == 0)
		{
			UE_LOG(LogVisualGraphUtils, Error, TEXT("Unsufficient parameters. Command usage:"));
			UE_LOG(LogVisualGraphUtils, Error, TEXT("Example: VisualGraphUtilsTLLRN_ControlRig.TraverseHierarchy /Game/Animation/TLLRN_ControlRig/BasicControls_CtrlRig event=update"));
			UE_LOG(LogVisualGraphUtils, Error, TEXT("Provide one path name to an instance of a control rig."));
			UE_LOG(LogVisualGraphUtils, Error, TEXT("Optionally provide the event name (%s, %s or %s)."), *BeginExecuteEventName.ToString(), *PrepareForExecuteEventName.ToString(), *InverseExecuteEventName.ToString());
			return;
		}

		TArray<FString> ObjectPathNames;
		FName EventName = BeginExecuteEventName;
		for(const FString& InParam : InParams)
		{
			static const FString ObjectPathToken = TEXT("path"); 
			static const FString EventPathToken = TEXT("event"); 
			FString Token = ObjectPathToken;
			FString Content = InParam;

			if(InParam.Contains(TEXT("=")))
			{
				const int32 Pos = InParam.Find(TEXT("="));
				Token = InParam.Left(Pos);
				Content = InParam.Mid(Pos+1);
				Token.TrimStartAndEndInline();
				Token.ToLowerInline();
				Content.TrimStartAndEndInline();
			}

			if(Token == ObjectPathToken)
			{
				ObjectPathNames.Add(Content);
			}
			else if(Token == EventPathToken)
			{
				EventName = *Content;
			}
		}

		TArray<UTLLRN_RigHierarchy*> Hierarchies;
		for(const FString& ObjectPathName : ObjectPathNames)
		{
			if(UObject* Object = FindObject<UObject>(nullptr, *ObjectPathName, false))
			{
				if(UTLLRN_ControlRig* CR = Cast<UTLLRN_ControlRig>(Object))
				{
					Hierarchies.Add(CR->GetHierarchy());
				}
				else if(UTLLRN_RigHierarchy* Hierarchy = Cast<UTLLRN_RigHierarchy>(Object))
				{
					Hierarchies.Add(Hierarchy);
				}
				else
				{
					UE_LOG(LogVisualGraphUtils, Error, TEXT("Object is not a hierarchy / nor a Control Rig / or short name was provided: '%s'"), *ObjectPathName);
					return;
				}
			}
			else
			{
				UE_LOG(LogVisualGraphUtils, Error, TEXT("Object with pathname '%s' not found."), *ObjectPathName);
				return;
			}
		}

		if(Hierarchies.Num() == 0)
		{
			UE_LOG(LogVisualGraphUtils, Error, TEXT("No hierarchy found."));
			return;
		}

		const FString DotGraphContent = FTLLRN_ControlRigVisualGraphUtils::DumpTLLRN_RigHierarchyToDotGraph(Hierarchies[0], EventName);
		FPlatformApplicationMisc::ClipboardCopy(*DotGraphContent);
		UE_LOG(LogVisualGraphUtils, Display, TEXT("The content has been copied to the clipboard."));
	})
);

#endif

FString FTLLRN_ControlRigVisualGraphUtils::DumpTLLRN_RigHierarchyToDotGraph(UTLLRN_RigHierarchy* InHierarchy, const FName& InEventName)
{
	check(InHierarchy);

	FVisualGraph Graph(TEXT("Rig"));

	struct Local
	{
		static FName GetNodeNameForElement(const FTLLRN_RigBaseElement* InElement)
		{
			check(InElement);
			return GetNodeNameForElementIndex(InElement->GetIndex());
		}

		static FName GetNodeNameForElementIndex(int32 InElementIndex)
		{
			return *FString::Printf(TEXT("Element_%d"), InElementIndex);
		}

		static TArray<int32> VisitParents(const FTLLRN_RigBaseElement* InElement, UTLLRN_RigHierarchy* InHierarchy, FVisualGraph& OutGraph)
		{
			TArray<int32> Result;
			FTLLRN_RigBaseElementParentArray Parents = InHierarchy->GetParents(InElement);

			for(int32 ParentIndex = 0; ParentIndex < Parents.Num(); ParentIndex++)
			{
				const int32 ParentNodeIndex = VisitElement(Parents[ParentIndex], InHierarchy, OutGraph);
				Result.Add(ParentNodeIndex);
			}

			return Result;
		}

		static int32 VisitElement(const FTLLRN_RigBaseElement* InElement, UTLLRN_RigHierarchy* InHierarchy, FVisualGraph& OutGraph)
		{
			if(InElement->GetType() == ETLLRN_RigElementType::Curve)
			{
				return INDEX_NONE;
			}
			
			const FName NodeName = GetNodeNameForElement(InElement);
			int32 NodeIndex = OutGraph.FindNode(NodeName);
			if(NodeIndex != INDEX_NONE)
			{
				return NodeIndex;
			}

			EVisualGraphShape Shape = EVisualGraphShape::Ellipse;
			TOptional<FLinearColor> Color;

			switch(InElement->GetType())
			{
				case ETLLRN_RigElementType::Bone:
				{
					Shape = EVisualGraphShape::Box;
						
					if(Cast<FTLLRN_RigBoneElement>(InElement)->BoneType == ETLLRN_RigBoneType::User)
					{
						Color = FLinearColor::Green;
					}
					break;
				}
				case ETLLRN_RigElementType::Null:
				{
					Shape = EVisualGraphShape::Diamond;
					break;
				}
				case ETLLRN_RigElementType::Control:
				{
					FLinearColor ShapeColor = Cast<FTLLRN_RigControlElement>(InElement)->Settings.ShapeColor; 
					ShapeColor.A = 1.f;
					Color = ShapeColor;
					break;
				}
				default:
				{
					break;
				}
			}

			NodeIndex = OutGraph.AddNode(NodeName, InElement->GetFName(), Color, Shape);

			if(NodeIndex != INDEX_NONE)
			{
				TArray<int32> Parents = VisitParents(InElement, InHierarchy, OutGraph);
				TArray<FTLLRN_RigElementWeight> Weights = InHierarchy->GetParentWeightArray(InElement);
				for(int32 ParentIndex = 0; ParentIndex < Parents.Num(); ParentIndex++)
				{
					const int32 ParentNodeIndex = Parents[ParentIndex];
					if(ParentNodeIndex != INDEX_NONE)
					{
						const TOptional<FLinearColor> EdgeColor;
						TOptional<EVisualGraphStyle> Style;
						if(Weights.IsValidIndex(ParentIndex))
						{
							if(Weights[ParentIndex].IsAlmostZero())
							{
								Style = EVisualGraphStyle::Dotted;
							}
						}
						OutGraph.AddEdge(
							NodeIndex,
							ParentNodeIndex,
							EVisualGraphEdgeDirection::SourceToTarget,
							NAME_None,
							TOptional<FName>(),
							EdgeColor,
							Style);
					}
				}
			}

			return NodeIndex;
		}
	};

	InHierarchy->ForEach([InHierarchy, &Graph](const FTLLRN_RigBaseElement* InElement)
	{
		Local::VisitElement(InElement, InHierarchy, Graph);
		return true;
	});

#if WITH_EDITOR
	
	if(!InEventName.IsNone())
	{
		if(UTLLRN_ControlRig* CR = InHierarchy->GetTypedOuter<UTLLRN_ControlRig>())
		{
			URigVM* VM = CR->GetVM();

			UTLLRN_RigHierarchy::TElementDependencyMap Dependencies = InHierarchy->GetDependenciesForVM(VM, InEventName);

			for(const UTLLRN_RigHierarchy::TElementDependencyMapPair& Dependency : Dependencies)
			{
				const int32 TargetElementIndex = Dependency.Key;
				
				for(const int32 SourceElementIndex : Dependency.Value)
				{
					FName EdgeName = *FString::Printf(TEXT("Dependency_%d_%d"), SourceElementIndex, TargetElementIndex);
					if(Graph.FindEdge(EdgeName) != INDEX_NONE)
					{
						continue;
					}
					
					const TOptional<FLinearColor> EdgeColor = FLinearColor::Gray;
					TOptional<EVisualGraphStyle> Style = EVisualGraphStyle::Dashed;

					const FName SourceNodeName = Local::GetNodeNameForElementIndex(SourceElementIndex);
					const FName TargetNodeName = Local::GetNodeNameForElementIndex(TargetElementIndex);
					const int32 SourceNodeIndex = Graph.FindNode(SourceNodeName);
					const int32 TargetNodeIndex = Graph.FindNode(TargetNodeName);

					if(SourceNodeIndex == INDEX_NONE || TargetNodeIndex == INDEX_NONE)
					{
						continue;
					}
					
					Graph.AddEdge(
						TargetNodeIndex,
						SourceNodeIndex,
						EVisualGraphEdgeDirection::SourceToTarget,
						EdgeName,
						TOptional<FName>(),
						EdgeColor,
						Style);
				}					
			}
		}
	}
	
#endif
	
	return Graph.DumpDot();
}
