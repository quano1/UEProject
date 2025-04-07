// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/STLLRN_ControlRigDetails.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Styling/CoreStyle.h"
#include "ScopedTransaction.h"
#include "TLLRN_ControlRig.h"
#include "UnrealEdGlobals.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "EditorModeManager.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Selection.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "EditMode/TLLRN_TLLRN_AnimDetailsProxy.h"
#include "EditMode/TLLRN_ControlRigEditModeSettings.h"
#include "Modules/ModuleManager.h"
#include "TimerManager.h"
#include "CurveEditor.h"
#include "MVVM/CurveEditorExtension.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "Tracks/MovieScenePropertyTrack.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieSceneIntegerTrack.h"
#include "Tracks/MovieSceneDoubleTrack.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Tracks/MovieSceneBoolTrack.h"
#include "MovieSceneCommonHelpers.h"

#define LOCTEXT_NAMESPACE "TLLRN_ControlRigDetails"

void FTLLRN_ControlRigEditModeGenericDetails::CustomizeDetails(class IDetailLayoutBuilder& DetailLayout)
{
}
void STLLRN_ControlRigDetails::Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode& InEditMode)
{
	using namespace UE::Sequencer;

	ModeTools = InEditMode.GetModeManager();
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = true;
		DetailsViewArgs.bCustomNameAreaLocation = true;
		DetailsViewArgs.bCustomFilterAreaLocation = false;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bAllowMultipleTopLevelObjects = false;
		DetailsViewArgs.bShowScrollBar = false; // Don't need to show this, as we are putting it in a scroll box
	}

	FDetailsViewArgs IndividualDetailsViewArgs = DetailsViewArgs;
	IndividualDetailsViewArgs.bAllowMultipleTopLevelObjects = true; //this is the secret sauce to show multiple objects in a details view

	auto CreateDetailsView = [this](FDetailsViewArgs InDetailsViewArgs) -> TSharedPtr<IDetailsView>
	{
		FTLLRN_ControlRigEditMode* EditMode = GetEditMode();
		TSharedPtr<IDetailsView> DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(InDetailsViewArgs);
		DetailsView->SetKeyframeHandler(EditMode->DetailKeyFrameCache);
		DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &STLLRN_ControlRigDetails::ShouldShowPropertyOnDetailCustomization));
		DetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &STLLRN_ControlRigDetails::IsReadOnlyPropertyOnDetailCustomization));
		DetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FTLLRN_ControlRigEditModeGenericDetails::MakeInstance, ModeTools));
		return DetailsView;
	};

	AllControlsView = CreateDetailsView(IndividualDetailsViewArgs);

	ChildSlot
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					AllControlsView.ToSharedRef()
				]
			]
		];

	SetEditMode(InEditMode);

}

void STLLRN_ControlRigDetails::SetEditMode(FTLLRN_ControlRigEditMode& InEditMode)
{
	FTLLRN_ControlRigBaseDockableView::SetEditMode(InEditMode);
	if (GetEditMode()->GetWeakSequencer().IsValid())
	{
		SequencerTracker.SetSequencerAndDetails(GetEditMode()->GetWeakSequencer(), this);
		UpdateProxies();
	}
}

STLLRN_ControlRigDetails::~STLLRN_ControlRigDetails()
{
	//base class handles control rig related cleanup
}

void STLLRN_ControlRigDetails::SelectedSequencerObjects(const TMap<UObject*, FArrayOfPropertyTracks>& InObjectsTracked)
{
	TMap<UObject*, FArrayOfPropertyTracks> SequencerObjects;
	for (const TPair<UObject*, FArrayOfPropertyTracks>& Pair : InObjectsTracked)
	{
		if(Pair.Key && (Pair.Key->IsA<AActor>() || Pair.Key->IsA<UActorComponent>()))
		{
			SequencerObjects.Add(Pair);
		}
	}

	HandleSequencerObjects(SequencerObjects);
	UpdateProxies();
}
void STLLRN_ControlRigDetails::HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControl, bool bSelected)
{
	FTLLRN_ControlRigBaseDockableView::HandleControlSelected(Subject, InControl, bSelected);
	UpdateProxies();
}

static TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> GetParentProxies(UTLLRN_ControlTLLRN_RigControlsProxy* ChildProxy, const TArray<UTLLRN_ControlTLLRN_RigControlsProxy*>& Proxies)
{
	if (!ChildProxy || !ChildProxy->OwnerTLLRN_ControlRig.IsValid())
	{
		return {};
	}
	if (!ChildProxy->OwnerControlElement.UpdateCache(ChildProxy->OwnerTLLRN_ControlRig->GetHierarchy()))
	{
		return {};
	}
	TArray<FTLLRN_RigBaseElement*> Parents;
	if(ChildProxy && ChildProxy->OwnerTLLRN_ControlRig.IsValid())
	{
		Parents = ChildProxy->OwnerTLLRN_ControlRig->GetHierarchy()->GetParents(ChildProxy->OwnerControlElement.GetElement()); 
	}

	TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> ParentProxies;
	for (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy : Proxies)
	{
		if (Proxy && Proxy->OwnerTLLRN_ControlRig.IsValid())
		{
			if (Proxy->OwnerControlElement.UpdateCache(Proxy->OwnerTLLRN_ControlRig->GetHierarchy()))
			{
				if(const FTLLRN_RigControlElement* OwnerControlElement = Cast<FTLLRN_RigControlElement>(Proxy->OwnerControlElement.GetElement()))
				{
					if (Parents.Contains(OwnerControlElement))
					{
						ParentProxies.AddUnique(Proxy);
					}
					if(const FTLLRN_RigControlElement* ChildControlElement = Cast<FTLLRN_RigControlElement>(ChildProxy->OwnerControlElement.GetElement()))
					{
						if(ChildControlElement->Settings.Customization.AvailableSpaces.Contains(OwnerControlElement->GetKey()))
						{
							ParentProxies.AddUnique(Proxy);
						}
					}
				}
			}
		}
	}
	return ParentProxies;
}
static UTLLRN_ControlTLLRN_RigControlsProxy* GetProxyWithSameType(TArray<TWeakObjectPtr<>>& AllProxies, ETLLRN_RigControlType ControlType, bool bIsEnum)
{
	for (TWeakObjectPtr<> ExistingProxy : AllProxies)
	{
		if (ExistingProxy.IsValid())
		{
			switch (ControlType)
			{
			case ETLLRN_RigControlType::Transform:
			case ETLLRN_RigControlType::TransformNoScale:
			case ETLLRN_RigControlType::EulerTransform:
			{
				if (ExistingProxy.Get()->IsA<UTLLRN_AnimDetailControlsProxyTransform>())
				{
					return  Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(ExistingProxy.Get());
				}
				break;
			}
			case ETLLRN_RigControlType::Float:
			case ETLLRN_RigControlType::ScaleFloat:
			{
				if (ExistingProxy.Get()->IsA<UTLLRN_AnimDetailControlsProxyFloat>())
				{
					return  Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(ExistingProxy.Get());
				}
				break;

			}
			case ETLLRN_RigControlType::Integer:
			{
				if (bIsEnum == false)
				{
					if (ExistingProxy.Get()->IsA<UTLLRN_AnimDetailControlsProxyInteger>())
					{
						return  Cast<UTLLRN_AnimDetailControlsProxyInteger>(ExistingProxy.Get());
					}
				}
				else
				{
					if (ExistingProxy.Get()->IsA<UTLLRN_AnimDetailControlsProxyEnum>())
					{
						return  Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(ExistingProxy.Get());
					}
				}
				break;

			}
			case ETLLRN_RigControlType::Position:
			{
				if (ExistingProxy.Get()->IsA<UTLLRN_AnimDetailControlsProxyLocation>())
				{
					return  Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(ExistingProxy.Get());
				}
				break;
			}
			case ETLLRN_RigControlType::Rotator:
				{
				if (ExistingProxy.Get()->IsA<UTLLRN_AnimDetailControlsProxyRotation>())
				{
					return  Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(ExistingProxy.Get());
				}
				break;
			}
			case ETLLRN_RigControlType::Scale:
			{
				if (ExistingProxy.Get()->IsA<UTLLRN_AnimDetailControlsProxyScale>())
				{
					return  Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(ExistingProxy.Get());
				}
				break;
			}
			case ETLLRN_RigControlType::Vector2D:
			{
				if (ExistingProxy.Get()->IsA<UTLLRN_AnimDetailControlsProxyVector2D>())
				{
					return  Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(ExistingProxy.Get());
				}
				break;
			}
			case ETLLRN_RigControlType::Bool:
			{
				if (ExistingProxy.Get()->IsA<UTLLRN_AnimDetailControlsProxyBool>())
				{
					return  Cast<UTLLRN_ControlTLLRN_RigControlsProxy>(ExistingProxy.Get());
				}
				break;
			}
			}
		}
	}
	return nullptr;
}

