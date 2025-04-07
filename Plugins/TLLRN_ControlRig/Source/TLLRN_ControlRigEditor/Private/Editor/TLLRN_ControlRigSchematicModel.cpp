// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigSchematicModel.h"

#include "TLLRN_ControlRig.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlRigEditor.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "TLLRN_ModularRig.h"
#include "TLLRN_ModularRigRuleManager.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Rigs/TLLRN_RigHierarchyController.h"
#include "Async/TaskGraphInterfaces.h"
#include <SchematicGraphPanel/SchematicGraphStyle.h>
#include "Editor/STLLRN_ModularRigModel.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "TLLRN_ControlRigDragOps.h"
#include "SchematicGraphPanel/SSchematicGraphPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "ScopedTransaction.h"
#include "Dialog/SCustomDialog.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigSchematicModel"

FString FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode::GetDragDropDecoratorLabel() const
{
	return Key.ToString();
}

bool FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode::IsDragSupported() const
{
	return Key.Type == ETLLRN_RigElementType::Connector;
}

const FText& FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode::GetLabel() const
{
	if(IsExpanded())
	{
		static const FText& EmptyText = FText();
		return EmptyText;
	}
	
	FText LabelText = FText::FromString(Key.ToString());
	if(const FTLLRN_ControlRigSchematicModel* TLLRN_ControlRigModel = Cast<FTLLRN_ControlRigSchematicModel>(Model))
	{
		if(const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigModel->TLLRN_ControlRigBeingDebuggedPtr.Get()))
		{
			if (const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy())
			{
				LabelText = Hierarchy->GetDisplayNameForUI(Key);

				switch(Key.Type)
				{
					case ETLLRN_RigElementType::Socket:
					{
						if(const FTLLRN_RigSocketElement* Socket = Hierarchy->Find<FTLLRN_RigSocketElement>(Key))
						{
							const FString Description = Socket->GetDescription(Hierarchy);
							if(!Description.IsEmpty())
							{
								static const FText NodeLabelSocketDescriptionFormat = LOCTEXT("NodeLabelSocketDescriptionFormat", "{0}\n{1}");
								LabelText = FText::Format(NodeLabelSocketDescriptionFormat, LabelText,  FText::FromString(Description));
							}
						}
						break;
					}
					default:
					{
						break;
					}
				}

				const FTLLRN_ModularRigModel& TLLRN_ModularRigModel = TLLRN_ModularRig->GetTLLRN_ModularRigModel();
				const TArray<FTLLRN_RigElementKey>& Connectors = TLLRN_ModularRigModel.Connections.FindConnectorsFromTarget(Key);
				for(const FTLLRN_RigElementKey& Connector : Connectors)
				{
					static const FText NodeLabelConnectionFormat = LOCTEXT("NodeLabelConnectionFormat", "{0}\nConnection: {1}");
					const FText ConnectorShortestPath = Hierarchy->GetDisplayNameForUI(Connector, false);
					LabelText = FText::Format(NodeLabelConnectionFormat, LabelText, ConnectorShortestPath);
				}
			}
		}
	}
	const_cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode*>(this)->Label = LabelText;
	return Label;
}

FTLLRN_ControlRigSchematicWarningTag::FTLLRN_ControlRigSchematicWarningTag()
: FSchematicGraphTag()
{
	BackgroundColor = FLinearColor::Black;
	ForegroundColor = FLinearColor::Yellow;
	
	static const FSlateBrush* WarningBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.Schematic.ConnectorWarning");
	ForegroundBrush = WarningBrush;
}

FTLLRN_ControlRigSchematicModel::~FTLLRN_ControlRigSchematicModel()
{
	if(TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		if(UTLLRN_ControlRig* TLLRN_ControlRigBeingDebugged = TLLRN_ControlRigBeingDebuggedPtr.Get())
		{
			if(!URigVMHost::IsGarbageOrDestroyed(TLLRN_ControlRigBeingDebugged))
			{
				TLLRN_ControlRigBeingDebugged->GetHierarchy()->OnModified().RemoveAll(this);
				TLLRN_ControlRigBeingDebugged->OnPostConstruction_AnyThread().RemoveAll(this);
			}
		}
	}

	TLLRN_ControlRigBeingDebuggedPtr.Reset();
	TLLRN_ControlRigEditor.Reset();
	TLLRN_ControlRigBlueprint.Reset();
}

void FTLLRN_ControlRigSchematicModel::SetEditor(const TSharedRef<FTLLRN_ControlRigEditor>& InEditor)
{
	TLLRN_ControlRigEditor = InEditor;
	TLLRN_ControlRigBlueprint = TLLRN_ControlRigEditor.Pin()->GetTLLRN_ControlRigBlueprint();
	OnSetObjectBeingDebugged(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
}

void FTLLRN_ControlRigSchematicModel::ApplyToPanel(SSchematicGraphPanel* InPanel)
{
	FSchematicGraphModel::ApplyToPanel(InPanel);
	
	InPanel->OnNodeClicked().BindRaw(this, &FTLLRN_ControlRigSchematicModel::HandleSchematicNodeClicked);
	InPanel->OnBeginDrag().BindRaw(this, &FTLLRN_ControlRigSchematicModel::HandleSchematicBeginDrag);
	InPanel->OnEndDrag().BindRaw(this, &FTLLRN_ControlRigSchematicModel::HandleSchematicEndDrag);
	InPanel->OnEnterDrag().BindRaw(this, &FTLLRN_ControlRigSchematicModel::HandleSchematicEnterDrag);
	InPanel->OnLeaveDrag().BindRaw(this, &FTLLRN_ControlRigSchematicModel::HandleSchematicLeaveDrag);
	InPanel->OnCancelDrag().BindRaw(this, &FTLLRN_ControlRigSchematicModel::HandleSchematicCancelDrag);
	InPanel->OnAcceptDrop().BindRaw(this, &FTLLRN_ControlRigSchematicModel::HandleSchematicDrop);
}

void FTLLRN_ControlRigSchematicModel::Reset()
{
	Super::Reset();
	TLLRN_RigElementKeyToGuid.Reset();
}

void FTLLRN_ControlRigSchematicModel::Tick(float InDeltaTime)
{
	Super::Tick(InDeltaTime);

	if(TLLRN_ControlRigBlueprint.IsValid())
	{
		//UE_LOG(LogTLLRN_ControlRig, Display, TEXT("Selected nodes %d"), GetSelectedNodes().Num());
		if (UTLLRN_ControlRig* DebuggedRig = TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig())
		{
			const UTLLRN_RigHierarchy* Hierarchy = DebuggedRig->GetHierarchy();
			check(Hierarchy);
			const FTLLRN_ModularRigConnections& Connections = TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections;

			TArray<FTLLRN_RigElementKey> KeysToRemove;
			for(const TPair<FTLLRN_RigElementKey,FGuid>& Pair : TLLRN_RigElementKeyToGuid)
			{
				if((Pair.Key.Type == ETLLRN_RigElementType::Socket) ||
					(Pair.Key.Type == ETLLRN_RigElementType::Connector))
				{
					continue;
				}
				if(TemporaryNodeGuids.Contains(Pair.Value))
				{
					continue;
				}

				const TArray<FTLLRN_RigElementKey>& Connectors = Connections.FindConnectorsFromTarget(Pair.Key);
				if(Connectors.Num() > 1)
				{
					continue;
				}
				if(Connectors.Num() == 1)
				{
					if(const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(Connectors[0]))
					{
						if(!Connector->IsPrimary())
						{
							continue;
						}
					}
				}
			
				KeysToRemove.Add(Pair.Key);
			}
			for(const FTLLRN_RigElementKey& KeyToRemove : KeysToRemove)
			{
				RemoveElementKeyNode(KeyToRemove);
			}

			TArray<FTLLRN_RigElementKey> ConnectorKeys;
			for(const TPair<FTLLRN_RigElementKey,FGuid>& Pair : TLLRN_RigElementKeyToGuid)
			{
				if(Pair.Key.Type == ETLLRN_RigElementType::Connector)
				{
					ConnectorKeys.Add(Pair.Key);
				}
			}

			for(const FTLLRN_RigElementKey& ConnectorKey : ConnectorKeys)
			{
				UpdateConnector(ConnectorKey);
			}
		}
	}
}

FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* FTLLRN_ControlRigSchematicModel::AddElementKeyNode(const FTLLRN_RigElementKey& InKey, bool bNotify)
{
	FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = AddNode<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(false);
	if(Node)
	{
		ConfigureElementKeyNode(Node, InKey);
		TLLRN_RigElementKeyToGuid.Add(Node->GetKey(), Node->GetGuid());
		UpdateConnector(InKey);

		if(InKey.Type == ETLLRN_RigElementType::Connector)
		{
			Node->AddTag<FTLLRN_ControlRigSchematicWarningTag>();
		}

		if (bNotify && OnNodeAddedDelegate.IsBound())
		{
			OnNodeAddedDelegate.Broadcast(Node);
		}

		UpdateElementKeyLinks();
	}
	return Node;
}

void FTLLRN_ControlRigSchematicModel::ConfigureElementKeyNode(FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* InNode, const FTLLRN_RigElementKey& InKey)
{
	InNode->Key = InKey;
}

const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* FTLLRN_ControlRigSchematicModel::FindElementKeyNode(const FTLLRN_RigElementKey& InKey) const
{
	return const_cast<FTLLRN_ControlRigSchematicModel*>(this)->FindElementKeyNode(InKey);
}

FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* FTLLRN_ControlRigSchematicModel::FindElementKeyNode(const FTLLRN_RigElementKey& InKey)
{
	if(const FGuid* FoundGuid = TLLRN_RigElementKeyToGuid.Find(InKey))
	{
		return FindNode< FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode >(*FoundGuid);
	}
	return nullptr;
}

bool FTLLRN_ControlRigSchematicModel::ContainsElementKeyNode(const FTLLRN_RigElementKey& InKey) const
{
	return FindElementKeyNode(InKey) != nullptr;
}

bool FTLLRN_ControlRigSchematicModel::RemoveNode(const FGuid& InGuid)
{
	FTLLRN_RigElementKey KeyToRemove;
	if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = FindNode<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InGuid))
	{
		KeyToRemove = Node->GetKey();
	}
	if(Super::RemoveNode(InGuid))
	{
		TLLRN_RigElementKeyToGuid.Remove(KeyToRemove);
		return true;
	}
	return false;
}

