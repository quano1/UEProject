// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SWidget.h"
#include "ISequencer.h"
#include "MovieSceneTrack.h"
#include "ISequencerSection.h"
#include "ISequencerTrackEditor.h"
#include "Sequencer/MovieSceneTLLRN_ControlRigParameterTrack.h"
#include "KeyframeTrackEditor.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "AcquiredResources.h"
#include "ContentBrowserDelegates.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneToolsModule.h"
#include "Engine/EngineTypes.h"
#include "EditorUndoClient.h"
#include "Filters/CurveEditorSmartReduceFilter.h"
#include "ScopedTransaction.h"

class UTickableTransformConstraint;
struct FAssetData;
class FMenuBuilder;
class USkeleton;
class UMovieSceneTLLRN_ControlRigParameterSection;
class UFKTLLRN_ControlRig;
class FEditorModeTools;
class FTLLRN_ControlRigEditMode;
struct FTLLRN_RigControlModifiedContext;
struct FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel;
struct FMovieSceneChannel;
struct FKeyAddOrDeleteEventItem;
struct FKeyMoveEventItem;
struct FBakingAnimationKeySettings;
class ISequencer;
class UMovieSceneTrack;
class IStructureDetailsView;

//////////////////////////////////////////////////////////////
/// SCollapseControlsWidget
///////////////////////////////////////////////////////////
DECLARE_DELEGATE_TwoParams(FCollapseControlsCB, TSharedPtr<ISequencer>& InSequencer, const FBakingAnimationKeySettings& InSettings);

/** Widget allowing collapsing of controls */
class SCollapseControlsWidget : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SCollapseControlsWidget)
		: _Sequencer(nullptr)
		{}
		SLATE_ARGUMENT(TWeakPtr<ISequencer>, Sequencer)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SCollapseControlsWidget() override {}

	FReply OpenDialog(bool bModal = true);
	void CloseDialog();

	void SetCollapseCB(FCollapseControlsCB& InCB) { CollapseCB = InCB; }
private:
	void Collapse();

	TWeakPtr<ISequencer> Sequencer;
	//static to be reused
	static TOptional<FBakingAnimationKeySettings> CollapseControlsSettings;
	//structonscope for details panel
	TSharedPtr < TStructOnScope<FBakingAnimationKeySettings>> Settings;
	TWeakPtr<SWindow> DialogWindow;
	TSharedPtr<IStructureDetailsView> DetailsView;
	FCollapseControlsCB CollapseCB;
};

/**
 * Tools for animation tracks
 */
