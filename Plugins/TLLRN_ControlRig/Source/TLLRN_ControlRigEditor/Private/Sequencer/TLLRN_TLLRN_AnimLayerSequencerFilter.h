// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Filters/SequencerTrackFilterExtension.h"
#include "TLLRN_TLLRN_AnimLayerSequencerFilter.generated.h"

UCLASS()
class UTLLRN_TLLRN_AnimLayerSequencerFilter : public USequencerTrackFilterExtension
{
public:
	GENERATED_BODY()

	//~ Begin USequencerTrackFilterExtension
	virtual void AddTrackFilterExtensions(ISequencerTrackFilters& InOutFilterInterface
		, const TSharedRef<FFilterCategory>& InPreferredCategory
		, TArray<TSharedRef<FSequencerTrackFilter>>& InOutFilterList) const override;
	//~ End USequencerTrackFilterExtension
};
