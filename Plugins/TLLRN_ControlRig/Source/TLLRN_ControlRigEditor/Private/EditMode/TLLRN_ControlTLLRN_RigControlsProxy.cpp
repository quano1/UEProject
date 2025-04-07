// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditMode/TLLRN_ControlTLLRN_RigControlsProxy.h"
#include "EditorModeManager.h"
#include "EditMode/TLLRN_ControlRigEditMode.h"
#include "TLLRN_ControlRig.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterSection.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "Components/SkeletalMeshComponent.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "MovieSceneCommonHelpers.h"
#include "Sequencer/TLLRN_ControlRigSequencerHelpers.h"
#include "ISequencer.h"
#include "CurveEditor.h"
#include "CurveModel.h"
#include "MovieScene.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieScenePropertyTrack.h"
#include "MovieSceneCommonHelpers.h"
#include "UnrealEdGlobals.h"
#include "UnrealEdMisc.h"
#include "Editor/UnrealEdEngine.h"
#include "ScopedTransaction.h"
#include "Channels/MovieSceneBoolChannel.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "Channels/MovieSceneIntegerChannel.h"
#include "Channels/MovieSceneByteChannel.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "SEnumCombo.h"
#include "LevelEditorViewport.h"
#include "ConstraintsManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_ControlTLLRN_RigControlsProxy)

FTLLRN_RigControlElement* FTLLRN_ControlRigProxyItem::GetControlElement(const FName& InName) const
{
	FTLLRN_RigControlElement* Element = nullptr;
	if (TLLRN_ControlRig.IsValid())
	{
		Element = TLLRN_ControlRig->FindControl(InName);
	}
	return Element;
}

void UTLLRN_ControlTLLRN_RigControlsProxy::AddTLLRN_ControlTLLRN_RigControl(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InName)
{
	if (InTLLRN_ControlRig == nullptr)
	{
		return;
	}
	FTLLRN_RigControlElement* ControlElement = InTLLRN_ControlRig->FindControl(InName);
	if (ControlElement == nullptr)
	{
		return;
	}

	FTLLRN_ControlRigProxyItem& Item = TLLRN_ControlRigItems.FindOrAdd(InTLLRN_ControlRig);
	Item.TLLRN_ControlRig = InTLLRN_ControlRig;
	if (Item.ControlElements.Contains(InName) == false)
	{
		Item.ControlElements.Add(InName);
	}
	//only change label to Multiple if not an individual(attribute) control
	if (bIsIndividual == false && (TLLRN_ControlRigItems.Num() > 1 || Item.ControlElements.Num() > 1 || SequencerItems.Num() > 0))
	{
		FString DisplayString = TEXT("Multiple");
		FName DisplayName(*DisplayString);
		Name = DisplayName;
	}
	else
	{
		FText ControlNameText = InTLLRN_ControlRig->GetHierarchy()->GetDisplayNameForUI(ControlElement);
		FString ControlNameString = ControlNameText.ToString();
		Name = FName(*ControlNameString);
	}
}

TArray<FTLLRN_RigControlElement*> UTLLRN_ControlTLLRN_RigControlsProxy::GetControlElements() const
{
	TArray<FTLLRN_RigControlElement*> Elements;
	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (Items.Key.IsValid() == false)
		{
			continue;
		}
		for (const FName& CName : Items.Value.ControlElements)
		{
			if (FTLLRN_RigControlElement* Element = Items.Value.GetControlElement(CName))
			{
				Elements.Add(Element);
			}
		}
	}
	return Elements;
}

void UTLLRN_ControlTLLRN_RigControlsProxy::ResetTLLRN_ControlRigItems()
{
	ChildProxies.Reset();
	TLLRN_ControlRigItems.Reset();
	if (OwnerTLLRN_ControlRig.IsValid())
	{
		if (OwnerControlElement.IsValid())
		{
			AddTLLRN_ControlTLLRN_RigControl(OwnerTLLRN_ControlRig.Get(), OwnerControlElement.GetKey().Name);
		}
	}
}