class FTLLRN_ControlRigParameterTrackEditor : public FKeyframeTrackEditor<UMovieSceneTLLRN_ControlRigParameterTrack>, public IMovieSceneToolsAnimationBakeHelper,
	public FEditorUndoClient
{
public:
	/**
	 * Constructor
	 *
	 * @param InSequencer The sequencer instance to be used by this tool
	 */
	FTLLRN_ControlRigParameterTrackEditor(TSharedRef<ISequencer> InSequencer);

	/** Virtual destructor. */
	virtual ~FTLLRN_ControlRigParameterTrackEditor();

	/**
	 * Creates an instance of this class.  Called by a sequencer
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

public:

	// ISequencerTrackEditor interface
	virtual void OnInitialize() override;
	virtual void OnRelease() override;
	virtual void BuildObjectBindingContextMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass) override;
	virtual void BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
	virtual bool SupportsSequence(UMovieSceneSequence* InSequence) const override;
	virtual bool HasTransformKeyBindings() const override { return true; }
	virtual bool CanAddTransformKeysForSelectedObjects() const override;
	virtual void OnAddTransformKeysForSelectedObjects(EMovieSceneTransformChannel Channel);
	virtual bool HasTransformKeyOverridePriority() const override;
	virtual void ObjectImplicitlyAdded(UObject* InObject)  override;
	virtual void ObjectImplicitlyRemoved(UObject* InObject)  override;
	virtual void BuildTrackContextMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* InTrack) override;
	virtual bool HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid) override;

	//IMovieSceneToolsAnimationBakeHelper
	virtual void PostEvaluation(UMovieScene* MovieScene, FFrameNumber Frame);

	//FEditorUndoClient Interface
	virtual bool MatchesContext(const FTransactionContext& InContext, const TArray<TPair<UObject*, FTransactionObjectEvent>>& TransactionObjects) const override;
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

private:

	void HandleAddTrackSubMenu(FMenuBuilder& MenuBuilder, TArray<FGuid> ObjectBindings, UMovieSceneTrack* Track);

	void ToggleIsAdditiveTLLRN_ControlRig();
	bool IsToggleIsAdditiveTLLRN_ControlRig();
	
	void ToggleFilterAssetBySkeleton();
	bool IsToggleFilterAssetBySkeleton();

	void ToggleFilterAssetByAnimatableControls();
	bool IsToggleFilterAssetByAnimatableControls();
	void SelectSequencerNodeInSection(UMovieSceneTLLRN_ControlRigParameterSection* ParamSection, const FName& ControlName, bool bSelected);

	/** Control Rig Picked */
	void AddTLLRN_ControlRig(UClass* InClass, UObject* BoundActor, FGuid ObjectBinding);
	void AddTLLRN_ControlRig(const UClass* InClass, UObject* BoundActor, FGuid ObjectBinding, UTLLRN_ControlRig* InExistingTLLRN_ControlRig);
	void AddTLLRN_ControlRig(const FAssetData& InAsset, UObject* BoundActor, FGuid ObjectBinding);
	void AddTLLRN_ControlRig(const TArray<FAssetData>& InAssets, UObject* BoundActor, FGuid ObjectBinding);
	void AddFKTLLRN_ControlRig(UObject* BoundActor, FGuid ObjectBinding);
	void AddTLLRN_ControlRigFromComponent(FGuid InGuid);

	bool IsTLLRN_ControlRigAllowed(const FAssetData& AssetData, TArray<UClass*> ExistingRigs, USkeleton* Skeleton);
	
	/** Delegate for Selection Changed Event */
	void OnSelectionChanged(TArray<UMovieSceneTrack*> InTracks);

	/** Returns the Edit Mode tools manager hosting the edit mode */
	FEditorModeTools* GetEditorModeTools() const;
	FTLLRN_ControlRigEditMode* GetEditMode(bool bForceActivate = false) const;

	/** Delegate for MovieScene Changing so we can see if our track got deleted*/
	void OnSequencerDataChanged(EMovieSceneDataChangeType DataChangeType);

	/** Delegate for when track immediately changes, so we need to do an interaction edit for things like fk/ik*/
	void OnChannelChanged(const FMovieSceneChannelMetaData* MetaData, UMovieSceneSection* InSection);

	/** Delegate for difference focused movie scene sequence*/
	void OnActivateSequenceChanged(FMovieSceneSequenceIDRef ID);

	/** Actor Added Delegate*/
	void HandleActorAdded(AActor* Actor, FGuid TargetObjectGuid);
	/** Add Control Rig Tracks For Skelmesh Components*/
	void AddTrackForComponent(USceneComponent* Component, FGuid Binding);

	/** Control Rig Delegates*/
	void HandleControlModified(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, const FTLLRN_RigControlModifiedContext& Context);
	void HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, bool bSelected);
	void HandleControlUndoBracket(UTLLRN_ControlRig* Subject, bool bOpenUndoBracket);
	void HandleOnPostConstructed(UTLLRN_ControlRig* Subject, const FName& InEventName);
	void HandleOnTLLRN_ControlRigBound(UTLLRN_ControlRig* InTLLRN_ControlRig);
	void HandleOnObjectBoundToTLLRN_ControlRig(UObject* InObject);

	//if rig not set then we clear delegates for everyone
	void ClearOutAllSpaceAndConstraintDelegates(const UTLLRN_ControlRig* InOptionalTLLRN_ControlRig = nullptr) const;

	/** SpaceChannel Delegates*/
	void HandleOnSpaceAdded(
		UMovieSceneTLLRN_ControlRigParameterSection* Section,
		const FName& ControlName,
		FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel);
	void HandleSpaceKeyDeleted(
		UMovieSceneTLLRN_ControlRigParameterSection* Section,
		FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel,
		const TArray<FKeyAddOrDeleteEventItem>& DeletedItems) const;
	static void HandleSpaceKeyMoved(
		UMovieSceneTLLRN_ControlRigParameterSection* Section,
		FMovieSceneTLLRN_ControlTLLRN_RigSpaceChannel* Channel,
		const TArray<FKeyMoveEventItem>& MovedItems);

	/** ConstraintChannel Delegates*/
	void HandleOnConstraintAdded(
		IMovieSceneConstrainedSection* InSection,
		FMovieSceneConstraintChannel* InConstraintChannel);
	void HandleConstraintKeyDeleted(
		IMovieSceneConstrainedSection* InSection,
		const FMovieSceneConstraintChannel* InConstraintChannel,
		const TArray<FKeyAddOrDeleteEventItem>& InDeletedItems) const;
	static void HandleConstraintKeyMoved(
		IMovieSceneConstrainedSection* InSection,
		const FMovieSceneConstraintChannel* InConstraintChannel,
		const TArray<FKeyMoveEventItem>& InMovedItems);
	void HandleConstraintRemoved(IMovieSceneConstrainedSection* InSection);
	void HandleConstraintPropertyChanged(UTickableTransformConstraint* InConstraint, const FPropertyChangedEvent& InPropertyChangedEvent) const;

	/** Select control rig if not selected, select controls from key areas */
	void SelectRigsAndControls(UTLLRN_ControlRig* Subject, const TArray<const IKeyArea*>& KeyAreas);

	/** Handle Creation for Skeleton, SkelMeshComp or Actor Owner, they may or may not have a binding, use optional control rig to pick one */
	FMovieSceneTrackEditor::FFindOrCreateHandleResult FindOrCreateHandleToObject(UObject* InObj, UTLLRN_ControlRig* InTLLRN_ControlRig = nullptr);

	/** Handle Creation for control rig track given the object binding and the control rig */	
	FMovieSceneTrackEditor::FFindOrCreateTrackResult FindOrCreateTLLRN_ControlRigTrackForObject(FGuid ObjectBinding, UTLLRN_ControlRig* TLLRN_ControlRig, FName PropertyName = NAME_None, bool bCreateTrackIfMissing = true);

	/** Import FBX*/
	void ImportFBX(UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, UMovieSceneTLLRN_ControlRigParameterSection* InSection, 
		TArray<FRigControlFBXNodeAndChannels>* NodeAndChannels);

	/** Export FBX*/
	void ExportFBX(UMovieSceneTLLRN_ControlRigParameterTrack* InTrack, UMovieSceneTLLRN_ControlRigParameterSection* InSection);
	
	/** Find Track for given TLLRN_ControlRig*/
	UMovieSceneTLLRN_ControlRigParameterTrack* FindTrack(const UTLLRN_ControlRig* InTLLRN_ControlRig) const;

	/** Select Bones to Animate on FK Rig*/
	void SelectFKBonesToAnimate(UFKTLLRN_ControlRig* FKTLLRN_ControlRig, UMovieSceneTLLRN_ControlRigParameterTrack* Track);

	/** Bake To Control Rig Sub Menu*/
	void BakeToTLLRN_ControlRigSubMenu(FMenuBuilder& MenuBuilder, FGuid ObjectBinding, UObject* BoundObject, USkeletalMeshComponent* SkelMeshComp,USkeleton* Skeleton);
	
	/** Bake To Control Rig Sub Menu*/
	void BakeToTLLRN_ControlRig(UClass* InClass, FGuid ObjectBinding,UObject* BoundObject, USkeletalMeshComponent* SkelMeshComp, USkeleton* Skeleton);

	/** Bake Inersion of Additive Control Rig to Rest Pose*/
	void BakeInvertedPose(UTLLRN_ControlRig* InTLLRN_ControlRig, UMovieSceneTLLRN_ControlRigParameterTrack* Track);

	/** Return true if the control rig is in layered mode */
	bool IsLayered(UMovieSceneTLLRN_ControlRigParameterTrack* Track) const;

	/** Convert an absolute control rit to a layered control rig */
	void ConvertIsLayered(UMovieSceneTLLRN_ControlRigParameterTrack* Track);

	/** Set Up EditMode for Specified Control Rig*/
	void SetUpEditModeIfNeeded(UTLLRN_ControlRig* TLLRN_ControlRig);

	/** Helper functions to iterate over UMovieSceneTLLRN_ControlRigParameterTracks in the currently focussed MovieScene*/
	void IterateTracks(TFunctionRef<bool(UMovieSceneTLLRN_ControlRigParameterTrack*)> Callback) const;
