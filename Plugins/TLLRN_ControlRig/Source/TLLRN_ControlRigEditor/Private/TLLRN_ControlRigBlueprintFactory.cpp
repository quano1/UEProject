// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigBlueprintFactory.h"
#include "UObject/Interface.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "AssetRegistry/AssetData.h"
#include "Editor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "TLLRN_ControlRigBlueprintActions.h"
#include "TLLRN_ControlRigEditorModule.h"
#include "TLLRN_ModularRig.h"
#include "RigVMBlueprintGeneratedClass.h"
#include "Graph/TLLRN_ControlRigGraphSchema.h"
#include "Graph/TLLRN_ControlRigGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "TLLRN_ModularTLLRN_RigController.h"
#include "Settings/TLLRN_ControlRigSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlRigBlueprintFactory)

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigBlueprintFactory"

/** Dialog to configure creation properties */
class STLLRN_ControlRigBlueprintCreateDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( STLLRN_ControlRigBlueprintCreateDialog ){}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs )
	{
		bOkClicked = false;
		ParentClass = UTLLRN_ControlRig::StaticClass(); // default to control rig

		ChildSlot
		[
			SNew(SBorder)
			.Visibility(EVisibility::Visible)
			.BorderImage(FAppStyle::GetBrush("Menu.Background"))
			[
				SNew(SBox)
				.Visibility(EVisibility::Visible)
				.WidthOverride(500.0f)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.FillHeight(1)
					[
						SNew(SBorder)
						.BorderImage( FAppStyle::GetBrush("ToolPanel.GroupBorder") )
						.Content()
						[
							SAssignNew(ParentClassContainer, SVerticalBox)
						]
					]

					// Ok/Cancel buttons
					+SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					.Padding(8)
					[
						SNew(SUniformGridPanel)
						.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
						.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
						.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
						+SUniformGridPanel::Slot(0,0)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.ContentPadding( FAppStyle::GetMargin("StandardDialog.ContentPadding") )
							.OnClicked(this, &STLLRN_ControlRigBlueprintCreateDialog::OkClicked)	
							.Text(LOCTEXT("CreateTLLRN_ControlRigBlueprintCreate", "Create"))
						]
						+SUniformGridPanel::Slot(1,0)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.ContentPadding( FAppStyle::GetMargin("StandardDialog.ContentPadding") )
							.OnClicked(this, &STLLRN_ControlRigBlueprintCreateDialog::CancelClicked)
							.Text(LOCTEXT("CreateTLLRN_ControlRigBlueprintCancel", "Cancel"))
						]
					]
				]
			]
		];

		MakeParentClassPicker();
	}
	
	bool ConfigureProperties(TWeakObjectPtr<UTLLRN_ControlRigBlueprintFactory> InTLLRN_ControlRigBlueprintFactory)
	{
		TLLRN_ControlRigBlueprintFactory = InTLLRN_ControlRigBlueprintFactory;

		TSharedRef<SWindow> Window = SNew(SWindow)
		.Title( LOCTEXT("CreateTLLRN_ControlRigBlueprintOptions", "Create Control Rig Blueprint") )
		.ClientSize(FVector2D(400, 400))
		.SupportsMinimize(false) 
		.SupportsMaximize(false)
		[
			AsShared()
		];

		PickerWindow = Window;

		GEditor->EditorAddModalWindow(Window);
		TLLRN_ControlRigBlueprintFactory.Reset();

		return bOkClicked;
	}

private:
	class FTLLRN_ControlRigBlueprintParentFilter : public IClassViewerFilter
	{
	public:
		/** All children of these classes will be included unless filtered out by another setting. */
		TSet< const UClass* > AllowedChildrenOfClasses;

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) override
		{
			// If it appears on the allowed child-of classes list (or there is nothing on that list)
			if (InClass)
			{
				if (InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) == EFilterReturn::Failed)
				{
					return false;
				}

				// in the future we might allow it to parent to BP classes, but right now, it won't work well because of multi rig graph
				// for now we disable it and only allow native class. 
				if (!InClass->HasAnyClassFlags(CLASS_Deprecated) && InClass->HasAnyClassFlags(CLASS_Native) && InClass->GetOutermost() != GetTransientPackage())
				{
					// see if it can be blueprint base
					const FString BlueprintBaseMetaKey(TEXT("IsBlueprintBase"));

					if (InClass->HasMetaData(*BlueprintBaseMetaKey))
					{
						if (InClass->GetMetaData(*BlueprintBaseMetaKey) == TEXT("true"))
						{
							return true;
						}
					}
				}
			}

			return false;
		}

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
		{
			// If it appears on the allowed child-of classes list (or there is nothing on that list)
			// in the future we might allow it to parent to BP classes, but right now, it won't work well because of multi rig graph
			// for now we disable it and only allow native class. 
			return false; // InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
		}
	};

	/** Creates the combo menu for the parent class */
	void MakeParentClassPicker()
	{
		// Load the classviewer module to display a class picker
		FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

		// Fill in options
		FClassViewerInitializationOptions Options;
		Options.Mode = EClassViewerMode::ClassPicker;

		TSharedPtr<FTLLRN_ControlRigBlueprintParentFilter> Filter = MakeShareable(new FTLLRN_ControlRigBlueprintParentFilter());
		Options.ClassFilters.Add(Filter.ToSharedRef());

		// All child child classes of UTLLRN_ControlRig are valid.
		Filter->AllowedChildrenOfClasses.Add(UTLLRN_ControlRig::StaticClass());

		ParentClassContainer->ClearChildren();
		ParentClassContainer->AddSlot()
		.AutoHeight()
		[
			SNew( STextBlock )
			.Text( LOCTEXT("ParentRig", "Parent Rig:") )
			.ShadowOffset( FVector2D(1.0f, 1.0f) )
		];

		ParentClassContainer->AddSlot()
		[
			ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &STLLRN_ControlRigBlueprintCreateDialog::OnClassPicked))
		];
	}

	/** Handler for when a parent class is selected */
	void OnClassPicked(UClass* ChosenClass)
	{
		ParentClass = ChosenClass;
	}

	/** Handler for when ok is clicked */
	FReply OkClicked()
	{
		if (TLLRN_ControlRigBlueprintFactory.IsValid())
		{
			TLLRN_ControlRigBlueprintFactory->ParentClass = ParentClass.Get();
		}

		CloseDialog(true);

		return FReply::Handled();
	}

	void CloseDialog(bool bWasPicked=false)
	{
		bOkClicked = bWasPicked;
		if ( PickerWindow.IsValid() )
		{
			PickerWindow.Pin()->RequestDestroyWindow();
		}
	}

	/** Handler for when cancel is clicked */
	FReply CancelClicked()
	{
		CloseDialog();
		return FReply::Handled();
	}

	FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			CloseDialog();
			return FReply::Handled();
		}
		return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
	}

