// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/LazyObjectPtr.h"
#include "TransformNoScale.h"
#include "EulerTransform.h"
#include "Math/Rotator.h"
#include "MovieSceneTrack.h"
#include "IDetailKeyframeHandler.h"
#include "IPropertyTypeCustomization.h"
#include "Rigs/TLLRN_RigHierarchyDefines.h"
#include "MovieSceneCommonHelpers.h"
#include "Rigs/TLLRN_RigHierarchyCache.h"

#include "TLLRN_ControlTLLRN_RigControlsProxy.generated.h"

struct FTLLRN_RigControlElement;
class UTLLRN_ControlRig;
class IPropertyHandle;
class FTLLRN_ControlRigInteractionScope;
struct FTLLRN_RigControlModifiedContext;
class ISequencer;
class FCurveEditor;
class IDetailLayoutBuilder;
class FProperty;

//channel selection states for selection matching with curves
enum class ETLLRN_AnimDetailSelectionState : uint8
{
	None = 0x0, Partial = 0x1, All = 0x2
};

//direction to find range of property names
enum class ETLLRN_AnimDetailRangeDirection : uint8
{
	Up = 0x0, Down = 0x1
};
struct FTLLRN_AnimDetailVectorSelection
{
	ETLLRN_AnimDetailSelectionState  XSelected = ETLLRN_AnimDetailSelectionState::None;
	ETLLRN_AnimDetailSelectionState  YSelected = ETLLRN_AnimDetailSelectionState::None;
	ETLLRN_AnimDetailSelectionState  ZSelected = ETLLRN_AnimDetailSelectionState::None;
};

//item to specify control rig, todo move to handle class
struct FTLLRN_ControlRigProxyItem
{
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;
	TArray<FName> ControlElements;
	FTLLRN_RigControlElement* GetControlElement(const FName& InName) const;
};

struct FBindingAndTrack
{
	FBindingAndTrack() : Binding(nullptr), WeakTrack(nullptr) {};
	FBindingAndTrack(TSharedPtr<FTrackInstancePropertyBindings>& InBinding, UMovieSceneTrack* InWeakTrack) :
	Binding(InBinding), WeakTrack(InWeakTrack){}
	TSharedPtr<FTrackInstancePropertyBindings> Binding;
	TWeakObjectPtr<UMovieSceneTrack> WeakTrack;
};
//item to specify sequencer item, todo move to handle class
struct FSequencerProxyItem
{
	TWeakObjectPtr<UObject> OwnerObject;
	TArray<FBindingAndTrack> Bindings;
};

UCLASS(Abstract)
class UTLLRN_ControlTLLRN_RigControlsProxy : public UObject
{
	GENERATED_BODY()

public:
	UTLLRN_ControlTLLRN_RigControlsProxy() : bSelected(false) {}
	//will add controlrig or sequencer item
	virtual void AddItem(UTLLRN_ControlTLLRN_RigControlsProxy* ControlProxy);
	virtual void AddChildProxy(UTLLRN_ControlTLLRN_RigControlsProxy* ControlProxy);
	virtual FName GetName() const { return Name; }
	virtual void UpdatePropertyNames(IDetailLayoutBuilder& DetailBuilder) {};
	virtual void ValueChanged() {}
	virtual void SelectionChanged(bool bInSelected);
	virtual void SetKey(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& KeyedPropertyHandle) {};
	virtual EPropertyKeyedStatus GetPropertyKeyedStatus(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& PropertyHandle) const { return EPropertyKeyedStatus::NotKeyed; }
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromPropertyName(const FName& PropertyName) const { return ETLLRN_ControlRigContextChannelToKey::AllTransform; }
	virtual ETLLRN_ControlRigContextChannelToKey GetChannelToKeyFromChannelName(const FString& InChannelName) const { return ETLLRN_ControlRigContextChannelToKey::AllTransform; }
	virtual TMap<FName, int32> GetPropertyNames() const { TMap<FName, int32> Empty; return Empty; }
	virtual bool IsMultiple(const FName& InPropertyName) const { return false; }
	virtual void SetTLLRN_ControlTLLRN_RigElementValueFromCurrent(UTLLRN_ControlRig* TLLRN_ControlRig, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context) {};
	virtual void SetBindingValueFromCurrent(UObject* InObject, TSharedPtr<FTrackInstancePropertyBindings>& Binding, const FTLLRN_RigControlModifiedContext& Context, bool bInteractive = false) {};
	virtual void GetChannelSelectionState(TWeakPtr<FCurveEditor>& CurveEditor, FTLLRN_AnimDetailVectorSelection& OutLocationSelection, FTLLRN_AnimDetailVectorSelection& OutRotationSelection,
		FTLLRN_AnimDetailVectorSelection& OutScaleSelection) {};
	virtual bool PropertyIsOnProxy(FProperty* Property, FProperty* MemberProperty){return false;}