bool FTLLRN_ControlRigSchematicModel::RemoveElementKeyNode(const FTLLRN_RigElementKey& InKey)
{
	if(const FGuid* GuidPtr = TLLRN_RigElementKeyToGuid.Find(InKey))
	{
		const FGuid Guid = *GuidPtr;
		const bool bResult = RemoveNode(Guid);
		if(bResult)
		{
			UpdateElementKeyLinks();
		}
		return bResult;
	}
	return false;
}

FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink* FTLLRN_ControlRigSchematicModel::AddElementKeyLink(const FTLLRN_RigElementKey& InSourceKey, const FTLLRN_RigElementKey& InTargetKey, bool bNotify)
{
	check(InSourceKey.IsValid());
	check(InTargetKey.IsValid());
	check(InSourceKey != InTargetKey);
	const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* SourceNode = FindElementKeyNode(InSourceKey);
	const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* TargetNode = FindElementKeyNode(InTargetKey);
	if(SourceNode && TargetNode)
	{
		FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink* Link = AddLink<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink>(SourceNode->GetGuid(), TargetNode->GetGuid());
		Link->SourceKey = InSourceKey;
		Link->TargetKey = InTargetKey;
		Link->Thickness = 16.f;
		return Link;
	}
	return nullptr;
}

void FTLLRN_ControlRigSchematicModel::UpdateElementKeyNodes()
{
	if (TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		if (const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy())
		{
			const TArray<FTLLRN_RigSocketElement*> Sockets = Hierarchy->GetElementsOfType<FTLLRN_RigSocketElement>();
			for (const FTLLRN_RigSocketElement* Socket : Sockets)
			{
				if(!ContainsElementKeyNode(Socket->GetKey()))
				{
					AddElementKeyNode(Socket->GetKey());
				}
			}

			const TArray<FTLLRN_RigConnectorElement*> Connectors = Hierarchy->GetElementsOfType<FTLLRN_RigConnectorElement>();
			for (const FTLLRN_RigConnectorElement* Connector : Connectors)
			{
				if(!ContainsElementKeyNode(Connector->GetKey()))
				{
					AddElementKeyNode(Connector->GetKey());
				}
				UpdateConnector(Connector->GetKey());
			}

			// remove obsolete nodes
			TArray<FGuid> GuidsToRemove;
			for(const TSharedPtr<FSchematicGraphNode>& Node : Nodes)
			{
				if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ElementKeyNode = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(Node.Get()))
				{
					if(!Hierarchy->Contains(ElementKeyNode->GetKey()))
					{
						GuidsToRemove.Add(ElementKeyNode->GetGuid());
					}
				}
			}
			for(const FGuid& Guid : GuidsToRemove)
			{
				RemoveNode(Guid);
			}
		}
	}
}

void FTLLRN_ControlRigSchematicModel::UpdateElementKeyLinks()
{
	if(!bUpdateElementKeyLinks)
	{
		return;
	}
	
	const TSharedPtr<FTLLRN_ControlRigEditor> Editor = TLLRN_ControlRigEditor.Pin();
	if(!Editor.IsValid())
	{
		return;
	}

	typedef TTuple< FTLLRN_RigElementKey, FTLLRN_RigElementKey > TElementKeyPair;
	typedef TTuple< FGuid, TElementKeyPair > TElementKeyLinkPair;

	struct FLinkTraverser
	{
		const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* VisitElement(
			const FTLLRN_RigBaseElement* InElement, 
			const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* InNode, 
			TArray< TElementKeyPair >& OutExpectedLinks) const
		{
			const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = InNode;
			if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* SelfNode = FindNode(InElement->GetKey()))
			{
				Node = SelfNode;
			}

			const TConstArrayView<FTLLRN_RigBaseElement*> Children = Hierarchy->GetChildren(InElement);
			for(const FTLLRN_RigBaseElement* Child : Children)
			{
				const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ChildNode = VisitElement(Child, Node, OutExpectedLinks);
				if(Node && ChildNode && Node != ChildNode)
				{
					const FTLLRN_RigElementKey ParentA = Hierarchy->GetFirstParent(Node->GetKey());
					const FTLLRN_RigElementKey ParentB = Hierarchy->GetFirstParent(ChildNode->GetKey());
					if(!ParentA || !ParentB.IsValid() || ParentA != ParentB)
					{
						OutExpectedLinks.Emplace(Node->GetKey(), ChildNode->GetKey());
					}
				}
			}

			return Node;
		}

		const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* FindNode(const FTLLRN_RigElementKey& InKey) const
		{
			if(const FGuid* ElementGuid = TLLRN_RigElementKeyToGuid->Find(InKey))
			{
				return Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(NodeByGuid->FindChecked(*ElementGuid).Get());
			}
			if(const FTLLRN_RigElementKey* SocketKey = SocketToParent.Find(InKey))
			{
				if(const FGuid* ElementGuid = TLLRN_RigElementKeyToGuid->Find(*SocketKey))
				{
					return Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(NodeByGuid->FindChecked(*ElementGuid).Get());
				}
			}
			if(const FTLLRN_RigElementKey* ParentKey = ParentToSocket.Find(InKey))
			{
				if(const FGuid* ElementGuid = TLLRN_RigElementKeyToGuid->Find(*ParentKey))
				{
					return Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(NodeByGuid->FindChecked(*ElementGuid).Get());
				}
			}
			return nullptr;
		}

		TArray< TElementKeyPair > ComputeExpectedLinks() const
		{
			SocketToParent.Reset();
			ParentToSocket.Reset();

			// create a map to look up sockets
			const TArray<FTLLRN_RigElementKey> SocketKeys = Hierarchy->GetSocketKeys();
			for(const FTLLRN_RigElementKey& SocketKey : SocketKeys)
			{
				const FTLLRN_RigElementKey ParentKey = Hierarchy->GetFirstParent(SocketKey);
				if(ParentKey.IsValid())
				{
					SocketToParent.Add(SocketKey, ParentKey);
					if(!ParentToSocket.Contains(ParentKey))
					{
						ParentToSocket.Add(ParentKey, SocketKey);
					}
				}
			}
			
			TArray< TElementKeyPair > ExpectedLinks;
			const TArray<FTLLRN_RigBaseElement*> RootElements = Hierarchy->GetRootElements();
			for(const FTLLRN_RigBaseElement* RootElement : RootElements)
			{
				if(RootElement->GetKey().Type == ETLLRN_RigElementType::Curve)
				{
					continue;
				}
				VisitElement(RootElement, nullptr, ExpectedLinks);
			}
			return ExpectedLinks;
		}

		const UTLLRN_ControlRig* TLLRN_ControlRig; 
		const UTLLRN_RigHierarchy* Hierarchy; 
		const TMap<FTLLRN_RigElementKey, FGuid>* TLLRN_RigElementKeyToGuid = nullptr;
		const TMap<FGuid, TSharedPtr<FSchematicGraphNode>>* NodeByGuid = nullptr;
		mutable TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> SocketToParent;
		mutable TMap<FTLLRN_RigElementKey, FTLLRN_RigElementKey> ParentToSocket;
	};

	FLinkTraverser Traverser;
	Traverser.TLLRN_ControlRig = Editor->GetTLLRN_ControlRig();
	if(Traverser.TLLRN_ControlRig == nullptr)
	{
		return;
	}
	Traverser.Hierarchy = Traverser.TLLRN_ControlRig->GetHierarchy();
	if(Traverser.Hierarchy == nullptr)
	{
		return;
	}
	Traverser.TLLRN_RigElementKeyToGuid = &TLLRN_RigElementKeyToGuid;
	Traverser.NodeByGuid = &NodeByGuid;

	const TArray< TElementKeyPair > ExpectedLinks = Traverser.ComputeExpectedLinks();

	TArray< TElementKeyLinkPair > ExistingLinks;
	for(const TSharedPtr<FSchematicGraphLink>& Link : Links)
	{
		if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink* ElementKeyLink = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink>(Link.Get()))
		{
			ExistingLinks.Emplace(ElementKeyLink->GetGuid(), TElementKeyPair(ElementKeyLink->GetSourceKey(), ElementKeyLink->GetTargetKey()));
		}
	}

	// remove the obsolete links
	for(const TTuple< FGuid, TTuple< FTLLRN_RigElementKey, FTLLRN_RigElementKey > >& ExistingLink : ExistingLinks)
	{
		if(!ExpectedLinks.Contains(ExistingLink.Get<1>()))
		{
			(void)RemoveLink(ExistingLink.Get<0>());
		}
	}

	// add missing links
	for(const TElementKeyPair& ExpectedLink : ExpectedLinks)
	{
		if(!ExistingLinks.ContainsByPredicate([ExpectedLink](const TElementKeyLinkPair& ExistingLink) -> bool
		{
			return ExistingLink.Get<1>() == ExpectedLink;
		}))
		{
			(void)AddElementKeyLink(ExpectedLink.Get<0>(), ExpectedLink.Get<1>());
		}
	}

	// todo: also introduce links for module relationships
}

