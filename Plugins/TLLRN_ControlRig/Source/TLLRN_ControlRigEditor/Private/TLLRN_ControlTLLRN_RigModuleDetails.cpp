// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlTLLRN_RigModuleDetails.h"
#include "Widgets/SWidget.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "TLLRN_ModularTLLRN_RigController.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlRigEditorStyle.h"
#include "TLLRN_ControlTLLRN_RigElementDetails.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "SEnumCombo.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Styling/AppStyle.h"
#include "Editor/STLLRN_ModularRigTreeView.h"
#include "StructViewerFilter.h"
#include "StructViewerModule.h"
#include "Features/IModularFeatures.h"
#include "IPropertyAccessEditor.h"
#include "TLLRN_ModularRigRuleManager.h"
#include "ScopedTransaction.h"
#include "Editor/STLLRN_RigHierarchyTreeView.h"
#include "Widgets/SRigVMVariantTagWidget.h"
#include "Algo/Sort.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlTLLRN_RigModuleDetails"

static const FText TLLRN_ControlTLLRN_RigModuleDetailsMultipleValues = LOCTEXT("MultipleValues", "Multiple Values");

static void TLLRN_RigModuleDetails_GetCustomizedInfo(TSharedRef<IPropertyHandle> InStructPropertyHandle, UTLLRN_ControlRigBlueprint*& OutBlueprint)
{
	TArray<UObject*> Objects;
	InStructPropertyHandle->GetOuterObjects(Objects);
	for (UObject* Object : Objects)
	{
		if (Object->IsA<UTLLRN_ControlRigBlueprint>())
		{
			OutBlueprint = CastChecked<UTLLRN_ControlRigBlueprint>(Object);
			break;
		}

		OutBlueprint = Object->GetTypedOuter<UTLLRN_ControlRigBlueprint>();
		if(OutBlueprint)
		{
			break;
		}

		if(const UTLLRN_ControlRig* TLLRN_ControlRig = Object->GetTypedOuter<UTLLRN_ControlRig>())
		{
			OutBlueprint = Cast<UTLLRN_ControlRigBlueprint>(TLLRN_ControlRig->GetClass()->ClassGeneratedBy);
			if(OutBlueprint)
			{
				break;
			}
		}
	}

	if (OutBlueprint == nullptr)
	{
		TArray<UPackage*> Packages;
		InStructPropertyHandle->GetOuterPackages(Packages);
		for (UPackage* Package : Packages)
		{
			if (Package == nullptr)
			{
				continue;
			}

			TArray<UObject*> SubObjects;
			Package->GetDefaultSubobjects(SubObjects);
			for (UObject* SubObject : SubObjects)
			{
				if (UTLLRN_ControlRig* Rig = Cast<UTLLRN_ControlRig>(SubObject))
				{
					UTLLRN_ControlRigBlueprint* Blueprint = Cast<UTLLRN_ControlRigBlueprint>(Rig->GetClass()->ClassGeneratedBy);
					if (Blueprint)
					{
						if(Blueprint->GetOutermost() == Package)
						{
							OutBlueprint = Blueprint;
							break;
						}
					}
				}
			}

			if (OutBlueprint)
			{
				break;
			}
		}
	}
}

static UTLLRN_ControlRigBlueprint* TLLRN_RigModuleDetails_GetBlueprintFromRig(UTLLRN_ModularRig* InRig)
{
	if(InRig == nullptr)
	{
		return nullptr;
	}

	UTLLRN_ControlRigBlueprint* Blueprint = InRig->GetTypedOuter<UTLLRN_ControlRigBlueprint>();
	if(Blueprint == nullptr)
	{
		Blueprint = Cast<UTLLRN_ControlRigBlueprint>(InRig->GetClass()->ClassGeneratedBy);
	}
	return Blueprint;
}