	// UObject interface
	virtual void PostEditChangeChainProperty( struct FPropertyChangedChainEvent& PropertyChangedEvent ) override;
#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif

	TArray<FTLLRN_RigControlElement*> GetControlElements() const;

	TArray<FBindingAndTrack> GetSequencerItems() const;
	//Default type
	ETLLRN_RigControlType Type = ETLLRN_RigControlType::Transform;

	//reset items it owns
	void ResetItems();

	//add correct item
	void AddSequencerProxyItem(UObject* InObject, TWeakObjectPtr<UMovieSceneTrack>& InTrack,  TSharedPtr<FTrackInstancePropertyBindings>& Binding);
	void AddTLLRN_ControlTLLRN_RigControl(UTLLRN_ControlRig* InTLLRN_ControlRig, const FName& InName);

private:

	//reset the  rig controls this also owns
	void ResetTLLRN_ControlRigItems();
	void ResetSequencerItems();

	FTLLRN_CachedTLLRN_RigElement& GetOwnerControlElement();

	void AddInteractions(ETLLRN_ControlRigContextChannelToKey ChannelsToKey, EPropertyChangeType::Type ChangeType);

public:

	//if individual it will show up independently, this will happen for certain nested controls
	bool bIsIndividual = false;

	UPROPERTY()
	bool bSelected;
	UPROPERTY(VisibleAnywhere, Category = "Control")
	FName Name;

	//refactor
	//We can set/get values form multiple control rig elements but only one owns this.
	TWeakObjectPtr<UTLLRN_ControlRig> OwnerTLLRN_ControlRig;
	FTLLRN_CachedTLLRN_RigElement OwnerControlElement;
	TMap<TWeakObjectPtr<UTLLRN_ControlRig>, FTLLRN_ControlRigProxyItem> TLLRN_ControlRigItems;
	
	TWeakObjectPtr<UObject> OwnerObject;
	FBindingAndTrack OwnerBindingAndTrack;
	TMap <TWeakObjectPtr<UObject>, FSequencerProxyItem> SequencerItems;

	//list of child/animation channel proxies that we will customize
	TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> ChildProxies;

#if WITH_EDITOR
	TMap<FTLLRN_RigControlElement*, FTLLRN_ControlRigInteractionScope*> InteractionScopes;
#endif
};

USTRUCT()
struct FTLLRN_NameToProxyMap
{
	GENERATED_BODY();
	UPROPERTY()
	TMap <FName, TObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy>> NameToProxy;;
};

struct FSequencerProxyPerType
{
	TMap<UObject*, TArray<FBindingAndTrack>> Bindings;
};

enum class ETLLRN_AnimDetailPropertySelectionType : uint8
{
	Toggle = 0x0, Select = 0x1, SelectRange = 0x2
};

/** Proxy in Details Panel */
UCLASS()
class UTLLRN_ControlRigDetailPanelControlProxies :public UObject
{
	GENERATED_BODY()

