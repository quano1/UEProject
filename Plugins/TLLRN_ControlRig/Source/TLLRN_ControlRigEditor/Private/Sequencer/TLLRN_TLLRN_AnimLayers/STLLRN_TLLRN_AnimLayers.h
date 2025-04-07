// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditMode/TLLRN_ControlRigBaseDockableView.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "ISequencer.h"
#include "Misc/Guid.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UTLLRN_ControlRig;
class FTLLRN_ControlRigEditMode;
struct FTLLRN_RigControlElement;
struct FTLLRN_RigElementKey;
struct FTLLRN_AnimLayerController;
class UTLLRN_TLLRN_AnimLayers;
enum class EMovieSceneDataChangeType;


class STLLRN_TLLRN_AnimLayers : public FTLLRN_ControlRigBaseDockableView, public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STLLRN_TLLRN_AnimLayers) {}
	SLATE_END_ARGS()
	STLLRN_TLLRN_AnimLayers();
	~STLLRN_TLLRN_AnimLayers();

	void Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode& InEditMode);
	//SCompuntWidget
	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	//FTLLRN_ControlRigBaseDockableView overrides
	virtual void SetEditMode(FTLLRN_ControlRigEditMode& InEditMode) override;
	virtual void HandleControlAdded(UTLLRN_ControlRig* TLLRN_ControlRig, bool bIsAdded) override;

private:
	void HandleOnTLLRN_ControlRigBound(UTLLRN_ControlRig* InTLLRN_ControlRig);
	void HandleOnObjectBoundToTLLRN_ControlRig(UObject* InObject);
	virtual void HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControl, bool bSelected) override;
	void OnObjectsReplaced(const TMap<UObject*, UObject*>& OldToNewInstanceMap);

	//set of control rigs we are bound too and need to clear delegates from
	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> BoundTLLRN_ControlRigs;
private:
	//actor selection changing
	void RegisterSelectionChanged();
	void OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh);
	FDelegateHandle OnSelectionChangedHandle;

	//sequencer changing so need to refresh list or update weight
	void OnActivateSequence(FMovieSceneSequenceIDRef ID);
	void OnGlobalTimeChanged();
	void OnMovieSceneDataChanged(EMovieSceneDataChangeType);
	FGuid LastMovieSceneSig = FGuid();

private:
	FReply OnAddClicked();
	FReply OnSelectionFilterClicked();
	bool IsSelectionFilterActive() const;
	TSharedPtr<FTLLRN_AnimLayerController> TLLRN_AnimLayerController;
	TWeakObjectPtr<UTLLRN_TLLRN_AnimLayers> TLLRN_TLLRN_AnimLayers;
private:
	//Keyboard interaction for Sequencer hotkeys
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
};

class SAnimWeightDetails : public SCompoundWidget
{

	SLATE_BEGIN_ARGS(SAnimWeightDetails)
	{}
	SLATE_END_ARGS()
	~SAnimWeightDetails();

	void Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode* InEditMode, UObject* InWeightObject);

private:

	TSharedPtr<IDetailsView> WeightView;

};