void FTLLRN_ControlRigSchematicModel::UpdateTLLRN_ControlRigContent()
{
	{
		const TGuardValue<bool> DisableUpdatingLinks(bUpdateElementKeyLinks, false);
		UpdateElementKeyNodes();
	}
	UpdateElementKeyLinks();
}

void FTLLRN_ControlRigSchematicModel::UpdateConnector(const FTLLRN_RigElementKey& InElementKey)
{
	if(InElementKey.Type == ETLLRN_RigElementType::Connector)
	{
		if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ConnectorNode = FindElementKeyNode(InElementKey))
		{
			FTLLRN_RigElementKey ResolvedKey;
			if(IsConnectorResolved(InElementKey, &ResolvedKey))
			{
				if(TLLRN_ControlRigBlueprint.IsValid() && TLLRN_ControlRigBeingDebuggedPtr.IsValid() && ResolvedKey.Type != ETLLRN_RigElementType::Socket)
				{
					if(const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy())
					{
						if(const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(InElementKey))
						{
							if(Connector->IsPrimary())
							{
								// if the resolved target has only one primary connector on it, and it is not a socket
								// let's not draw the node in the schematic for now
								if(TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections.FindConnectorsFromTarget(ResolvedKey).Num() == 1)
								{
									ResolvedKey = FTLLRN_RigElementKey();
								}
							}
						}
					}
				}
			}

			if(ResolvedKey.IsValid())
			{
				const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ResolvedNode = FindElementKeyNode(ResolvedKey);
				if(ResolvedNode == nullptr)
				{
					ResolvedNode = AddElementKeyNode(ResolvedKey);
				}

				if(ConnectorNode->GetParentNode() != ResolvedNode)
				{
					SetParentNode(ConnectorNode->GetGuid(), ResolvedNode->GetGuid());
					const_cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode*>(ConnectorNode)->OnMouseLeave();
				}
			}
			else if(ConnectorNode->HasParentNode())
			{
				RemoveFromParentNode(ConnectorNode->GetGuid());
				const_cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode*>(ConnectorNode)->OnMouseLeave();
			}
		}
	}
}

void FTLLRN_ControlRigSchematicModel::OnSetObjectBeingDebugged(UObject* InObject)
{
	if(TLLRN_ControlRigBeingDebuggedPtr.Get() == InObject)
	{
		return;
	}

	if(TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		if(UTLLRN_ControlRig* TLLRN_ControlRigBeingDebugged = TLLRN_ControlRigBeingDebuggedPtr.Get())
		{
			if(!URigVMHost::IsGarbageOrDestroyed(TLLRN_ControlRigBeingDebugged))
			{
				TLLRN_ControlRigBeingDebugged->GetHierarchy()->OnModified().RemoveAll(this);
				TLLRN_ControlRigBeingDebugged->OnPostConstruction_AnyThread().RemoveAll(this);
			}
		}
	}

	TLLRN_ControlRigBeingDebuggedPtr.Reset();
	
	if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(InObject))
	{
		TLLRN_ControlRigBeingDebuggedPtr = TLLRN_ControlRig;
		if(UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
		{
			TLLRN_ControlRig->OnPostConstruction_AnyThread().AddRaw(this, &FTLLRN_ControlRigSchematicModel::HandlePostConstruction);

			UpdateTLLRN_ControlRigContent();
		}
	}
}

