// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SViewportToolBar.h"

class FExtender;
class FUICommandList;
class STLLRN_UVEditor3DViewport;

/**
 * Toolbar that shows up at the top of the 3d viewport (has camera controls)
 */
class STLLRN_UVEditor3DViewportToolBar : public SViewportToolBar
{
public:
	SLATE_BEGIN_ARGS(STLLRN_UVEditor3DViewportToolBar) {}
		SLATE_ARGUMENT(TSharedPtr<FUICommandList>, CommandList)
		SLATE_ARGUMENT(TSharedPtr<FExtender>, Extenders)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class STLLRN_UVEditor3DViewport> InTLLRN_UVEditor3DViewport);

private:
	TSharedRef<SWidget> MakeDisplayToolBar(const TSharedPtr<FExtender> InExtenders);
	TSharedRef<SWidget> MakeToolBar(const TSharedPtr<FExtender> InExtenders);

	/** The viewport that we are in */
	TWeakPtr<class STLLRN_UVEditor3DViewport> TLLRN_UVEditor3DViewportPtr;

	TSharedPtr<FUICommandList> CommandList;
};
