// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "IDetailKeyframeHandler.h"
#include "RigVMModel/RigVMGraph.h"
#include "Editor/STLLRN_RigHierarchyTreeView.h"
#include "Editor/STLLRN_RigSpacePickerWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SConstraintsEditionWidget;
class FTLLRN_ControlRigEditMode;
class IDetailsView;
class ISequencer;
class SControlPicker;
class SExpandableArea;
class STLLRN_RigHierarchyTreeView;
class UTLLRN_ControlRig;
class UTLLRN_RigHierarchy;
class FToolBarBuilder;
class FEditorModeTools;
class FTLLRN_ControlRigEditModeToolkit;

#define USE_LOCAL_DETAILS 0

class STLLRN_ControlRigEditModeTools : public SCompoundWidget, public IDetailKeyframeHandler
{
public:
	SLATE_BEGIN_ARGS(STLLRN_ControlRigEditModeTools) {}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, TSharedPtr<FTLLRN_ControlRigEditModeToolkit> InOwningToolkit, FTLLRN_ControlRigEditMode& InEditMode);
	void Cleanup();

	/** Set the objects to be displayed in the details panel */
	void SetSettingsDetailsObject(const TWeakObjectPtr<>& InObject);
#if USE_LOCAL_DETAILS
	void SetEulerTransformDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects);
	void SetTransformDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects);
	void SetTransformNoScaleDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects);
	void SetFloatDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects);
	void SetBoolDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects);
	void SetIntegerDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects);
	void SetEnumDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects);
	void SetVectorDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects);
	void SetVector2DDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects);
#endif

	/** Set the sequencer we are bound to */
	void SetSequencer(TWeakPtr<ISequencer> InSequencer);

	/** Set The Control Rig we are using*/
	void SetTLLRN_ControlRigs(const TArrayView<TWeakObjectPtr<UTLLRN_ControlRig>>& InTLLRN_ControlRigs);

	/** Returns the hierarchy currently being used */
	const UTLLRN_RigHierarchy* GetHierarchy() const;
	// IDetailKeyframeHandler interface
	virtual bool IsPropertyKeyable(const UClass* InObjectClass, const class IPropertyHandle& PropertyHandle) const override;
	virtual bool IsPropertyKeyingEnabled() const override;
	virtual void OnKeyPropertyClicked(const IPropertyHandle& KeyedPropertyHandle) override;
	virtual bool IsPropertyAnimated(const class IPropertyHandle& PropertyHandle, UObject *ParentObject) const override;

	//reuse settings for space and constraint baking baking
	static FTLLRN_TLLRN_RigSpacePickerBakeSettings BakeSpaceSettings;

private:

	/** Sequencer we are currently bound to */

	TWeakPtr<ISequencer> WeakSequencer;
	TSharedPtr<IDetailsView> SettingsDetailsView;

#if USE_LOCAL_DETAILS
	/** The details views we do most of our work within */
	TSharedPtr<IDetailsView> ControlEulerTransformDetailsView;
	TSharedPtr<IDetailsView> ControlTransformDetailsView;
	TSharedPtr<IDetailsView> ControlTransformNoScaleDetailsView;
	TSharedPtr<IDetailsView> ControlFloatDetailsView;
	TSharedPtr<IDetailsView> ControlBoolDetailsView;
	TSharedPtr<IDetailsView> ControlIntegerDetailsView;
	TSharedPtr<IDetailsView> ControlEnumDetailsView;
	TSharedPtr<IDetailsView> ControlVector2DDetailsView;
	TSharedPtr<IDetailsView> ControlVectorDetailsView;
#endif
	/** Expander to interact with the options of the rig  */
	TSharedPtr<SExpandableArea> RigOptionExpander;
	TSharedPtr<IDetailsView> RigOptionsDetailsView;
#if USE_LOCAL_DETAILS
	/** Hierarchy picker for controls*/
	TSharedPtr<STLLRN_RigHierarchyTreeView> HierarchyTreeView;
#endif

	/** Space Picker controls*/
	TSharedPtr<SExpandableArea> PickerExpander;
	TSharedPtr<STLLRN_RigSpacePickerWidget> SpacePickerWidget;

	/** Storage for control rigs */
	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> TLLRN_ControlRigs;

	/** Constraint edition widget. */
	TSharedPtr<SExpandableArea> ConstraintPickerExpander = nullptr;
	TSharedPtr<SConstraintsEditionWidget> ConstraintsEditionWidget = nullptr; 

	/** Display or edit set up for property */
	bool ShouldShowPropertyOnDetailCustomization(const struct FPropertyAndParent& InPropertyAndParent) const;
	bool IsReadOnlyPropertyOnDetailCustomization(const struct FPropertyAndParent& InPropertyAndParent) const;

	/** Called when a manipulator is selected in the picker */
	void OnManipulatorsPicked(const TArray<FName>& Manipulators);

	void HandleModifiedEvent(ERigVMGraphNotifType InNotifType, URigVMGraph* InGraph, UObject* InSubject);
	void HandleSelectionChanged(TSharedPtr<FRigTreeElement> Selection, ESelectInfo::Type SelectInfo);
	void OnTLLRN_RigElementSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* ControlElement, bool bSelected);

	const FTLLRN_RigControlElementCustomization* HandleGetControlElementCustomization(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey);
	void HandleActiveSpaceChanged(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey, const FTLLRN_RigElementKey& InSpaceKey);
	void HandleSpaceListChanged(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InControlKey, const TArray<FTLLRN_RigElementKey>& InSpaceList);
	FReply HandleAddSpaceClicked();
	EVisibility GetAddSpaceButtonVisibility() const;
	bool IsSpaceSwitchingRestricted() const;
	FReply OnBakeControlsToNewSpaceButtonClicked();
	FReply OnCompensateKeyClicked();
	FReply OnCompensateAllClicked();
	void Compensate(TOptional<FFrameNumber> OptionalKeyTime, bool bSetPreviousTick);
	bool ReadyForBakeOrCompensation() const;
	FReply HandleAddConstraintClicked();

	EVisibility GetRigOptionExpanderVisibility() const;

	void OnRigOptionFinishedChange(const FPropertyChangedEvent& PropertyChangedEvent);
	
	/** constraint type to show selection*/
	void OnSelectShowConstraints(int32 Index);
	FText GetShowConstraintsName() const;
	FText GetShowConstraintsTooltip() const;

private:
	/** Toolbar functions and windows*/
	void ToggleEditPivotMode();

	//TODO may put back void MakeSelectionSetDialog();
	//TWeakPtr<SWindow> SelectionSetWindow;

	FEditorModeTools* ModeTools = nullptr;
	FRigTreeDisplaySettings DisplaySettings;
	const FRigTreeDisplaySettings& GetDisplaySettings() const { return DisplaySettings; }
	bool bIsChangingTLLRN_RigHierarchy = false;

	// The toolkit that created this UI
	TWeakPtr<FTLLRN_ControlRigEditModeToolkit> OwningToolkit;

	//array of handles to clear when getting new control rigs
	TArray<TPair<FDelegateHandle, TWeakObjectPtr<UTLLRN_ControlRig>>> HandlesToClear;

public:
	/** Modes Panel Header Information **/
	void CustomizeToolBarPalette(FToolBarBuilder& ToolBarBuilder);
	FText GetActiveToolName() const;
	FText GetActiveToolMessage() const;
};