private:
	/** Command Bindings added by the Transform Track Editor to Sequencer and curve editor. */
	TSharedPtr<FUICommandList> CommandBindings;
	FAcquiredResources AcquiredResources;

public:

	void AddControlKeys(UObject* InObject, UTLLRN_ControlRig* InTLLRN_ControlRig, FName PropertyName,
		FName ParameterName, ETLLRN_ControlRigContextChannelToKey ChannelsToKey, ESequencerKeyMode KeyMode,
		float InLocalTime, const bool bInConstraintSpace = false);
	void GetTLLRN_ControlRigKeys(UTLLRN_ControlRig* InTLLRN_ControlRig, FName ParameterName, ETLLRN_ControlRigContextChannelToKey ChannelsToKey,
		ESequencerKeyMode KeyMode, UMovieSceneTLLRN_ControlRigParameterSection* SectionToKey,	FGeneratedTrackKeys& OutGeneratedKeys,
		const bool bInConstraintSpace = false);
	FKeyPropertyResult AddKeysToTLLRN_ControlRig(
		UObject* InObject, UTLLRN_ControlRig* InTLLRN_ControlRig, FFrameNumber KeyTime, FFrameNumber EvaluateTime, FGeneratedTrackKeys& GeneratedKeys,
		ESequencerKeyMode KeyMode, TSubclassOf<UMovieSceneTrack> TrackClass, FName TLLRN_ControlTLLRN_RigName, FName TLLRN_RigControlName);
	FKeyPropertyResult AddKeysToTLLRN_ControlRigHandle(UObject* InObject, UTLLRN_ControlRig* InTLLRN_ControlRig,
		FGuid ObjectHandle, FFrameNumber KeyTime, FFrameNumber EvaluateTime, FGeneratedTrackKeys& GeneratedKeys,
		ESequencerKeyMode KeyMode, TSubclassOf<UMovieSceneTrack> TrackClass, FName TLLRN_ControlTLLRN_RigName, FName TLLRN_RigControlName);
	/**
	 * Modify the passed in Generated Keys by the current tracks values and weight at the passed in time.

	 * @param Object The handle to the object modify
	 * @param Track The track we are modifying
	 * @param SectionToKey The Sections Channels we will be modifiying
	 * @param Time The Time at which to evaluate
	 * @param InOutGeneratedTrackKeys The Keys we need to modify. We change these values.
	 * @param Weight The weight we need to modify the values by.
	 */
	bool ModifyOurGeneratedKeysByCurrentAndWeight(UObject* Object, UTLLRN_ControlRig* InTLLRN_ControlRig, FName TLLRN_RigControlName, UMovieSceneTrack *Track, UMovieSceneSection* SectionToKey, FFrameNumber EvaluateTime, FGeneratedTrackKeys& InOutGeneratedTotalKeys, float Weight) const;

	//**Function to collapse all layers from this section onto the first absoluate layer.*/
	static bool CollapseAllLayers(TSharedPtr<ISequencer>& SequencerPtr, UMovieSceneTrack* OwnerTrack, const FBakingAnimationKeySettings& InSettings);

	//** Function to load animation into a section, returns true if successful
	static bool LoadAnimationIntoSection(TSharedPtr<ISequencer>& SequencerPtr,  UAnimSequence* AnimSequence, USkeletalMeshComponent* SkelMeshComp,
		FFrameNumber StartFrame, bool bReduceKeys, const FSmartReduceParams& ReduceParams, bool bResetControls, UMovieSceneTLLRN_ControlRigParameterSection* ParamSection);

	//** Function to smart reduce all keys on all constraints based on the parameters
	static void SmartReduce(TSharedPtr<ISequencer>& SequencerPtr, const FSmartReduceParams& InParams, UMovieSceneTLLRN_ControlRigParameterSection* InSection);

	/** Adds the constraint to sequencer and create the bindings if needed. Note that TLLRN_ControlRig Track auto-generation is disabled in this function. */
	static void AddConstraintToSequencer(const TSharedPtr<ISequencer>& InSequencer, UTickableTransformConstraint* InConstraint);

