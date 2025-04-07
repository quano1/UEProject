// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Rigs/TLLRN_RigHierarchyDefines.h"
#include "TLLRN_ControlRigBlueprint.h"
#include "Styling/SlateTypes.h"
#include "IPropertyUtilities.h"

class IPropertyHandle;

class FTLLRN_RigInfluenceMapPerEventDetails : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FTLLRN_RigInfluenceMapPerEventDetails);
	}

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static UTLLRN_ControlRigBlueprint* GetBlueprintFromDetailBuilder(IDetailLayoutBuilder& DetailBuilder);

protected:

	UTLLRN_ControlRigBlueprint* BlueprintBeingCustomized;
};
