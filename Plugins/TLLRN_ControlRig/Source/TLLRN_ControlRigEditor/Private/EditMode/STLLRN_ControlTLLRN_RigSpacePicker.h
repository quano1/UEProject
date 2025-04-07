// Copyright Epic Games, Inc. All Rights Reserved.

/**
* Space Picker View
*/
#pragma once

#include "CoreMinimal.h"
#include "EditMode/TLLRN_ControlRigBaseDockableView.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Misc/FrameNumber.h"
#include "Editor/STLLRN_RigSpacePickerWidget.h"


class ISequencer;
class SExpandableArea;
class SSearchableTLLRN_RigHierarchyTreeView;
class UTLLRN_ControlRig;

class STLLRN_ControlTLLRN_RigSpacePicker : public SCompoundWidget, public FTLLRN_ControlRigBaseDockableView
{

	SLATE_BEGIN_ARGS(STLLRN_ControlTLLRN_RigSpacePicker)
	{}
	SLATE_END_ARGS()
		~STLLRN_ControlTLLRN_RigSpacePicker();

	void Construct(const FArguments& InArgs, FTLLRN_ControlRigEditMode& InEditMode);

private:
	virtual void HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControl, bool bSelected) override;

	/** Space picker widget*/
	TSharedPtr<STLLRN_RigSpacePickerWidget> SpacePickerWidget;
	TSharedPtr<SExpandableArea> PickerExpander;

	const FTLLRN_RigControlElementCustomization* HandleGetControlElementCustomization(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey);
	void HandleActiveSpaceChanged(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey, const FTLLRN_RigElementKey& InSpaceKey);
	void HandleSpaceListChanged(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey, const TArray<FTLLRN_RigElementKey>& InSpaceList);
	FReply HandleAddSpaceClicked();
	FReply OnBakeControlsToNewSpaceButtonClicked();
	FReply OnCompensateKeyClicked();
	FReply OnCompensateAllClicked();
	void Compensate(TOptional<FFrameNumber> OptionalKeyTime, bool bSetPreviousTick);
	EVisibility GetAddSpaceButtonVisibility() const;

	bool ReadyForBakeOrCompensation() const;
	//for now picker works off of one TLLRN_ControlRig, this function gets the first control rig with a selection
	UTLLRN_ControlRig* GetTLLRN_ControlRig() const;

};