private:
	FDelegateHandle SelectionChangedHandle;
	FDelegateHandle SequencerChangedHandle;
	FDelegateHandle OnActivateSequenceChangedHandle;
	FDelegateHandle CurveChangedHandle;
	FDelegateHandle OnChannelChangedHandle;
	FDelegateHandle OnMovieSceneChannelChangedHandle;
	FDelegateHandle OnActorAddedToSequencerHandle;

	//map used for space channel delegates since we loose them

	//used to sync curve editor selections/displays on next tick for performance reasons
	TSet<FName> DisplayedControls;
	TSet<FName> UnDisplayedControls;
	bool bCurveDisplayTickIsPending;
	void BindTLLRN_ControlRig(UTLLRN_ControlRig* TLLRN_ControlRig);
	void UnbindTLLRN_ControlRig(UTLLRN_ControlRig* TLLRN_ControlRig);
	void UnbindAllTLLRN_ControlRigs();
	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> BoundTLLRN_ControlRigs;

private:

	/** Guard to stop infinite loops when handling control selections*/
	bool bIsDoingSelection;

	/** A flag to determine if the next update coming from the timer should be skipped */
	bool bSkipNextSelectionFromTimer;

	/** Whether or not this rig will be used as an layered control rig */
	bool bIsLayeredTLLRN_ControlRig;

	/** Whether or not we should check Skeleton when filtering*/
	bool bFilterAssetBySkeleton;

	/** Whether or not we should check for Animatable Controls when filtering*/
	bool bFilterAssetByAnimatableControls;

	FRefreshAssetViewDelegate RefreshTLLRN_ControlRigPickerDelegate;

	/** Handle to help updating selection on tick tick to avoid too many flooded selections*/
	FTimerHandle UpdateSelectionTimerHandle;

	/** Array of sections that are getting undone, we need to recreate any space channel add,move key delegates to them*/
	mutable TArray<UMovieSceneTLLRN_ControlRigParameterSection*> SectionsGettingUndone;

	/** An index counter for the opened undo brackets */
	int32 ControlUndoBracket;

	/** A counter for occurred control changes during a control undo bracket */
	int32 ControlChangedDuringUndoBracket;

	/** Lock to avoid registering multiple transactions from different tracks at the same time */
	static FCriticalSection ControlUndoTransactionMutex;

	/** A transaction used to group multiple key events */
	TSharedPtr<FScopedTransaction> ControlUndoTransaction;

	/** Controls if the control rig track for the default animating rig should be created */
	static bool bAutoGenerateTLLRN_ControlRigTrack;

	/** Set of delegate handles we have added delegate's too, need to clear them*/
	TSet<FDelegateHandle> ConstraintHandlesToClear;

	friend class FTLLRN_ControlRigBlueprintActions;

	/** Whether or not animation/control rig edit mode was open when we closed*/
	static bool bTLLRN_ControlRigEditModeWasOpen;
	/** Previous selection if open*/
	static TArray <TPair<UClass*, TArray<FName>>> PreviousSelectedTLLRN_ControlRigs;
};