void FTLLRN_RigModuleInstanceDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	PerModuleInfos.Reset();

	TArray<TWeakObjectPtr<UObject>> DetailObjects;
	DetailBuilder.GetObjectsBeingCustomized(DetailObjects);
	for(TWeakObjectPtr<UObject> DetailObject : DetailObjects)
	{
		if(UTLLRN_ControlRig* ModuleInstance = Cast<UTLLRN_ControlRig>(DetailObject))
		{
			if(const UTLLRN_ModularRig* TLLRN_ModularRig = Cast<UTLLRN_ModularRig>(ModuleInstance->GetOuter()))
			{
				if(const FTLLRN_RigModuleInstance* Module = TLLRN_ModularRig->FindModule(ModuleInstance))
				{
					const FString Path = Module->GetPath();

					FPerModuleInfo Info;
					Info.Path = Path;
					Info.Module = TLLRN_ModularRig->GetHandle(Path);
					if(!Info.Module.IsValid())
					{
						return;
					}
					
					if(const UTLLRN_ControlRigBlueprint* Blueprint = Info.GetBlueprint())
					{
						if(const UTLLRN_ModularRig* DefaultTLLRN_ModularRig = Cast<UTLLRN_ModularRig>(Blueprint->GeneratedClass->GetDefaultObject()))
						{
							Info.DefaultModule = DefaultTLLRN_ModularRig->GetHandle(Path);
						}
					}

					PerModuleInfos.Add(Info);
				}
			}
		}
	}

	// don't customize if the 
	if(PerModuleInfos.IsEmpty())
	{
		return;
	}

	TArray<FName> OriginalCategoryNames;
	DetailBuilder.GetCategoryNames(OriginalCategoryNames);

	IDetailCategoryBuilder& GeneralCategory = DetailBuilder.EditCategory(TEXT("General"), LOCTEXT("General", "General"));
	{
		static const FText NameTooltip = LOCTEXT("NameTooltip", "The name is used to determine the long name (the full path) and to provide a unique address within the rig.");
		GeneralCategory.AddCustomRow(FText::FromString(TEXT("Name")))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Name")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ToolTipText(NameTooltip)
		]
		.ValueContent()
		[
			SNew(SInlineEditableTextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FTLLRN_RigModuleInstanceDetails::GetName)
			.OnTextCommitted(this, &FTLLRN_RigModuleInstanceDetails::SetName, DetailBuilder.GetPropertyUtilities())
			.ToolTipText(NameTooltip)
			.OnVerifyTextChanged(this, &FTLLRN_RigModuleInstanceDetails::OnVerifyNameChanged)
		];

		static const FText ShortNameTooltip = LOCTEXT("ShortNameTooltip", "The short name is used for the user interface, for example the sequencer channels.\nThis value can be edited and adjusted as needed.");
		GeneralCategory.AddCustomRow(FText::FromString(TEXT("Short Name")))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Short Name")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ToolTipText(ShortNameTooltip)
			.IsEnabled(PerModuleInfos.Num() == 1)
		]
		.ValueContent()
		[
			SNew(SInlineEditableTextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FTLLRN_RigModuleInstanceDetails::GetShortName)
			.OnTextCommitted(this, &FTLLRN_RigModuleInstanceDetails::SetShortName, DetailBuilder.GetPropertyUtilities())
			.ToolTipText(ShortNameTooltip)
			.IsEnabled(PerModuleInfos.Num() == 1)
			.OnVerifyTextChanged(this, &FTLLRN_RigModuleInstanceDetails::OnVerifyShortNameChanged)
		];

		static const FText LongNameTooltip = LOCTEXT("LongNameTooltip", "The long name represents a unique address within the rig but isn't used for the user interface.");
		GeneralCategory.AddCustomRow(FText::FromString(TEXT("Long Name")))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Long Name")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ToolTipText(LOCTEXT("LongNameTooltip", "The long name represents a unique address within the rig but isn't used for the user interface."))
			.ToolTipText(LongNameTooltip)
			.IsEnabled(false)
		]
		.ValueContent()
		[
			SNew(SInlineEditableTextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FTLLRN_RigModuleInstanceDetails::GetLongName)
			.ToolTipText(LongNameTooltip)
			.IsEnabled(false)
		];

		GeneralCategory.AddCustomRow(FText::FromString(TEXT("RigClass")))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("RigClass")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.IsEnabled(true)
		]
		.ValueContent()
		[
			SNew(SInlineEditableTextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(this, &FTLLRN_RigModuleInstanceDetails::GetRigClassPath)
			.IsEnabled(true)
		];

		GeneralCategory.AddCustomRow(FText::FromString(TEXT("Variant Tags")))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Variant Tags")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.IsEnabled(true)
		]
		.ValueContent()
		[
			SNew(SRigVMVariantTagWidget)
			.Orientation(EOrientation::Orient_Horizontal)
			.CanAddTags(false)
			.EnableContextMenu(false)
			.OnGetTags_Lambda([this]() -> TArray<FRigVMTag>
			{
				TArray<FRigVMTag> Tags;
				for (int32 InfoIndex=0; InfoIndex<PerModuleInfos.Num(); ++InfoIndex)
				{
					const FPerModuleInfo& ModuleInfo = PerModuleInfos[InfoIndex]; 
					if(ModuleInfo.Module.IsValid())
					{
						if (const FTLLRN_RigModuleInstance* Module = ModuleInfo.GetModule())
						{
							if (const UTLLRN_ControlRigBlueprint* ModuleBlueprint = Cast<UTLLRN_ControlRigBlueprint>(Module->GetRig()->GetClass()->ClassGeneratedBy))
							{
								if (InfoIndex == 0)
								{
									Tags = ModuleBlueprint->GetAssetVariant().Tags;
								}
								else
								{
									const TArray<FRigVMTag>& OtherTags = ModuleBlueprint->GetAssetVariant().Tags;
									bool bSameArray = Tags.Num() == OtherTags.Num();
									if (bSameArray)
									{
										for (const FRigVMTag& OtherTag : OtherTags)
										{
											if (!Tags.ContainsByPredicate([OtherTag](const FRigVMTag& Tag) { return OtherTag.Name == Tag.Name; }))
											{
												return {};
											}
										}
									}
									else
									{
										return {};
									}
								}
							}
						}
					}
				}
				return Tags;
			})
		];
	}

	IDetailCategoryBuilder& ConnectionsCategory = DetailBuilder.EditCategory(TEXT("Connections"), LOCTEXT("Connections", "Connections"));
	{
		bool bDisplayConnectors = PerModuleInfos.Num() >= 1;
		if (PerModuleInfos.Num() > 1)
		{
			UTLLRN_ModularRig* TLLRN_ModularRig = PerModuleInfos[0].GetTLLRN_ModularRig();
			for (FPerModuleInfo& Info : PerModuleInfos)
			{
				if (Info.GetTLLRN_ModularRig() != TLLRN_ModularRig)
				{
					bDisplayConnectors = false;
					break;
				}
			}
		}
		if (bDisplayConnectors)
		{
			TArray<FTLLRN_RigModuleConnector> Connectors = GetConnectors();

			// sort connectors primary first, then secondary, then optional
			Algo::SortBy(Connectors, [](const FTLLRN_RigModuleConnector& Connector) -> int32
			{
				return Connector.IsPrimary() ? 0 : (Connector.IsOptional() ? 2 : 1);
			});
			
			for(const FTLLRN_RigModuleConnector& Connector : Connectors)
			{
				const FText Label = FText::FromString(Connector.Name);
				TSharedPtr<SVerticalBox> ButtonBox;

				TArray<FTLLRN_RigElementResolveResult> Matches;
				for (int32 ModuleIndex=0; ModuleIndex<PerModuleInfos.Num(); ++ModuleIndex)
				{
					const FPerModuleInfo& Info = PerModuleInfos[ModuleIndex];
					if (const FTLLRN_RigModuleInstance* Module = Info.GetModule())
					{
						if (UTLLRN_ModularRig* TLLRN_ModularRig = Info.GetTLLRN_ModularRig())
						{
							if (UTLLRN_RigHierarchy* Hierarchy = TLLRN_ModularRig->GetHierarchy())
							{
								FString ConnectorPath = FString::Printf(TEXT("%s:%s"), *Info.Path, *Connector.Name);
								FTLLRN_RigElementKey ConnectorKey(*ConnectorPath, ETLLRN_RigElementType::Connector);
								if (FTLLRN_RigConnectorElement* ConnectorElement = Cast<FTLLRN_RigConnectorElement>(Hierarchy->Find(ConnectorKey)))
								{
									const UTLLRN_ModularRigRuleManager* RuleManager = TLLRN_ModularRig->GetHierarchy()->GetRuleManager();
									if (ModuleIndex == 0)
									{
										Matches = RuleManager->FindMatches(ConnectorElement, Module, TLLRN_ModularRig->GetElementKeyRedirector()).GetMatches();
									}
									else
									{
										const FTLLRN_ModularRigResolveResult& ConnectorMatches = RuleManager->FindMatches(ConnectorElement, Module, TLLRN_ModularRig->GetElementKeyRedirector());
										Matches.FilterByPredicate([ConnectorMatches](const FTLLRN_RigElementResolveResult& Match)
										{
											return ConnectorMatches.ContainsMatch(Match.GetKey());
										});
									}
								}
							}
						}
					}
				}

				FRigTreeDelegates TreeDelegates;
				TreeDelegates.OnGetHierarchy = FOnGetRigTreeHierarchy::CreateLambda([this]()
				{
					return PerModuleInfos[0].GetTLLRN_ModularRig()->GetHierarchy();
				});

				TArray<FTLLRN_RigElementKey> MatchingKeys;
				for(const FTLLRN_RigElementResolveResult& SingleMatch : Matches)
				{
					MatchingKeys.Add(SingleMatch.GetKey());
				}
				TreeDelegates.OnRigTreeIsItemVisible = FOnRigTreeIsItemVisible::CreateLambda([MatchingKeys](const FTLLRN_RigElementKey& InTarget)
				{
					return MatchingKeys.Contains(InTarget);
				});
				TreeDelegates.OnGetSelection.BindLambda([this, Connector]() -> TArray<FTLLRN_RigElementKey>
				{
					FTLLRN_RigElementKey Target;
					FTLLRN_RigElementKeyRedirector Redirector = PerModuleInfos[0].GetTLLRN_ModularRig()->GetElementKeyRedirector();
					for (int32 ModuleIndex=0; ModuleIndex<PerModuleInfos.Num(); ++ModuleIndex)
					{
						FString ConnectorPath = FString::Printf(TEXT("%s:%s"), *PerModuleInfos[ModuleIndex].Path, *Connector.Name);
						FTLLRN_RigElementKey ConnectorKey(*ConnectorPath, ETLLRN_RigElementType::Connector);
						if (const FTLLRN_RigElementKey* Key = Redirector.FindExternalKey(ConnectorKey))
						{
							if (ModuleIndex == 0)
							{
								Target = *Key;
							}
							else if (Target != *Key)
							{
								Target.Name = *TLLRN_ControlTLLRN_RigModuleDetailsMultipleValues.ToString();
								return {Target};
							}
						}
						else
						{
							Target.Name = *TLLRN_ControlTLLRN_RigModuleDetailsMultipleValues.ToString();
							return {Target};
						}
					}
					return {};
				});
				TreeDelegates.OnSelectionChanged.BindSP(this, &FTLLRN_RigModuleInstanceDetails::OnConnectorTargetChanged, Connector);
			
				static const FSlateBrush* PrimaryBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorPrimary");
				static const FSlateBrush* SecondaryBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorSecondary");
				static const FSlateBrush* OptionalBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush("TLLRN_ControlRig.ConnectorOptional");

				const FSlateBrush* IconBrush = Connector.IsPrimary() ? PrimaryBrush : (Connector.IsOptional() ? OptionalBrush : SecondaryBrush);
				TSharedPtr<SSearchableTLLRN_RigHierarchyTreeView>& SearchableTreeView = ConnectionListBox.FindOrAdd(Connector.Name);

				ConnectionsCategory.AddCustomRow(Label)
					.NameContent()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.f, 0.f, 4.f, 0.f)
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(IconBrush)
							.ColorAndOpacity(FSlateColor::UseForeground())
							.DesiredSizeOverride(FVector2D(16, 16))
						]
						
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.f, 0.f, 0.f, 0.f)
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(Label)
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.IsEnabled(true)
						]
					]
					.ValueContent()
					[
						SNew(SHorizontalBox)

						// Combo button
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.f, 0.f, 0.f, 0.f)
						[
							SNew( SComboButton )
							.ContentPadding(3)
							.MenuPlacement(MenuPlacement_BelowAnchor)
							.OnComboBoxOpened(this, &FTLLRN_RigModuleInstanceDetails::PopulateConnectorTargetList, Connector.Name)
							.ButtonContent()
							[
								// Wrap in configurable box to restrain height/width of menu
								SNew(SBox)
								.MinDesiredWidth(150.0f)
								[
									SAssignNew(ButtonBox, SVerticalBox)
								]
							]
							.MenuContent()
							[
								SNew(SBorder)
								.Visibility(EVisibility::Visible)
								.BorderImage(FAppStyle::GetBrush("Menu.Background"))
								[
									SAssignNew(SearchableTreeView, SSearchableTLLRN_RigHierarchyTreeView)
										.RigTreeDelegates(TreeDelegates)
								]
							]
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.f, 0.f, 0.f, 0.f)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							.AutoHeight()
							[
								// Reset button
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.f, 0.f, 0.f, 0.f)
								[
									SAssignNew(ResetConnectorButton.FindOrAdd(Connector.Name), SButton)
									.ButtonStyle( FAppStyle::Get(), "NoBorder" )
									.ButtonColorAndOpacity_Lambda([this, Connector]()
									{
										const TSharedPtr<SButton>& Button = ResetConnectorButton.FindRef(Connector.Name);
										return Button.IsValid() && Button->IsHovered()
											? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
											: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
									})
									.OnClicked_Lambda([this, Connector]()
									{
										for (FPerModuleInfo& Info : PerModuleInfos)
										{
											FString ConnectorPath = FString::Printf(TEXT("%s:%s"), *Info.Path, *Connector.Name);
											FTLLRN_RigElementKey ConnectorKey(*ConnectorPath, ETLLRN_RigElementType::Connector);
											Info.GetBlueprint()->GetTLLRN_ModularTLLRN_RigController()->DisconnectConnector(ConnectorKey);
										}
										return FReply::Handled();
									})
									.ContentPadding(1.f)
									.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigModuleDetails", "Reset_Connector", "Reset Connector"))
									[
										SNew(SImage)
										.ColorAndOpacity_Lambda( [this, Connector]()
										{
											const TSharedPtr<SButton>& Button = ResetConnectorButton.FindRef(Connector.Name);
											return Button.IsValid() && Button->IsHovered()
											? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
											: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
										})
										.Image(FSlateIcon(FAppStyle::Get().GetStyleSetName(), "PropertyWindow.DiffersFromDefault").GetIcon())
									]
								]

								// Use button
								+SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.f, 0.f, 0.f, 0.f)
								[
									SAssignNew(UseSelectedButton.FindOrAdd(Connector.Name), SButton)
									.ButtonStyle( FAppStyle::Get(), "NoBorder" )
									.ButtonColorAndOpacity_Lambda([this, Connector]()
									{
										const TSharedPtr<SButton>& Button = UseSelectedButton.FindRef(Connector.Name);
										return Button.IsValid() && Button->IsHovered()
											? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
											: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
									})
									.OnClicked_Lambda([this, Connector]()
									{
										if (UTLLRN_ModularRig* TLLRN_ModularRig = PerModuleInfos[0].GetTLLRN_ModularRig())
										{
											const TArray<FTLLRN_RigElementKey>& Selected = TLLRN_ModularRig->GetHierarchy()->GetSelectedKeys();
											if (Selected.Num() > 0)
											{
												for (FPerModuleInfo& Info : PerModuleInfos)
												{
													FString ConnectorPath = FString::Printf(TEXT("%s:%s"), *Info.Path, *Connector.Name);
													FTLLRN_RigElementKey ConnectorKey(*ConnectorPath, ETLLRN_RigElementType::Connector);
													Info.GetBlueprint()->GetTLLRN_ModularTLLRN_RigController()->ConnectConnectorToElement(ConnectorKey, Selected[0], true, TLLRN_ModularRig->GetTLLRN_ModularRigSettings().bAutoResolve);
												}
											}
										}
										return FReply::Handled();
									})
									.ContentPadding(1.f)
									.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigModuleDetails", "Use_Selected", "Use Selected"))
									[
										SNew(SImage)
										.ColorAndOpacity_Lambda( [this, Connector]()
										{
											const TSharedPtr<SButton>& Button = UseSelectedButton.FindRef(Connector.Name);
											return Button.IsValid() && Button->IsHovered()
											? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
											: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
										})
										.Image(FAppStyle::GetBrush("Icons.CircleArrowLeft"))
									]
								]

								// Select in hierarchy button
								+SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.f, 0.f, 0.f, 0.f)
								[
									SAssignNew(SelectElementButton.FindOrAdd(Connector.Name), SButton)
									.ButtonStyle( FAppStyle::Get(), "NoBorder" )
									.ButtonColorAndOpacity_Lambda([this, Connector]()
									{
										const TSharedPtr<SButton>& Button = SelectElementButton.FindRef(Connector.Name);
										return Button.IsValid() && Button->IsHovered()
											? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
											: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
									})
									.OnClicked_Lambda([this, Connector]()
									{
										if (UTLLRN_ModularRig* TLLRN_ModularRig = PerModuleInfos[0].GetTLLRN_ModularRig())
										{
											FString ConnectorPath = FString::Printf(TEXT("%s:%s"), *PerModuleInfos[0].Path, *Connector.Name);
											FTLLRN_RigElementKey ConnectorKey(*ConnectorPath, ETLLRN_RigElementType::Connector);
											if (const FTLLRN_RigElementKey* TargetKey = TLLRN_ModularRig->GetElementKeyRedirector().FindExternalKey(ConnectorKey))
											{
												TLLRN_ModularRig->GetHierarchy()->GetController()->SelectElement(*TargetKey, true, true);
											}
										}
										return FReply::Handled();
									})
									.ContentPadding(1.f)
									.ToolTipText(NSLOCTEXT("TLLRN_ControlTLLRN_RigModuleDetails", "Select_Element", "Select Element"))
									[
										SNew(SImage)
										.ColorAndOpacity_Lambda( [this, Connector]()
										{
											const TSharedPtr<SButton>& Button = SelectElementButton.FindRef(Connector.Name);
											return Button.IsValid() && Button->IsHovered()
											? FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.8))
											: FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.4));
										})
										.Image(FAppStyle::GetBrush("Icons.Search"))
									]
								]
							]
						]
					];

				FTLLRN_RigElementKey CurrentTargetKey;
				if (PerModuleInfos.Num() >= 1)
				{
					for (int32 ModuleIndex=0; ModuleIndex<PerModuleInfos.Num(); ++ModuleIndex)
					{
						FTLLRN_RigElementKey TargetKey;
						FString ConnectorPath = FString::Printf(TEXT("%s:%s"), *PerModuleInfos[ModuleIndex].Path, *Connector.Name);
						FTLLRN_RigElementKey ConnectorKey(*ConnectorPath, ETLLRN_RigElementType::Connector);
						if (const FTLLRN_RigElementKey* Key = PerModuleInfos[ModuleIndex].GetTLLRN_ModularRig()->GetElementKeyRedirector().FindExternalKey(ConnectorKey))
						{
							TargetKey = *Key;
						}
						if (ModuleIndex == 0)
						{
							CurrentTargetKey = TargetKey;
						}
						else
						{
							if (CurrentTargetKey != TargetKey)
							{
								CurrentTargetKey.Name = *TLLRN_ControlTLLRN_RigModuleDetailsMultipleValues.ToString();
							}
						}
					}
				}
				TPair<const FSlateBrush*, FSlateColor> IconAndColor = STLLRN_RigHierarchyItem::GetBrushForElementType(PerModuleInfos[0].GetTLLRN_ModularRig()->GetHierarchy(), CurrentTargetKey);
				PopulateConnectorCurrentTarget(ButtonBox, IconAndColor.Key, IconAndColor.Value, FText::FromName(CurrentTargetKey.Name));
			}
		}
	}

	for(const FName& OriginalCategoryName : OriginalCategoryNames)
	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(OriginalCategoryName);
		{
			IPropertyAccessEditor& PropertyAccessEditor = IModularFeatures::Get().GetModularFeature<IPropertyAccessEditor>("PropertyAccessEditor");

			TArray<TSharedRef<IPropertyHandle>> DefaultProperties;
			Category.GetDefaultProperties(DefaultProperties, true, true);

			for(const TSharedRef<IPropertyHandle>& DefaultProperty : DefaultProperties)
			{
				const FProperty* Property = DefaultProperty->GetProperty();
				if(Property == nullptr)
				{
					DetailBuilder.HideProperty(DefaultProperty);
					continue;
				}

				// skip advanced properties for now
				const bool bAdvancedDisplay = Property->HasAnyPropertyFlags(CPF_AdvancedDisplay);
				if(bAdvancedDisplay)
				{
					DetailBuilder.HideProperty(DefaultProperty);
					continue;
				}

				// skip non-public properties for now
				const bool bIsPublic = Property->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst);
				const bool bIsInstanceEditable = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
				if(!bIsPublic || !bIsInstanceEditable)
				{
					DetailBuilder.HideProperty(DefaultProperty);
					continue;
				}

				const FSimpleDelegate OnValueChangedDelegate = FSimpleDelegate::CreateSP(this, &FTLLRN_RigModuleInstanceDetails::OnConfigValueChanged, Property->GetFName());
				DefaultProperty->SetOnPropertyValueChanged(OnValueChangedDelegate);
				DefaultProperty->SetOnChildPropertyValueChanged(OnValueChangedDelegate);

				FPropertyBindingWidgetArgs BindingArgs;
				BindingArgs.Property = (FProperty*)Property;
				BindingArgs.CurrentBindingText = TAttribute<FText>::CreateLambda([this, Property]()
				{
					return GetBindingText(Property);
				});
				BindingArgs.CurrentBindingImage = TAttribute<const FSlateBrush*>::CreateLambda([this, Property]()
				{
					return GetBindingImage(Property);
				});
				BindingArgs.CurrentBindingColor = TAttribute<FLinearColor>::CreateLambda([this, Property]()
				{
					return GetBindingColor(Property);
				});

				BindingArgs.OnCanBindPropertyWithBindingChain.BindLambda([](const FProperty* InProperty, TConstArrayView<FBindingChainElement> InBindingChain) -> bool { return true; });
				BindingArgs.OnCanBindToClass.BindLambda([](UClass* InClass) -> bool { return false; });
				BindingArgs.OnCanRemoveBinding.BindRaw(this, &FTLLRN_RigModuleInstanceDetails::CanRemoveBinding);
				BindingArgs.OnRemoveBinding.BindSP(this, &FTLLRN_RigModuleInstanceDetails::HandleRemoveBinding);

				BindingArgs.bGeneratePureBindings = true;
				BindingArgs.bAllowNewBindings = true;
				BindingArgs.bAllowArrayElementBindings = false;
				BindingArgs.bAllowStructMemberBindings = false;
				BindingArgs.bAllowUObjectFunctions = false;

				BindingArgs.MenuExtender = MakeShareable(new FExtender);
				BindingArgs.MenuExtender->AddMenuExtension(
					"Properties",
					EExtensionHook::After,
					nullptr,
					FMenuExtensionDelegate::CreateSPLambda(this, [this, Property](FMenuBuilder& MenuBuilder)
					{
						FillBindingMenu(MenuBuilder, Property);
					})
				);

				TSharedPtr<SWidget> ValueWidget = DefaultProperty->CreatePropertyValueWidgetWithCustomization(DetailBuilder.GetDetailsView());

				const bool bShowChildren = true;
				Category.AddProperty(DefaultProperty).CustomWidget(bShowChildren)
				.NameContent()
				[
					DefaultProperty->CreatePropertyNameWidget()
				]

				.ValueContent()
				[
					ValueWidget ? ValueWidget.ToSharedRef() : SNullWidget::NullWidget
					// todo: if the property is bound / or partially bound
					// mark the property value widget as disabled / read only.
				]

				.ExtensionContent()
				[
					PropertyAccessEditor.MakePropertyBindingWidget(nullptr, BindingArgs)
				];
			}
		}
	}
}