void STLLRN_ControlRigDetails::HandleSequencerObjects(TMap<UObject*, FArrayOfPropertyTracks>& SequencerObjects)
{
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		if (UTLLRN_ControlRigDetailPanelControlProxies* ControlProxy = EditMode->GetDetailProxies())
		{
			TMap<ETLLRN_RigControlType, FSequencerProxyPerType>  ProxyPerType;
			for (TPair<UObject*, FArrayOfPropertyTracks>& Pair : SequencerObjects)
			{
				for (UMovieSceneTrack* Track : Pair.Value.PropertyTracks)
				{
					if (UMovieScenePropertyTrack* PropTrack = Cast<UMovieScenePropertyTrack>(Track))
					{
						auto AddBinding = [this, PropTrack](UObject* InObject,FSequencerProxyPerType& Binding)
						{
							TArray<FBindingAndTrack>& Bindings = Binding.Bindings.FindOrAdd(InObject);
							TSharedPtr<FTrackInstancePropertyBindings> PropertyBindings = MakeShareable(new FTrackInstancePropertyBindings(PropTrack->GetPropertyName(), PropTrack->GetPropertyPath().ToString()));
							FBindingAndTrack BindingAndTrack(PropertyBindings, PropTrack);
							Bindings.Add(BindingAndTrack);
						};
						if (PropTrack->IsA<UMovieScene3DTransformTrack>())
						{
							FSequencerProxyPerType& Binding = ProxyPerType.FindOrAdd(ETLLRN_RigControlType::Transform);
							AddBinding(Pair.Key, Binding);
						}
						else if (PropTrack->IsA<UMovieSceneBoolTrack>())
						{
							FSequencerProxyPerType& Binding = ProxyPerType.FindOrAdd(ETLLRN_RigControlType::Bool);
							AddBinding(Pair.Key, Binding);
						}
						else if (PropTrack->IsA<UMovieSceneIntegerTrack>())
						{
							FSequencerProxyPerType& Binding = ProxyPerType.FindOrAdd(ETLLRN_RigControlType::Integer);
							AddBinding(Pair.Key, Binding);
						}
						else if (PropTrack->IsA<UMovieSceneDoubleTrack>() ||
							PropTrack->IsA<UMovieSceneFloatTrack>())
						{
							FSequencerProxyPerType& Binding = ProxyPerType.FindOrAdd(ETLLRN_RigControlType::Float);
							AddBinding(Pair.Key, Binding);
						}
					}
				}
			}
			ControlProxy->ResetSequencerProxies(ProxyPerType);
		}
	}
}