void FTLLRN_ControlRigSchematicModel::HandleTLLRN_ModularRigModified(ETLLRN_ModularRigNotification InNotification, const FTLLRN_RigModuleReference* InModule)
{
	switch (InNotification)
	{
		case ETLLRN_ModularRigNotification::ConnectionChanged:
		{
			if (TLLRN_ControlRigBeingDebuggedPtr.IsValid() && InModule)
			{
				if (const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy())
				{
					const TArray<FTLLRN_RigElementKey> Connectors = Hierarchy->GetConnectorKeys();
					for(const FTLLRN_RigElementKey& ConnectorKey : Connectors)
					{
						if(const FSchematicGraphNode* ConnectorNode = FindElementKeyNode(ConnectorKey))
						{
							const FString NameString = ConnectorKey.Name.ToString();
							if(NameString.StartsWith(InModule->GetNamespace(), ESearchCase::IgnoreCase))
							{
								const FString LocalName = NameString.Mid(InModule->GetNamespace().Len());
								if(!LocalName.Contains(UTLLRN_ModularRig::NamespaceSeparator))
								{
									UpdateConnector(ConnectorKey);
								}
							}
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
}

FSchematicGraphGroupNode* FTLLRN_ControlRigSchematicModel::AddAutoGroupNode()
{
	FSchematicGraphGroupNode* GroupNode = Super::AddAutoGroupNode();
	GroupNode->AddTag<FTLLRN_ControlRigSchematicWarningTag>();
	return GroupNode;
}

FVector2d FTLLRN_ControlRigSchematicModel::GetPositionForNode(const FSchematicGraphNode* InNode) const
{
	if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode))
	{
		if(!InNode->HasParentNode())
		{
			FTLLRN_RigElementKey Key = Node->GetKey();
			if(Key.Type == ETLLRN_RigElementType::Connector)
			{
				FTLLRN_RigElementKey ResolvedKey;
				if(IsConnectorResolved(Key, &ResolvedKey))
				{
					Key = ResolvedKey;
				}
			}

			if(Key.IsValid())
			{
				if(const TSharedPtr<FTLLRN_ControlRigEditor> Editor = TLLRN_ControlRigEditor.Pin())
				{
					if(Editor.IsValid())
					{
						if(const UTLLRN_ControlRig* TLLRN_ControlRig = Editor->GetTLLRN_ControlRig())
						{
							if(const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
							{
								const FTransform Transform = Hierarchy->GetGlobalTransform(Key);
								return Editor->ComputePersonaProjectedScreenPos(Transform.GetLocation());
							}
						}
					}
				}
			}
		}
	}
	return Super::GetPositionForNode(InNode);
}

bool FTLLRN_ControlRigSchematicModel::GetPositionAnimationEnabledForNode(const FSchematicGraphNode* InNode) const
{
	if(InNode->IsA<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>())
	{
		return false;
	}
	return Super::GetPositionAnimationEnabledForNode(InNode);
}

int32 FTLLRN_ControlRigSchematicModel::GetNumLayersForNode(const FSchematicGraphNode* InNode) const
{
	if(Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode))
	{
		return 3;
	}
	return Super::GetNumLayersForNode(InNode);
}

const FSlateBrush* FTLLRN_ControlRigSchematicModel::GetBrushForKey(const FTLLRN_RigElementKey& InKey, const FSchematicGraphNode* InNode) const
{
	static const FSlateBrush* UnresolvedSocketBrush = FSchematicGraphStyle::Get().GetBrush( "Schematic.Dot.Small");
	static const FSlateBrush* ResolvedSocketBrush = FSchematicGraphStyle::Get().GetBrush( "Schematic.Dot.Large");
	static const FSlateBrush* ResolvedMultipleSocketBrush = FSchematicGraphStyle::Get().GetBrush( "Schematic.Dot.Medium");
	static const FSlateBrush* ConnectorPrimaryBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush( "TLLRN_ControlRig.Schematic.ConnectorPrimary");
	static const FSlateBrush* ConnectorOptionalBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush( "TLLRN_ControlRig.Schematic.ConnectorOptional");
	static const FSlateBrush* ConnectorSecondaryBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush( "TLLRN_ControlRig.Schematic.ConnectorSecondary");
	static const FSlateBrush* BoneBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush( "TLLRN_ControlRig.Schematic.Bone");
	static const FSlateBrush* ControlBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush( "TLLRN_ControlRig.Schematic.Control");
	static const FSlateBrush* NullBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush( "TLLRN_ControlRig.Schematic.Null");

	switch(InKey.Type)
	{
		case ETLLRN_RigElementType::Socket:
		{
			if (TLLRN_ControlRigBlueprint.IsValid())
			{
				int32 Count = 0;
				for(const FTLLRN_ModularRigSingleConnection& Connection : TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections)
				{
					if(Connection.Target == InKey)
					{
						Count++;
						if(Count == 2)
						{
							break;
						}
					}
				}

				if(Count == 1)
				{
					return ResolvedSocketBrush;
				}
				if(Count == 2)
				{
					return ResolvedMultipleSocketBrush;
				}
			}

			return UnresolvedSocketBrush;
		}
		case ETLLRN_RigElementType::Connector:
		{
			if (TLLRN_ControlRigBlueprint.IsValid())
			{
				if(const UTLLRN_ModularRig* TLLRN_ControlRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig()))
				{
					if(const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
					{
						if(const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(InKey))
						{
							if((Connector->IsPrimary()) && !IsConnectorResolved(InKey))
							{
								if(const FTLLRN_RigModuleInstance* ModuleInstance = TLLRN_ControlRig->FindModule(Connector))
								{
									const FSoftObjectPath& IconPath = ModuleInstance->GetRig()->GetTLLRN_RigModuleSettings().Icon;
									const FSlateBrush* IconBrush = ModuleIcons.Find(IconPath);
									if (!IconBrush)
									{
										if(UTexture2D* Icon = Cast<UTexture2D>(IconPath.TryLoad()))
										{
											IconBrush = &ModuleIcons.Add(IconPath, UWidgetBlueprintLibrary::MakeBrushFromTexture(Icon, 16.0f, 16.0f));
										}
									}
									if(IconBrush)
									{
										return IconBrush;
									}
								}
							}

							if(Connector->IsSecondary())
							{
								return Connector->IsOptional() ? 	ConnectorOptionalBrush : ConnectorSecondaryBrush;
							}
						}
					}
				}
			}
			return ConnectorPrimaryBrush;
		}
		default:
		{
			// check if the key has a resolved connector
			if(const FSchematicGraphGroupNode* GroupNode = Cast<FSchematicGraphGroupNode>(InNode))
			{
				if(!GroupNode->IsExpanded() && !GroupNode->IsExpanding())
				{
					if (TLLRN_ControlRigBlueprint.IsValid())
					{
						int32 Count = 0;
						for(const FTLLRN_ModularRigSingleConnection& Connection : TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections)
						{
							if(Connection.Target == InKey)
							{
								Count++;
								if(Count == 2)
								{
									return ResolvedMultipleSocketBrush;
								}
							}
						}
						if(Count > 0)
						{
							return ResolvedSocketBrush;
						}
					}
				}
			}
			
			switch(InKey.Type)
			{
				case ETLLRN_RigElementType::Bone:
				{
					return BoneBrush;
				}
				case ETLLRN_RigElementType::Control:
				{
					return ControlBrush;
				}
				case ETLLRN_RigElementType::Null:
				{
					return NullBrush;
				}
				default:
				{
					break;
				}
			}
			break;
		}
	}
	return nullptr;
}

const FSlateBrush* FTLLRN_ControlRigSchematicModel::GetBrushForNode(const FSchematicGraphNode* InNode, int32 InLayerIndex) const
{
	if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode))
	{
		if(InLayerIndex == 0)
		{
			static const FSlateBrush* BackgroundBrush = FSchematicGraphStyle::Get().GetBrush( "Schematic.Background");
			return BackgroundBrush;
		}
		if(InLayerIndex == 1)
		{
			static const FSlateBrush* OutlineSingleBrush = FSchematicGraphStyle::Get().GetBrush( "Schematic.Outline.Single");
			static const FSlateBrush* OutlineDoubleBrush = FSchematicGraphStyle::Get().GetBrush( "Schematic.Outline.Double");

			if (TLLRN_ControlRigBlueprint.IsValid())
			{
				int32 Count = 0;
				for(const FTLLRN_ModularRigSingleConnection& Connection : TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections)
				{
					if(Connection.Target == Node->GetKey())
					{
						Count++;
						if(Count == 2)
						{
							return OutlineDoubleBrush;
						}
					}
				}
			}
			return OutlineSingleBrush;
		}

		if(const FSlateBrush* Brush = GetBrushForKey(Node->GetKey(), Node))
		{
			return Brush;
		}
	}
	return Super::GetBrushForNode(InNode, InLayerIndex);
}

FLinearColor FTLLRN_ControlRigSchematicModel::GetColorForNode(const FSchematicGraphNode* InNode, int32 InLayerIndex) const
{
	if (!TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		return Super::GetColorForNode(InNode, InLayerIndex);
	}

	const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy();
	if(Hierarchy == nullptr)
	{
		return Super::GetColorForNode(InNode, InLayerIndex);
	}

	static const FLinearColor SelectionColor = FLinearColor(FColor::FromHex(TEXT("#EBA30A")));
			
	if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ElementKeyNode = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode))
	{
		if(Hierarchy->IsSelected(ElementKeyNode->GetKey()))
		{
			return SelectionColor;
		}

		if(InLayerIndex == 0) // background
		{
			return FLinearColor(0, 0, 0, 0.75);
		}
		
		switch(ElementKeyNode->GetKey().Type)
		{
			case ETLLRN_RigElementType::Bone:
			{
				return FTLLRN_ControlRigEditorStyle::Get().BoneUserInterfaceColor;
			}
			case ETLLRN_RigElementType::Null:
			{
				return FTLLRN_ControlRigEditorStyle::Get().NullUserInterfaceColor;
			}
			case ETLLRN_RigElementType::Control:
			{
				if(const FTLLRN_RigControlElement* Control = Hierarchy->Find<FTLLRN_RigControlElement>(ElementKeyNode->GetKey()))
				{
					return Control->Settings.ShapeColor;
				}
				break;
			}
			case ETLLRN_RigElementType::Socket:
			{
				if(const FTLLRN_RigSocketElement* Socket = Hierarchy->Find<FTLLRN_RigSocketElement>(ElementKeyNode->GetKey()))
				{
					return Socket->GetColor(Hierarchy);
				}
				break;
			}
			case ETLLRN_RigElementType::Connector:
			{
				return FLinearColor::White;
				//return FTLLRN_ControlRigEditorStyle::Get().ConnectorUserInterfaceColor;
			}
			default:
			{
				break;
			}
		}
	}
	else if(const FSchematicGraphAutoGroupNode* AutoGroupNode = Cast<FSchematicGraphAutoGroupNode>(InNode))
	{
		for(int32 ChildIndex = 0; ChildIndex < AutoGroupNode->GetNumChildNodes(); ChildIndex++)
		{
			if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ChildElementKeyNode = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(AutoGroupNode->GetChildNode(ChildIndex)))
			{
				if(Hierarchy->IsSelected(ChildElementKeyNode->GetKey()))
				{
					return SelectionColor;
				}
			}
		}
	}
	return Super::GetColorForNode(InNode, InLayerIndex);
}