void UTLLRN_ControlTLLRN_RigControlsProxy::ResetItems()
{
	ResetTLLRN_ControlRigItems();
	ResetSequencerItems();
}

void UTLLRN_ControlTLLRN_RigControlsProxy::AddItem(UTLLRN_ControlTLLRN_RigControlsProxy* ControlProxy)
{
	if (ControlProxy->OwnerTLLRN_ControlRig.IsValid() && ControlProxy->OwnerControlElement.IsValid())
	{
		AddTLLRN_ControlTLLRN_RigControl(ControlProxy->OwnerTLLRN_ControlRig.Get(), ControlProxy->OwnerControlElement.GetKey().Name);
	}
	else if(ControlProxy->OwnerObject.IsValid())
	{
		AddSequencerProxyItem(ControlProxy->OwnerObject.Get(), ControlProxy->OwnerBindingAndTrack.WeakTrack, ControlProxy->OwnerBindingAndTrack.Binding);
	}
}

void UTLLRN_ControlTLLRN_RigControlsProxy::AddSequencerProxyItem(UObject* InObject, TWeakObjectPtr<UMovieSceneTrack>& InTrack, TSharedPtr<FTrackInstancePropertyBindings>& InBinding)
{
	if (InObject == nullptr)
	{
		return;
	}

	FSequencerProxyItem& Item = SequencerItems.FindOrAdd(InObject);
	Item.OwnerObject = InObject;
	bool bContainsBinding = false;
	for (FBindingAndTrack& Binding : Item.Bindings)
	{
		if (Binding.WeakTrack == InTrack && Binding.Binding->GetPropertyName() == InBinding->GetPropertyName())
		{
			bContainsBinding = true;
		}
	}
	if (bContainsBinding == false)
	{
		FBindingAndTrack BindingAndTrack(InBinding, InTrack.Get());
		Item.Bindings.Add(BindingAndTrack);
	}
	if (SequencerItems.Num() > 1 || Item.Bindings.Num() > 1 || TLLRN_ControlRigItems.Num() > 0)
	{
		FString DisplayString = TEXT("Multiple");
		FName DisplayName(*DisplayString);
		Name = DisplayName;
	}
	else
	{
		if (AActor* Actor = Cast<AActor>(InObject))
		{
			FName DisplayName(*Actor->GetActorLabel());
			Name = DisplayName;
		}
		else if (UActorComponent* Component = Cast<UActorComponent>(InObject))
		{
			FName DisplayName(*Component->GetName());
			Name = DisplayName;
		}
		else
		{
			Name = InBinding->GetPropertyName();
		}
	}
}

TArray<FBindingAndTrack> UTLLRN_ControlTLLRN_RigControlsProxy::GetSequencerItems() const
{
	TArray<FBindingAndTrack> Elements;
	for (const TPair<TWeakObjectPtr<UObject>, FSequencerProxyItem>& Items : SequencerItems)
	{
		if (Items.Key.IsValid() == false)
		{
			continue;
		}
		for (const FBindingAndTrack& Element : Items.Value.Bindings)
		{
			Elements.Add(Element);
		}
	}
	return Elements;
}

void UTLLRN_ControlTLLRN_RigControlsProxy::ResetSequencerItems()
{
	ChildProxies.Reset();
	SequencerItems.Reset();
	if (OwnerObject.IsValid())
	{
		AddSequencerProxyItem(OwnerObject.Get(), OwnerBindingAndTrack.WeakTrack, OwnerBindingAndTrack.Binding);
	}
}

FTLLRN_CachedTLLRN_RigElement& UTLLRN_ControlTLLRN_RigControlsProxy::GetOwnerControlElement()
{
	static FTLLRN_CachedTLLRN_RigElement EmptyElement;
	if (OwnerTLLRN_ControlRig.IsValid())
	{
		if (OwnerControlElement.UpdateCache(OwnerTLLRN_ControlRig->GetHierarchy()))
		{
			return OwnerControlElement;
		}
	}
	return EmptyElement;
}

