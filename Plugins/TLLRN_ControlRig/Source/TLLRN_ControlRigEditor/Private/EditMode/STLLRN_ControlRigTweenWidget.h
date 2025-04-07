// Copyright Epic Games, Inc. All Rights Reserved.
/**
* Hold the View for the Tween Widget
*/
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Tools/TLLRN_ControlTLLRN_RigPose.h"
#include "Tools/TLLRN_ControlRigTweener.h"
#include "Widgets/Input/SSpinBox.h"

class UTLLRN_ControlRig;
class ISequencer;
class FTLLRN_ControlRigEditModeToolkit;

class STLLRN_ControlRigTweenSlider : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STLLRN_ControlRigTweenSlider) {}
	SLATE_ARGUMENT(TSharedPtr<FBaseAnimSlider>, InAnimSlider)
	SLATE_ARGUMENT(TWeakPtr<FTLLRN_ControlRigEditMode>, InWeakEditMode)
	SLATE_END_ARGS()
	~STLLRN_ControlRigTweenSlider()
	{
	}

	void Construct(const FArguments& InArgs);
	void SetAnimSlider(TSharedPtr<FBaseAnimSlider>& InAnimSlider) { AnimSlider = InAnimSlider; }
	void DragAnimSliderTool(double Val);
	bool Setup();

	void ResetAnimSlider();

	// SWidget interface
	virtual void Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	
private:


	/*
	* Delegates and Helpers
	*/
	void OnPoseBlendChanged(double ChangedVal);
	void OnPoseBlendCommited(double ChangedVal, ETextCommit::Type Type);
	void OnBeginSliderMovement();
	void OnEndSliderMovement(double NewValue);
	double OnGetPoseBlendValue() const { return PoseBlendValue; }

	double PoseBlendValue;
	bool bIsBlending;
	bool bSliderStartedTransaction;
	
	TWeakPtr<FTLLRN_ControlRigEditMode> WeakEditMode;
	TWeakPtr<ISequencer> WeakSequencer;
	TSharedPtr<FBaseAnimSlider> AnimSlider;
	TSharedPtr<SSpinBox<double>> SpinBox;

	// Pending blend function called on tick to avoid blending values for each mouse move
	TFunction<void()> PendingBlendFunction;
};


class STLLRN_ControlRigTweenWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STLLRN_ControlRigTweenWidget) {}
	SLATE_ARGUMENT(TSharedPtr<FTLLRN_ControlRigEditModeToolkit>, InOwningToolkit)
	SLATE_ARGUMENT(TSharedPtr<FTLLRN_ControlRigEditMode>, InOwningEditMode)
	SLATE_END_ARGS()
		~STLLRN_ControlRigTweenWidget()
	{
	}

	void Construct(const FArguments& InArgs);

	void GetToNextActiveSlider();
	void DragAnimSliderTool(double Val);
	void ResetAnimSlider();
	void StartAnimSliderTool();
private:

	void OnSelectSliderTool(int32 Index);
	FText GetActiveSliderName() const;
	FText GetActiveSliderTooltip() const;

	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	};
	FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void FinishDraggingWidget(const FVector2D InLocation);

	TWeakPtr<ISequencer> WeakSequencer;
	TWeakPtr<FTLLRN_ControlRigEditModeToolkit> OwningToolkit;
	FAnimBlendToolManager  AnimBlendTools;

	TWeakPtr<FTLLRN_ControlRigEditMode> OwningEditMode;

	TSharedPtr<STLLRN_ControlRigTweenSlider> SliderWidget;
	static int32 ActiveSlider;

};
