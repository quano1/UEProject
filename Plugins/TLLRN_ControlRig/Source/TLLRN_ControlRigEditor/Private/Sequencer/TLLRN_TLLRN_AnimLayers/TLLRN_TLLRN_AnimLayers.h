// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/SparseDelegate.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/AssetUserData.h"
#include "Rigs/TLLRN_RigHierarchyDefines.h"
#include "TLLRN_ControlRig.h"
#include "Styling/SlateTypes.h"
#include "ISequencerPropertyKeyedStatus.h"
#include "TLLRN_TLLRN_AnimLayers.generated.h"

class ULevelSequence;
class UTLLRN_ControlRig;
class UMovieSceneTrack;
class UMovieSceneSection;
class ISequencer;
class UMovieSceneTLLRN_ControlRigParameterTrack;
class UMovieSceneTLLRN_ControlRigParameterSection;
struct FBakingAnimationKeySettings;
struct FMovieSceneFloatChannel;
class IPropertyHandle;

USTRUCT(BlueprintType)
// Name of a property and control and the specific channels that make up the layer
struct FTLLRN_AnimLayerPropertyAndChannels
{
	GENERATED_BODY()

	FTLLRN_AnimLayerPropertyAndChannels() : Name(NAME_None) , Channels((uint32)ETLLRN_ControlRigContextChannelToKey::AllTransform) {};
	
	//Name of the property or control
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FName Name; 

	//Mask of channels for that property/control, currently not used
	uint32 Channels;
};

//Bound object/control rig and the properties/channels it is made of
//A layer will consist of a bunch of these
USTRUCT(BlueprintType)
struct FTLLRN_AnimLayerSelectionSet
{
	GENERATED_BODY()
	FTLLRN_AnimLayerSelectionSet() :BoundObject(nullptr) {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	TWeakObjectPtr<UObject> BoundObject; //bound object is either a CR or a bound sequencer object

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	TMap<FName, FTLLRN_AnimLayerPropertyAndChannels> Names; //property/control name and channels

	bool MergeWithAnotherSelection(const FTLLRN_AnimLayerSelectionSet& Selection);
};

//The set with it's section
USTRUCT(BlueprintType)
struct FTLLRN_AnimLayerSectionItem
{
	GENERATED_BODY()
	FTLLRN_AnimLayerSectionItem() : Section(nullptr) {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FTLLRN_AnimLayerSelectionSet TLLRN_AnimLayerSet;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	TWeakObjectPtr<UMovieSceneSection> Section;
};

//individual layer items that make up the layer
USTRUCT(BlueprintType)
struct FTLLRN_AnimLayerItem
{
	GENERATED_BODY()
	FTLLRN_AnimLayerItem()  {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	TArray< FTLLRN_AnimLayerSectionItem>  SectionItems;

	//find section that matches based upon incoming section (same type and property)
	FTLLRN_AnimLayerSectionItem* FindMatchingSectionItem(UMovieSceneSection* InMovieSceneSection);
};

UENUM()
enum ETLLRN_AnimLayerType : uint32
{
	Base = 0x0,
	Additive = 0x1,
	Override = 0x2,
};
ENUM_CLASS_FLAGS(ETLLRN_AnimLayerType)

//state and properties of a layer
USTRUCT(BlueprintType)
struct FTLLRN_AnimLayerState
{

	GENERATED_BODY()

	FTLLRN_AnimLayerState();
	FText TLLRN_AnimLayerTypeToText()const;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	mutable ECheckBoxState bKeyed = ECheckBoxState::Unchecked;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	mutable ECheckBoxState bSelected = ECheckBoxState::Unchecked;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	mutable bool bActive;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	mutable bool bLock;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	FText Name;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	mutable double Weight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data", meta = (Bitmask, BitmaskEnum = "/Script/TLLRN_ControlRigEditor.ETLLRN_AnimLayerType"))
	mutable int32 Type;

};

USTRUCT(BlueprintType)
struct FTLLRN_AnimLayerTLLRN_ControlRigObject
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	TWeakObjectPtr<UTLLRN_ControlRig> TLLRN_ControlRig;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	TArray<FName>  ControlNames;
};

USTRUCT(BlueprintType)
struct FTLLRN_AnimLayerSceneObject
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	TWeakObjectPtr<UObject> SceneObjectOrComponent;
	
	//just doing transform for now
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	//TArray<FName>  PropertyNames;
};

USTRUCT(BlueprintType)
struct FTLLRN_AnimLayerObjects
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	TArray<FTLLRN_AnimLayerTLLRN_ControlRigObject> TLLRN_ControlRigObjects;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
	TArray<FTLLRN_AnimLayerSceneObject> SceneObjects;
};

UCLASS(EditInlineNew, CollapseCategories, HideDropdown)
class UTLLRN_AnimLayerWeightProxy : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Interp, DisplayName = "", Category = "NoCategory", meta = (SliderExponent = "1.0"))
	double Weight = 1.0;

	//UObect overrides for setting values when weight changes
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif
};

//for friend access to State variables directly
class UTLLRN_TLLRN_AnimLayers;
struct FTLLRN_AnimLayerSourceUIEntry;