ESchematicGraphVisibility::Type FTLLRN_ControlRigSchematicModel::GetVisibilityForNode(const FSchematicGraphNode* InNode) const
{
	if(TLLRN_ControlRigBlueprint.IsValid())
	{
		if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode))
		{
			if(Node->GetKey().Type == ETLLRN_RigElementType::Connector)
			{
				if (const UTLLRN_ModularRig* TLLRN_ControlRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig()))
				{
					if(const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy())
					{
						if(const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(Node->GetKey()))
						{
							if(IsConnectorResolved(Connector->GetKey()))
							{
								if(Connector->IsPrimary())
								{
									if(const FTLLRN_RigModuleInstance* ModuleInstance = TLLRN_ControlRig->FindModule(Connector))
									{
										if(ModuleInstance->IsRootModule())
										{
											return ESchematicGraphVisibility::Hidden;
										}
									}
								}
							}
							else
							{
								return ESchematicGraphVisibility::Hidden;
							}
						}
					}
				}
			}
		}
	}
	return Super::GetVisibilityForNode(InNode);
}

const FSlateBrush* FTLLRN_ControlRigSchematicModel::GetBrushForLink(const FSchematicGraphLink* InLink) const
{
	if(InLink->IsA<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyLink>())
	{
		if(const FSchematicGraphNode* SourceNode = FindNode(InLink->GetSourceNodeGuid()))
		{
			if(const FSchematicGraphNode* TargetNode = FindNode(InLink->GetTargetNodeGuid()))
			{
				if(SourceNode->IsA<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>() &&
					TargetNode->IsA<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>())
				{
					static const FSlateBrush* LinkBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush( "TLLRN_ControlRig.Schematic.Link");
					return LinkBrush;
				}
			}
		}
	}
	return Super::GetBrushForLink(InLink);
}

FLinearColor FTLLRN_ControlRigSchematicModel::GetColorForLink(const FSchematicGraphLink* InLink) const
{
	if(const FSchematicGraphNode* SourceNode = FindNode(InLink->GetSourceNodeGuid()))
	{
		if(const FSchematicGraphNode* TargetNode = FindNode(InLink->GetTargetNodeGuid()))
		{
			if(SourceNode->IsA<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>() &&
				TargetNode->IsA<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>())
			{
				// semi transparent dark gray
				return FLinearColor(0.7, 0.7, 0.7, .5);
			}
		}
	}
	return Super::GetColorForLink(InLink);
}

ESchematicGraphVisibility::Type FTLLRN_ControlRigSchematicModel::GetVisibilityForTag(const FSchematicGraphTag* InTag) const
{
	if(InTag->IsA<FSchematicGraphGroupTag>())
	{
		if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InTag->GetNode()))
		{
			if(Node->GetNumChildNodes() < 3)
			{
				return ESchematicGraphVisibility::Hidden;
			}
		}
	}
	if(InTag->IsA<FTLLRN_ControlRigSchematicWarningTag>())
	{
		if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ConnectorNode = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InTag->GetNode()))
		{
			if(ConnectorNode->GetKey().Type == ETLLRN_RigElementType::Connector)
			{
				FTLLRN_RigElementKey ResolvedKey;
				if(!IsConnectorResolved(ConnectorNode->GetKey(), &ResolvedKey))
				{
					return ESchematicGraphVisibility::Visible;
				}
			}
		}
		else if(const FSchematicGraphAutoGroupNode* AutoGroupNode = Cast<FSchematicGraphAutoGroupNode>(InTag->GetNode()))
		{
			for(int32 Index = 0; Index < AutoGroupNode->GetNumChildNodes(); Index++)
			{
				if(const FSchematicGraphNode* ChildNode = AutoGroupNode->GetChildNode(Index))
				{
					if(const FTLLRN_ControlRigSchematicWarningTag* ChildTag = ChildNode->FindTag<FTLLRN_ControlRigSchematicWarningTag>())
					{
						const ESchematicGraphVisibility::Type ChildVisibility = GetVisibilityForTag(ChildTag); 
						if(ChildVisibility != ESchematicGraphVisibility::Hidden)
						{
							return ChildVisibility;
						}
					}
				}
			}
		}
		return ESchematicGraphVisibility::Hidden;
	}
	return Super::GetVisibilityForTag(InTag);
}

const FText FTLLRN_ControlRigSchematicModel::GetToolTipForTag(const FSchematicGraphTag* InTag) const
{
	if(InTag->IsA<FTLLRN_ControlRigSchematicWarningTag>())
	{
		if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ConnectorNode = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InTag->GetNode()))
		{
			if(ConnectorNode->GetKey().Type == ETLLRN_RigElementType::Connector)
			{
				if(!IsConnectorResolved(ConnectorNode->GetKey()))
				{
					static const FText UnresolvedConnectorToolTip = LOCTEXT("UnresolvedConnectorToolTip", "Connector is unresolved.");
					return UnresolvedConnectorToolTip;
				}
			}
		}
		else if(const FSchematicGraphAutoGroupNode* AutoGroupNode = Cast<FSchematicGraphAutoGroupNode>(InTag->GetNode()))
		{
			for(int32 Index = 0; Index < AutoGroupNode->GetNumChildNodes(); Index++)
			{
				if(const FSchematicGraphNode* ChildNode = AutoGroupNode->GetChildNode(Index))
				{
					if(const FTLLRN_ControlRigSchematicWarningTag* ChildTag = ChildNode->FindTag<FTLLRN_ControlRigSchematicWarningTag>())
					{
						const FText ChildToolTip = GetToolTipForTag(ChildTag); 
						if(!ChildToolTip.IsEmpty())
						{
							return ChildToolTip;
						}
					}
				}
			}
		}
	}
	return Super::GetToolTipForTag(InTag);
}

bool FTLLRN_ControlRigSchematicModel::GetForwardedNodeForDrag(FGuid& InOutGuid) const
{
	if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = FindNode<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InOutGuid))
	{
		if(TLLRN_ControlRigBlueprint.IsValid())
		{
			const UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig()->GetHierarchy();
			check(Hierarchy);
			const FTLLRN_ModularRigConnections& Connections = TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections;
			const TArray<FTLLRN_RigElementKey>& Connectors = Connections.FindConnectorsFromTarget(Node->GetKey());
			if(Connectors.Num() == 1)
			{
				if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ConnectorNode = FindElementKeyNode(Connectors[0]))
				{
					InOutGuid = ConnectorNode->GetGuid();
					return true;
				}
			}
		}
	}
	return Super::GetForwardedNodeForDrag(InOutGuid);
}

bool FTLLRN_ControlRigSchematicModel::GetContextMenuForNode(const FSchematicGraphNode* InNode, FMenuBuilder& OutMenu) const
{
	bool bSuccess = false;
	if(Super::GetContextMenuForNode(InNode, OutMenu))
	{
		bSuccess = true;
	}
	
	if(TLLRN_ControlRigBlueprint.IsValid())
	{
		if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ElementKeyNode = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode))
		{
			const UTLLRN_ModularRig* TLLRN_ModularRig = CastChecked<UTLLRN_ModularRig>(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
			const FTLLRN_ModularRigConnections& Connections = TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections;
			const TArray<FTLLRN_RigElementKey>& Connectors = Connections.FindConnectorsFromTarget(ElementKeyNode->Key);
			if(!Connectors.IsEmpty())
			{
				OutMenu.BeginSection(TEXT("DisconnectConnectors"), LOCTEXT("DisconnectConnectors", "Disconnect"));

				// note: this is a copy on purpose since it is passed into the lambda
				for(const FTLLRN_RigElementKey Connector : Connectors)
				{
					FText Label = TLLRN_ModularRig->GetHierarchy()->GetDisplayNameForUI(Connector, false);
					const FText Description = FText::FromString(FString::Printf(TEXT("Disconnect %s"), *Label.ToString()));
					
					OutMenu.AddMenuEntry(Description, Description, FSlateIcon(), FUIAction(
						FExecuteAction::CreateLambda([this, Connector]()
						{
							if(TLLRN_ControlRigBlueprint.IsValid())
							{
								if (UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
								{
									FScopedTransaction Transaction(LOCTEXT("DisconnectConnector", "Disconnect Connector"));
									TLLRN_ControlRigBlueprint->Modify();
									Controller->DisconnectConnector(Connector);
								}
							}								
						})
					));
				}
				OutMenu.EndSection();
				bSuccess = true;
			}
		}
	}
	return bSuccess;
}

