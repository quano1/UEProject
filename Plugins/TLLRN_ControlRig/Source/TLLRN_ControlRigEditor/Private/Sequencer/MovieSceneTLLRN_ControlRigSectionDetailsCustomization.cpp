// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/MovieSceneTLLRN_ControlRigSectionDetailsCustomization.h"
#include "DetailLayoutBuilder.h"

TSharedRef<IDetailCustomization> FMovieSceneTLLRN_ControlRigSectionDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FMovieSceneTLLRN_ControlRigSectionDetailsCustomization);
}

void FMovieSceneTLLRN_ControlRigSectionDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Hide recording UI
	DetailLayout.HideCategory("Sequence Recording");

	// @TODO: restrict subsequences property to only accept control rig sequences
}