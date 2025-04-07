// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"

class IPropertyHandle;
class FPropertyRestriction;
class ITLLRN_UVEditorTransformToolQuickAction;

// Customization for UTLLRN_UVEditorUVTransformProperties
class FTLLRN_UVEditorUVTransformToolDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

protected:
	void BuildQuickTranslateMenu(IDetailLayoutBuilder& DetailBuilder);
	void BuildQuickRotateMenu(IDetailLayoutBuilder& DetailBuilder);
};

// Customization for UTLLRN_UVEditorUVQuickTransformProperties
class FTLLRN_UVEditorUVQuickTransformToolDetails : public FTLLRN_UVEditorUVTransformToolDetails
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

// Customization for UTLLRN_UVEditorUVDistributeProperties
class FTLLRN_UVEditorUVDistributeToolDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

protected:

	TSharedPtr<IPropertyHandle> DistributeModeHandle;
	TSharedPtr<IPropertyHandle> EnableManualDistancesHandle;

	TSharedPtr<IPropertyHandle> OrigManualExtentHandle;
	TSharedPtr<IPropertyHandle> OrigManualSpacingHandle;

	void BuildDistributeModeButtons(IDetailLayoutBuilder& DetailBuilder);

};

// Customization for UTLLRN_UVEditorUVAlignProperties
class FTLLRN_UVEditorUVAlignToolDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

protected:

	TSharedPtr<IPropertyHandle> AlignDirectionHandle;

	void BuildAlignModeButtons(IDetailLayoutBuilder& DetailBuilder);

};