/** Class for control rig sections */
class FTLLRN_ControlRigParameterSection : public FSequencerSection
{
public:

	/**
	* Creates a new control rig property section.
	*
	* @param InSection The section object which is being displayed and edited.
	* @param InSequencer The sequencer which is controlling this parameter section.
	*/
	FTLLRN_ControlRigParameterSection(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer)
		: FSequencerSection(InSection), WeakSequencer(InSequencer)
	{
	}

public:

	virtual void BuildSectionContextMenu(FMenuBuilder& MenuBuilder, const FGuid& InObjectBinding) override;

	//~ ISequencerSection interface
	virtual bool RequestDeleteCategory(const TArray<FName>& CategoryNamePath) override;
	virtual bool RequestDeleteKeyArea(const TArray<FName>& KeyAreaNamePath) override;
protected:
	/** Add Sub Menu */
	void AddAnimationSubMenuForFK(FMenuBuilder& MenuBuilder, FGuid ObjectBinding, USkeleton* Skeleton, UMovieSceneTLLRN_ControlRigParameterSection* Section);

	/** Animation sub menu filter function */
	bool ShouldFilterAssetForFK(const FAssetData& AssetData);

	/** Animation asset selected */
	void OnAnimationAssetSelectedForFK(const FAssetData& AssetData,FGuid ObjectBinding, UMovieSceneTLLRN_ControlRigParameterSection* Section);

	/** Animation asset enter pressed */
	void OnAnimationAssetEnterPressedForFK(const TArray<FAssetData>& AssetData, FGuid ObjectBinding, UMovieSceneTLLRN_ControlRigParameterSection* Section);

	/** Select controls channels from selected pressed*/
	void ShowSelectedControlsChannels();
	/** Show all control channels*/
	void ShowAllControlsChannels();

	void KeyZeroValue();
	void KeyWeightValue(float Val);
	void CollapseAllLayers();

	/** The sequencer which is controlling this section. */
	TWeakPtr<ISequencer> WeakSequencer;

};