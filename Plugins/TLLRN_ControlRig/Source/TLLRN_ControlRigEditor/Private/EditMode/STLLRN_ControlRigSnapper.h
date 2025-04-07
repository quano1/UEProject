// Copyright Epic Games, Inc. All Rights Reserved.
/**
* Hold the View for the Snapper Widget
*/
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Tools/TLLRN_ControlRigSnapper.h"
#include "Misc/FrameNumber.h"
#include "IDetailsView.h"
#include "MovieSceneSequenceID.h"

class UTLLRN_ControlRig;
class ISequencer;
class AActor;


class STLLRN_ControlRigSnapper : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STLLRN_ControlRigSnapper) {}

	SLATE_END_ARGS()
	~STLLRN_ControlRigSnapper();

	void Construct(const FArguments& InArgs);


private:

	void GetTLLRN_ControlRigs(TArray<UTLLRN_ControlRig*>& OutTLLRN_ControlRigs) const;

	/*
	* Delegates and Helpers
	*/
	void ActorParentPicked(FActorForWorldTransforms Selection);
	void ActorParentSocketPicked(const FName SocketName, FActorForWorldTransforms Selection);
	void ActorParentComponentPicked(FName ComponentName, FActorForWorldTransforms Selection);
	void OnActivateSequenceChanged(FMovieSceneSequenceIDRef ID);

	FReply OnActorToSnapClicked();
	FReply OnParentToSnapToClicked();
	FText GetActorToSnapText();
	FText GetParentToSnapText();

	FReply OnStartFrameClicked();
	FReply OnEndFrameClicked();
	FText GetStartFrameToSnapText();
	FText GetEndFrameToSnapText();

	void ClearActors();
	void SetStartEndFrames();
	FReply OnSnapAnimationClicked();

	FTLLRN_ControlRigSnapperSelection GetSelection(bool bGetAll);

	//FTLLRN_ControlRigSnapper  TLLRN_ControlRigSnapper;
	TSharedPtr<IDetailsView> SnapperDetailsView;

	FTLLRN_ControlRigSnapperSelection ActorToSnap;
	FTLLRN_ControlRigSnapperSelection ParentToSnap;

	FFrameNumber StartFrame;
	FFrameNumber EndFrame;

	FTLLRN_ControlRigSnapper Snapper;

};