FText FTLLRN_RigModuleInstanceDetails::GetName() const
{
	const FTLLRN_RigModuleInstance* FirstModule = PerModuleInfos[0].GetModule();
	if(FirstModule == nullptr)
	{
		return FText();
	}
	
	const FName FirstValue = FirstModule->Name;
	if(PerModuleInfos.Num() > 1)
	{
		bool bSame = true;
		for (int32 i=1; i<PerModuleInfos.Num(); ++i)
		{
			if(const FTLLRN_RigModuleInstance* Module = PerModuleInfos[i].GetModule())
			{
				if (!Module->Name.IsEqual(FirstValue, ENameCase::IgnoreCase))
				{
					bSame = false;
					break;
				}
			}
		}
		if (!bSame)
		{
			return TLLRN_ControlTLLRN_RigModuleDetailsMultipleValues;
		}
	}
	return FText::FromName(FirstValue);
}

void FTLLRN_RigModuleInstanceDetails::SetName(const FText& InValue, ETextCommit::Type InCommitType, const TSharedRef<IPropertyUtilities> PropertyUtilities)
{
	if(InValue.IsEmpty())
	{
		return;
	}
	
	for (FPerModuleInfo& Info : PerModuleInfos)
	{
		if (const FTLLRN_RigModuleInstance* ModuleInstance = Info.GetModule())
		{
			if (UTLLRN_ControlRigBlueprint* Blueprint = Info.GetBlueprint())
			{
				UTLLRN_ModularTLLRN_RigController* Controller = Blueprint->GetTLLRN_ModularTLLRN_RigController();
				const FString OldPath = ModuleInstance->GetPath();
				(void)Controller->RenameModule(OldPath, *InValue.ToString(), true);
			}
		}
	}
}

