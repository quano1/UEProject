// Copyright Epic Games, Inc. All Rights Reserved.

#include "Graph/TLLRN_ControlRigGraphPanelPinFactory.h"
#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "Graph/TLLRN_ControlRigGraphNode.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "Widgets/SRigVMGraphPinNameList.h"
#include "Widgets/SRigVMGraphPinCurveFloat.h"
#include "Widgets/SRigVMGraphPinVariableName.h"
#include "Widgets/SRigVMGraphPinVariableBinding.h"
#include "Widgets/SRigVMGraphPinUserDataNameSpace.h"
#include "Widgets/SRigVMGraphPinUserDataPath.h"
#include "KismetPins/SGraphPinExec.h"
#include "SGraphPinComboBox.h"
#include "TLLRN_ControlRig.h"
#include "NodeFactory.h"
#include "EdGraphSchema_K2.h"
#include "Curves/CurveFloat.h"
#include "RigVMModel/Nodes/RigVMUnitNode.h"
#include "RigVMCore/RigVMExecuteContext.h"
#include "Units/Execution/TLLRN_RigUnit_DynamicHierarchy.h"
#include "Units/Hierarchy/TLLRN_RigUnit_Metadata.h"
#include "TLLRN_ControlTLLRN_RigElementDetails.h"
#include "IPropertyAccessEditor.h"

