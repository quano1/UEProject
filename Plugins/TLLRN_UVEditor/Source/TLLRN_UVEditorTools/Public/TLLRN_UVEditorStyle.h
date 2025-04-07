// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

/**
 * Slate style set for UV Editor
 */
class TLLRN_UVEDITORTOOLS_API FTLLRN_UVEditorStyle
	: public FSlateStyleSet
{
public:
	static FName StyleName;

	/** Access the singleton instance for this style set */
	static FTLLRN_UVEditorStyle& Get();

private:

	FTLLRN_UVEditorStyle();
	~FTLLRN_UVEditorStyle();
};