bool FTLLRN_RigModuleInstanceDetails::OnVerifyNameChanged(const FText& InText, FText& OutErrorMessage)
{
	if(InText.IsEmpty())
	{
		static const FText EmptyNameIsNotAllowed = LOCTEXT("EmptyNameIsNotAllowed", "Empty name is not allowed.");
		OutErrorMessage = EmptyNameIsNotAllowed;
		return false;
	}

	for (FPerModuleInfo& Info : PerModuleInfos)
	{
		if (const FTLLRN_RigModuleInstance* ModuleInstance = Info.GetModule())
		{
			if (UTLLRN_ControlRigBlueprint* Blueprint = Info.GetBlueprint())
			{
				UTLLRN_ModularTLLRN_RigController* Controller = Blueprint->GetTLLRN_ModularTLLRN_RigController();
				if (!Controller->CanRenameModule(ModuleInstance->GetPath(), *InText.ToString(), OutErrorMessage))
				{
					return false;
				}
			}
		}
	}

	return true;
}

FText FTLLRN_RigModuleInstanceDetails::GetShortName() const
{
	const FTLLRN_RigModuleInstance* FirstModule = PerModuleInfos[0].GetModule();
	if(FirstModule == nullptr)
	{
		return FText();
	}

	const FString FirstValue = FirstModule->GetShortName();
	if(PerModuleInfos.Num() > 1)
	{
		bool bSame = true;
		for (int32 i=1; i<PerModuleInfos.Num(); ++i)
		{
			if(const FTLLRN_RigModuleInstance* Module = PerModuleInfos[i].GetModule())
			{
				if (!Module->GetShortName().Equals(FirstValue, ESearchCase::IgnoreCase))
				{
					bSame = false;
					break;
				}
			}
		}
		if (!bSame)
		{
			return TLLRN_ControlTLLRN_RigModuleDetailsMultipleValues;
		}
	}
	return FText::FromString(FirstValue);
}