TSharedPtr<SGraphPin> FTLLRN_ControlRigGraphPanelPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	if(InPin == nullptr)
	{
		return nullptr;
	}

	// if the graph we are looking at is not a control rig graph - let's not do this
	if (const UEdGraphNode* OwningNode = InPin->GetOwningNode())
	{
		const UTLLRN_ControlRigGraph* EdGraph = Cast<UTLLRN_ControlRigGraph>(OwningNode->GetGraph());
		if(!EdGraph)
		{
			return nullptr;
		}
	}
	
	TSharedPtr<SGraphPin> InternalResult = CreatePin_Internal(InPin);
	if(InternalResult.IsValid())
	{
		return InternalResult;
	}

	TSharedPtr<SGraphPin> K2PinWidget = FNodeFactory::CreateK2PinWidget(InPin);
	if(K2PinWidget.IsValid())
	{
		if(InPin->Direction == EEdGraphPinDirection::EGPD_Input)
		{
			// if we are an enum pin - and we are inside a TLLRN_RigElementKey,
			// let's remove the "all" entry.
			if(InPin->PinType.PinSubCategoryObject == StaticEnum<ETLLRN_RigElementType>())
			{
				if(InPin->ParentPin)
				{
					if(InPin->ParentPin->PinType.PinSubCategoryObject == FTLLRN_RigElementKey::StaticStruct())
					{
						TSharedPtr<SWidget> ValueWidget = K2PinWidget->GetValueWidget();
						if(ValueWidget.IsValid())
						{
							if(TSharedPtr<SPinComboBox> EnumCombo = StaticCastSharedPtr<SPinComboBox>(ValueWidget))
							{
								if(EnumCombo.IsValid())
								{
									EnumCombo->RemoveItemByIndex(StaticEnum<ETLLRN_RigElementType>()->GetIndexByValue((int64)ETLLRN_RigElementType::All));
								}
							}
						}
					}
				}
			}

			const UEnum* TLLRN_RigControlTransformChannelEnum = StaticEnum<ETLLRN_RigControlTransformChannel>();
			if (InPin->PinType.PinSubCategoryObject == TLLRN_RigControlTransformChannelEnum)
			{
				TSharedPtr<SWidget> ValueWidget = K2PinWidget->GetValueWidget();
				if(ValueWidget.IsValid())
				{
					if(TSharedPtr<SPinComboBox> EnumCombo = StaticCastSharedPtr<SPinComboBox>(ValueWidget))
					{
						if(EnumCombo.IsValid())
						{
							if (const UTLLRN_ControlRigGraphNode* RigNode = Cast<UTLLRN_ControlRigGraphNode>(InPin->GetOwningNode()))
							{
								if (const URigVMPin* ModelPin = RigNode->GetModelPinFromPinPath(InPin->GetName()))
								{
									if(const URigVMUnitNode* UnitNode = Cast<URigVMUnitNode>(ModelPin->GetNode()))
									{
										if(UnitNode->GetScriptStruct() &&
											UnitNode->GetScriptStruct()->IsChildOf(FTLLRN_RigUnit_HierarchyAddControlElement::StaticStruct()))
										{
											const TSharedPtr<FStructOnScope> StructInstanceScope = UnitNode->ConstructStructInstance();
											const FTLLRN_RigUnit_HierarchyAddControlElement* StructInstance = 
												(const FTLLRN_RigUnit_HierarchyAddControlElement*)StructInstanceScope->GetStructMemory();

											if(const TArray<ETLLRN_RigControlTransformChannel>* VisibleChannels =
												FTLLRN_RigControlTransformChannelDetails::GetVisibleChannelsForControlType(StructInstance->GetControlTypeToSpawn()))
											{
												for(int32 Index = 0; Index < TLLRN_RigControlTransformChannelEnum->NumEnums(); Index++)
												{
													const ETLLRN_RigControlTransformChannel Value =
														(ETLLRN_RigControlTransformChannel)TLLRN_RigControlTransformChannelEnum->GetValueByIndex(Index);
													if(!VisibleChannels->Contains(Value))
													{
														EnumCombo->RemoveItemByIndex(Index);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return K2PinWidget;
}

FName FTLLRN_ControlRigGraphPanelPinFactory::GetFactoryName() const
{
	return UTLLRN_ControlRigBlueprint::TLLRN_ControlRigPanelNodeFactoryName;
}

TSharedPtr<SGraphPin> FTLLRN_ControlRigGraphPanelPinFactory::CreatePin_Internal(UEdGraphPin* InPin) const
{
	TSharedPtr<SGraphPin> SuperResult = FRigVMEdGraphPanelPinFactory::CreatePin_Internal(InPin);
	if(SuperResult.IsValid())
	{
		return SuperResult;
	}

	if (InPin)
	{
		if (const UEdGraphNode* OwningNode = InPin->GetOwningNode())
		{
			// only create pins within control rig graphs
			if(const URigVMEdGraph* EdGraph = Cast<URigVMEdGraph>(OwningNode->GetGraph()))
			{
				if ((Cast<UTLLRN_ControlRigGraph>(EdGraph) == nullptr) &&
					!EdGraph->IsPreviewGraph())
				{
					return nullptr;
				}
			}
		}

		if (UTLLRN_ControlRigGraphNode* RigNode = Cast<UTLLRN_ControlRigGraphNode>(InPin->GetOwningNode()))
		{
			UTLLRN_ControlRigGraph* RigGraph = Cast<UTLLRN_ControlRigGraph>(RigNode->GetGraph());

			if (URigVMPin* ModelPin = RigNode->GetModelPinFromPinPath(InPin->GetName()))
			{
				FName CustomWidgetName = ModelPin->GetCustomWidgetName();
				if (CustomWidgetName == TEXT("BoneName"))
				{
					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.OnGetNameFromSelection_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetSelectedElementsNameList)
						.OnGetNameListContent_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetBoneNameList)
						.OnGetSelectedClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleGetSelectedClicked)
						.OnBrowseClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleBrowseClicked);
			}
				else if (CustomWidgetName == TEXT("ControlName"))
				{
					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.OnGetNameFromSelection_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetSelectedElementsNameList)
						.OnGetNameListContent_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetControlNameListWithoutAnimationChannels)
						.OnGetNameListContentForValidation_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetControlNameList)
						.OnGetSelectedClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleGetSelectedClicked)
						.OnBrowseClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleBrowseClicked);
				}
				else if (CustomWidgetName == TEXT("SpaceName") || CustomWidgetName == TEXT("NullName"))
				{
					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.OnGetNameFromSelection_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetSelectedElementsNameList)
						.OnGetNameListContent_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetNullNameList)
						.OnGetSelectedClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleGetSelectedClicked)
						.OnBrowseClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleBrowseClicked);
				}
				else if (CustomWidgetName == TEXT("CurveName"))
				{
					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.OnGetNameFromSelection_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetSelectedElementsNameList)
						.OnGetNameListContent_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetCurveNameList);
				}
				else if (CustomWidgetName == TEXT("ElementName"))
				{
					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.OnGetNameFromSelection_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetSelectedElementsNameList)
						.OnGetNameListContent_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetElementNameList)
						.OnGetSelectedClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleGetSelectedClicked)
						.OnBrowseClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleBrowseClicked);
				}
				else if (CustomWidgetName == TEXT("ConnectorName"))
				{
					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.OnGetNameFromSelection_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetSelectedElementsNameList)
						.OnGetNameListContent_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetConnectorNameList)
						.OnGetSelectedClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleGetSelectedClicked)
						.OnBrowseClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleBrowseClicked);
				}
				else if (CustomWidgetName == TEXT("DrawingName"))
				{
					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.OnGetNameFromSelection_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetSelectedElementsNameList)
						.OnGetNameListContent_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetDrawingNameList);
				}
				else if (CustomWidgetName == TEXT("ShapeName"))
				{
					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.OnGetNameListContent_UObject(RigGraph, &UTLLRN_ControlRigGraph::GetShapeNameList);
				}
				else if (CustomWidgetName == TEXT("AnimationChannelName"))
				{
					struct FCachedAnimationChannelNames
					{
						int32 TopologyVersion;
						TSharedPtr<TArray<TSharedPtr<FRigVMStringWithTag>>> Names;
						
						FCachedAnimationChannelNames()
						: TopologyVersion(INDEX_NONE)
						{}
					};
					
					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.OnGetNameListContent_Lambda([RigGraph](const URigVMPin* InPin)
						{
							if (const UTLLRN_ControlRigBlueprint* Blueprint = RigGraph->GetTypedOuter<UTLLRN_ControlRigBlueprint>())
							{
								FTLLRN_RigElementKey ControlKey;

								// find the pin that holds the control
								for(URigVMPin* Pin : InPin->GetRootPin()->GetNode()->GetPins())
								{
									if(Pin->GetCPPType() == RigVMTypeUtils::FNameType && Pin->GetCustomWidgetName() == TEXT("ControlName"))
									{
										const FString DefaultValue = Pin->GetDefaultValue();
										const FName ControlName = DefaultValue.IsEmpty() ? FName(NAME_None) : FName(*DefaultValue);
										ControlKey = FTLLRN_RigElementKey(ControlName, ETLLRN_RigElementType::Control);
										break;
									}

									if(Pin->GetCPPType() == RigVMTypeUtils::GetUniqueStructTypeName(FTLLRN_RigElementKey::StaticStruct()))
									{
										const FString DefaultValue = Pin->GetDefaultValue();
										if(!DefaultValue.IsEmpty())
										{
											FTLLRN_RigElementKey::StaticStruct()->ImportText(*DefaultValue, &ControlKey, nullptr, EPropertyPortFlags::PPF_None, nullptr, FTLLRN_RigElementKey::StaticStruct()->GetName(), true);
										}
										break;
									}
								}

								const UTLLRN_RigHierarchy* Hierarchy = Blueprint->Hierarchy;
								if (UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(Blueprint->GetObjectBeingDebugged()))
								{
									Hierarchy = TLLRN_ControlRig->GetHierarchy();
								}
								
								if(Hierarchy->Find<FTLLRN_RigControlElement>(ControlKey) == nullptr)
								{
									ControlKey.Reset();
								}

								const FString MapHash = ControlKey.IsValid() ?
									Blueprint->GetPathName() + TEXT("|") + ControlKey.Name.ToString() :
									FName(NAME_None).ToString();

								static TMap<FString, FCachedAnimationChannelNames> ChannelNameLists;
								FCachedAnimationChannelNames& ChannelNames = ChannelNameLists.FindOrAdd(MapHash);

								bool bRefreshList = !ChannelNames.Names.IsValid();

								if(!bRefreshList)
								{
									const int32 TopologyVersion = Hierarchy->GetTopologyVersion();
									if(ChannelNames.TopologyVersion != TopologyVersion)
									{
										bRefreshList = true;
										ChannelNames.TopologyVersion = TopologyVersion;
									}
								}

								if(bRefreshList)
								{
									if(!ChannelNames.Names.IsValid())
									{
										ChannelNames.Names = MakeShareable(new TArray<TSharedPtr<FRigVMStringWithTag>>());
									}
									ChannelNames.Names->Reset();
									ChannelNames.Names->Add(MakeShareable(new FRigVMStringWithTag(FName(NAME_None).ToString())));

									if(const FTLLRN_RigControlElement* ControlElement = Hierarchy->Find<FTLLRN_RigControlElement>(ControlKey))
									{
										for(const FTLLRN_RigBaseElement* Child : Hierarchy->GetChildren(ControlElement))
										{
											if(const FTLLRN_RigControlElement* ChildControl = Cast<FTLLRN_RigControlElement>(Child))
											{
												if(ChildControl->IsAnimationChannel())
												{
													ChannelNames.Names->Add(MakeShareable(new FRigVMStringWithTag(ChildControl->GetDisplayName().ToString())));
												}
											}
										}
									}
								}
								return ChannelNames.Names.Get();
							}

							static TArray<TSharedPtr<FRigVMStringWithTag>> EmptyNameList;
							return &EmptyNameList;
						})
						.OnGetSelectedClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleGetSelectedClicked)
						.OnBrowseClicked_UObject(RigGraph, &UTLLRN_ControlRigGraph::HandleBrowseClicked);
				}
				else if (CustomWidgetName == TEXT("MetadataName"))
				{
					struct FCachedMetadataNames
					{
						int32 MetadataVersion;
						TSharedPtr<TArray<TSharedPtr<FRigVMStringWithTag>>> Names;
						
						FCachedMetadataNames()
						: MetadataVersion(INDEX_NONE)
						{}
					};

					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.SearchHintText(NSLOCTEXT("FTLLRN_ControlRigGraphPanelPinFactory", "MetadataName", "Metadata Name"))
						.AllowUserProvidedText(true)
						.EnableNameListCache(false)
						.OnGetNameListContent_Lambda([RigGraph](const URigVMPin* InPin)
						{
							if (const UTLLRN_ControlRigBlueprint* Blueprint = RigGraph->GetTypedOuter<UTLLRN_ControlRigBlueprint>())
							{
								if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(Blueprint->GetObjectBeingDebugged()))
								{
									const FString MapHash = Blueprint->GetPathName();
									const int32 MetadataVersion = TLLRN_ControlRig->GetHierarchy()->GetMetadataVersion();
									
									ETLLRN_RigMetaDataNameSpace NameSpace = ETLLRN_RigMetaDataNameSpace::None;
									if (const URigVMNode* ModelNode = InPin->GetNode())
									{
										if(const URigVMPin* NameSpacePin = ModelNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HasMetadata, NameSpace)))
										{
											NameSpace = (ETLLRN_RigMetaDataNameSpace)StaticEnum<ETLLRN_RigMetaDataNameSpace>()->GetValueByNameString(NameSpacePin->GetDefaultValue());
										}
									}

									const bool bUseShortNames = NameSpace != ETLLRN_RigMetaDataNameSpace::None;

									static TMap<FString, FCachedMetadataNames> MetadataNameLists;
									FCachedMetadataNames& MetadataNames = MetadataNameLists.FindOrAdd(MapHash);

									if(MetadataNames.MetadataVersion != MetadataVersion)
									{
										TArray<FName> Names;
										for(int32 ElementIndex=0; ElementIndex < TLLRN_ControlRig->GetHierarchy()->Num(); ElementIndex++)
										{
											const FTLLRN_RigBaseElement* OtherElement = TLLRN_ControlRig->GetHierarchy()->Get(ElementIndex);
											for(FName MetadataName: TLLRN_ControlRig->GetHierarchy()->GetMetadataNames(OtherElement->GetKey()))
											{
												Names.AddUnique(MetadataName);
											}
										}

										if(!MetadataNames.Names.IsValid())
										{
											MetadataNames.Names = MakeShareable(new TArray<TSharedPtr<FRigVMStringWithTag>>());
										}
										MetadataNames.Names->Reset();

										for(const FName& Name : Names)
										{
											FString NameString = Name.ToString();
											if(bUseShortNames)
											{
												int32 Index = INDEX_NONE;
												if(NameString.FindLastChar(TEXT(':'), Index))
												{
													NameString.MidInline(Index + 1);
												}
											}
											MetadataNames.Names->Add(MakeShareable(new FRigVMStringWithTag(NameString)));
										}

										MetadataNames.Names->Sort([](const TSharedPtr<FRigVMStringWithTag>& A, const TSharedPtr<FRigVMStringWithTag>& B)
										{
											return A.Get() > B.Get();
										});
										MetadataNames.Names->Insert(MakeShareable(new FRigVMStringWithTag(FName(NAME_None).ToString())), 0);

										MetadataNames.MetadataVersion = MetadataVersion;
									}
									return MetadataNames.Names.Get();
								}
							}

							static TArray<TSharedPtr<FRigVMStringWithTag>> EmptyNameList;
							return &EmptyNameList;
						});
				}
				else if (CustomWidgetName == TEXT("MetadataTagName"))
				{
					struct FCachedMetadataTagNames
					{
						int32 MetadataTagVersion;
						TSharedPtr<TArray<TSharedPtr<FRigVMStringWithTag>>> Names;
						FCachedMetadataTagNames()
						: MetadataTagVersion(INDEX_NONE)
						{}
					};

					return SNew(SRigVMGraphPinNameList, InPin)
						.ModelPin(ModelPin)
						.SearchHintText(NSLOCTEXT("FTLLRN_ControlRigGraphPanelPinFactory", "TagName", "Tag Name"))
						.AllowUserProvidedText(true)
						.EnableNameListCache(false)
						.OnGetNameListContent_Lambda([RigGraph](const URigVMPin* InPin)
						{
							if (const UTLLRN_ControlRigBlueprint* Blueprint = RigGraph->GetTypedOuter<UTLLRN_ControlRigBlueprint>())
							{
								if(UTLLRN_ControlRig* TLLRN_ControlRig = Cast<UTLLRN_ControlRig>(Blueprint->GetObjectBeingDebugged()))
								{
									const FString MapHash = Blueprint->GetPathName();
									const int32 MetadataTagVersion = TLLRN_ControlRig->GetHierarchy()->GetMetadataTagVersion(); 

									ETLLRN_RigMetaDataNameSpace NameSpace = ETLLRN_RigMetaDataNameSpace::None;
									if (const URigVMNode* ModelNode = InPin->GetNode())
									{
										if(const URigVMPin* NameSpacePin = ModelNode->FindPin(GET_MEMBER_NAME_STRING_CHECKED(FTLLRN_RigUnit_HasMetadata, NameSpace)))
										{
											NameSpace = (ETLLRN_RigMetaDataNameSpace)StaticEnum<ETLLRN_RigMetaDataNameSpace>()->GetValueByNameString(NameSpacePin->GetDefaultValue());
										}
									}
									const bool bUseShortNames = NameSpace != ETLLRN_RigMetaDataNameSpace::None;

									static TMap<FString, FCachedMetadataTagNames> MetadataTagNameLists;
									FCachedMetadataTagNames& MetadataTagNames = MetadataTagNameLists.FindOrAdd(MapHash);

									if(MetadataTagNames.MetadataTagVersion != MetadataTagVersion)
									{
										TArray<FName> Tags; 
										for(int32 ElementIndex=0; ElementIndex < TLLRN_ControlRig->GetHierarchy()->Num(); ElementIndex++)
										{
											const FTLLRN_RigBaseElement* Element = TLLRN_ControlRig->GetHierarchy()->Get(ElementIndex);
											if(const FTLLRN_RigNameArrayMetadata* Md = Cast<FTLLRN_RigNameArrayMetadata>(Element->GetMetadata(UTLLRN_RigHierarchy::TagMetadataName, ETLLRN_RigMetadataType::NameArray)))
											{
												for(const FName& Tag : Md->GetValue())
												{
													Tags.AddUnique(Tag);
												}
											}
										}

										if(!MetadataTagNames.Names.IsValid())
										{
											MetadataTagNames.Names = MakeShareable(new TArray<TSharedPtr<FRigVMStringWithTag>>());
										}
										MetadataTagNames.Names->Reset();

										for(const FName& Tag : Tags)
										{
											FString TagString = Tag.ToString();
											if(bUseShortNames)
											{
												int32 Index = INDEX_NONE;
												if(TagString.FindLastChar(TEXT(':'), Index))
												{
													TagString.MidInline(Index + 1);
												}
											}
											MetadataTagNames.Names->Add(MakeShareable(new FRigVMStringWithTag(TagString)));
										}
										MetadataTagNames.Names->Sort([](const TSharedPtr<FRigVMStringWithTag>& A, const TSharedPtr<FRigVMStringWithTag>& B)
										{
											return A.Get() > B.Get();
										});
										MetadataTagNames.Names->Insert(MakeShareable(new FRigVMStringWithTag(FName(NAME_None).ToString())), 0);

										MetadataTagNames.MetadataTagVersion = MetadataTagVersion;
									}

									return MetadataTagNames.Names.Get();
								}
							}

							static TArray<TSharedPtr<FRigVMStringWithTag>> EmptyNameList;
							return &EmptyNameList;
						});
				}
			}
		}
	}

	return nullptr;
}