void UTLLRN_ControlTLLRN_RigControlsProxy::AddChildProxy(UTLLRN_ControlTLLRN_RigControlsProxy* ControlProxy)
{
	//check to see if the child proxy already has attribute that matches in which case we reuse it and make it a multiple
	if (ChildProxies.Contains(ControlProxy) == false)
	{
		for (UTLLRN_ControlTLLRN_RigControlsProxy* ChildProxy : ChildProxies)
		{
			if(ChildProxy->GetClass() == ControlProxy->GetClass())
			{
				FTLLRN_CachedTLLRN_RigElement& ChildTLLRN_RigElement = ChildProxy->GetOwnerControlElement();
				FTLLRN_CachedTLLRN_RigElement& TLLRN_ControlTLLRN_RigElement = ControlProxy->GetOwnerControlElement();
				if (ChildTLLRN_RigElement.IsValid() && TLLRN_ControlTLLRN_RigElement.IsValid() && ChildTLLRN_RigElement.GetElement()->GetDisplayName() == TLLRN_ControlTLLRN_RigElement.GetElement()->GetDisplayName())
				{
					ChildProxy->AddItem(ControlProxy);
					return;
				}
			}
		}
		ChildProxies.Add(ControlProxy);
	}
}

void UTLLRN_ControlTLLRN_RigControlsProxy::SelectionChanged(bool bInSelected)
{
	if (OwnerTLLRN_ControlRig.IsValid())
	{
		if (OwnerControlElement.UpdateCache(OwnerTLLRN_ControlRig->GetHierarchy()))
		{
			Modify();
			const FName PropertyName("bSelected");
			FTrackInstancePropertyBindings Binding(PropertyName, PropertyName.ToString());
			Binding.CallFunction<bool>(*this, bInSelected);
		}
	}
}

void UTLLRN_ControlTLLRN_RigControlsProxy::AddInteractions(ETLLRN_ControlRigContextChannelToKey ChannelsToKey, EPropertyChangeType::Type ChangeType)
{
	if (ChangeType == EPropertyChangeType::Interactive || ChangeType == EPropertyChangeType::ValueSet)
	{
		ETLLRN_ControlRigInteractionType InteractionType = ETLLRN_ControlRigInteractionType::None;
		if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationX)
			|| EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationY)
			|| EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::TranslationZ))
		{
			EnumAddFlags(InteractionType, ETLLRN_ControlRigInteractionType::Translate);
		}
		if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationX)
			|| EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationY)
			|| EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::RotationZ))
		{
			EnumAddFlags(InteractionType, ETLLRN_ControlRigInteractionType::Rotate);
		}
		if (EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleX)
			|| EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleY)
			|| EnumHasAnyFlags(ChannelsToKey, ETLLRN_ControlRigContextChannelToKey::ScaleZ))
		{
			EnumAddFlags(InteractionType, ETLLRN_ControlRigInteractionType::Scale);
		}
		for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
		{
			if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
			{
				for (const FName& CName : Items.Value.ControlElements)
				{
					if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
					{
						if (InteractionScopes.Contains(ControlElement) == false)
						{
							FTLLRN_ControlRigInteractionScope* InteractionScope = new FTLLRN_ControlRigInteractionScope(TLLRN_ControlRig, ControlElement->GetKey(), InteractionType);
							InteractionScopes.Add(ControlElement, InteractionScope);
						}
					}
				}
			}
		}
	}
}

void UTLLRN_ControlTLLRN_RigControlsProxy::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ToggleEditable)//hack so we can clear the reset cache for this property and not actually send this to our controls
	{
		return;
	}
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTLLRN_ControlTLLRN_RigControlsProxy, bSelected))
	{
		if (OwnerControlElement.IsValid() && OwnerTLLRN_ControlRig.IsValid())
		{
			FTLLRN_ControlRigInteractionScope InteractionScope(OwnerTLLRN_ControlRig.Get(), OwnerControlElement.GetKey());
			OwnerTLLRN_ControlRig.Get()->SelectControl(OwnerControlElement.GetKey().Name, bSelected);
			OwnerTLLRN_ControlRig.Get()->Evaluate_AnyThread();
		}
		return;
	}