void FTLLRN_RigModuleInstanceDetails::SetShortName(const FText& InValue, ETextCommit::Type InCommitType, const TSharedRef<IPropertyUtilities> PropertyUtilities)
{
	if(InValue.IsEmpty())
	{
		return;
	}
	
	check(PerModuleInfos.Num() == 1);
	const FPerModuleInfo& Info = PerModuleInfos[0];
	if (const FTLLRN_RigModuleInstance* ModuleInstance = Info.GetModule())
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = Info.GetBlueprint())
		{
			UTLLRN_ModularTLLRN_RigController* Controller = Blueprint->GetTLLRN_ModularTLLRN_RigController();
			Controller->SetModuleShortName(ModuleInstance->GetPath(), *InValue.ToString(), true);
		}
	}
}

bool FTLLRN_RigModuleInstanceDetails::OnVerifyShortNameChanged(const FText& InText, FText& OutErrorMessage)
{
	check(PerModuleInfos.Num() == 1);

	if(InText.IsEmpty())
	{
		return true;
	}

	const FPerModuleInfo& Info = PerModuleInfos[0];
	if (const FTLLRN_RigModuleInstance* ModuleInstance = Info.GetModule())
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = Info.GetBlueprint())
		{
			UTLLRN_ModularTLLRN_RigController* Controller = Blueprint->GetTLLRN_ModularTLLRN_RigController();
			return Controller->CanSetModuleShortName(ModuleInstance->GetPath(), *InText.ToString(), OutErrorMessage);
		}
	}

	return false;
}