TArray<FTLLRN_RigElementKey> FTLLRN_ControlRigSchematicModel::GetElementKeysFromDragDropEvent(const FDragDropOperation& InDragDropOperation, const UTLLRN_ControlRig* InTLLRN_ControlRig)
{
	TArray<FTLLRN_RigElementKey> DraggedKeys;

	if(InDragDropOperation.IsOfType<FTLLRN_RigElementHierarchyDragDropOp>())
	{
		const FTLLRN_RigElementHierarchyDragDropOp* RigDragDropOp = StaticCast<const FTLLRN_RigElementHierarchyDragDropOp*>(&InDragDropOperation);
		return RigDragDropOp->GetElements();
	}

	if(InDragDropOperation.IsOfType<FTLLRN_RigHierarchyTagDragDropOp>())
	{
		const FTLLRN_RigHierarchyTagDragDropOp* TagDragDropOp = StaticCast<const FTLLRN_RigHierarchyTagDragDropOp*>(&InDragDropOperation);
		FTLLRN_RigElementKey DraggedKey;
		FTLLRN_RigElementKey::StaticStruct()->ImportText(*TagDragDropOp->GetIdentifier(), &DraggedKey, nullptr, EPropertyPortFlags::PPF_None, nullptr, FTLLRN_RigElementKey::StaticStruct()->GetName(), true);
		return {DraggedKey};
	}
	
	if(InDragDropOperation.IsOfType<FTLLRN_ModularTLLRN_RigModuleDragDropOp>())
	{
		const FTLLRN_ModularTLLRN_RigModuleDragDropOp* ModuleDropOp = StaticCast<const FTLLRN_ModularTLLRN_RigModuleDragDropOp*>(&InDragDropOperation);
		const TArray<FString> Sources = ModuleDropOp->GetElements();

		UTLLRN_RigHierarchy* Hierarchy = InTLLRN_ControlRig->GetHierarchy();
		if (!Hierarchy)
		{
			return DraggedKeys;
		}

		const TArray<FTLLRN_RigConnectorElement*> Connectors = Hierarchy->GetConnectors();
		for (const FString& ModulePathOrConnectorName : Sources)
		{
			if(const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(FTLLRN_RigElementKey(*ModulePathOrConnectorName, ETLLRN_RigElementType::Connector)))
			{
				DraggedKeys.Add(Connector->GetKey());
			}

			if(const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(InTLLRN_ControlRig))
			{
				if(const FTLLRN_RigModuleInstance* ModuleInstance = TLLRN_ModularRig->FindModule(ModulePathOrConnectorName))
				{
					const FString ModuleNameSpace = ModuleInstance->GetNamespace();
					for(const FTLLRN_RigConnectorElement* Connector : Connectors)
					{
						const FString ConnectorNameSpace = Hierarchy->GetNameSpace(Connector->GetKey());
						if(ConnectorNameSpace.Equals(ModuleNameSpace))
						{
							if(Connector->IsPrimary())
							{
								DraggedKeys.Add(Connector->GetKey());
							}
						}
					}
				}
			}
		}
	}

	if(InDragDropOperation.IsOfType<FSchematicGraphNodeDragDropOp>())
	{
		const FSchematicGraphNodeDragDropOp* SchematicGraphNodeDragDropOp = StaticCast<const FSchematicGraphNodeDragDropOp*>(&InDragDropOperation);
		const TArray<const FSchematicGraphNode*> Nodes = SchematicGraphNodeDragDropOp->GetNodes();

		for(const FSchematicGraphNode* Node : Nodes)
		{
			if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ElementKeyNode = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(Node))
			{
				DraggedKeys.Add(ElementKeyNode->GetKey());
			}
		}
	}

	return DraggedKeys;
}

void FTLLRN_ControlRigSchematicModel::HandleSchematicNodeClicked(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const FPointerEvent& InMouseEvent)
{
	const bool bClearSelection = !InMouseEvent.IsShiftDown() && !InMouseEvent.IsControlDown();
	for (const TSharedPtr<FSchematicGraphNode>& Node : Nodes)
	{
		if (bClearSelection)
		{
			Node->SetSelected(Node->GetGuid() == InNode->GetGuid());
		}
		else if(Node->GetGuid() == InNode->GetGuid())
		{
			Node->SetSelected();
		}
	}

	if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node =
			Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode->GetNodeData()))
	{
		if (TLLRN_ControlRigBeingDebuggedPtr.IsValid())
		{
			if (UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy())
			{
				if (UTLLRN_RigHierarchyController* Controller = Hierarchy->GetController())
				{
					bool bSelect = true;
					if(InMouseEvent.IsControlDown())
					{
						if(Hierarchy->IsSelected(Node->GetKey()))
						{
							bSelect = false;
						}
					}
					Controller->SelectElement(Node->GetKey(), bSelect, bClearSelection);
				}
			}
		}
	}
}

void FTLLRN_ControlRigSchematicModel::HandleSchematicBeginDrag(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const TSharedPtr<FDragDropOperation>& InDragDropOperation)
{
	if (!TLLRN_ControlRigBlueprint.IsValid())
	{
		return;
	}

	if (!TLLRN_ControlRigBeingDebuggedPtr.IsValid())
	{
		return;
	}

	FTLLRN_RigElementKey DraggedKey;
	if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ElementKeyNode =
		Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode->GetNodeData()))
	{
		DraggedKey = ElementKeyNode->GetKey();
	}
	else if(InDragDropOperation.IsValid())
	{
		const TArray<FTLLRN_RigElementKey> DraggedKeys = GetElementKeysFromDragDropEvent(*InDragDropOperation.Get(), TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
		if(!DraggedKeys.IsEmpty())
		{
			DraggedKey = DraggedKeys[0];
		}
	}

	OnShowCandidatesForConnector(DraggedKey);
}

void FTLLRN_ControlRigSchematicModel::HandleSchematicEndDrag(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const TSharedPtr<FDragDropOperation>& InDragDropOperation)
{
	OnHideCandidatesForConnector();
}

void FTLLRN_ControlRigSchematicModel::HandleSchematicEnterDrag(SSchematicGraphPanel* InPanel, const TSharedPtr<FDragDropOperation>& InDragDropOperation)
{
	if(!TLLRN_ControlRigBlueprint.IsValid())
	{
		return;
	}
	if(!InDragDropOperation.IsValid())
	{
		return;
	}
	
	const TArray<FTLLRN_RigElementKey> DraggedKeys = GetElementKeysFromDragDropEvent(*InDragDropOperation.Get(), TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
	if(!DraggedKeys.IsEmpty())
	{
		OnShowCandidatesForConnector(DraggedKeys[0]);
		return;
	}

	if(InDragDropOperation->IsOfType<FAssetDragDropOp>())
	{
		const TSharedPtr<FAssetDragDropOp> AssetDragDropOp = StaticCastSharedPtr<FAssetDragDropOp>(InDragDropOperation);
		for (const FAssetData& AssetData : AssetDragDropOp->GetAssets())
		{
			const UClass* AssetClass = AssetData.GetClass();
			if (!AssetClass->IsChildOf(UTLLRN_ControlRigBlueprint::StaticClass()))
			{
				continue;
			}

			if(const UTLLRN_ControlRigBlueprint* AssetBlueprint = Cast<UTLLRN_ControlRigBlueprint>(AssetData.GetAsset()))
			{
				if(AssetBlueprint->IsTLLRN_ControlTLLRN_RigModule())
				{
					if(const FTLLRN_RigModuleConnector* PrimaryConnector = AssetBlueprint->TLLRN_RigModuleSettings.FindPrimaryConnector())
					{
						OnShowCandidatesForConnector(PrimaryConnector);
					}
				}
			}
		}
	}
}

void FTLLRN_ControlRigSchematicModel::HandleSchematicLeaveDrag(SSchematicGraphPanel* InPanel, const TSharedPtr<FDragDropOperation>& InDragDropOperation)
{
	OnHideCandidatesForConnector();
}

void FTLLRN_ControlRigSchematicModel::HandleSchematicCancelDrag(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const TSharedPtr<FDragDropOperation>& InDragDropOperation)
{
	if (!TLLRN_ControlRigBlueprint.IsValid())
	{
		return;
	}

	const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ElementKeyNode =
		Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode->GetNodeData());
	if(ElementKeyNode == nullptr)
	{
		return;
	}

	if (InDragDropOperation.IsValid() && InDragDropOperation->IsOfType<FSchematicGraphNodeDragDropOp>())
	{
		const TSharedPtr<FSchematicGraphNodeDragDropOp> SchematicDragDropOp = StaticCastSharedPtr<FSchematicGraphNodeDragDropOp>(InDragDropOperation);
		const TArray<FGuid> Sources = SchematicDragDropOp->GetElements();
		TArray<FTLLRN_RigElementKey> Keys;
		for(const FGuid& Source : Sources)
		{
			if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* SourceNode = FindNode<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(Source))
			{
				Keys.Add(SourceNode->GetKey());
			}
		}

		FFunctionGraphTask::CreateAndDispatchWhenReady([this, Keys]()
		{
			if (UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
			{
				FScopedTransaction Transaction(LOCTEXT("DisconnectConnector", "Disconnect Connector"));
				TLLRN_ControlRigBlueprint->Modify();
				for(const FTLLRN_RigElementKey& Key : Keys)
				{
					Controller->DisconnectConnector(Key);
				}
			}
		}, TStatId(), NULL, ENamedThreads::GameThread);
	}
}

