// Copyright Epic Games, Inc. All Rights Reserved.

#include "STLLRN_RigModuleAssetBrowser.h"

#include "ContentBrowserModule.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "IContentBrowserSingleton.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/AssetRegistryTagsContext.h"

#include "TLLRN_ControlRigEditor.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SRigVMVariantWidget.h"

#define LOCTEXT_NAMESPACE "TLLRN_RigModuleAssetBrowser"

void STLLRN_RigModuleAssetBrowser::Construct(
	const FArguments& InArgs,
	TSharedRef<FTLLRN_ControlRigEditor> InEditor)
{
	TLLRN_ControlRigEditor = InEditor;

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
			SAssignNew(AssetBrowserBox, SBox)
		]
	];

	RefreshView();
}

void STLLRN_RigModuleAssetBrowser::RefreshView()
{
	FAssetPickerConfig AssetPickerConfig;
	
	// setup filtering
	AssetPickerConfig.Filter.ClassPaths.Add(UTLLRN_ControlRigBlueprint::StaticClass()->GetClassPathName());
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.bAddFilterUI = true;
	AssetPickerConfig.bShowPathInColumnView = true;
	AssetPickerConfig.bShowTypeInColumnView = true;
	AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &STLLRN_RigModuleAssetBrowser::OnShouldFilterAsset);
	AssetPickerConfig.DefaultFilterMenuExpansion = EAssetTypeCategories::Blueprint;
	AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &STLLRN_RigModuleAssetBrowser::OnGetAssetContextMenu);
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.bFocusSearchBoxWhenOpened = false;
	AssetPickerConfig.bAllowDragging = true;
	AssetPickerConfig.bAllowRename = false;
	AssetPickerConfig.bForceShowPluginContent = true;
	AssetPickerConfig.bForceShowEngineContent = true;
	AssetPickerConfig.InitialThumbnailSize = EThumbnailSize::Small;
	AssetPickerConfig.OnGetCustomAssetToolTip = FOnGetCustomAssetToolTip::CreateSP(this, &STLLRN_RigModuleAssetBrowser::CreateCustomAssetToolTip);

	// hide all asset registry columns by default (we only really want the name and path)
	UObject* DefaultTLLRN_ControlRigBlueprint = UTLLRN_ControlRigBlueprint::StaticClass()->GetDefaultObject();
	FAssetRegistryTagsContextData Context(DefaultTLLRN_ControlRigBlueprint, EAssetRegistryTagsCaller::Uncategorized);
	DefaultTLLRN_ControlRigBlueprint->GetAssetRegistryTags(Context);
	for (TPair<FName, UObject::FAssetRegistryTag>& AssetRegistryTagPair : Context.Tags)
	{
		AssetPickerConfig.HiddenColumnNames.Add(AssetRegistryTagPair.Value.Name.ToString());
	}

	// Also hide the type column by default (but allow users to enable it, so don't use bShowTypeInColumnView)
	AssetPickerConfig.HiddenColumnNames.Add(TEXT("Class"));
	AssetPickerConfig.HiddenColumnNames.Add(TEXT("Has Virtualized Data"));

	// allow to open the rigs directly on double click
	AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &STLLRN_RigModuleAssetBrowser::OnAssetDoubleClicked);

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	AssetBrowserBox->SetContent(ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig));
}