UCLASS(BlueprintType, MinimalAPI)
class UTLLRN_AnimLayer : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	UTLLRN_AnimLayer() {};

	void GetTLLRN_AnimLayerObjects(FTLLRN_AnimLayerObjects& InLayerObjects) const;

	UFUNCTION(BlueprintCallable, Category = "State")
	ECheckBoxState GetKeyed() const;
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetKeyed();

	UFUNCTION(BlueprintCallable, Category = "State")
	ECheckBoxState GetSelected() const;
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetSelected(bool bInSelected, bool bClearSelection);

	UFUNCTION(BlueprintCallable, Category = "State")
	bool GetActive() const;
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetActive(bool bInActive);

	UFUNCTION(BlueprintCallable, Category = "State")
	bool GetLock() const;
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetLock(bool bInLock);

	UFUNCTION(BlueprintCallable, Category = "State")
	FText GetName() const;
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetName(const FText& InName);

	UFUNCTION(BlueprintCallable, Category = "State")
	double GetWeight() const;
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetWeight(double InWeight);

	UFUNCTION(BlueprintCallable, Category = "State")
	ETLLRN_AnimLayerType GetType() const;
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetType(ETLLRN_AnimLayerType LayerType);

	UFUNCTION(BlueprintCallable, Category = "State")
	bool AddSelectedInSequencer();
	UFUNCTION(BlueprintCallable, Category = "State")
	bool RemoveSelectedInSequencer();

	bool RemoveTLLRN_AnimLayerItem(UObject* InObject);
	void SetSelectedInList(bool bInValue) { bIsSelectedInList = bInValue; }
	bool GetSelectedInList() const { return bIsSelectedInList; }

	//for key property status for weights
	void SetKey(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& KeyedPropertyHandle);
	EPropertyKeyedStatus GetPropertyKeyedStatus(TSharedPtr<ISequencer>& Sequencer, const IPropertyHandle& PropertyHandle);
private:
	UPROPERTY(VisibleAnywhere, Category = "Data")
	TMap<TWeakObjectPtr<UObject>, FTLLRN_AnimLayerItem> TLLRN_AnimLayerItems;

	UPROPERTY(VisibleAnywhere, Category = "Data")
	FTLLRN_AnimLayerState State;

	UPROPERTY(Transient)
	TObjectPtr<UTLLRN_AnimLayerWeightProxy> WeightProxy;
	
	void SetSectionToKey() const;

	//static helper functions
	static bool IsAccepableNonTLLRN_ControlRigObject(UObject* InObject);

	bool bIsSelectedInList = false;

	friend class UTLLRN_TLLRN_AnimLayers;
	friend struct FTLLRN_AnimLayerSourceUIEntry;
};

/** Link To Set of Anim Sequences that we may belinked to.*/
UCLASS(BlueprintType, MinimalAPI)
class UTLLRN_TLLRN_AnimLayers : public UAssetUserData
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif
	virtual bool IsEditorOnly() const override { return true; }

	UPROPERTY(BlueprintReadWrite, Category = Layers)
	TArray<TObjectPtr<UTLLRN_AnimLayer>> TLLRN_TLLRN_AnimLayers;

	int32 GetTLLRN_AnimLayerIndex(UTLLRN_AnimLayer* InTLLRN_AnimLayer) const;
	bool DeleteTLLRN_AnimLayer(ISequencer* InSequencer, int32 Index);
	int32 DuplicateTLLRN_AnimLayer(ISequencer* InSequencer, int32 Index);
	int32 AddTLLRN_AnimLayerFromSelection(ISequencer* InSequencer);
	void GetTLLRN_AnimLayerStates(TArray<FTLLRN_AnimLayerState>& OutStates);
	bool MergeTLLRN_TLLRN_AnimLayers(ISequencer* InSequencer, const TArray<int32>& Indices, const FBakingAnimationKeySettings* InSettings);
	bool SetPassthroughKey(ISequencer* InSequencer, int32 Index);

	//will always blend to base fo rnow
	bool AdjustmentBlendLayers(ISequencer* InSequencer, int32 LayerIndex);

	bool IsTrackOnSelectedLayer(const UMovieSceneTrack* InTrack)const; 
	TArray<UMovieSceneSection*> GetSelectedLayerSections() const;

	//Get
	static UTLLRN_TLLRN_AnimLayers* GetTLLRN_TLLRN_AnimLayers(ISequencer* InSequencer);
	static UTLLRN_TLLRN_AnimLayers* GetTLLRN_TLLRN_AnimLayers(ULevelSequence* InLevelSequence);
	static 	TSharedPtr<ISequencer> GetSequencerFromAsset();
public:
	DECLARE_EVENT_OneParam(UTLLRN_TLLRN_AnimLayers, FTLLRN_AnimLayerListChanged, UTLLRN_TLLRN_AnimLayers*);

	FTLLRN_AnimLayerListChanged& TLLRN_AnimLayerListChanged() { return OnTLLRN_AnimLayerListChanged; };

private:
	void TLLRN_AnimLayerListChangedBroadcast();
	FTLLRN_AnimLayerListChanged OnTLLRN_AnimLayerListChanged;
	void AddBaseLayer();
public:
	void SetUpBaseLayerSections();

	static void SetUpSectionDefaults(ISequencer* SequencerPtr, UTLLRN_AnimLayer* Layer, UMovieSceneTrack* InTrack, UMovieSceneSection* InSection, FMovieSceneFloatChannel* FloatChannel);
	static void SetUpTLLRN_ControlRigSection(UMovieSceneTLLRN_ControlRigParameterSection* InSection, TArray<FName>& ControlNames);

};




