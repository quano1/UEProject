// Copyright Epic Games, Inc. All Rights Reserved.
/**
* Hold the Slate Views for the different Control Rig Asset Views
* This is shown in the Bottom of The STLLRN_ControlRigBaseListWidget
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
#include "Misc/NotifyHook.h"

class FAssetThumbnail;
class IDetailsView;
class UTLLRN_ControlRig;
class STLLRN_ControlRigBaseListWidget;

//Class to Hold Statics that are shared and externally callable
class FTLLRN_ControlRigView
{
public:
	static void CaptureThumbnail(UObject* Asset);
};

class STLLRN_ControlTLLRN_RigPoseView : public SCompoundWidget, public FNotifyHook
{
	SLATE_BEGIN_ARGS(STLLRN_ControlTLLRN_RigPoseView) {}
	SLATE_ARGUMENT(UTLLRN_ControlTLLRN_RigPoseAsset*, PoseAsset)
	SLATE_ARGUMENT(TSharedPtr<STLLRN_ControlRigBaseListWidget>, OwningWidget)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	~STLLRN_ControlTLLRN_RigPoseView();

	//FNotifyHook
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

private:
	void UpdateStatusBlocks();

	/*
	* Delegates and Helpers
	*/
	ECheckBoxState IsKeyPoseChecked() const;
	void OnKeyPoseChecked(ECheckBoxState NewState);
	ECheckBoxState IsMirrorPoseChecked() const;
	void OnMirrorPoseChecked(ECheckBoxState NewState);
	bool IsMirrorEnabled() const;
	void OnPoseBlendChanged(float ChangedVal);
	void OnPoseBlendCommited(float ChangedVal, ETextCommit::Type Type);
	void OnBeginSliderMovement();
	void OnEndSliderMovement(float NewValue);
	float OnGetPoseBlendValue()const {return PoseBlendValue;}
	FReply OnPastePose();
	FReply OnSelectControls();
	FReply OnCaptureThumbnail();
	void HandleControlSelected(UTLLRN_ControlRig* Subject, FTLLRN_RigControlElement* InControl, bool bSelected);
	void HandleControlAdded(UTLLRN_ControlRig* TLLRN_ControlRig, bool bIsAdded);
	TArray<UTLLRN_ControlRig*> GetTLLRN_ControlRigs();

	static bool bIsKey;
	static bool bIsMirror;
	float PoseBlendValue;
	bool bIsBlending;
	bool bSliderStartedTransaction;
	FTLLRN_ControlTLLRN_RigControlPose TempPose;
	TSharedPtr< FAssetThumbnail > Thumbnail;
	TSharedRef<SWidget> GetThumbnailWidget();

	TWeakObjectPtr<UTLLRN_ControlTLLRN_RigPoseAsset> PoseAsset;
	TArray<TWeakObjectPtr<UTLLRN_ControlRig>> CurrentTLLRN_ControlRigs;

	TSharedPtr<STextBlock> TextStatusBlock1;
	TSharedPtr<STextBlock> TextStatusBlock2;

	TWeakPtr<STLLRN_ControlRigBaseListWidget> OwningWidget;

	/* Mirroring*/
	TSharedPtr<IDetailsView> MirrorDetailsView;

	TSharedRef<ITableRow> OnGenerateWidgetForList(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
	{

		return SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
			.Content()
			[
				SNew(SBox)
				.Padding(2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString((*InItem)))
			]
			];
	}

public:
	static bool IsMirror() { return bIsMirror; }
};