TSharedPtr<SWidget> STLLRN_RigModuleAssetBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const
{
	if (SelectedAssets.Num() <= 0)
	{
		return nullptr;
	}

	UObject* SelectedAsset = SelectedAssets[0].GetAsset();
	if (SelectedAsset == nullptr)
	{
		return nullptr;
	}
	
	FMenuBuilder MenuBuilder(true, MakeShared<FUICommandList>());

	MenuBuilder.BeginSection(TEXT("Asset"), LOCTEXT("AssetSectionLabel", "Asset"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Browse", "Browse to Asset"),
			LOCTEXT("BrowseTooltip", "Browses to the associated asset and selects it in the most recently used Content Browser (summoning one if necessary)"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SystemWideCommands.FindInContentBrowser.Small"),
			FUIAction(
				FExecuteAction::CreateLambda([SelectedAsset] ()
				{
					if (SelectedAsset)
					{
						const TArray<FAssetData>& Assets = { SelectedAsset };
						const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
						ContentBrowserModule.Get().SyncBrowserToAssets(Assets);
					}
				}),
				FCanExecuteAction::CreateLambda([] () { return true; })
			)
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

bool STLLRN_RigModuleAssetBrowser::OnShouldFilterAsset(const struct FAssetData& AssetData)
{
	// is this an control rig blueprint asset?
	if (!AssetData.IsInstanceOf(UTLLRN_ControlRigBlueprint::StaticClass()))
	{
		return true;
	}
	
	static const UEnum* ControlTypeEnum = StaticEnum<ETLLRN_ControlRigType>();
	const FString TLLRN_ControlRigTypeStr = AssetData.GetTagValueRef<FString>(TEXT("TLLRN_ControlRigType"));
	if (TLLRN_ControlRigTypeStr.IsEmpty())
	{
		return true;
	}

	const ETLLRN_ControlRigType TLLRN_ControlRigType = (ETLLRN_ControlRigType)(ControlTypeEnum->GetValueByName(*TLLRN_ControlRigTypeStr));
	if(TLLRN_ControlRigType != ETLLRN_ControlRigType::TLLRN_RigModule)
	{
		return true;
	}

	// static const FName AssetVariantPropertyName = GET_MEMBER_NAME_CHECKED(URigVMBlueprint, AssetVariant);
	// const FProperty* AssetVariantProperty = CastField<FProperty>(URigVMBlueprint::StaticClass()->FindPropertyByName(AssetVariantPropertyName));
	// const FString VariantStr = AssetData.GetTagValueRef<FString>(AssetVariantPropertyName);
	// if(!VariantStr.IsEmpty())
	// {
	// 	FRigVMVariant AssetVariant;
	// 	AssetVariantProperty->ImportText_Direct(*VariantStr, &AssetVariant, nullptr, EPropertyPortFlags::PPF_None);

	// 	for(const FRigVMTag& VariantTag : AssetVariant.Tags)
	// 	{
	// 		if(VariantTag.bMarksSubjectAsInvalid)
	// 		{
	// 			return true;
	// 		}
	// 	}
	// }

	return false;
}

void STLLRN_RigModuleAssetBrowser::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	if (UAssetEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
	{
		EditorSubsystem->OpenEditorForAsset(AssetData.ToSoftObjectPath());
	}
}

TSharedRef<SToolTip> STLLRN_RigModuleAssetBrowser::CreateCustomAssetToolTip(FAssetData& AssetData)
{
	// Make a list of tags to show
	TArray<UObject::FAssetRegistryTag> Tags;
	UClass* AssetClass = FindObject<UClass>(AssetData.AssetClassPath);
	check(AssetClass);
	UObject* DefaultObject = AssetClass->GetDefaultObject();
	FAssetRegistryTagsContextData TagsContext(DefaultObject, EAssetRegistryTagsCaller::Uncategorized);
	DefaultObject->GetAssetRegistryTags(TagsContext);

	TArray<FName> TagsToShow;
	static const FName ModulePath(TEXT("Path"));
	static const FName ModuleSettings(TEXT("TLLRN_RigModuleSettings"));
	for (const TPair<FName, UObject::FAssetRegistryTag>& TagPair : TagsContext.Tags)
	{
		if(TagPair.Key == ModulePath ||
			TagPair.Key == ModuleSettings)
		{
			TagsToShow.Add(TagPair.Key);
		}
	}

	TMap<FName, FText> TagsAndValuesToShow;

	// Add asset registry tags to a text list; except skeleton as that is implied in Persona
	TSharedRef<SVerticalBox> DescriptionBox = SNew(SVerticalBox);

	// static const FName AssetVariantPropertyName = GET_MEMBER_NAME_CHECKED(URigVMBlueprint, AssetVariant);
	// const FProperty* AssetVariantProperty = CastField<FProperty>(URigVMBlueprint::StaticClass()->FindPropertyByName(AssetVariantPropertyName));
	// const FString VariantStr = AssetData.GetTagValueRef<FString>(AssetVariantPropertyName);
	// if(!VariantStr.IsEmpty())
	// {
	// 	FRigVMVariant AssetVariant;
	// 	AssetVariantProperty->ImportText_Direct(*VariantStr, &AssetVariant, nullptr, EPropertyPortFlags::PPF_None);

	// 	if(!AssetVariant.Tags.IsEmpty())
	// 	{
	// 		DescriptionBox->AddSlot()
	// 		.AutoHeight()
	// 		.Padding(0,0,5,0)
	// 		[
	// 			SNew(SHorizontalBox)
	// 			+ SHorizontalBox::Slot()
	// 			.HAlign(HAlign_Left)
	// 			.VAlign(VAlign_Center)
	// 			.AutoWidth()
	// 			[
	// 				SNew(STextBlock)
	// 				.Text(LOCTEXT("AssetBrowser_RigVMTagsLabel", "Tags :"))
	// 				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
	// 			]

	// 			+ SHorizontalBox::Slot()
	// 			.AutoWidth()
	// 			.HAlign(HAlign_Left)
	// 			.VAlign(VAlign_Center)
	// 			.Padding(4, 0, 0, 0)
	// 			[
	// 				SNew(SRigVMVariantTagWidget)
	// 				.Visibility(EVisibility::Visible)
	// 				.CanAddTags(false)
	// 				.EnableContextMenu(false)
	// 				.EnableTick(false)
	// 				.Orientation(EOrientation::Orient_Horizontal)
	// 				.OnGetTags_Lambda([AssetVariant]() { return AssetVariant.Tags; })
	// 			]
	// 		];
	// 	}
	// }
	
	for(TPair<FName, FAssetTagValueRef> TagPair : AssetData.TagsAndValues)
	{
		if(TagsToShow.Contains(TagPair.Key))
		{
			// Check for DisplayName metadata
			FName DisplayName;
			if (FProperty* Field = FindFProperty<FProperty>(AssetClass, TagPair.Key))
			{
				DisplayName = *Field->GetDisplayNameText().ToString();
			}
			else
			{
				DisplayName = TagPair.Key;
			}

			if (TagPair.Key == ModuleSettings)
			{
				FTLLRN_RigModuleSettings Settings;
				FRigVMPinDefaultValueImportErrorContext ErrorPipe;
				FTLLRN_RigModuleSettings::StaticStruct()->ImportText(*TagPair.Value.GetValue(), &Settings, nullptr, PPF_None, &ErrorPipe, FString());
				if (ErrorPipe.NumErrors == 0)
				{
					TagsAndValuesToShow.Add(TEXT("Default Name"), FText::FromString(Settings.Identifier.Name));
					TagsAndValuesToShow.Add(TEXT("Category"), FText::FromString(Settings.Category));
					TagsAndValuesToShow.Add(TEXT("Keywords"), FText::FromString(Settings.Keywords));
					TagsAndValuesToShow.Add(TEXT("Description"), FText::FromString(Settings.Description));
				}
			}
			else
			{
				TagsAndValuesToShow.Add(DisplayName, TagPair.Value.AsText());
			}
		}
	}

	for (const TPair<FName, FText>& TagPair : TagsAndValuesToShow)
	{
		DescriptionBox->AddSlot()
		.AutoHeight()
		.Padding(0,0,5,0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("AssetTagKey", "{0}: "), FText::FromName(TagPair.Key)))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(TagPair.Value)
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
	}

	DescriptionBox->AddSlot()
		.AutoHeight()
		.Padding(0,0,5,0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AssetBrowser_FolderPathLabel", "Folder :"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromName(AssetData.PackagePath))
				.ColorAndOpacity(FSlateColor::UseForeground())
				.WrapTextAt(300.f)
			]
		];

	TSharedPtr<SHorizontalBox> ContentBox = nullptr;
	TSharedRef<SToolTip> ToolTipWidget = SNew(SToolTip)
	.TextMargin(1.f)
	.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
	[
		SNew(SBorder)
		.Padding(6.f)
		.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder"))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0,0,0,4)
			[
				SNew(SBorder)
				.Padding(6.f)
				.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
				[
					SNew(SBox)
					.HAlign(HAlign_Left)
					[
						SNew(STextBlock)
						.Text(FText::FromName(AssetData.AssetName))
						.Font(FAppStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
					]
				]
			]
		
			+ SVerticalBox::Slot()
			[
				SAssignNew(ContentBox, SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBorder)
					.Padding(6.f)
					.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
					[
						DescriptionBox
					]
				]
			]
		]
	];
	return ToolTipWidget;
}



#undef LOCTEXT_NAMESPACE
