// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"
#include "TLLRN_UVEditorSeamTool.h"

class IPropertyHandle;
class FPropertyRestriction;
class ITLLRN_UVEditorTransformToolQuickAction;

// Customization for UTLLRN_UVEditorSeamToolProperties
class FTLLRN_UVEditorSeamToolPropertiesDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	ETLLRN_UVEditorSeamMode GetCurrentMode(TSharedRef<IPropertyHandle> PropertyHandle) const;
	void OnCurrentModeChanged(ETLLRN_UVEditorSeamMode Mode, TSharedRef<IPropertyHandle> PropertyHandle);
};