#if WITH_EDITOR
	if (PropertyChangedEvent.Property)
	{
		
		//set values
		FProperty* Property = PropertyChangedEvent.Property;
		FProperty* MemberProperty = nullptr;
		if (PropertyChangedEvent.PropertyChain.GetActiveMemberNode())
		{
			MemberProperty = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue();
		}
		if (PropertyIsOnProxy(Property, MemberProperty))
		{
			ETLLRN_ControlRigContextChannelToKey ChannelToKeyContext = GetChannelToKeyFromPropertyName(Property->GetFName());
			AddInteractions(ChannelToKeyContext, PropertyChangedEvent.ChangeType);
			FTLLRN_RigControlModifiedContext Context;
			Context.SetKey = ETLLRN_ControlRigSetKey::DoNotCare;
			Context.KeyMask = (uint32)ChannelToKeyContext;
			FTLLRN_RigControlModifiedContext NotifyDrivenContext; //we key all for them
			Context.SetKey = ETLLRN_ControlRigSetKey::DoNotCare;

			UWorld* World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr;
			const FConstraintsManagerController& Controller = FConstraintsManagerController::Get(World);
			Controller.EvaluateAllConstraints();

			for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
			{
				if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
				{
					bool bDoReavaluate = false;
					//we do this backwards so ValueChanged later is set up correctly since that iterates in the other direction
					for (int32 Index = Items.Value.ControlElements.Num() - 1; Index >= 0; --Index)
					{
						if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(Items.Value.ControlElements[Index]))
						{
							SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(TLLRN_ControlRig, ControlElement, Context);
							FTLLRN_ControlRigEditMode::NotifyDrivenControls(TLLRN_ControlRig, ControlElement->GetKey(), NotifyDrivenContext);
							bDoReavaluate = true;
						}
					}
					if (bDoReavaluate)
					{
						TLLRN_ControlRig->Evaluate_AnyThread();
					}
				}
			}
			for (TPair <TWeakObjectPtr<UObject>, FSequencerProxyItem>& SItems : SequencerItems)
			{
				if (SItems.Key.IsValid() == false)
				{
					continue;
				}
				//we do this backwards so ValueChanged later is set up correctly since that iterates in the other direction
				for (int32 Index = SItems.Value.Bindings.Num() - 1; Index >= 0; --Index)
				{
					FBindingAndTrack& Binding = SItems.Value.Bindings[Index];
					SetBindingValueFromCurrent(SItems.Key.Get(), Binding.Binding, Context, PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive);
				}
			}
			if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
			{
				for (TPair<FTLLRN_RigControlElement*, FTLLRN_ControlRigInteractionScope*>& Scope : InteractionScopes)
				{
					if (Scope.Value)
					{
						delete Scope.Value;
					}
				}
				InteractionScopes.Reset();
			}
			ValueChanged();
		}
	}
#endif
}

#if WITH_EDITOR
void UTLLRN_ControlTLLRN_RigControlsProxy::PostEditUndo()
{
	for (const TPair<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem>& Items : TLLRN_ControlRigItems)
	{
		if (UTLLRN_ControlRig* TLLRN_ControlRig = Items.Value.TLLRN_ControlRig.Get())
		{
			for (const FName& CName : Items.Value.ControlElements)
			{
				if (FTLLRN_RigControlElement* ControlElement = Items.Value.GetControlElement(CName))
				{
					if (TLLRN_ControlRig->GetHierarchy()->Contains(FTLLRN_RigElementKey(ControlElement->GetKey().Name, ETLLRN_RigElementType::Control)))
					{
						TLLRN_ControlRig->SelectControl(ControlElement->GetKey().Name, bSelected);
					}
				}
			}
		}
	}
}
#endif