void STLLRN_ControlRigDetails::UpdateProxies()
{
	if(NextTickTimerHandle.IsValid() == false)
	{ 
		TWeakPtr<SWidget> WeakPtr = AsWeak();

		//proxies that are in edit mode are also listening to the same messages so they may not be set up yet so need to wait
		NextTickTimerHandle = GEditor->GetTimerManager()->SetTimerForNextTick([WeakPtr]()
		{
			if(!WeakPtr.IsValid())
			{
				return;
			}
			TSharedPtr<STLLRN_ControlRigDetails> StrongThis = StaticCastSharedPtr<STLLRN_ControlRigDetails>(WeakPtr.Pin());
			if(!StrongThis.IsValid())
			{
				return;
			}
		
			TArray<TWeakObjectPtr<>> AllProxies;
			TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> ChildProxies; //list of 'child' proxies that will show up as custom attributes
			if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(StrongThis->ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
			{
				if (UTLLRN_ControlRigDetailPanelControlProxies* ControlProxy = EditMode->GetDetailProxies())
				{
					const TArray<UTLLRN_ControlTLLRN_RigControlsProxy*>& Proxies = ControlProxy->GetAllSelectedProxies();
					for (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy : Proxies)
					{
						if (Proxy == nullptr || !IsValid(Proxy))
						{
							continue;
						}
						Proxy->ResetItems();

						if (Proxy->bIsIndividual)
						{
							ChildProxies.Add(Proxy);
						}
						else
						{
							TObjectPtr<UEnum> EnumPtr = nullptr;
							if (Proxy->OwnerTLLRN_ControlRig.IsValid())
							{
								if (Proxy->OwnerControlElement.UpdateCache(Proxy->OwnerTLLRN_ControlRig->GetHierarchy()))
								{
									if (const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(Proxy->OwnerControlElement.GetElement()))
									{
										EnumPtr = ControlElement->Settings.ControlEnum;
									}
								}
							}
							if (UTLLRN_ControlTLLRN_RigControlsProxy* ExistingProxy = GetProxyWithSameType(AllProxies, Proxy->Type, EnumPtr != nullptr))
							{
								ExistingProxy->AddItem(Proxy);
								ExistingProxy->ValueChanged();
							}
							else
							{
								AllProxies.Add(Proxy);
							}
						}
					}
					//now add child proxies to parents if parents also selected...
					for (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy : ChildProxies)
					{
						TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> ParentProxies = GetParentProxies(Proxy, Proxies);
						for (UTLLRN_ControlTLLRN_RigControlsProxy* ParentProxy : ParentProxies)
						{
							TObjectPtr<UEnum> EnumPtr = nullptr;
							if (ParentProxy->OwnerTLLRN_ControlRig.IsValid())
							{
								if (ParentProxy->OwnerControlElement.UpdateCache(ParentProxy->OwnerTLLRN_ControlRig->GetHierarchy()))
								{
									if (const FTLLRN_RigControlElement* ControlElement = Cast<FTLLRN_RigControlElement>(ParentProxy->OwnerControlElement.GetElement()))
									{
										EnumPtr = ControlElement->Settings.ControlEnum;
									}
								}
							}
							if (UTLLRN_ControlTLLRN_RigControlsProxy* ExistingProxy = GetProxyWithSameType(AllProxies, ParentProxy->Type, EnumPtr != nullptr))
							{
								ExistingProxy->AddChildProxy(Proxy);
							}
						}

						if(ParentProxies.IsEmpty())
						{
							AllProxies.Add(Proxy);
						}
					}
					for (UTLLRN_ControlTLLRN_RigControlsProxy* Proxy : Proxies)
					{
						Proxy->ValueChanged();
					}
				}
			}
		
			StrongThis->AllControlsView->SetObjects(AllProxies,true);
			StrongThis->NextTickTimerHandle.Invalidate();
		});
	}
}

FReply STLLRN_ControlRigDetails::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (FTLLRN_ControlRigEditMode* EditMode = static_cast<FTLLRN_ControlRigEditMode*>(ModeTools->GetActiveMode(FTLLRN_ControlRigEditMode::ModeName)))
	{
		TWeakPtr<ISequencer> Sequencer = EditMode->GetWeakSequencer();
		if (Sequencer.IsValid())
		{
			using namespace UE::Sequencer;
			const TSharedPtr<FSequencerEditorViewModel> SequencerViewModel = Sequencer.Pin()->GetViewModel();
			const FCurveEditorExtension* CurveEditorExtension = SequencerViewModel->CastDynamic<FCurveEditorExtension>();
			check(CurveEditorExtension);
			TSharedPtr<FCurveEditor> CurveEditor = CurveEditorExtension->GetCurveEditor();
			if (CurveEditor->GetCommands()->ProcessCommandBindings(InKeyEvent))
			{
				return FReply::Handled();
			}
		}
	}
	return FReply::Unhandled();
}

