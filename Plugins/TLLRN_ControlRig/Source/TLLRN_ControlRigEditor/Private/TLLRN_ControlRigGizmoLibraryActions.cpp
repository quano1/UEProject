// Copyright Epic Games, Inc. All Rights Reserved.

#include "TLLRN_ControlRigGizmoLibraryActions.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FTLLRN_ControlRigShapeLibraryActions::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	//FAssetTypeActions_AnimationAsset::GetActions(InObjects, MenuBuilder);
}

const TArray<FText>& FTLLRN_ControlRigShapeLibraryActions::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AnimTLLRN_ControlRigSubMenu", "TLLRN Control Rig")
	};
	return SubMenus;
}

#undef LOCTEXT_NAMESPACE