	UTLLRN_ControlRigDetailPanelControlProxies();
	~UTLLRN_ControlRigDetailPanelControlProxies();

protected:

	UPROPERTY()
	TMap<TObjectPtr<UTLLRN_ControlRig>, FTLLRN_NameToProxyMap> TLLRN_ControlRigOnlyProxies; //proxies themselves contain weakobjectptr to the controlrig

	UPROPERTY()
	TArray< TObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy>> SelectedTLLRN_ControlRigProxies;
	
	UPROPERTY()
	TMap<TObjectPtr<UObject>, FTLLRN_NameToProxyMap> SequencerOnlyProxies;

	UPROPERTY()
	TArray< TObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy>> SelectedSequencerProxies;
	
	TPair<TWeakObjectPtr<UTLLRN_ControlTLLRN_RigControlsProxy>, FName> LastSelection;

	TWeakPtr<ISequencer> Sequencer;

public:

	void SelectProxy(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_RigControlElement* TLLRN_RigElement, bool bSelected);
	UTLLRN_ControlTLLRN_RigControlsProxy* AddProxy(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_RigControlElement* TLLRN_RigElement);
	UTLLRN_ControlTLLRN_RigControlsProxy* AddProxy(UObject* InObject, ETLLRN_RigControlType Type, TWeakObjectPtr<UMovieSceneTrack>& Track, TSharedPtr<FTrackInstancePropertyBindings>& Binding);
	void RemoveTLLRN_ControlRigProxies(UTLLRN_ControlRig* InTLLRN_ControlRig);
	void RemoveSequencerProxies(UObject* InObject);
	void RemoveAllProxies();
	void ProxyChanged(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_RigControlElement* TLLRN_RigElement, bool bModify = true);
	void RecreateAllProxies(UTLLRN_ControlRig* InTLLRN_ControlRig);
	const TArray<UTLLRN_ControlTLLRN_RigControlsProxy*> GetAllSelectedProxies();
	bool IsSelected(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_RigControlElement* TLLRN_RigElement) const;
	void SetSequencer(TWeakPtr<ISequencer> InSequencer) { Sequencer = InSequencer; }
	ISequencer* GetSequencer() const { return Sequencer.Pin().Get(); }
	void ValuesChanged();
	void ResetSequencerProxies(TMap<ETLLRN_RigControlType, FSequencerProxyPerType>& ProxyPerType);

	//property selection to handle shift selection ranges
	void SelectProperty(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName, ETLLRN_AnimDetailPropertySelectionType SelectionType);

	bool IsPropertyEditingEnabled() const;
private:


	UTLLRN_ControlTLLRN_RigControlsProxy* FindProxy(UTLLRN_ControlRig* InTLLRN_ControlRig, FTLLRN_RigControlElement* TLLRN_RigElement) const;
	UTLLRN_ControlTLLRN_RigControlsProxy* FindProxy(UObject* InObject, FName PropertyName) const;

	UTLLRN_ControlTLLRN_RigControlsProxy* NewProxyFromType(ETLLRN_RigControlType Type, TObjectPtr<UEnum>& EnumPtr);

	//if SelectedProxy == nullptr then always clear it
	void ClearSelectedProperty(UTLLRN_ControlTLLRN_RigControlsProxy* SelectedProxy = nullptr);
	//select/deslect property based on selection type  return true if it is selected
	bool SelectPropertyInternal(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName, ETLLRN_AnimDetailPropertySelectionType SelectionType);
	//using the Last Section, get the range of properties from that (used for shift selections).
	TArray<TPair< UTLLRN_ControlTLLRN_RigControlsProxy*, FName>> GetPropertiesFromLastSelection(UTLLRN_ControlTLLRN_RigControlsProxy* Proxy, const FName& PropertyName) const;

	//delegate for changed objects
	void OnPostPropertyChanged(UObject* InObject, FPropertyChangedEvent& InPropertyChangedEvent);
};