void FTLLRN_ControlRigSchematicModel::HandleSchematicDrop(SSchematicGraphPanel* InPanel, SSchematicGraphNode* InNode, const FDragDropEvent& InDragDropEvent)
{
	if (!TLLRN_ControlRigBlueprint.IsValid())
	{
		return;
	}

	struct Local
	{
		static void CollectTargetKeys(TArray<FTLLRN_RigElementKey>& OutKeys, const FSchematicGraphNode* InNode)
		{
			check(InNode);
			
			if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ElementKeyNode = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(InNode))
			{
				OutKeys.Add(ElementKeyNode->GetKey());
			}

			if(const FSchematicGraphGroupNode* GroupNode = Cast<FSchematicGraphGroupNode>(InNode))
			{
				for(int32 Index = 0; Index < GroupNode->GetNumChildNodes(); Index++)
				{
					if(const FSchematicGraphNode* ChildNode = GroupNode->GetChildNode(Index))
					{
						CollectTargetKeys(OutKeys, ChildNode);
					}
				}
			}
		}
	};

	TArray<FTLLRN_RigElementKey> TargetKeys;
	TArray<TSharedPtr<FSchematicGraphNode>> SelectedNodesShared = GetSelectedNodes();
	TArray<FSchematicGraphNode*> SelectedNodes;
	SelectedNodes.Reserve(SelectedNodesShared.Num());
	bool bApplyToSelection = false;
	for (TSharedPtr<FSchematicGraphNode>& Node : SelectedNodesShared)
	{
		if (Node.IsValid())
		{
			SelectedNodes.AddUnique(Node.Get());
			if (Node.Get() == InNode->GetNodeData())
			{
				bApplyToSelection = true;
			}
		}
	}

	// If the dropped target node is not part of the selection, ignore the selection
	if (!bApplyToSelection)
	{
		SelectedNodes.Reset();
		SelectedNodes.Add(InNode->GetNodeData());
	}

	for (FSchematicGraphNode* Node : SelectedNodes)
	{
		TArray<FTLLRN_RigElementKey> NodeTargetKeys;
		Local::CollectTargetKeys(NodeTargetKeys, Node);
		TargetKeys.Append(NodeTargetKeys);
	}
	
	UTLLRN_ControlRig* TLLRN_ControlRig = TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig();
	if (!TLLRN_ControlRig)
	{
		return;
	}

	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
	if (!Hierarchy)
	{
		return;
	}

	TSharedPtr<FAssetDragDropOp> AssetDragDropOp = InDragDropEvent.GetOperationAs<FAssetDragDropOp>();
	TSharedPtr<FSchematicGraphNodeDragDropOp> SchematicDragDropOp = InDragDropEvent.GetOperationAs<FSchematicGraphNodeDragDropOp>();
	TSharedPtr<FTLLRN_ModularTLLRN_RigModuleDragDropOp> ModuleDragDropOperation = InDragDropEvent.GetOperationAs<FTLLRN_ModularTLLRN_RigModuleDragDropOp>();
	if (AssetDragDropOp.IsValid())
	{
		for (const FAssetData& AssetData : AssetDragDropOp->GetAssets())
		{
			UClass* AssetClass = AssetData.GetClass();
			if (!AssetClass->IsChildOf(UTLLRN_ControlRigBlueprint::StaticClass()))
			{
				continue;
			}

			if(UTLLRN_ControlRigBlueprint* AssetBlueprint = Cast<UTLLRN_ControlRigBlueprint>(AssetData.GetAsset()))
			{
				FFunctionGraphTask::CreateAndDispatchWhenReady([this, AssetBlueprint, TargetKeys]()
				{
					if (UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
					{
						UTLLRN_ModularRig* TLLRN_ControlRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
						if (!TLLRN_ControlRig)
						{
							return;
						}

						UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
						if (!Hierarchy)
						{
							return;
						}

						FScopedTransaction Transaction(LOCTEXT("AddAndConnectModule", "Add and Connect Module"));

						for(const FTLLRN_RigElementKey& TargetKey : TargetKeys)
						{
							const FName ModuleName = Controller->GetSafeNewName(FString(), FTLLRN_RigName(AssetBlueprint->TLLRN_RigModuleSettings.Identifier.Name));
							const FString ModulePath = Controller->AddModule(ModuleName, AssetBlueprint->GetTLLRN_ControlRigClass(), FString());
							if(!ModulePath.IsEmpty())
							{
								FTLLRN_RigElementKey PrimaryConnectorKey;
								TArray<FTLLRN_RigConnectorElement*> Connectors = Hierarchy->GetElementsOfType<FTLLRN_RigConnectorElement>();
								for (FTLLRN_RigConnectorElement* Connector : Connectors)
								{
									if (Connector->IsPrimary())
									{
										FString Path, Name;
										(void)UTLLRN_RigHierarchy::SplitNameSpace(Connector->GetName(), &Path, &Name);
										if (Path == ModulePath)
										{
											PrimaryConnectorKey = Connector->GetKey();
											break;
										}
									}
								}
								Controller->ConnectConnectorToElement(PrimaryConnectorKey, TargetKey, true, TLLRN_ControlRig->GetTLLRN_ModularRigSettings().bAutoResolve);
							}
						}

						ClearSelection();
					}
				}, TStatId(), NULL, ENamedThreads::GameThread);
			}
		}
	}
	else if(SchematicDragDropOp.IsValid())
	{
		const TArray<FGuid> Sources = SchematicDragDropOp->GetElements();
		FFunctionGraphTask::CreateAndDispatchWhenReady([this, Sources, TargetKeys]()
		{
			UTLLRN_ModularRig* TLLRN_ControlRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
			if (!TLLRN_ControlRig)
			{
				return;
			}

			UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
			if (!Hierarchy)
			{
				return;
			}

			if (UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
			{
				for (const TPair<FTLLRN_RigElementKey, FGuid> Pair : TLLRN_RigElementKeyToGuid)
				{
					if(Sources.Contains(Pair.Value))
					{
						if (Hierarchy->Find<FTLLRN_RigConnectorElement>(Pair.Key))
						{
							for(const FTLLRN_RigElementKey& TargetKey : TargetKeys)
							{
								if(Controller->ConnectConnectorToElement(Pair.Key, TargetKey, true, TLLRN_ControlRig->GetTLLRN_ModularRigSettings().bAutoResolve))
								{
									break;
								}
							}
							break;
						}
					}
				}

				ClearSelection();
			}
		}, TStatId(), NULL, ENamedThreads::GameThread);
	}
	else if(ModuleDragDropOperation.IsValid())
	{
		const TArray<FString> Sources = ModuleDragDropOperation->GetElements();
		FFunctionGraphTask::CreateAndDispatchWhenReady([this, Sources, TargetKeys]()
		{
			UTLLRN_ModularRig* TLLRN_ControlRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigBlueprint->GetDebuggedTLLRN_ControlRig());
			if (!TLLRN_ControlRig)
			{
				return;
			}

			UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRig->GetHierarchy();
			if (!Hierarchy)
			{
				return;
			}

			if (UTLLRN_ModularTLLRN_RigController* Controller = TLLRN_ControlRigBlueprint->GetTLLRN_ModularTLLRN_RigController())
			{
				const TArray<FTLLRN_RigConnectorElement*> Connectors = Hierarchy->GetConnectors();
				for (const FString& ModulePathOrConnectorName : Sources)
				{
					if(const FTLLRN_RigConnectorElement* Connector = Hierarchy->Find<FTLLRN_RigConnectorElement>(FTLLRN_RigElementKey(*ModulePathOrConnectorName, ETLLRN_RigElementType::Connector)))
					{
						for(const FTLLRN_RigElementKey& TargetKey : TargetKeys)
						{
							if(Controller->ConnectConnectorToElement(Connector->GetKey(), TargetKey, true, TLLRN_ControlRig->GetTLLRN_ModularRigSettings().bAutoResolve))
							{
								break;
							}
						}
						ClearSelection();
						return;
					}
					if(const FTLLRN_RigModuleReference* Module = Controller->FindModule(ModulePathOrConnectorName))
					{
						const FString ModuleNameSpace = Module->GetNamespace();
						for(const FTLLRN_RigConnectorElement* Connector : Connectors)
						{
							const FString ConnectorNameSpace = Hierarchy->GetNameSpace(Connector->GetKey());
							if(ConnectorNameSpace.Equals(ModuleNameSpace))
							{
								if(Connector->IsPrimary())
								{
									const FTLLRN_RigElementKey ConnectorKey = Connector->GetKey();
									for(const FTLLRN_RigElementKey& TargetKey : TargetKeys)
									{
										if(Controller->ConnectConnectorToElement(ConnectorKey, TargetKey, true, TLLRN_ControlRig->GetTLLRN_ModularRigSettings().bAutoResolve))
										{
											break;
										}
									}
									ClearSelection();
									return;
								}
							}
						}
					}
				}
			}
		}, TStatId(), NULL, ENamedThreads::GameThread);
	}
}

