// Copyright Epic Games, Inc. All Rights Reserved.
/**
* View for containing details for various controls
*/
#pragma once

#include "CoreMinimal.h"
#include "EditMode/TLLRN_ControlRigBaseDockableView.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Misc/FrameNumber.h"
#include "IDetailsView.h"
#include "Rigs/TLLRN_RigHierarchy.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "TLLRN_TLLRN_AnimDetailsProxy.h"
#include "Engine/TimerHandle.h"

class ISequencer;
class UTLLRN_ControlRig;
class FUICommandList;
class UMovieSceneTrack;
class STLLRN_ControlRigDetails;

struct FArrayOfPropertyTracks
{
	TArray<UMovieSceneTrack*> PropertyTracks;
};

class FSequencerTracker
{
public:
	FSequencerTracker() = default;
	~FSequencerTracker();
	void SetSequencerAndDetails(TWeakPtr<ISequencer> InWeakSequencer, STLLRN_ControlRigDetails* InTLLRN_ControlRigDetails);
	TMap<UObject*, FArrayOfPropertyTracks>& GetObjectsTracked() { return ObjectsTracked; }
private:

	void RemoveDelegates();
	void UpdateSequencerBindings(TArray<FGuid> SequencerBindings);
	TWeakPtr<ISequencer> WeakSequencer;
	TMap<UObject*, FArrayOfPropertyTracks> ObjectsTracked;
	STLLRN_ControlRigDetails* TLLRN_ControlRigDetails = nullptr;
};

class FTLLRN_ControlRigEditModeGenericDetails : public IDetailCustomization
{
public:
	FTLLRN_ControlRigEditModeGenericDetails() = delete;
	FTLLRN_ControlRigEditModeGenericDetails(FEditorModeTools* InModeTools) : ModeTools(InModeTools) {}

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance(FEditorModeTools* InModeTools)
	{
		return MakeShareable(new FTLLRN_ControlRigEditModeGenericDetails(InModeTools));
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailLayout) override;

protected:
	FEditorModeTools* ModeTools = nullptr;
};

class STLLRN_ControlRigDetails: public SCompoundWidget, public FTLLRN_ControlRigBaseDockableView
{

	SLATE_BEGIN_ARGS(STLLRN_ControlRigDetails)
	{}
	SLATE_END_ARGS()
	~STLLRN_ControlRigDetails();

	void Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode& InEditMode);

	//FTLLRN_ControlRigBaseDockableView overrides
	virtual void SetEditMode(FTLLRN_ControlRigEditMode& InEditMode) override;

	/** Display or edit set up for property */
	bool ShouldShowPropertyOnDetailCustomization(const struct FPropertyAndParent& InPropertyAndParent) const;
	bool IsReadOnlyPropertyOnDetailCustomization(const struct FPropertyAndParent& InPropertyAndParent) const;
	void SelectedSequencerObjects(const TMap<UObject*, FArrayOfPropertyTracks>& ObjectsTracked);

private:

	void UpdateProxies();
	void HandleSequencerObjects(TMap<UObject*, FArrayOfPropertyTracks>& InObjectsTracked);
	virtual void HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControl, bool bSelected) override;

	TSharedPtr<IDetailsView> AllControlsView;

private:
	/*~ Keyboard interaction */
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	FSequencerTracker SequencerTracker;

	/** Handle for the timer used to recreate detail panel */
	FTimerHandle NextTickTimerHandle;

};