FText FTLLRN_RigModuleInstanceDetails::GetLongName() const
{
	const FTLLRN_RigModuleInstance* FirstModule = PerModuleInfos[0].GetModule();
	if(FirstModule == nullptr)
	{
		return FText();
	}

	const FString FirstValue = FirstModule->GetLongName();
	if(PerModuleInfos.Num() > 1)
	{
		bool bSame = true;
		for (int32 i=1; i<PerModuleInfos.Num(); ++i)
		{
			if(const FTLLRN_RigModuleInstance* Module = PerModuleInfos[i].GetModule())
			{
				if (!Module->GetLongName().Equals(FirstValue, ESearchCase::IgnoreCase))
				{
					bSame = false;
					break;
				}
			}
		}
		if (!bSame)
		{
			return TLLRN_ControlTLLRN_RigModuleDetailsMultipleValues;
		}
	}
	return FText::FromString(FirstValue);
}
FText FTLLRN_RigModuleInstanceDetails::GetRigClassPath() const
{
	if(PerModuleInfos.Num() > 1)
	{
		if(const FTLLRN_RigModuleInstance* FirstModule = PerModuleInfos[0].GetModule())
		{
			bool bSame = true;
			for (int32 i=1; i<PerModuleInfos.Num(); ++i)
			{
				if(const FTLLRN_RigModuleInstance* Module = PerModuleInfos[i].GetModule())
				{
					if (Module->GetRig()->GetClass() != FirstModule->GetRig()->GetClass())
					{
						bSame = false;
						break;
					}
				}
			}
			if (!bSame)
			{
				return TLLRN_ControlTLLRN_RigModuleDetailsMultipleValues;
			}
		}
	}

	if (const FTLLRN_RigModuleInstance* Module = PerModuleInfos[0].GetModule())
	{
		if (const UTLLRN_ControlRig* ModuleRig = Module->GetRig())
		{
			return FText::FromString(ModuleRig->GetClass()->GetClassPathName().ToString());
		}
	}

	return FText();
}

TArray<FTLLRN_RigModuleConnector> FTLLRN_RigModuleInstanceDetails::GetConnectors() const
{
	if(PerModuleInfos.Num() > 1)
	{
		TArray<FTLLRN_RigModuleConnector> CommonConnectors;
		if (const FTLLRN_RigModuleInstance* Module = PerModuleInfos[0].GetModule())
		{
			if (const UTLLRN_ControlRig* ModuleRig = Module->GetRig())
			{
				CommonConnectors = ModuleRig->GetTLLRN_RigModuleSettings().ExposedConnectors;
			}
		}
		for (int32 ModuleIndex=1; ModuleIndex<PerModuleInfos.Num(); ++ModuleIndex)
		{
			if (const FTLLRN_RigModuleInstance* Module = PerModuleInfos[ModuleIndex].GetModule())
			{
				if (const UTLLRN_ControlRig* ModuleRig = Module->GetRig())
				{
					const TArray<FTLLRN_RigModuleConnector>& ModuleConnectors = ModuleRig->GetTLLRN_RigModuleSettings().ExposedConnectors;
					CommonConnectors = CommonConnectors.FilterByPredicate([ModuleConnectors](const FTLLRN_RigModuleConnector& Connector)
					{
						return ModuleConnectors.Contains(Connector);
					});
				}
			}
		}
		return CommonConnectors;
	}

	if (const FTLLRN_RigModuleInstance* Module = PerModuleInfos[0].GetModule())
	{
		if (const UTLLRN_ControlRig* ModuleRig = Module->GetRig())
		{
			return ModuleRig->GetTLLRN_RigModuleSettings().ExposedConnectors;
		}
	}

	return TArray<FTLLRN_RigModuleConnector>();
}

FTLLRN_RigElementKeyRedirector FTLLRN_RigModuleInstanceDetails::GetConnections() const
{
	if(PerModuleInfos.Num() > 1)
	{
		return FTLLRN_RigElementKeyRedirector();
	}

	if (const FTLLRN_RigModuleInstance* Module = PerModuleInfos[0].GetModule())
	{
		if (UTLLRN_ControlRig* ModuleRig = Module->GetRig())
		{
			return ModuleRig->GetElementKeyRedirector();
		}
	}

	return FTLLRN_RigElementKeyRedirector();
}

void FTLLRN_RigModuleInstanceDetails::PopulateConnectorTargetList(const FString InConnectorName)
{
	ConnectionListBox.FindRef(InConnectorName)->GetTreeView()->RefreshTreeView(true);
}

void FTLLRN_RigModuleInstanceDetails::PopulateConnectorCurrentTarget(TSharedPtr<SVerticalBox> InListBox, const FSlateBrush* InBrush, const FSlateColor& InColor, const FText& InTitle)
{
	static const FSlateBrush* RoundedBoxBrush = FTLLRN_ControlRigEditorStyle::Get().GetBrush(TEXT("TLLRN_ControlRig.SpacePicker.RoundedRect"));
	
	TSharedPtr<SHorizontalBox> RowBox, ButtonBox;
	InListBox->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Fill)
	.Padding(4.0, 0.0, 4.0, 0.0)
	[
		SNew( SButton )
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(0.0))
		[
			SAssignNew(RowBox, SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)
			.Padding(0)
			[
				SNew(SBorder)
				.Padding(FMargin(5.0, 2.0, 5.0, 2.0))
				.BorderImage(RoundedBoxBrush)
				.BorderBackgroundColor(FSlateColor(FLinearColor::Transparent))
				.Content()
				[
					SAssignNew(ButtonBox, SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Left)
					.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
					[
						SNew(SImage)
						.Image(InBrush)
						.ColorAndOpacity(InColor)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Left)
					.Padding(0)
					[
						SNew( STextBlock )
						.Text( InTitle )
						.Font( IDetailLayoutBuilder::GetDetailFont() )
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(SSpacer)
					]
				]
			]
		]
	];
}