void FTLLRN_ControlRigSchematicModel::HandlePostConstruction(UTLLRN_ControlRig* Subject, const FName& InEventName)
{
	UpdateTLLRN_ControlRigContent();
}

bool FTLLRN_ControlRigSchematicModel::IsConnectorResolved(const FTLLRN_RigElementKey& InConnectorKey, FTLLRN_RigElementKey* OutKey) const
{
	if(TLLRN_ControlRigBlueprint.IsValid() && InConnectorKey.Type == ETLLRN_RigElementType::Connector)
	{
		const FTLLRN_RigElementKey TargetKey = TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.Connections.FindTargetFromConnector(InConnectorKey);
		if(TargetKey.IsValid())
		{
			// make sure the target exists
			if(TLLRN_ControlRigBeingDebuggedPtr.IsValid())
			{
				if(const UTLLRN_RigHierarchy* DebuggedHierarchy = TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy())
				{
					if(!DebuggedHierarchy->Contains(TargetKey))
					{
						return false;
					}
				}
			}
			
			if(OutKey)
			{
				*OutKey = TargetKey;
			}
			return true;
		}
	}
	return false;
}

void FTLLRN_ControlRigSchematicModel::OnShowCandidatesForConnector(const FTLLRN_RigElementKey& InConnectorKey)
{
	if(!InConnectorKey.IsValid())
	{
		return;
	}
	
	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy();
	if (!Hierarchy)
	{
		return;
	}

	FTLLRN_RigBaseElement* Element = Hierarchy->Find(InConnectorKey);
	if (!Element)
	{
		return;
	}
	
	const FTLLRN_RigConnectorElement* Connector = Cast<FTLLRN_RigConnectorElement>(Element);
	if (!Connector)
	{
		return;
	}

	const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(TLLRN_ControlRigBeingDebuggedPtr);
	if (!TLLRN_ModularRig)
	{
		return;
	}

	const FString ModulePath = Hierarchy->GetModulePath(InConnectorKey);
	const FTLLRN_RigModuleInstance* ModuleInstance = TLLRN_ModularRig->FindModule(ModulePath);
	if (!ModuleInstance)
	{
		return;
	}

	if(const FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = FindElementKeyNode(InConnectorKey))
	{
		// close all expanded groups
		FGuid ParentGuid = Node->GetParentNodeGuid();
		while(ParentGuid.IsValid())
		{
			if(FSchematicGraphNode* ParentNode = FindNode(ParentGuid))
			{
				if(FSchematicGraphGroupNode* GroupNode = Cast<FSchematicGraphGroupNode>(ParentNode))
				{
					GroupNode->SetExpanded(false);
				}
				ParentGuid = ParentNode->GetParentNodeGuid();
			}
			else
			{
				break;
			}
		}
	}
	
	const UTLLRN_ModularRigRuleManager* RuleManager = Hierarchy->GetRuleManager();
	const FTLLRN_ModularRigResolveResult Result = RuleManager->FindMatches(Connector, ModuleInstance, TLLRN_ControlRigBeingDebuggedPtr->GetElementKeyRedirector());
	OnShowCandidatesForMatches(Result);
}

void FTLLRN_ControlRigSchematicModel::OnShowCandidatesForConnector(const FTLLRN_RigModuleConnector* InModuleConnector)
{
	check(InModuleConnector);
	
	UTLLRN_RigHierarchy* Hierarchy = TLLRN_ControlRigBeingDebuggedPtr->GetHierarchy();
	if (!Hierarchy)
	{
		return;
	}
	
	const UTLLRN_ModularRigRuleManager* RuleManager = Hierarchy->GetRuleManager();
	const FTLLRN_ModularRigResolveResult Result = RuleManager->FindMatches(InModuleConnector);
	OnShowCandidatesForMatches(Result);
}

void FTLLRN_ControlRigSchematicModel::OnShowCandidatesForMatches(const FTLLRN_ModularRigResolveResult& InMatches)
{
	const TArray<FTLLRN_RigElementResolveResult> Matches = InMatches.GetMatches();

	for (const FTLLRN_RigElementResolveResult& Match : Matches)
	{
		// Create a temporary node that will be active only while this drag operation exists
		if (!ContainsElementKeyNode(Match.GetKey()))
		{
			FSchematicGraphNode* NewNode = AddElementKeyNode(Match.GetKey());
			NewNode->SetScaleOffset(0.6f);
			TemporaryNodeGuids.Add(NewNode->GetGuid());
		}
	}

	// Fade all the unmatched nodes
	for (const TSharedPtr<FSchematicGraphNode>& Node : Nodes)
	{
		if(FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* ExistingElementKeyNode = Cast<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(Node.Get()))
		{
			PreDragVisibilityPerNode.Add(Node->GetGuid(), ExistingElementKeyNode->Visibility);
			ExistingElementKeyNode->SetVisibility(Matches.ContainsByPredicate([ExistingElementKeyNode](const FTLLRN_RigElementResolveResult& Match)
			{
				return ExistingElementKeyNode->GetKey() == Match.GetKey();
			}) ? ESchematicGraphVisibility::Visible : ESchematicGraphVisibility::Hidden);
		}
	};
}

void FTLLRN_ControlRigSchematicModel::OnHideCandidatesForConnector()
{
	if(TemporaryNodeGuids.IsEmpty() && PreDragVisibilityPerNode.IsEmpty())
	{
		return;
	}
	
	for (FGuid& TempNodeGuid : TemporaryNodeGuids)
	{
		RemoveNode(TempNodeGuid);
	}
	TemporaryNodeGuids.Reset();

	for(const TPair<FGuid, ESchematicGraphVisibility::Type>& Pair : PreDragVisibilityPerNode)
	{
		if(FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = FindNode<FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode>(Pair.Key))
		{
			Node->Visibility = Pair.Value;
		}
	}
	PreDragVisibilityPerNode.Reset();

	UpdateElementKeyLinks();
}

void FTLLRN_ControlRigSchematicModel::OnHierarchyModified(ETLLRN_RigHierarchyNotification InNotif, UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement)
{
	switch(InNotif)
	{
		case ETLLRN_RigHierarchyNotification::ElementSelected:
		{
			if(FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = FindElementKeyNode(InElement->GetKey()))
			{
				if(InHierarchy->GetSelectedKeys().Num() == 1)
				{
					if(FSchematicGraphGroupNode* GroupNode = Node->GetGroupNode())
					{
						GroupNode->SetExpanded(true);
					}
				}
				Node->SetSelected(true);
			}
			break;
		}
		case ETLLRN_RigHierarchyNotification::ElementDeselected:
		{
			if(FTLLRN_ControlRigSchematicTLLRN_RigElementKeyNode* Node = FindElementKeyNode(InElement->GetKey()))
			{
				Node->SetExpanded(false);
				Node->SetSelected(false);
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

#undef LOCTEXT_NAMESPACE
