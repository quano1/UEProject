// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ContentBrowserDelegates.h"
#include "Widgets/Layout/SBox.h"


class FTLLRN_IKRigEditorController;

class TLLRN_SIKRigAssetBrowser : public SBox
{
public:
	SLATE_BEGIN_ARGS(TLLRN_SIKRigAssetBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTLLRN_IKRigEditorController> InEditorController);

private:

	void RefreshView();
	void OnPathChange(const FString& NewPath);
	void OnAssetDoubleClicked(const FAssetData& AssetData);
	bool OnShouldFilterAsset(const struct FAssetData& AssetData);
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets) const;
	
	TWeakPtr<FTLLRN_IKRigEditorController> EditorController;
	TSharedPtr<SBox> AssetBrowserBox;

	friend FTLLRN_IKRigEditorController;
};
