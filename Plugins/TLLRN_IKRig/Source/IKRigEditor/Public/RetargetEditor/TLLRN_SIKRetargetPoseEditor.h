// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DetailLayoutBuilder.h"
#include "Widgets/Input/SComboBox.h"

class UTLLRN_IKRetargeterController;
class FTLLRN_IKRetargetEditorController;

class TLLRN_SIKRetargetPoseEditor : public SCompoundWidget
{
	
public:
	
	SLATE_BEGIN_ARGS(TLLRN_SIKRetargetPoseEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTLLRN_IKRetargetEditorController> InEditorController);

	void Refresh();
	
private:

	TSharedPtr<SVerticalBox> Contents;
	
	TSharedRef<SWidget> MakeToolbar(TSharedPtr<FUICommandList> Commands);
	TSharedRef<SWidget> GenerateResetMenuContent(TSharedPtr<FUICommandList> Commands);
	TSharedRef<SWidget> GenerateEditMenuContent(TSharedPtr<FUICommandList> Commands);
	TSharedRef<SWidget> GenerateNewMenuContent(TSharedPtr<FUICommandList> Commands);
	TObjectPtr<UTLLRN_IKRetargeterController> GetAssetControllerFromSelectedObjects(IDetailLayoutBuilder& DetailBuilder) const;
	
	TWeakPtr<FTLLRN_IKRetargetEditorController> EditorController;
	TArray<TSharedPtr<FName>> PoseNames;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> PoseListComboBox;
};