void FTLLRN_RigModuleInstanceDetails::OnConfigValueChanged(const FName InVariableName)
{
	TMap<FString, FString> ModuleValues;
	for(const FPerModuleInfo& Info : PerModuleInfos)
	{
		if (const FTLLRN_RigModuleInstance* ModuleInstance = Info.GetModule())
		{
			if (const UTLLRN_ControlRig* ModuleRig = ModuleInstance->GetRig())
			{
				FString ValueStr = ModuleRig->GetVariableAsString(InVariableName);
				ModuleValues.Add(ModuleInstance->GetPath(), ValueStr);
			}
		}
	}
	
	for (const TPair<FString, FString>& Value : ModuleValues)
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = PerModuleInfos[0].GetBlueprint())
		{
			UTLLRN_ModularTLLRN_RigController* Controller = Blueprint->GetTLLRN_ModularTLLRN_RigController();
			Controller->SetConfigValueInModule(Value.Key, InVariableName, Value.Value);
		}
	}
}

void FTLLRN_RigModuleInstanceDetails::OnConnectorTargetChanged(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo, const FTLLRN_RigModuleConnector InConnector)
{
	if (SelectInfo == ESelectInfo::OnNavigation)
	{
		return;
	}
	
	FScopedTransaction Transaction(LOCTEXT("ModuleHierarchyResolveConnector", "Resolve Connector"));
	for (FPerModuleInfo& Info : PerModuleInfos)
	{
		if (UTLLRN_ModularTLLRN_RigController* Controller = Info.GetBlueprint()->GetTLLRN_ModularTLLRN_RigController())
		{
			FString ConnectorPath = FString::Printf(TEXT("%s:%s"), *Info.Path, *InConnector.Name);
			FTLLRN_RigElementKey ConnectorKey(*ConnectorPath, ETLLRN_RigElementType::Connector);
			if (Selection.IsValid())
			{
				const FTLLRN_ModularRigSettings& Settings = Info.GetTLLRN_ModularRig()->GetTLLRN_ModularRigSettings();
				Controller->ConnectConnectorToElement(ConnectorKey, Selection->Key, true, Settings.bAutoResolve);
			}
			else
			{
				Controller->DisconnectConnector(ConnectorKey);
			}
		}
	}
}

const FTLLRN_RigModuleInstanceDetails::FPerModuleInfo& FTLLRN_RigModuleInstanceDetails::FindModule(const FString& InPath) const
{
	const FPerModuleInfo* Info = FindModuleByPredicate([InPath](const FPerModuleInfo& Info)
	{
		if(const FTLLRN_RigModuleInstance* Module = Info.GetModule())
		{
			return Module->GetPath() == InPath;
		}
		return false;
	});

	if(Info)
	{
		return *Info;
	}

	static const FPerModuleInfo EmptyInfo;
	return EmptyInfo;
}

const FTLLRN_RigModuleInstanceDetails::FPerModuleInfo* FTLLRN_RigModuleInstanceDetails::FindModuleByPredicate(const TFunction<bool(const FPerModuleInfo&)>& InPredicate) const
{
	return PerModuleInfos.FindByPredicate(InPredicate);
}

bool FTLLRN_RigModuleInstanceDetails::ContainsModuleByPredicate(const TFunction<bool(const FPerModuleInfo&)>& InPredicate) const
{
	return PerModuleInfos.ContainsByPredicate(InPredicate);
}

void FTLLRN_RigModuleInstanceDetails::RegisterSectionMappings(FPropertyEditorModule& PropertyEditorModule, UClass* InClass)
{
	TSharedRef<FPropertySection> MetadataSection = PropertyEditorModule.FindOrCreateSection(InClass->GetFName(), "Metadata", LOCTEXT("Metadata", "Metadata"));
	MetadataSection->AddCategory("Metadata");
}

FText FTLLRN_RigModuleInstanceDetails::GetBindingText(const FProperty* InProperty) const
{
	const FName VariableName = InProperty->GetFName();
	FText FirstValue;
	for (int32 ModuleIndex=0; ModuleIndex<PerModuleInfos.Num(); ++ModuleIndex)
	{
		if (const FTLLRN_RigModuleReference* ModuleReference = PerModuleInfos[ModuleIndex].GetReference())
		{
			if(ModuleReference->Bindings.Contains(VariableName))
			{
				const FText BindingText = FText::FromString(ModuleReference->Bindings.FindChecked(VariableName));
				if(ModuleIndex == 0)
				{
					FirstValue = BindingText;
				}
				else if(!FirstValue.EqualTo(BindingText))
				{
					return TLLRN_ControlTLLRN_RigModuleDetailsMultipleValues;
				}
			}
		}
	}
	return FirstValue;
}

const FSlateBrush* FTLLRN_RigModuleInstanceDetails::GetBindingImage(const FProperty* InProperty) const
{
	static const FLazyName TypeIcon(TEXT("Kismet.VariableList.TypeIcon"));
	static const FLazyName ArrayTypeIcon(TEXT("Kismet.VariableList.ArrayTypeIcon"));

	if(CastField<FArrayProperty>(InProperty))
	{
		return FAppStyle::GetBrush(ArrayTypeIcon);
	}
	return FAppStyle::GetBrush(TypeIcon);
}

FLinearColor FTLLRN_RigModuleInstanceDetails::GetBindingColor(const FProperty* InProperty) const
{
	if(InProperty)
	{
		FEdGraphPinType PinType;
		const UEdGraphSchema_K2* Schema_K2 = GetDefault<UEdGraphSchema_K2>();
		if (Schema_K2->ConvertPropertyToPinType(InProperty, PinType))
		{
			const URigVMEdGraphSchema* Schema = GetDefault<URigVMEdGraphSchema>();
			return Schema->GetPinTypeColor(PinType);
		}
	}
	return FLinearColor::White;
}