private:
	/** The factory for which we are setting up properties */
	TWeakObjectPtr<UTLLRN_ControlRigBlueprintFactory> TLLRN_ControlRigBlueprintFactory;

	/** A pointer to the window that is asking the user to select a parent class */
	TWeakPtr<SWindow> PickerWindow;

	/** The container for the Parent Class picker */
	TSharedPtr<SVerticalBox> ParentClassContainer;

	/** The selected class */
	TWeakObjectPtr<UClass> ParentClass;

	/** True if Ok was clicked */
	bool bOkClicked;
};

UTLLRN_ControlRigBlueprintFactory::UTLLRN_ControlRigBlueprintFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UTLLRN_ControlRigBlueprint::StaticClass();
	ParentClass = UTLLRN_ControlRig::StaticClass(); // default to control rig
}

bool UTLLRN_ControlRigBlueprintFactory::ConfigureProperties()
{
	if (CVarTLLRN_ControlTLLRN_RigHierarchyEnableModules.GetValueOnAnyThread())
	{
		TSharedRef<STLLRN_ControlRigBlueprintCreateDialog> Dialog = SNew(STLLRN_ControlRigBlueprintCreateDialog);
		return Dialog->ConfigureProperties(this);
	}
	return true;
};

UObject* UTLLRN_ControlRigBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	// Make sure we are trying to factory a Control Rig Blueprint, then create and init one
	check(Class->IsChildOf(UTLLRN_ControlRigBlueprint::StaticClass()));

	if ((ParentClass == nullptr) || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass) || !ParentClass->IsChildOf(UTLLRN_ControlRig::StaticClass()))
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ClassName"), (ParentClass != nullptr) ? FText::FromString( ParentClass->GetName() ) : LOCTEXT("Null", "(null)") );
		FMessageDialog::Open( EAppMsgType::Ok, FText::Format( LOCTEXT("CannotCreateTLLRN_ControlRigBlueprint", "Cannot create an Control Rig Blueprint based on the class '{0}'."), Args ) );
		return nullptr;
	}
	else
	{
		UTLLRN_ControlRigBlueprint* TLLRN_ControlRigBlueprint = CastChecked<UTLLRN_ControlRigBlueprint>(FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BPTYPE_Normal, UTLLRN_ControlRigBlueprint::StaticClass(), URigVMBlueprintGeneratedClass::StaticClass(), CallingContext));
		FTLLRN_ControlRigEditorModule::Get().CreateRootGraphIfRequired(TLLRN_ControlRigBlueprint);

		// add the default module
		if(ParentClass->IsChildOf(UTLLRN_ModularRig::StaticClass()))
		{
			const FSoftObjectPath DefaultRootModulePath = UTLLRN_ControlRigSettings::Get()->DefaultRootModule;
			if(const UTLLRN_ControlRigBlueprint* DefaultRootModule = Cast<UTLLRN_ControlRigBlueprint>(DefaultRootModulePath.TryLoad()))
			{
				if(UClass* DefaultRootModuleClass = Cast<UClass>(DefaultRootModule->GetTLLRN_ControlRigClass()))
				{
					if(DefaultRootModuleClass->IsChildOf(UTLLRN_ControlRig::StaticClass()))
					{
						static const FName RootName = TEXT("Root");
						TLLRN_ControlRigBlueprint->TLLRN_ModularRigModel.GetController()->AddModule(RootName, DefaultRootModuleClass, FString(), false);
					}
				}
			}
		}
		return TLLRN_ControlRigBlueprint;
	}
}

UObject* UTLLRN_ControlRigBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

UTLLRN_ControlRigBlueprint* UTLLRN_ControlRigBlueprintFactory::CreateNewTLLRN_ControlRigAsset(const FString& InDesiredPackagePath, const bool bTLLRN_ModularRig)
{
	return FTLLRN_ControlRigBlueprintActions::CreateNewTLLRN_ControlRigAsset(InDesiredPackagePath, bTLLRN_ModularRig);
}

UTLLRN_ControlRigBlueprint* UTLLRN_ControlRigBlueprintFactory::CreateTLLRN_ControlRigFromSkeletalMeshOrSkeleton(UObject* InSelectedObject, const bool bTLLRN_ModularRig)
{
	return FTLLRN_ControlRigBlueprintActions::CreateTLLRN_ControlRigFromSkeletalMeshOrSkeleton(InSelectedObject, bTLLRN_ModularRig);
}

#undef LOCTEXT_NAMESPACE