bool STLLRN_ControlRigDetails::ShouldShowPropertyOnDetailCustomization(const FPropertyAndParent& InPropertyAndParent) const
{
	auto ShouldPropertyBeVisible = [](const FProperty& InProperty)
	{
		bool bShow = InProperty.HasAnyPropertyFlags(CPF_Interp) || InProperty.HasMetaData(FRigVMStruct::InputMetaName) || InProperty.HasMetaData(FRigVMStruct::OutputMetaName);
		return bShow;
	};

	bool bContainsVisibleProperty = false;
	if (InPropertyAndParent.Property.IsA<FStructProperty>())
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(&InPropertyAndParent.Property);
		for (TFieldIterator<FProperty> PropertyIt(StructProperty->Struct); PropertyIt; ++PropertyIt)
		{
			if (ShouldPropertyBeVisible(**PropertyIt))
			{
				return true;
			}
		}
	}

	return ShouldPropertyBeVisible(InPropertyAndParent.Property) ||
		(InPropertyAndParent.ParentProperties.Num() > 0 && ShouldPropertyBeVisible(*InPropertyAndParent.ParentProperties[0]));
}

bool STLLRN_ControlRigDetails::IsReadOnlyPropertyOnDetailCustomization(const FPropertyAndParent& InPropertyAndParent) const
{
	auto ShouldPropertyBeEnabled = [](const FProperty& InProperty)
	{
		bool bShow = InProperty.HasAnyPropertyFlags(CPF_Interp) || InProperty.HasMetaData(FRigVMStruct::InputMetaName);
		return bShow;
	};

	bool bContainsVisibleProperty = false;
	if (InPropertyAndParent.Property.IsA<FStructProperty>())
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(&InPropertyAndParent.Property);
		for (TFieldIterator<FProperty> PropertyIt(StructProperty->Struct); PropertyIt; ++PropertyIt)
		{
			if (ShouldPropertyBeEnabled(**PropertyIt))
			{
				return false;
			}
		}
	}

	return !(ShouldPropertyBeEnabled(InPropertyAndParent.Property) ||
		(InPropertyAndParent.ParentProperties.Num() > 0 && ShouldPropertyBeEnabled(*InPropertyAndParent.ParentProperties[0])));
}


FSequencerTracker::~FSequencerTracker()
{
	RemoveDelegates();
}

void FSequencerTracker::RemoveDelegates()
{
	if (TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin())
	{
		Sequencer->GetSelectionChangedObjectGuids().RemoveAll(this);
	}
}

void FSequencerTracker::SetSequencerAndDetails(TWeakPtr<ISequencer> InWeakSequencer, STLLRN_ControlRigDetails* InTLLRN_ControlRigDetails)
{
	RemoveDelegates();
	WeakSequencer = InWeakSequencer;
	TLLRN_ControlRigDetails = InTLLRN_ControlRigDetails;
	if (WeakSequencer.IsValid() == false || InTLLRN_ControlRigDetails == nullptr)
	{
		return;
	}
	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();

	TArray<FGuid> SequencerSelectedObjects;
	Sequencer->GetSelectedObjects(SequencerSelectedObjects);
	UpdateSequencerBindings(SequencerSelectedObjects);

	Sequencer->GetSelectionChangedObjectGuids().AddRaw(this, &FSequencerTracker::UpdateSequencerBindings);
	
}

void FSequencerTracker::UpdateSequencerBindings(TArray<FGuid> SequencerBindings)
{
	const FDateTime StartTime = FDateTime::Now();

	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	check(Sequencer);
	ObjectsTracked.Reset();
	for (FGuid BindingGuid : SequencerBindings)
	{
		FArrayOfPropertyTracks Properties;
		Properties.PropertyTracks = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->FindTracks(UMovieScenePropertyTrack::StaticClass(), BindingGuid);
		if (Properties.PropertyTracks.Num() == 0)
		{
			continue;
		}
		for (TWeakObjectPtr<> BoundObject : Sequencer->FindBoundObjects(BindingGuid, Sequencer->GetFocusedTemplateID()))
		{
			if (!BoundObject.IsValid())
			{
				continue;
			}
			ObjectsTracked.FindOrAdd(BoundObject.Get(), Properties);

		}
	}
	if (TLLRN_ControlRigDetails)
	{
		TLLRN_ControlRigDetails->SelectedSequencerObjects(ObjectsTracked);
	}
}


#undef LOCTEXT_NAMESPACE