void FTLLRN_RigModuleInstanceDetails::FillBindingMenu(FMenuBuilder& MenuBuilder, const FProperty* InProperty) const
{
	if(PerModuleInfos.IsEmpty())
	{
		return;
	}

	UTLLRN_ControlRigBlueprint* Blueprint = PerModuleInfos[0].GetBlueprint();
	UTLLRN_ModularTLLRN_RigController* Controller = Blueprint->GetTLLRN_ModularTLLRN_RigController();

	TArray<FString> CombinedBindings;
	for(int32 Index = 0; Index < PerModuleInfos.Num(); Index++)
	{
		const FPerModuleInfo& Info  = PerModuleInfos[Index];
		const TArray<FString> Bindings = Controller->GetPossibleBindings(Info.GetPath(), InProperty->GetFName());
		if(Index == 0)
		{
			CombinedBindings = Bindings;
		}
		else
		{
			// reduce the set of bindings to the overall possible bindings
			CombinedBindings.RemoveAll([Bindings](const FString& Binding)
			{
				return !Bindings.Contains(Binding);
			});
		}
	}

	if(CombinedBindings.IsEmpty())
	{
		MenuBuilder.AddMenuEntry(
			FUIAction(FExecuteAction()),
			SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoBindingAvailable", "No bindings available for this property."))
					.ColorAndOpacity(FLinearColor::White)
				]
			);
		return;
	}

	// sort lexically
	CombinedBindings.Sort();

	// create a map of all of the variables per menu prefix (the module path the variables belong to)
	struct FPerMenuData
	{
		FString Name;
		FString ParentMenuPath;
		TArray<FString> SubMenuPaths;
		TArray<FString> Variables;

		static void SetupMenu(
			TSharedRef<FTLLRN_RigModuleInstanceDetails const> ThisDetails,
			const FProperty* InProperty,
			FMenuBuilder& InMenuBuilder,
			const FString& InMenuPath,
			TSharedRef<TMap<FString, FPerMenuData>> PerMenuData)
		{
			FPerMenuData& Data = PerMenuData->FindChecked((InMenuPath));

			Data.SubMenuPaths.Sort();
			Data.Variables.Sort();

			for(const FString& VariablePath : Data.Variables)
			{
				FString VariableName = VariablePath;
				(void)UTLLRN_RigHierarchy::SplitNameSpace(VariablePath, nullptr, &VariableName);
				
				InMenuBuilder.AddMenuEntry(
					FUIAction(FExecuteAction::CreateLambda([ThisDetails, InProperty, VariablePath]()
					{
						ThisDetails->HandleChangeBinding(InProperty, VariablePath);
					})),
					SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(1.0f, 0.0f)
						[
							SNew(SImage)
							.Image(ThisDetails->GetBindingImage(InProperty))
							.ColorAndOpacity(ThisDetails->GetBindingColor(InProperty))
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(VariableName))
							.ColorAndOpacity(FLinearColor::White)
						]
					);
			}

			for(const FString& SubMenuPath : Data.SubMenuPaths)
			{
				const FPerMenuData& SubMenuData = PerMenuData->FindChecked(SubMenuPath);

				const FText Label = FText::FromString(SubMenuData.Name);
				static const FText TooltipFormat = LOCTEXT("BindingMenuTooltipFormat", "Access to all variables of the {0} module");
				const FText Tooltip = FText::Format(TooltipFormat, Label);  
				InMenuBuilder.AddSubMenu(Label, Tooltip, FNewMenuDelegate::CreateLambda([ThisDetails, InProperty, SubMenuPath, PerMenuData](FMenuBuilder& SubMenuBuilder)
				{
					SetupMenu(ThisDetails, InProperty, SubMenuBuilder, SubMenuPath, PerMenuData);
				}));
			}
		}
	};
	
	// define the root menu
	const TSharedRef<TMap<FString, FPerMenuData>> PerMenuData = MakeShared<TMap<FString, FPerMenuData>>();
	PerMenuData->FindOrAdd(FString());

	// make sure all levels of the menu are known and we have the variables available
	for(const FString& BindingPath : CombinedBindings)
	{
		FString MenuPath;
		(void)UTLLRN_RigHierarchy::SplitNameSpace(BindingPath, &MenuPath, nullptr);

		FString PreviousMenuPath = MenuPath;
		FString ParentMenuPath = MenuPath, RemainingPath;
		while(UTLLRN_RigHierarchy::SplitNameSpace(ParentMenuPath, &ParentMenuPath, &RemainingPath))
		{
			// scope since the map may change at the end of this block
			{
				FPerMenuData& Data = PerMenuData->FindOrAdd(MenuPath);
				if(Data.Name.IsEmpty())
				{
					Data.Name = RemainingPath;
				}
			}
			
			PerMenuData->FindOrAdd(ParentMenuPath).SubMenuPaths.AddUnique(PreviousMenuPath);
			PerMenuData->FindOrAdd(PreviousMenuPath).ParentMenuPath = ParentMenuPath;
			PerMenuData->FindOrAdd(PreviousMenuPath).Name = RemainingPath;
			if(!ParentMenuPath.Contains(UTLLRN_ModularRig::NamespaceSeparator))
			{
				PerMenuData->FindOrAdd(FString()).SubMenuPaths.AddUnique(ParentMenuPath);
				PerMenuData->FindOrAdd(ParentMenuPath).Name = ParentMenuPath;
			}
			PreviousMenuPath = ParentMenuPath;
		}

		FPerMenuData& Data = PerMenuData->FindOrAdd(MenuPath);
		if(Data.Name.IsEmpty())
		{
			Data.Name = MenuPath;
		}

		Data.Variables.Add(BindingPath);
		if(!MenuPath.IsEmpty())
		{
			PerMenuData->FindChecked(Data.ParentMenuPath).SubMenuPaths.AddUnique(MenuPath);
		}
	}

	// build the menu
	FPerMenuData::SetupMenu(SharedThis(this), InProperty, MenuBuilder, FString(), PerMenuData);
}

bool FTLLRN_RigModuleInstanceDetails::CanRemoveBinding(FName InPropertyName) const
{
	// offer the "removing binding" button if any of the selected module instances
	// has a binding for the given variable
	for(const FPerModuleInfo& Info : PerModuleInfos)
	{
		if (const FTLLRN_RigModuleInstance* ModuleInstance = Info.GetModule())
		{
			if(ModuleInstance->VariableBindings.Contains(InPropertyName))
			{
				return true;
			}
		}
	}
	return false; 
}

void FTLLRN_RigModuleInstanceDetails::HandleRemoveBinding(FName InPropertyName) const
{
	FScopedTransaction Transaction(LOCTEXT("RemoveModuleVariableTransaction", "Remove Binding"));
	for(const FPerModuleInfo& Info : PerModuleInfos)
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = Info.GetBlueprint())
		{
			if (const FTLLRN_RigModuleInstance* ModuleInstance = Info.GetModule())
			{
				UTLLRN_ModularTLLRN_RigController* Controller = Blueprint->GetTLLRN_ModularTLLRN_RigController();
				Controller->UnBindModuleVariable(ModuleInstance->GetPath(), InPropertyName);
			}
		}
	}
}

void FTLLRN_RigModuleInstanceDetails::HandleChangeBinding(const FProperty* InProperty, const FString& InNewVariablePath) const
{
	FScopedTransaction Transaction(LOCTEXT("BindModuleVariableTransaction", "Bind Module Variable"));
	for(const FPerModuleInfo& Info : PerModuleInfos)
	{
		if (UTLLRN_ControlRigBlueprint* Blueprint = Info.GetBlueprint())
		{
			if (const FTLLRN_RigModuleInstance* ModuleInstance = Info.GetModule())
			{
				UTLLRN_ModularTLLRN_RigController* Controller = Blueprint->GetTLLRN_ModularTLLRN_RigController();
				Controller->BindModuleVariable(ModuleInstance->GetPath(), InProperty->GetFName(), InNewVariablePath);
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE
